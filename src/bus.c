#include "bus.h"
#include "registry.h"
#include <stdio.h>

#define RPC_EXTRACT_TYPES struct bus * : _rpc_extract_obj
#define RPC_PRINT_TYPES struct bus * : print_bus
#include "rpc.h"

void print_bus(FILE *fp, va_list *ap) {
  struct bus *bus = va_arg(*ap, typeof(bus));
  fprintf(fp,
          "{\"id\":\"%p\",\"port\":[\"%s\",\"%s\"],\"gain\":%f,\"balance\":%f}",
          bus, bus->port[0], bus->port[1], bus->gain, bus->balance);
}

#define MIN_GAIN -90.0
#define MAX_BAL 100.0
static inline void bus_update_vol(struct bus *bus) {
  double A = bus->gain > MIN_GAIN ? pow(10.0, 0.05 * bus->gain) : 0.0;
  /*
  double theta = 0.5 * M_PI * (1.0 + 0.5 * bus->balance / MAX_BAL);
  bus->vol[0] = A * cos(theta);
  bus->vol[1] = A * sin(theta);
  */
  bus->vol[0] = A * fminf(1.0, 1.0 - bus->balance / MAX_BAL);
  bus->vol[1] = A * fminf(1.0, 1.0 + bus->balance / MAX_BAL);
}

#define PATH_NONE ""

struct args_bus_new {
  struct bus *bus;
};
static int impl_bus_new(struct spa_loop *, bool, uint32_t, const void *_args,
                        size_t, void *_data) {
  struct data *data = _data;
  const struct args_bus_new *args = _args;

  struct bus *bus = args->bus;

  if (data->buffer_size)
    bus->buffer = realloc(bus->buffer, data->buffer_size * sizeof *bus->buffer);
  vec_push(&data->buses, bus);
  bus_update_vol(bus);

  return 0;
}
RPC_DEFN(bus_new) {
  struct data *data = rpc_data();

  struct bus *bus = malloc(sizeof *bus);
  *bus = (typeof(*bus)){
      .port = {strdup(PATH_NONE), strdup(PATH_NONE)},
      .buffer = malloc(0),
  };

  struct args_bus_new args = {
      .bus = bus,
  };
  pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop), impl_bus_new,
                 SPA_ID_INVALID, &args, sizeof args, false, data);

  rpc_relay("bus_new", bus);
  rpc_return(bus);
}

struct args_bus_delete {
  struct bus *bus;
};
static int impl_bus_delete(struct spa_loop *, bool, uint32_t, const void *_args,
                           size_t, void *_data) {
  struct data *data = _data;
  const struct args_bus_delete *args = _args;

  struct bus *bus = args->bus;

  vec_foreach_ref(port, data->ports) {
    for (size_t j = 0; j < 2; j++)
      if (bus->port_data[j] && bus->port_data[j] == port->buf) {
        port_disconnect(port);
        bus->port_data[j] = nullptr;
      }
  }
  for (size_t i = 0; i < 2; i++)
    free(bus->port[i]);
  free(bus->buffer);
  free(bus);

  vec_delete(&data->buses, vec_idx(data->buses, == bus));

  return 0;
}
RPC_DEFN(bus_delete, (struct bus *, bus)) {
  struct data *data = rpc_data();
  struct args_bus_delete args = {
      .bus = bus,
  };
  pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop), impl_bus_delete,
                 SPA_ID_INVALID, &args, sizeof args, false, data);
  rpc_relay("bus_delete", ((void *)bus));
  rpc_return();
}

struct args_bus_set_port {
  struct bus *bus;
  size_t idx;
  char *path;
};
static int impl_bus_set_port(struct spa_loop *, bool, uint32_t,
                             const void *_args, size_t, void *_data) {
  struct data *data = _data;
  const struct args_bus_set_port *args = _args;

  struct bus *bus = args->bus;
  size_t idx = args->idx;
  char *path = args->path;

  if (strlen(bus->port[idx])) {
    struct port *port =
        vec_bsearch_ref(data->ports, port_cmp, .path = bus->port[idx]);
    if (port) {
      port_disconnect(port);
      bus->port_data[idx] = nullptr;
    }
  }
  free(bus->port[idx]);
  struct port *port = vec_bsearch_ref(data->ports, port_cmp, .path = path);
  if (port) {
    port_connect(data, port);
    bus->port_data[idx] = port->buf;
  }
  bus->port[idx] = path;

  return 0;
}
RPC_DEFN(bus_set_port, (struct bus *, bus), (long, idx), (char *, path)) {
  struct data *data = rpc_data();
  if (!strcmp(path, bus->port[idx])) {
    free(path);
    rpc_return();
  }

  struct args_bus_set_port args = {
      .bus = bus,
      .idx = idx,
      .path = path,
  };
  pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop), impl_bus_set_port,
                 SPA_ID_INVALID, &args, sizeof args, false, data);
  rpc_relay("bus_set_port", ((void *)bus), idx, path);
  rpc_return();
}

struct args_bus_update_vol {
  struct bus *bus;
};
static int impl_bus_update_vol(struct spa_loop *, bool, uint32_t,
                               const void *_args, size_t, void *) {
  const struct args_bus_update_vol *args = _args;

  struct bus *bus = args->bus;
  bus_update_vol(bus);

  return 0;
}
RPC_DEFN(bus_set_gain, (struct bus *, bus), (double, gain)) {
  struct data *data = rpc_data();

  bus->gain = fmax(gain, MIN_GAIN);

  struct args_bus_update_vol args = {
      .bus = bus,
  };
  pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop),
                 impl_bus_update_vol, SPA_ID_INVALID, &args, sizeof args, false,
                 nullptr);

  rpc_relay("bus_set_gain", ((void *)bus), bus->gain);
  if (gain != bus->gain)
    rpc_return(bus->gain);
  else
    rpc_return();
}
RPC_DEFN(bus_set_balance, (struct bus *, bus), (double, balance)) {
  struct data *data = rpc_data();

  bus->balance = fmax(-MAX_BAL, fmin(MAX_BAL, balance));

  struct args_bus_update_vol args = {
      .bus = bus,
  };
  pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop),
                 impl_bus_update_vol, SPA_ID_INVALID, &args, sizeof args, false,
                 nullptr);

  rpc_relay("bus_set_balance", ((void *)bus), bus->balance);
  if (balance != bus->balance)
    rpc_return(bus->balance);
  else
    rpc_return();
}
