#ifndef __MIXUP_MIXUP_H__
#define __MIXUP_MIXUP_H__

#include <pipewire/pipewire.h>

#define MIXUP "mixup"

#include "vector.h"

struct data {
  bool quit;
  void *output[2];

  struct pw_thread_loop *thread_loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct spa_hook listener;

  uint32_t filter_id;

  struct srv *srv;

  vec(struct node) nodes;

  vec(struct port) ports;

  enum {
    MIXUP_EVENT_NONE = 0,
    MIXUP_EVENT_BUFFER_SIZE = 1 << 0,
    MIXUP_EVENT_SAMPLE_RATE = 1 << 1,
  } event;

  size_t buffer_size;
  double sample_rate;

  vec(struct bus *) buses;
  vec(struct mixer *) mixers;
};

typedef float float2[2];

struct src {
  enum {
    MIXUP_SRC_BUS,
  } type;
  void *ptr;
};

#endif
