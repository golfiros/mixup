#ifndef __MIXUP_MIXUP_H__
#define __MIXUP_MIXUP_H__

#define MIXUP "mixup"

#include "vector.h"

struct data {
  void *output[2];

  struct core *core;
  struct srv *srv;

  size_t buffer_size;
  double sample_rate;

  vec(struct bus *) buses;
  vec(struct mixer *) mixers;
};

typedef float float2[2];

struct src {
  enum {
    MIXUP_SRC_NONE,
    MIXUP_SRC_BUS,
  } type;
  void *ptr;
};

#endif
