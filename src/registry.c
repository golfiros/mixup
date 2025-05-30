#include "registry.h"
#include "bus.h"

#define RPC_PRINT_TYPES struct port : print_port
#include "rpc.h"

void print_port(FILE *fp, va_list *ap) {
  struct port port = va_arg(*ap, typeof(port));
  fprintf(fp, "{\"path\":\"%s\",\"node\":\"%s\",\"input\":\"%s\"}", port.path,
          port.node, port.input ? "true" : "false");
}

void port_connect(struct data *data, struct port *port) {
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
    port->buf = pw_filter_add_port(data->filter, PW_DIRECTION_INPUT,
                                   PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                                   sizeof *port->buf, port_props, nullptr, 0);
    pw_properties_setf(link_props, PW_KEY_LINK_OUTPUT_PORT, "%u", port->id);
    pw_properties_setf(link_props, PW_KEY_LINK_INPUT_NODE, "%u",
                       data->filter_id);
  } else {
    port->buf = pw_filter_add_port(data->filter, PW_DIRECTION_OUTPUT,
                                   PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                                   sizeof *port->buf, port_props, nullptr, 0);
    pw_properties_setf(link_props, PW_KEY_LINK_INPUT_PORT, "%u", port->id);
    pw_properties_setf(link_props, PW_KEY_LINK_OUTPUT_NODE, "%u",
                       data->filter_id);
  }
  port->link = pw_core_create_object(pw_filter_get_core(data->filter),
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

static void registry_global(void *_data, uint32_t id, uint32_t,
                            const char *type, uint32_t,
                            const struct spa_dict *props) {
  struct data *data = _data;
  const char *value;
  if (!strcmp(type, PW_TYPE_INTERFACE_Node)) {
    if (!(value = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION)))
      value = spa_dict_lookup(props, PW_KEY_NODE_NAME);

    struct node node = {
        .id = id,
        .name = strdup(value),
    };
    vec_push(&data->nodes, node);
    vec_qsort(&data->nodes, node_cmp);
    qsort(data->nodes.data, data->nodes.n, sizeof node, node_cmp);
  } else if (!strcmp(type, PW_TYPE_INTERFACE_Port)) {
    uint32_t node_id;
    if (!(value = spa_dict_lookup(props, PW_KEY_NODE_ID)) ||
        (node_id = atoi(value)) == data->filter_id ||
        ((value = spa_dict_lookup(props, PW_KEY_PORT_MONITOR)) &&
         !strcmp(value, "true")))
      return;

    struct node node = vec_bsearch(data->nodes, node_cmp, .id = node_id);

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
    };
    vec_foreach(bus, data->buses) {
      for (size_t j = 0; j < 2; j++)
        if (!strcmp(path, bus->port[j])) {
          port_connect(data, &port);
          bus->port_data[j] = port.buf;
        }
    }

    vec_push(&data->ports, port);
    vec_qsort(&data->ports, port_cmp);

    rpc_broadcast(data->srv, "port_new", port);
  }
}
static void registry_global_remove(void *_data, uint32_t id) {
  struct data *data = _data;
  size_t idx;
  if ((idx = vec_idx(data->nodes, .id == id)) < data->nodes.n) {
    struct node node = vec_delete(&data->nodes, idx);
    free(node.name);
  }
  if ((idx = vec_idx(data->ports, .id == id)) < data->ports.n) {
    struct port port = vec_delete(&data->ports, idx);

    vec_foreach(bus, data->buses) {
      for (size_t j = 0; j < 2; j++)
        if (!strcmp(port.path, bus->port[j]))
          bus->port_data[j] = nullptr;
    }
    while (port.conn)
      port_disconnect(&port);

    rpc_broadcast(data->srv, "port_delete", port.path);
  }
}
const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = registry_global,
    .global_remove = registry_global_remove,
};
