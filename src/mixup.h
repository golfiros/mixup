#ifndef __MIXUP_MIXUP_H__
#define __MIXUP_MIXUP_H__

#include "vector.h"
#include <math.h>
#include <stdio.h>

struct obj {
  enum {
    MIXUP_INPUT,
    MIXUP_MIXER,
    MIXUP_CHANNEL,
  } type;
  void *ptr;
};

struct data {
  void *output[2];

  struct core *core;
  struct srv *srv;

  size_t buffer_size;
  double sample_rate;

  struct map *map;

  vec(struct input *) inputs;
  vec(struct mixer *) mixers;
};

typedef float float2[2];

#define MIN_GAIN -90.0f
#define MAX_BAL 100.0f
#define DB_TO_AMP(db) ((db) > MIN_GAIN ? pow(10.0, 0.05 * (db)) : 0.0)
#define PAN_LINL(bal) fminf(1.0, 1.0 - (bal) / MAX_BAL)
#define PAN_LINR(bal) fminf(1.0, 1.0 + (bal) / MAX_BAL)
#define PAN_POWL(bal) cos(0.25 * M_PI * (1.0 + (bal) / MAX_BAL))
#define PAN_POWR(bal) sin(0.25 * M_PI * (1.0 + (bal) / MAX_BAL))

#endif
