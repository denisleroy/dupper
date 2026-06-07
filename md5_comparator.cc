#include "md5_comparator.h"
#include "common.h"
#include "md5.h"

#include <map>

void Md5Comparator::run(vector<File>& files,
                        FTable& candidates, FTable& duplicates)
{
    map<vector<unsigned char>, vector<File>> table;

    // Collect all MD5 digests
    for (const auto& f: files) {
        MD5_CTX md5;
        if (MDFile(md5, f.path.c_str(), offset, maxsize) < 0)
            continue;
        table[md5.digest].push_back(f);
    }

    // Delete all entries with unique sizes;
    for (auto it = table.begin(); it != table.end();) {
        if ((*it).second.size() <= 1)
            table.erase(it++);
        else
            it++;
    }

    // Populate the result array.
    for (auto& i: table) {
        candidates.push_back(std::move(i.second));
    }
}
