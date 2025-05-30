#ifndef __MIXUP_REGISTRY_H__
#define __MIXUP_REGISTRY_H__

#include "mixup.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct node {
  uint32_t id;
  char *name;
};
static int node_cmp(const void *_n1, const void *_n2) {
  const struct node *n1 = _n1, *n2 = _n2;
  return n1->id - n2->id;
}

struct port {
  char *path;
  const char *node;
  bool input;

  float **buf;

  uint32_t id;
  size_t conn;
  struct pw_proxy *link;
};
static int port_cmp(const void *_p1, const void *_p2) {
  const struct port *p1 = _p1, *p2 = _p2;
  return strcmp(p1->path, p2->path);
}
void print_port(FILE *, va_list *);

void port_connect(struct data *, struct port *);
void port_disconnect(struct port *);

extern const struct pw_registry_events registry_events;

#endif // !__MIXUP_REGISTRY_H__
