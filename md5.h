#ifndef MD5_H
#define MD5_H

#include <vector>
#include <stdlib.h>

/* typedef a 32 bit type */
typedef unsigned long int UINT4;

typedef struct MD5Struct {
    MD5Struct() : digest(16) {}
    UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
    UINT4 buf[4];                                    /* scratch buffer */
    unsigned char in[64];                              /* input buffer */
    std::vector<unsigned char> digest;     /*[16] actual digest after MD5Final call */
} MD5_CTX;

int  MDFile(MD5_CTX&, const char* filename, size_t map_offset = 0LL, size_t map_len = 0LL);
void MDString(MD5_CTX&, const char* string);

#endif
