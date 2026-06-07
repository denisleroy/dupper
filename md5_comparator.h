#ifndef MD5_COMPARATOR_H
#define MD5_COMPARATOR_H

#include "comparator.h"

class Md5Comparator: public Comparator
{
private:
    size_t offset, maxsize;

public:
  Md5Comparator(Context& ctx, size_t _offset = 0LL, size_t _maxsize = 0LL) :
      Comparator(ctx), offset(_offset), maxsize(_maxsize) {}

  void run(vector<File>& list, FTable& candidates, FTable& duplicates);
};

#endif
