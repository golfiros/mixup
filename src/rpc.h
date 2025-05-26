#ifndef __MIXUP_RPC_H__
#define __MIXUP_RPC_H__

#include "magic.h"
#include "mixup.h"

struct rpc_data {
  struct data *data;
  struct mg_connection *conn;
};

bool _rpc_extract_str(struct mg_str, size_t, void *);
bool _rpc_extract_num(struct mg_str, size_t, void *);
bool _rpc_extract_bool(struct mg_str, size_t, void *);
bool _rpc_extract_long(struct mg_str, size_t, void *);
bool _rpc_extract_ulong(struct mg_str, size_t, void *);
bool _rpc_extract_obj(struct mg_str, size_t, void *);
#define RPC_EXTRACT_ARG(type, name, idx)                                       \
  if (!_Generic(name,                                                          \
          char *: _rpc_extract_str,                                            \
          double: _rpc_extract_num,                                            \
          bool: _rpc_extract_bool,                                             \
          long: _rpc_extract_long,                                             \
          unsigned long: _rpc_extract_ulong,                                   \
          struct bus *: _rpc_extract_obj)(_r->frame, idx, &(name))) {          \
    mg_rpc_err(_r, -1, "%m",                                                   \
               MG_ESC("Missing parameter " #name " of type " #type));          \
    return;                                                                    \
  }

#define RPC_DECL(name) void rpc_##name(struct mg_rpc_req *)

#define UNIQUE(name) CAT(name##_, __LINE__)
#define RPC_FUNC_ARG(type, name) type name
#define RPC_DECL_ARG(type, name) type name;
#define RPC_PASS_ARG(type, name) name
#define RPC_DEFN(name, ...)                                                    \
  static inline void UNIQUE(name)(IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(          \
      FOR_EACH_MAP(0, RPC_FUNC_ARG, __VA_ARGS__, (, ...)), ...));              \
  void rpc_##name(struct mg_rpc_req *_r) {                                     \
    struct rpc_data *rpc_data = _r->req_data;                                  \
    FOR_EACH(0, RPC_DECL_ARG, __VA_ARGS__)                                     \
    FOR_EACH_IDX(0, RPC_EXTRACT_ARG, __VA_ARGS__)                              \
    UNIQUE(name)(IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                           \
        FOR_EACH_MAP(0, RPC_PASS_ARG, __VA_ARGS__, (, _r)), _r));              \
  }                                                                            \
  static inline void UNIQUE(name)(IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(          \
      FOR_EACH_MAP(0, RPC_FUNC_ARG, __VA_ARGS__, (, ...)), ...))

#define RPC_BEGIN(...)                                                         \
  va_list UNIQUE(args);                                                        \
  va_start(UNIQUE(args));                                                      \
  struct mg_rpc_req *_r = va_arg(UNIQUE(args), typeof(_r));                    \
  va_end(UNIQUE(args)) IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                     \
      ; struct data * DEFER(HEAD)(__VA_ARGS__) =                               \
            ((struct rpc_data *)_r->req_data)->data)

size_t _rpc_print_str(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_num(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_bool(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_long(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_ulong(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_obj(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_bus(void (*)(char, void *), void *, va_list *);
size_t _rpc_print_port(void (*)(char, void *), void *, va_list *);
#define RPC_PRINT(arg)                                                         \
  _Generic(arg,                                                                \
      char *: _rpc_print_str,                                                  \
      double: _rpc_print_num,                                                  \
      bool: _rpc_print_bool,                                                   \
      long: _rpc_print_long,                                                   \
      unsigned long: _rpc_print_ulong,                                         \
      void *: _rpc_print_obj,                                                  \
      struct bus *: _rpc_print_bus,                                            \
      struct port: _rpc_print_port),                                           \
      arg

#define RPC_RETURN(...)                                                        \
  do {                                                                         \
    mg_rpc_ok(_r, IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                          \
                      DEFER("%M", RPC_PRINT(HEAD(__VA_ARGS__))), "null"));     \
    return;                                                                    \
  } while (0)

#define RPC_FMT_INDIRECT() RPC_FMT_NO_EVAL
#define RPC_FMT_NO_EVAL(...)                                                   \
  IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                                          \
      ",%M" DEFER2(RPC_FMT_INDIRECT)()(TAIL(__VA_ARGS__)))
#define RPC_FMT(...)                                                           \
  IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))("%M")                                     \
      EVAL0(RPC_FMT_NO_EVAL(TAIL(__VA_ARGS__)))

#define RPC_INIT() ((struct rpc_data *)_r->req_data)->conn->data[1] = 'R'

#define RPC_CALL(conn, name, ...)                                              \
  do {                                                                         \
    if ((conn)->data[1] == 'R')                                                \
      mg_ws_printf((conn), WEBSOCKET_OP_TEXT,                                  \
                   "{%m:%m,%m:[" RPC_FMT(__VA_ARGS__) "]}", MG_ESC("method"),  \
                   MG_ESC(#name),                                              \
                   MG_ESC("params") __VA_OPT__(, )                             \
                       FOR_EACH_MAP(0, RPC_PRINT, __VA_ARGS__));               \
  } while (0)

#define RPC_REPLY(name, ...)                                                   \
  RPC_CALL(((struct rpc_data *)_r->req_data)->conn, name, __VA_ARGS__)

#define RPC_RELAY(name, ...)                                                   \
  do {                                                                         \
    for (struct mg_connection *_c =                                            \
             ((struct rpc_data *)_r->req_data)->data->mgr.conns;               \
         _c; _c = _c->next)                                                    \
      if (_c != ((struct rpc_data *)_r->req_data)->conn)                       \
        RPC_CALL(_c, name, __VA_ARGS__);                                       \
  } while (0);

void server_run(struct mg_connection *, int, void *);

#endif // !__MIXUP_RPC_H__
