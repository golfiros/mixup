#ifndef __MIXUP_MIXUP_H__
#define __MIXUP_MIXUP_H__

#include <mongoose.h>
#include <pipewire/pipewire.h>
#include <stdatomic.h>
#include <unistd.h>

#define MIXUP "mixup"
#define WEB_ROOT "web"
#define QUEUE_SIZE 512

#include "vector.h"

static_assert(QUEUE_SIZE && !(QUEUE_SIZE & (QUEUE_SIZE - 1)));
struct data {
  bool quit;
  void *output[2];

  struct pw_thread_loop *thread_loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct spa_hook listener;

  uint32_t filter_id;

  vec(struct node) nodes;

  struct {
    enum {
      MIXUP_QUEUE_PORT_NEW,
      MIXUP_QUEUE_PORT_DELETE,
    } type;
    void *ptr;
  } queue[QUEUE_SIZE];
  atomic_size_t queue_head;
  atomic_size_t queue_tail;

  vec(struct port) ports;

  struct mg_mgr mgr;
  struct mg_rpc *rpc;

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
