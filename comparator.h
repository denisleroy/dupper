#ifndef COMPARATOR_H
#define COMPARATOR_H

#include <vector>
#include <string>

#include "common.h"
#include "context.h"

using namespace std;

//
// This is the base class for various comparators, i.e. from a set of
// files, generate subsets of duplicate candidates.

class Comparator
{
protected:
    Context& context;

public:
    Comparator(Context& _context) : context(_context) {}
    virtual ~Comparator() {}

    virtual void run(vector<File>& list, vector<vector<File>>& candidates, vector<vector<File>>& duplicates) = 0;
};

#endif
