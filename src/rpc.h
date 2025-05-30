#ifndef __MIXUP_RPC_H__
#define __MIXUP_RPC_H__

#include "magic.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

struct srv *srv_new();
void srv_del(struct srv *);

void srv_reg(struct srv *, void (*)(void *), const char *, void *);
void srv_run(struct srv *);

#define RPC_FUNC_ARG(type, name) type name
#define RPC_DECL_ARG(type, name) type name;
bool _rpc_extract_str(void *, size_t, void *);
bool _rpc_extract_num(void *, size_t, void *);
bool _rpc_extract_bool(void *, size_t, void *);
bool _rpc_extract_long(void *, size_t, void *);
bool _rpc_extract_obj(void *, size_t, void *);
#ifndef RPC_EXTRACT_TYPES
#define RPC_EXTRACT_ARG(type, name, idx)                                       \
  if (!_Generic((name),                                                        \
          char *: _rpc_extract_str,                                            \
          double: _rpc_extract_num,                                            \
          bool: _rpc_extract_bool,                                             \
          long: _rpc_extract_long,                                             \
          void *: _rpc_extract_obj)(_r, idx, &(name))) {                       \
    _rpc_err(_r, -32602, "Missing parameter " #name " of type " #type);        \
    return;                                                                    \
  }
#else
#define RPC_EXTRACT_ARG(type, name, idx)                                       \
  if (!_Generic((name),                                                        \
          char *: _rpc_extract_str,                                            \
          double: _rpc_extract_num,                                            \
          bool: _rpc_extract_bool,                                             \
          long: _rpc_extract_long,                                             \
          void *: _rpc_extract_obj,                                            \
          RPC_EXTRACT_TYPES)(_r, idx, &(name))) {                              \
    _rpc_err(_r, -32602, "Missing parameter " #name " of type " #type);        \
    return;                                                                    \
  }
#endif
#define RPC_PASS_ARG(type, name) name
#define RPC_DEFN(name, ...)                                                    \
  static inline void UNIQUE(_impl_##name)(                                     \
      FOR_EACH_MAP(0, RPC_FUNC_ARG, ##__VA_ARGS__, (, ...)));                  \
  void(name)(void *_r) {                                                       \
    FOR_EACH(0, RPC_DECL_ARG, __VA_ARGS__)                                     \
    FOR_EACH_IDX(0, RPC_EXTRACT_ARG, __VA_ARGS__)                              \
    UNIQUE(_impl_##name)(                                                      \
        FOR_EACH_MAP(0, RPC_PASS_ARG, ##__VA_ARGS__, (, _r)));                 \
  }                                                                            \
  static inline void UNIQUE(_impl_##name)(                                     \
      FOR_EACH_MAP(0, RPC_FUNC_ARG, ##__VA_ARGS__, (, ...)))

void *_rpc_data(void *);
#define rpc_data()                                                             \
  ({                                                                           \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    _rpc_data(UNIQUE(r));                                                      \
  })

void _rpc_init(void *);
#define rpc_init()                                                             \
  do {                                                                         \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    _rpc_init(UNIQUE(r));                                                      \
  } while (false)

void print_str(FILE *, va_list *);
void print_num(FILE *, va_list *);
void print_bool(FILE *, va_list *);
void print_long(FILE *, va_list *);
void print_obj(FILE *, va_list *);
#ifndef RPC_PRINT_TYPES
#define RPC_PRINT(arg)                                                         \
  _Generic(arg,                                                                \
      char *: print_str,                                                       \
      const char *: print_str,                                                 \
      double: print_num,                                                       \
      bool: print_bool,                                                        \
      long: print_long,                                                        \
      void *: print_obj),                                                      \
      arg
#else
#define RPC_PRINT(arg)                                                         \
  _Generic(arg,                                                                \
      char *: print_str,                                                       \
      const char *: print_str,                                                 \
      double: print_num,                                                       \
      bool: print_bool,                                                        \
      long: print_long,                                                        \
      void *: print_obj,                                                       \
      RPC_PRINT_TYPES),                                                        \
      arg
#endif

void _rpc_return(void *, ...);
#define rpc_return(...)                                                        \
  do {                                                                         \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    _rpc_return(UNIQUE(r), FOR_EACH_MAP(0, RPC_PRINT, __VA_ARGS__)             \
                               __VA_OPT__(, ) nullptr);                        \
  } while (false)

void _rpc_err(void *, int, const char *, ...);
#define rpc_err(code, msg)                                                     \
  do {                                                                         \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    _rpc_err(UNIQUE(r), (code), (msg));                                        \
  } while (false)

void _rpc_reply(void *, const char *, ...);
#define rpc_reply(name, ...)                                                   \
  do {                                                                         \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    _rpc_reply(UNIQUE(r), (name),                                              \
               FOR_EACH_MAP(0, RPC_PRINT, __VA_ARGS__)                         \
                   __VA_OPT__(, ) nullptr);                                    \
  } while (false)

void _rpc_relay(void *, const char *, ...);
#define rpc_relay(name, ...)                                                   \
  do {                                                                         \
    va_list UNIQUE(args);                                                      \
    va_start(UNIQUE(args));                                                    \
    void *UNIQUE(r) = va_arg(UNIQUE(args), typeof(UNIQUE(r)));                 \
    va_end(UNIQUE(args));                                                      \
    _rpc_relay(UNIQUE(r), (name),                                              \
               FOR_EACH_MAP(0, RPC_PRINT, __VA_ARGS__)                         \
                   __VA_OPT__(, ) nullptr);                                    \
  } while (false)

void _rpc_broadcast(struct srv *, const char *, ...);
#define rpc_broadcast(srv, name, ...)                                          \
  _rpc_broadcast((srv), (name),                                                \
                 FOR_EACH_MAP(0, RPC_PRINT, __VA_ARGS__)                       \
                     __VA_OPT__(, ) nullptr)

#endif // !__MIXUP_RPC_H__
