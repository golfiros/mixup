#ifndef __MIXUP_CORE_H__
#define __MIXUP_CORE_H__

#include "magic.h"
#include <stddef.h>
#include <stdio.h>

struct port_info;
struct core_events {
  void (*port_new)(void *, struct port_info);
  void (*port_delete)(void *, const char *);
  void (*buffer_size)(void *, size_t);
  void (*sample_rate)(void *, double);
  void (*process)(void *);
};
struct core *core_new(void *, struct core_events);
void core_del(struct core *);

void core_start(struct core *);
void core_stop(struct core *);

bool core_quit(struct core *);

#define list_foreach(node, list)

struct port_info {
  char *path;
  char *node;
  bool input;
  struct port_info *next;
};
void print_port(FILE *, va_list *);

struct port_info *core_ports(struct core *);
struct port *core_get_port(struct core *, const char *);

void port_connect(struct port *);
void port_disconnect(struct port *);
float **port_buf(const struct port *);

#define CBK_FUNC_ARG(type, name) type name
#define CBK_DECL_ARG(type, name) type name;
#define CBK_EXTRACT_ARG(type, name) type name = _args->name;
#define CBK_PASS_ARG(type, name) name

#define CBK_STRUCT(name) struct _args_##name
#define CBK_DEFN(name, ...)                                                    \
  CBK_STRUCT(name){FOR_EACH(0, CBK_DECL_ARG, __VA_ARGS__)};                    \
  static inline void UNIQUE(_impl_##name)(                                     \
      FOR_EACH_MAP(0, CBK_FUNC_ARG, ##__VA_ARGS__, (, ...)));                  \
  void(name)(const void *_a, void *_d) {                                       \
    const struct _args_##name *_args = _a;                                     \
    FOR_EACH(0, CBK_EXTRACT_ARG, __VA_ARGS__)                                  \
    UNIQUE(_impl_##name)(                                                      \
        FOR_EACH_MAP(0, CBK_PASS_ARG, ##__VA_ARGS__, (, _d)));                 \
  }                                                                            \
  static inline void UNIQUE(_impl_##name)(                                     \
      FOR_EACH_MAP(0, CBK_FUNC_ARG, ##__VA_ARGS__, (, ...)))

void _core_cbk(struct core *, void (*)(const void *, void *), size_t,
               const void *);
#define core_cbk(core, cbk, ...)                                               \
  _core_cbk((core), (cbk), sizeof(CBK_STRUCT(cbk)),                            \
            &(CBK_STRUCT(cbk)){FOR_EACH_MAP(0, , __VA_ARGS__)})

#define cbk_data()                                                             \
  ({                                                                           \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    UNIQUE(r);                                                                 \
  })

#endif
