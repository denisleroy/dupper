#include <cstdio>

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

int main(int argc, char** argv)
{
    Context context(argc, argv);

    if (context.verbose >= 1) {
        if (context.minsize > 0)
            printf("Ignore files under %llu bytes.\n", context.minsize);
    }

    printf("Counting file in %s.\n", context.path);

    vector<File> allfiles;
    {
        FileScanner scanner;
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

    return 0;
}
