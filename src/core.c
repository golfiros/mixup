#include "core.h"
#include "vector.h"
#include <pipewire/pipewire.h>
#include <string.h>

#define MIXUP "mixup"

struct core {
  bool quit;

  struct pw_thread_loop *thread_loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct spa_hook listener;

  uint32_t filter_id;

  enum {
    CORE_EVENT_NONE = 0,
    CORE_EVENT_BUFFER_SIZE = 1 << 0,
    CORE_EVENT_SAMPLE_RATE = 1 << 1,
  } event;

  size_t buffer_size;
  double sample_rate;

  vec(struct node) nodes;
  vec(struct port) ports;

  void *data;
  struct core_events ev;
};

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

  struct core *core;
  uint32_t id;
  size_t conn;
  struct pw_proxy *link;
};
static int port_cmp(const void *_p1, const void *_p2) {
  const struct port *p1 = _p1, *p2 = _p2;
  return strcmp(p1->path, p2->path);
}

static void _quit(void *_data, int) {
  struct core *core = _data;
  core->quit = true;
}

static int _buffer_size(struct spa_loop *, bool, uint32_t, const void *_args,
                        size_t, void *_data) {
  struct core *core = _data;
  if (core->ev.buffer_size)
    core->ev.buffer_size(core->data, *(size_t *)_args);
  core->event &= ~CORE_EVENT_BUFFER_SIZE;
  return 0;
}

static int _sample_rate(struct spa_loop *, bool, uint32_t, const void *_args,
                        size_t, void *_data) {
  struct core *core = _data;
  if (core->ev.sample_rate)
    core->ev.sample_rate(core->data, *(double *)_args);
  core->event &= ~CORE_EVENT_SAMPLE_RATE;
  return 0;
}

static void _process(void *_data, struct spa_io_position *pos) {
  struct core *core = _data;
  size_t buffer_size = pos->clock.duration;
  vec_foreach(port, core->ports) {
    if (port.buf) {
      *port.buf = pw_filter_get_dsp_buffer(port.buf, buffer_size);
      if (!port.input)
        for (size_t t = 0; t < buffer_size; t++)
          (*port.buf)[t] = 0.0f;
    }
  }
  if (buffer_size != core->buffer_size) {
    core->buffer_size = buffer_size;
    core->event = core->event | CORE_EVENT_BUFFER_SIZE;
    pw_loop_invoke(pw_thread_loop_get_loop(core->thread_loop), _buffer_size,
                   SPA_ID_INVALID, &buffer_size, sizeof buffer_size, false,
                   core);
  }
  double sample_rate = pos->clock.rate.denom;
  sample_rate /= pos->clock.rate.num;
  if (sample_rate != core->sample_rate) {
    core->sample_rate = sample_rate;
    core->event = core->event | CORE_EVENT_SAMPLE_RATE;
    pw_loop_invoke(pw_thread_loop_get_loop(core->thread_loop), _sample_rate,
                   SPA_ID_INVALID, &sample_rate, sizeof sample_rate, false,
                   core);
  }
  if (core->event)
    return;
  core->ev.process(core->data);
}

static const struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = _process,
};

static void _global(void *_data, uint32_t id, uint32_t, const char *type,
                    uint32_t, const struct spa_dict *props) {
  struct core *core = _data;
  const char *value;
  if (!strcmp(type, PW_TYPE_INTERFACE_Node)) {
    if (!(value = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION)))
      value = spa_dict_lookup(props, PW_KEY_NODE_NAME);

    struct node node = {
        .id = id,
        .name = strdup(value),
    };
    vec_push(&core->nodes, node);
    vec_qsort(&core->nodes, node_cmp);
    qsort(core->nodes.data, core->nodes.n, sizeof node, node_cmp);
  } else if (!strcmp(type, PW_TYPE_INTERFACE_Port)) {
    uint32_t node_id;
    if (!(value = spa_dict_lookup(props, PW_KEY_NODE_ID)) ||
        (node_id = atoi(value)) == core->filter_id ||
        ((value = spa_dict_lookup(props, PW_KEY_PORT_MONITOR)) &&
         !strcmp(value, "true")))
      return;

    struct node node = vec_bsearch(core->nodes, node_cmp, .id = node_id);

    if (!(value = spa_dict_lookup(props, PW_KEY_FORMAT_DSP)) ||
        !strstr(value, "audio"))
      return;

    const char *path = spa_dict_lookup(props, PW_KEY_OBJECT_PATH);

    if (!(value = spa_dict_lookup(props, PW_KEY_PORT_DIRECTION)))
      return;
    bool input = strstr(value, "out");

    struct port port = {
        .path = strdup(path),
        .id = id,
        .node = node.name,
        .input = input,
        .core = core,
    };
    vec_push(&core->ports, port);
    vec_qsort(&core->ports, port_cmp);

    core->ev.port_new(core->data, (struct port_info){
                                      .path = strdup(path),
                                      .node = strdup(node.name),
                                      .input = input,
                                  });
  }
}
static void _global_remove(void *_data, uint32_t id) {
  struct core *core = _data;
  size_t idx;
  if ((idx = vec_idx(core->nodes, .id == id)) < core->nodes.n) {
    struct node node = vec_delete(&core->nodes, idx);
    free(node.name);
  }
  if ((idx = vec_idx(core->ports, .id == id)) < core->ports.n) {
    struct port port = vec_delete(&core->ports, idx);
    core->ev.port_delete(core->data, port.path);
    while (port.conn)
      port_disconnect(&port);
    free(port.path);
  }
}
const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = _global,
    .global_remove = _global_remove,
};

struct core *core_new(void *data, struct core_events ev) {
  struct core *core = malloc(sizeof *core);
  if (!core)
    return nullptr;
  *core = (typeof(*core)){
      .data = data,
      .ev = ev,
  };
  spa_zero(core->listener);

  pw_init(nullptr, nullptr);

  if (!(core->thread_loop = pw_thread_loop_new(MIXUP, nullptr))) {
    free(core);
    return nullptr;
  }

  pw_thread_loop_lock(core->thread_loop);

  struct pw_loop *loop = pw_thread_loop_get_loop(core->thread_loop);
  pw_loop_add_signal(loop, SIGINT, _quit, core);
  pw_loop_add_signal(loop, SIGTERM, _quit, core);

  if (!(core->filter = pw_filter_new_simple(
            loop, MIXUP,
            pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                              "Filter", PW_KEY_MEDIA_ROLE, "DSP", nullptr),
            &filter_events, core))) {
    pw_thread_loop_destroy(core->thread_loop);
    free(core);
    return nullptr;
  }

  if (pw_filter_connect(core->filter, PW_FILTER_FLAG_RT_PROCESS, nullptr, 0) <
          0 ||
      !(core->registry = pw_core_get_registry(pw_filter_get_core(core->filter),
                                              PW_VERSION_REGISTRY, 0))) {
    pw_filter_destroy(core->filter);
    pw_thread_loop_destroy(core->thread_loop);
    free(core);
    return nullptr;
  }

  if (pw_registry_add_listener(core->registry, &core->listener,
                               &registry_events, core) < 0) {
    pw_proxy_destroy((struct pw_proxy *)core->registry);
    pw_filter_destroy(core->filter);
    pw_thread_loop_destroy(core->thread_loop);
    free(core);
    return nullptr;
  }

  vec_init(&core->nodes);
  vec_init(&core->ports);

  return core;
}

void core_del(struct core *core) {
  vec_foreach(port, core->ports) {
    while (port.conn)
      port_disconnect(&port);
    free(port.path);
  }
  vec_free(&core->ports);

  vec_foreach(node, core->nodes) { free(node.name); }
  vec_free(&core->nodes);

  spa_hook_remove(&core->listener);
  pw_proxy_destroy((struct pw_proxy *)core->registry);
  pw_filter_destroy(core->filter);
  pw_thread_loop_destroy(core->thread_loop);
  free(core);
}

void core_start(struct core *core) {
  pw_thread_loop_start(core->thread_loop);
  pw_thread_loop_unlock(core->thread_loop);

  while ((core->filter_id = pw_filter_get_node_id(core->filter)) ==
         SPA_ID_INVALID)
    ;
}

void core_stop(struct core *core) { pw_thread_loop_stop(core->thread_loop); }

bool core_quit(struct core *core) { return core->quit; }

void print_port(FILE *fp, va_list *ap) {
  struct port_info port = va_arg(*ap, typeof(port));
  fprintf(fp, "{\"path\":\"%s\",\"node\":\"%s\",\"input\":%s}", port.path,
          port.node, port.input ? "true" : "false");
}

struct port_info *core_ports(struct core *core) {
  pw_thread_loop_lock(core->thread_loop);
  struct port_info *head = nullptr, **tail = &head;
  vec_foreach(port, core->ports) {
    *tail = malloc(sizeof **tail);
    **tail = (typeof(**tail)){
        .path = strdup(port.path),
        .node = strdup(port.node),
        .input = port.input,
    };
    tail = &(*tail)->next;
  }
  pw_thread_loop_unlock(core->thread_loop);
  return head;
}

struct port *core_get_port(struct core *core, const char *path) {
  return vec_bsearch_ref(core->ports, port_cmp, .path = (char *)path);
}

void port_connect(struct port *port) {
  if (port->conn++)
    return;
  char name[16];
  sprintf(name, "port_%u", port->id);
  struct pw_properties *port_props =
      pw_properties_new(PW_KEY_PORT_NAME, name, PW_KEY_FORMAT_DSP,
                        "32 bit float mono audio", nullptr);
  struct pw_properties *link_props =
      pw_properties_new(PW_KEY_MEDIA_NAME, MIXUP, nullptr);
  if (port->input) {
    port->buf = pw_filter_add_port(port->core->filter, PW_DIRECTION_INPUT,
                                   PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                                   sizeof *port->buf, port_props, nullptr, 0);
    pw_properties_setf(link_props, PW_KEY_LINK_OUTPUT_PORT, "%u", port->id);
    pw_properties_setf(link_props, PW_KEY_LINK_INPUT_NODE, "%u",
                       port->core->filter_id);
  } else {
    port->buf = pw_filter_add_port(port->core->filter, PW_DIRECTION_OUTPUT,
                                   PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                                   sizeof *port->buf, port_props, nullptr, 0);
    pw_properties_setf(link_props, PW_KEY_LINK_INPUT_PORT, "%u", port->id);
    pw_properties_setf(link_props, PW_KEY_LINK_OUTPUT_NODE, "%u",
                       port->core->filter_id);
  }
  port->link = pw_core_create_object(pw_filter_get_core(port->core->filter),
                                     "link-factory", PW_TYPE_INTERFACE_Link,
                                     PW_VERSION_LINK, &link_props->dict, 0);
  pw_properties_free(link_props);
}

void port_disconnect(struct port *port) {
  if (--port->conn)
    return;
  pw_proxy_destroy(port->link);
  pw_filter_remove_port(port->buf);
  port->buf = nullptr;
}

float **port_buf(const struct port *port) { return port->buf; }

struct _cbk_data {
  void (*fn)(const void *, void *);
  char data[];
};
static int _callback(struct spa_loop *, bool, uint32_t, const void *_args,
                     size_t, void *_data) {
  const struct _cbk_data *args = _args;
  args->fn(args->data, _data);
  return 0;
}
void _core_cbk(struct core *core, void (*fn)(const void *, void *), size_t size,
               const void *args) {
  struct _cbk_data *p;
  char x[sizeof *p + size];
  p = (typeof(p))x;
  p->fn = fn;
  memcpy(p->data, args, size);
  pw_loop_invoke(pw_thread_loop_get_loop(core->thread_loop), _callback,
                 SPA_ID_INVALID, p, sizeof x, false, core->data);
}
