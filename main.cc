#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <map>
#include <set>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"
#include "scanner.h"
#include "size_comparator.h"
#include "md5_comparator.h"
#include "context.h"

FTable* apply_comparator(Comparator& comparator,
                                        FTable& duplicates,
                                       FTable* candidates)
{
  auto nextgen = new FTable();

  for (auto& i: *candidates) {
    comparator.run(i, *nextgen, duplicates);
  }
  return nextgen;
}

void show(FTable* table)
{
  for (const auto& i: *table) {
    printf("------------------------\n");
    for (const auto& f: i) {
      printf("%8llu : %s\n", f.metadata.st_size, f.path.c_str());
    }
  }
}

// Wrap a path in single quotes so it can be pasted into a shell command.
static string shell_quote(const string& s)
{
  string r = "'";
  for (char c: s) {
    if (c == '\'')
      r += "'\\''";
    else
      r += c;
  }
  r += "'";
  return r;
}

// Paths are built by the scanner as <dir>/<name>.
static string dir_of(const string& path)
{
  return path.substr(0, path.rfind('/'));
}

static bool same_dir(const vector<File>& group)
{
  string dir = dir_of(group[0].path);
  for (const auto& f: group) {
    if (dir_of(f.path) != dir)
      return false;
  }
  return true;
}

// Delete `path`, or in dry-run mode write the rm command to `script` instead.
static void remove_file(const string& path, FILE* script)
{
  if (script) {
    fprintf(script, "rm -- %s\n", shell_quote(path).c_str());
  } else if (unlink(path.c_str()) == 0) {
    printf("Deleted %s\n", path.c_str());
  } else {
    fprintf(stderr, "Failed to delete %s: %s\n", path.c_str(), strerror(errno));
  }
}

static void keep_one(const vector<File>& group, size_t keep, FILE* script)
{
  printf("Keeping %s\n", group[keep].path.c_str());
  for (size_t i = 0; i < group.size(); i++) {
    if (i != keep)
      remove_file(group[i].path, script);
  }
}

// For each group of identical files that all live in the same directory,
// keep the one with the shortest file name and remove the others. In
// dry-run mode, the rm commands are written to `script` instead.
void keep_shortest(FTable* table, FILE* script)
{
  for (const auto& group: *table) {
    if (!same_dir(group))
      continue;

    // Names share the directory prefix, so the shortest path has the
    // shortest name. Ties go to the first one found.
    size_t keep = 0;
    for (size_t i = 1; i < group.size(); i++) {
      if (group[i].path.size() < group[keep].path.size())
        keep = i;
    }
    keep_one(group, keep, script);
  }
}

// Choices made in interactive mode: the directories a group spanned, and
// the one whose copy the user kept.
typedef map<set<string>, string> Rules;

// Return the directories of the files in `group` if each one holds exactly
// one of them, or an empty set otherwise. Only such groups have a choice
// that can be remembered and replayed unambiguously.
static set<string> distinct_dirs(const vector<File>& group)
{
  set<string> dirs;
  for (const auto& f: group) {
    if (!dirs.insert(dir_of(f.path)).second)
      return set<string>();
  }
  return dirs;
}

// Find which directory to keep for a group spanning `dirs`, based on earlier
// choices. A choice made over the same directories wins. Otherwise, a choice
// made over more directories applies if the directory kept back then is one
// of `dirs`; all such choices must agree. Returns "" when there's no answer.
static string lookup_rule(const Rules& rules, const set<string>& dirs)
{
  auto exact = rules.find(dirs);
  if (exact != rules.end())
    return exact->second;

  string keep;
  for (const auto& r: rules) {
    if (!dirs.count(r.second))
      continue;
    if (!includes(r.first.begin(), r.first.end(), dirs.begin(), dirs.end()))
      continue;
    if (keep.empty())
      keep = r.second;
    else if (keep != r.second)
      return "";
  }
  return keep;
}

// Read one key press without waiting for Enter. Falls back to buffered
// input when stdin is not a terminal.
static int read_key()
{
  struct termios saved;
  if (tcgetattr(STDIN_FILENO, &saved) != 0) {
    int c;
    do {
      c = getchar();
    } while (c == '\n' || c == ' ');
    return c;
  }

  // ISIG is cleared too, so Ctrl-C arrives as a key and the terminal
  // settings are always restored.
  struct termios raw = saved;
  raw.c_lflag &= ~(ICANON | ECHO | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  unsigned char c;
  ssize_t n = read(STDIN_FILENO, &c, 1);
  tcsetattr(STDIN_FILENO, TCSANOW, &saved);
  return n == 1 ? c : EOF;
}

static const int ASK_SKIP = -1;
static const int ASK_QUIT = -2;

// Ask which file of `group` to keep. Returns its index, ASK_SKIP to leave
// the group alone, or ASK_QUIT to stop asking.
static int ask_keep(const vector<File>& group)
{
  size_t n = group.size();
  printf("------------------------\n");
  for (size_t i = 0; i < n; i++)
    printf("%3zu) %s\n", i + 1, group[i].path.c_str());

  while (true) {
    printf("Keep which file? [1-%zu, s = skip, q = quit] ", n);
    fflush(stdout);

    // Up to 9 files, a single key press picks one. Past that, the number
    // needs more than one digit, so read a whole line.
    string answer;
    if (n <= 9) {
      int c = read_key();
      if (c == EOF || c == 3 || c == 4) {  // Ctrl-C, Ctrl-D
        printf("\n");
        return ASK_QUIT;
      }
      printf("%c\n", isprint(c) ? c : ' ');
      answer = string(1, (char)c);
    } else {
      char line[64];
      if (!fgets(line, sizeof(line), stdin)) {
        printf("\n");
        return ASK_QUIT;
      }
      answer = line;
      while (!answer.empty() && isspace((unsigned char)answer.back()))
        answer.pop_back();
    }

    if (answer == "q")
      return ASK_QUIT;
    if (answer == "s")
      return ASK_SKIP;
    char* end;
    long choice = strtol(answer.c_str(), &end, 10);
    if (!answer.empty() && *end == '\0' && choice >= 1 && (size_t)choice <= n)
      return choice - 1;
  }
}

// Ask the user which file of each group to keep, and remove the others.
// When each file of a group is in its own directory, the choice is
// remembered and applied without asking to later groups over the same
// directories (see lookup_rule). Groups that --keep-shortest already
// handled are skipped.
void interactive(FTable* table, const Context& context, FILE* script)
{
  Rules rules;
  for (const auto& group: *table) {
    if (context.keep_shortest && same_dir(group))
      continue;

    set<string> dirs = distinct_dirs(group);
    if (!dirs.empty()) {
      string dir = lookup_rule(rules, dirs);
      if (!dir.empty()) {
        for (size_t i = 0; i < group.size(); i++) {
          if (dir_of(group[i].path) == dir) {
            printf("------------------------\n");
            printf("Reusing earlier choice: keep the copy in %s\n", dir.c_str());
            keep_one(group, i, script);
            break;
          }
        }
        continue;
      }
    }

    int keep = ask_keep(group);
    if (keep == ASK_QUIT)
      break;
    if (keep == ASK_SKIP)
      continue;
    if (!dirs.empty())
      rules[dirs] = dir_of(group[keep].path);
    keep_one(group, keep, script);
  }
}

int main(int argc, char** argv)
{
    Context context(argc, argv);

    if (context.verbose >= 1) {
        if (context.minsize > 0)
            printf("Ignore files under %llu bytes.\n", context.minsize);
        for (const auto& d: context.exclude_dirs)
            printf("Exclude directories named %s.\n", d.c_str());
    }

    printf("Counting file in %s.\n", context.path);

    vector<File> allfiles;
    {
        FileScanner scanner(context);
        scanner.scan(context.path, allfiles);
    }
    u64 count = allfiles.size();
    printf("\033[0K%llu files found.\n", count);

    FTable duplicates;
    auto candidates = new FTable();
    candidates->push_back(allfiles);
    FTable* next_candidates = NULL;

    {
      SizeComparator sc(context);
      next_candidates = apply_comparator(sc, duplicates, candidates);
      free(candidates);
      candidates = next_candidates;
    }

    {
      Md5Comparator md5(context, 0, 512);
      next_candidates = apply_comparator(md5, duplicates, candidates);
      free(candidates);
      candidates = next_candidates;
    }

    {
      Md5Comparator md5(context, 0, 0);
      next_candidates = apply_comparator(md5, duplicates, candidates);
      show(next_candidates);
      free(candidates);
      candidates = next_candidates;
    }

    if (context.keep_shortest || context.interactive) {
      FILE* script = NULL;
      if (context.dry_run) {
        script = fopen("DELETE.sh", "w");
        if (!script) {
          fprintf(stderr, "Cannot create DELETE.sh: %s\n", strerror(errno));
          return 1;
        }
        fprintf(script, "#!/bin/sh\n");
      }
      if (context.keep_shortest)
        keep_shortest(candidates, script);
      if (context.interactive)
        interactive(candidates, context, script);
      if (script) {
        fclose(script);
        chmod("DELETE.sh", 0755);
        printf("Wrote rm commands to DELETE.sh.\n");
      }
    }

    return 0;
}
