#include "rpc.h"

#include <mongoose.h>
#include <stdatomic.h>
#include <stdio.h>

#define WEB_ROOT "web"

#define QUEUE_SIZE 512
static_assert(QUEUE_SIZE && !(QUEUE_SIZE & (QUEUE_SIZE - 1)));

struct srv {
  struct mg_mgr mgr;
  struct mg_rpc *rpc;

  struct mg_iobuf queue[QUEUE_SIZE];
  atomic_size_t queue_head;
  atomic_size_t queue_tail;

  void *data;
};

static void _srv_callback(struct mg_connection *c, int ev, void *ev_data) {
  switch (ev) {
  case MG_EV_HTTP_MSG: {
    struct mg_http_message *hm = ev_data;
    if (mg_match(hm->uri, mg_str("/rpc"), nullptr)) {
      mg_ws_upgrade(c, hm, nullptr);
      c->data[0] = 'S';
    } else {
      struct mg_http_serve_opts opts = {.root_dir = WEB_ROOT};
      mg_http_serve_dir(c, ev_data, &opts);
    }
  } break;
  case MG_EV_WS_MSG: {
    struct mg_ws_message *wm = ev_data;
    if (c->data[0] == 'S') {
      struct mg_iobuf io = {
          .buf = nullptr,
          .size = 0,
          .len = 0,
          .align = 512,
      };
      struct mg_rpc_req r = {
          .head = &((struct srv *)c->fn_data)->rpc,
          .rpc = nullptr,
          .pfn = mg_pfn_iobuf,
          .pfn_data = &io,
          .req_data = c,
          .frame = wm->data,
      };
      mg_rpc_process(&r);
      if (io.buf)
        mg_ws_send(c, io.buf, io.len, WEBSOCKET_OP_TEXT);
      mg_iobuf_free(&io);
    }
  } break;
  default:
  }
}

struct srv *srv_new(void *data) {
  struct srv *srv = malloc(sizeof *srv);
  if (!srv)
    return nullptr;
  *srv = (typeof(*srv)){.data = data};

  atomic_init(&srv->queue_head, 0);
  atomic_init(&srv->queue_tail, 0);

  mg_rpc_add(&srv->rpc, mg_str("rpc.list"), mg_rpc_list, nullptr);

  mg_mgr_init(&srv->mgr);

  if (!mg_http_listen(&srv->mgr, "0.0.0.0:8000", _srv_callback, srv))
    srv_del(srv);

  return srv;
}

void srv_del(struct srv *srv) {
  if (!srv)
    return;
  mg_mgr_free(&srv->mgr);
  mg_rpc_del(&srv->rpc, nullptr);
}

void srv_reg(struct srv *srv, void(_fn)(void *), const char *name) {
  void (*fn)(struct mg_rpc_req *) = (typeof(fn))_fn;
  mg_rpc_add(&srv->rpc, mg_str(name), fn, srv->data);
}

void srv_run(struct srv *srv) {
  if (!srv)
    return;
  mg_mgr_poll(&srv->mgr, 50);

  size_t head = atomic_load_explicit(&srv->queue_head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&srv->queue_tail, memory_order_relaxed);

  while (head != tail) {
    for (struct mg_connection *c = srv->mgr.conns; c; c = c->next)
      if (c->data[1] == 'R')
        mg_ws_send(c, srv->queue[tail].buf, srv->queue[tail].len,
                   WEBSOCKET_OP_TEXT);
    mg_iobuf_free(&srv->queue[tail]);

    size_t next = (tail + 1) & (QUEUE_SIZE - 1);
    atomic_store_explicit(&srv->queue_tail, next, memory_order_release);

    head = atomic_load_explicit(&srv->queue_head, memory_order_acquire);
    tail = atomic_load_explicit(&srv->queue_tail, memory_order_relaxed);
  }
}

bool _rpc_extract_str(void *_r, size_t idx, void *_ptr) {
  struct mg_rpc_req *r = _r;
  struct mg_str frame = r->frame;
  char **ptr = _ptr, buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  *ptr = mg_json_get_str(frame, buf);
  return *ptr;
}
bool _rpc_extract_num(void *_r, size_t idx, void *_ptr) {
  struct mg_rpc_req *r = _r;
  struct mg_str frame = r->frame;
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  return mg_json_get_num(frame, buf, _ptr);
}
bool _rpc_extract_bool(void *_r, size_t idx, void *_ptr) {
  struct mg_rpc_req *r = _r;
  struct mg_str frame = r->frame;
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  return mg_json_get_bool(frame, buf, _ptr);
}
bool _rpc_extract_long(void *_r, size_t idx, void *_ptr) {
  struct mg_rpc_req *r = _r;
  struct mg_str frame = r->frame;
  long *ptr = _ptr;
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  *ptr = mg_json_get_long(frame, buf, LONG_MAX);
  return *ptr != LONG_MAX;
}
bool _rpc_extract_obj(void *_r, size_t idx, void *_ptr) {
  struct mg_rpc_req *r = _r;
  struct mg_str frame = r->frame;
  char *str, buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  str = mg_json_get_str(frame, buf);
  if (!str)
    return false;
  else {
    void **ptr = _ptr;
    *ptr = nullptr;
    sscanf(str, "%p", ptr);
    free(str);
    return *ptr;
  }
}

void *_rpc_data(void *_r) {
  struct mg_rpc_req *r = _r;
  return r->rpc->fn_data;
}

void _rpc_init(void *_r) {
  struct mg_rpc_req *r = _r;
  ((struct mg_connection *)r->req_data)->data[1] = 'R';
}

void print_str(FILE *fp, va_list *ap) {
  const char *str = va_arg(*ap, typeof(str));
  fprintf(fp, "\"%s\"", str);
}

void print_num(FILE *fp, va_list *ap) {
  double num = va_arg(*ap, typeof(num));
  fprintf(fp, "%f", num);
}

void print_bool(FILE *fp, va_list *ap) {
  int val = va_arg(*ap, typeof(val));
  fprintf(fp, val ? "true" : "false");
}

void print_long(FILE *fp, va_list *ap) {
  long num = va_arg(*ap, typeof(num));
  fprintf(fp, "%ld", num);
}

void print_obj(FILE *fp, va_list *ap) {
  void *obj = va_arg(*ap, typeof(obj));
  fprintf(fp, "\"%p\"", obj);
}

void _fprint_arg(FILE *fp, void(fn)(FILE *, va_list *), ...) {
  va_list args;
  va_start(args);
  fn(fp, &args);
  va_end(args);
}

ssize_t _write(void *cookie, const char *buf, size_t size) {
  struct mg_iobuf *io = cookie;
  for (size_t i = 0; i < size; i++)
    mg_pfn_iobuf(buf[i], io);
  return size;
}
int _close(void *) { return 0; }
void _rpc_return(void *_r, ...) {
  va_list args;
  va_start(args);
  size_t (*fn)(FILE *, va_list *) = va_arg(args, typeof(fn));
  if (!fn)
    mg_rpc_ok(_r, "null");
  else {
    struct mg_iobuf io = {
        .buf = nullptr,
        .size = 0,
        .len = 0,
        .align = 512,
    };
    FILE *fp = fopencookie(
        &io, "w", (cookie_io_functions_t){.write = _write, .close = _close});
    fn(fp, &args);
    fclose(fp);
    mg_rpc_ok(_r, "%.*s", io.len, io.buf);
    mg_iobuf_free(&io);
  }
  va_end(args);
}

void _rpc_err(void *_r, int c, const char *m, ...) {
  struct mg_rpc_req *r = _r;
  va_list args;
  va_start(args);
  mg_rpc_verr(r, c, m, &args);
  va_end(args);
}

static struct mg_iobuf _rpc_vframe(const char *name, va_list *ap) {
  struct mg_iobuf io = {
      .buf = nullptr,
      .size = 0,
      .len = 0,
      .align = 512,
  };
  mg_xprintf(mg_pfn_iobuf, &io, "{%m:%m,%m:[", MG_ESC("method"), MG_ESC(name),
             MG_ESC("params"));
  FILE *fp = fopencookie(&io, "w", (cookie_io_functions_t){.write = _write});

  void (*fn)(FILE *, va_list *) = va_arg(*ap, typeof(fn));
  for (size_t i = 0; fn; fn = va_arg(*ap, typeof(fn))) {
    if (i++)
      fprintf(fp, ",");
    fn(fp, ap);
  }
  fclose(fp);
  mg_xprintf(mg_pfn_iobuf, &io, "]}");
  return io;
}

void _rpc_reply(void *_r, const char *name, ...) {
  va_list args;
  va_start(args);
  struct mg_iobuf io = _rpc_vframe(name, &args);
  va_end(args);

  struct mg_rpc_req *r = _r;
  struct mg_connection *conn = r->req_data;

  if (conn->data[1] == 'R')
    mg_ws_send(r->req_data, io.buf, io.len, WEBSOCKET_OP_TEXT);

  mg_iobuf_free(&io);
}

void _rpc_relay(void *_r, const char *name, ...) {
  va_list args;
  va_start(args);
  struct mg_iobuf io = _rpc_vframe(name, &args);
  va_end(args);

  struct mg_rpc_req *r = _r;
  struct mg_connection *conn = r->req_data;
  struct srv *srv = conn->fn_data;

  for (struct mg_connection *c = srv->mgr.conns; c; c = c->next)
    if (c != conn && c->data[1] == 'R')
      mg_ws_send(c, io.buf, io.len, WEBSOCKET_OP_TEXT);

  mg_iobuf_free(&io);
}

void _rpc_broadcast(struct srv *srv, const char *name, ...) {
  va_list args;
  va_start(args);
  struct mg_iobuf io = _rpc_vframe(name, &args);
  va_end(args);

  size_t head = atomic_load_explicit(&srv->queue_head, memory_order_relaxed);

  srv->queue[head] = io;
  size_t next = (head + 1) & (QUEUE_SIZE - 1);
  atomic_store_explicit(&srv->queue_head, next, memory_order_release);
}
