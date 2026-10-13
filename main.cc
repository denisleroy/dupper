#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
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

// For each group of identical files that all live in the same directory,
// keep the one with the shortest file name and remove the others. In
// dry-run mode, the rm commands are written to `script` instead.
void keep_shortest(FTable* table, FILE* script)
{
  for (const auto& group: *table) {
    // Paths are built by the scanner as <dir>/<name>.
    const string& first = group[0].path;
    string dir = first.substr(0, first.rfind('/'));
    bool same_dir = true;
    for (const auto& f: group) {
      if (f.path.substr(0, f.path.rfind('/')) != dir) {
        same_dir = false;
        break;
      }
    }
    if (!same_dir)
      continue;

    // Names share the directory prefix, so the shortest path has the
    // shortest name. Ties go to the first one found.
    size_t keep = 0;
    for (size_t i = 1; i < group.size(); i++) {
      if (group[i].path.size() < group[keep].path.size())
        keep = i;
    }

    printf("Keeping %s\n", group[keep].path.c_str());
    for (size_t i = 0; i < group.size(); i++) {
      if (i == keep)
        continue;
      const string& path = group[i].path;
      if (script) {
        fprintf(script, "rm -- %s\n", shell_quote(path).c_str());
      } else if (unlink(path.c_str()) == 0) {
        printf("Deleted %s\n", path.c_str());
      } else {
        fprintf(stderr, "Failed to delete %s: %s\n", path.c_str(), strerror(errno));
      }
    }
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

    if (context.keep_shortest) {
      FILE* script = NULL;
      if (context.dry_run) {
        script = fopen("DELETE.sh", "w");
        if (!script) {
          fprintf(stderr, "Cannot create DELETE.sh: %s\n", strerror(errno));
          return 1;
        }
        fprintf(script, "#!/bin/sh\n");
      }
      keep_shortest(candidates, script);
      if (script) {
        fclose(script);
        chmod("DELETE.sh", 0755);
        printf("Wrote rm commands to DELETE.sh.\n");
      }
    }

    return 0;
}
