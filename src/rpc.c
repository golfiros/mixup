#include "rpc.h"

bool _rpc_extract_str(struct mg_str frame, size_t idx, void *_ptr) {
  char **ptr = _ptr, buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  *ptr = mg_json_get_str(frame, buf);
  return *ptr;
}
bool _rpc_extract_num(struct mg_str frame, size_t idx, void *_ptr) {
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  return mg_json_get_num(frame, buf, _ptr);
}
bool _rpc_extract_bool(struct mg_str frame, size_t idx, void *_ptr) {
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  return mg_json_get_bool(frame, buf, _ptr);
}
bool _rpc_extract_long(struct mg_str frame, size_t idx, void *_ptr) {
  long *ptr = _ptr;
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  *ptr = mg_json_get_long(frame, buf, LONG_MAX);
  return *ptr != LONG_MAX;
}
bool _rpc_extract_ulong(struct mg_str frame, size_t idx, void *_ptr) {
  ulong *ptr = _ptr;
  char buf[32];
  sprintf(buf, "$.params[%lu]", idx);
  *ptr = mg_json_get_long(frame, buf, ULONG_MAX);
  return *ptr != ULONG_MAX;
}
bool _rpc_extract_obj(struct mg_str frame, size_t idx, void *_ptr) {
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

size_t _rpc_print_str(void(out)(char, void *), void *ptr, va_list *ap) {
  char *str = va_arg(*ap, typeof(str));
  return mg_xprintf(out, ptr, "%m", MG_ESC(str));
}

size_t _rpc_print_num(void(out)(char, void *), void *ptr, va_list *ap) {
  double num = va_arg(*ap, typeof(num));
  return mg_xprintf(out, ptr, "%g", num);
}

size_t _rpc_print_bool(void(out)(char, void *), void *ptr, va_list *ap) {
  if (va_arg(*ap, int))
    return mg_xprintf(out, ptr, "true");
  else
    return mg_xprintf(out, ptr, "false");
}

size_t _rpc_print_long(void(out)(char, void *), void *ptr, va_list *ap) {
  long num = va_arg(*ap, long);
  return mg_xprintf(out, ptr, "%ld", num);
}

size_t _rpc_print_ulong(void(out)(char, void *), void *ptr, va_list *ap) {
  unsigned long num = va_arg(*ap, unsigned long);
  return mg_xprintf(out, ptr, "%lu", num);
}

size_t _rpc_print_obj(void(out)(char, void *), void *ptr, va_list *ap) {
  void *obj = va_arg(*ap, typeof(obj));
  return mg_xprintf(out, ptr, "\"%p\"", obj);
}

void server_run(struct mg_connection *c, int ev, void *ev_data) {
  struct data *data = c->fn_data;
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
      struct rpc_data rpc_data = {
          .data = data,
          .conn = c,
      };
      struct mg_iobuf io = {
          .buf = 0,
          .size = 0,
          .len = 0,
          .align = 512,
      };
      struct mg_rpc_req r = {
          .head = &data->rpc,
          .rpc = nullptr,
          .pfn = mg_pfn_iobuf,
          .pfn_data = &io,
          .req_data = &rpc_data,
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
