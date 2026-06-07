#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <vector>
#include <map>

#include "common.h"

using namespace std;

class FileScanner
{
public:
    void scan(string path, vector<File>& result);
};

#endif
