#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <vector>
#include <map>

#include "common.h"
#include "context.h"

using namespace std;

class FileScanner
{
public:
    FileScanner(const Context& _context) : context(_context) {}

    void scan(string path, vector<File>& result);

private:
    const Context& context;
};

#endif
