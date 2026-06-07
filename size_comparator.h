#ifndef SIZE_COMPARATOR_H
#define SIZE_COMPARATOR_H

#include "comparator.h"

class SizeComparator: public Comparator
{
public:
    SizeComparator(Context& ctx) : Comparator(ctx) {}
    virtual ~SizeComparator() {}

    void run(vector<File>& list, vector<vector<File>>& candidates, vector<vector<File>>& duplicates);
};

#endif
