#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <utility>
#include <stdint.h>
#include <sys/stat.h>
#include <vector>

typedef uint64_t u64;
typedef unsigned char u8;

typedef struct File_s {
    File_s(std::string& _path) : path(_path) {}

    File_s(const File_s& src) : metadata(src.metadata), path(src.path) {}
    File_s(File_s && src) : metadata(src.metadata), path(std::move(src.path)) {}

    struct stat metadata;
    std::string path;
} File;

typedef std::vector<std::vector<File>> FTable;

#endif
