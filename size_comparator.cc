#include "size_comparator.h"
#include "common.h"

#include <sys/stat.h>
#include <map>
#include <utility>

void SizeComparator::run(vector<File>& files,
                         vector<vector<File>>& candidates,
                         vector<vector<File>>& duplicates)
{
    map<off_t, vector<File>> table;

    // Collect all sizes.
    for (auto& f: files) {
        if (stat(f.path.c_str(), &f.metadata) == 0 && f.metadata.st_size > 0) {
            table[f.metadata.st_size].push_back(f);
        }
    }

    // Delete all entries with unique sizes.
    for (auto it = table.begin(); it != table.end();) {
        if ((*it).second.size() <= 1)
            table.erase(it++);
        else
            it++;
    }

    // Populate the candidates and duplicates array.
    for (auto& i: table) {

        if (i.first == 0) {
            if (!context.ignore_empty_files)
                duplicates.push_back(std::move(i.second));
        } else {
            candidates.push_back(std::move(i.second));
        }
    }
}
