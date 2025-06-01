#include "bus.h"
#include "core.h"
#include "mixer.h"
#include "mixup.h"
#include <string.h>

#define RPC_PRINT_TYPES                                                        \
  struct bus * : print_bus,                                                    \
                 struct mixer * : print_mixer,                                 \
                                  struct port_info : print_port
#include "rpc.h"

static void port_new(void *_data, struct port_info info) {
  struct data *data = _data;
  vec_foreach(bus, data->buses) {
    for (size_t j = 0; j < 2; j++)
      if (!strcmp(info.path, bus->port[j])) {
        struct port *port = core_get_port(data->core, info.path);
        port_connect(port);
        bus->port_data[j] = port_buf(port);
      }
  }
  rpc_broadcast(data->srv, "port_new", info);
}

static void port_delete(void *_data, const char *path) {
  struct data *data = _data;
  vec_foreach(bus, data->buses) {
    for (size_t j = 0; j < 2; j++)
      if (!strcmp(path, bus->port[j]))
        bus->port_data[j] = nullptr;
  }
  rpc_broadcast(data->srv, "port_delete", path);
}

static void process(void *_data) {
  struct data *data = _data;
  vec_foreach(bus, data->buses) {
    if (bus->port_data[0])
      for (size_t i = 0; i < data->buffer_size; i++)
        bus->buffer[i][0] = (*bus->port_data[0])[i];
    else
      for (size_t i = 0; i < data->buffer_size; i++)
        bus->buffer[i][0] = 0.0f;
    if (bus->port_data[1])
      for (size_t i = 0; i < data->buffer_size; i++)
        bus->buffer[i][1] = (*bus->port_data[1])[i];
    else
      for (size_t i = 0; i < data->buffer_size; i++)
        bus->buffer[i][1] = bus->buffer[i][0];
    for (size_t i = 0; i < data->buffer_size; i++)
      for (size_t j = 0; j < 2; j++)
        bus->buffer[i][j] *= bus->vol[j];
  }
  vec_foreach(mixer, data->mixers) {
    for (size_t c = 0; c < mixer->channels.n; c++) {
      if (mixer->sources[c])
        for (size_t i = 0; i < data->buffer_size; i++)
          for (size_t j = 0; j < 2; j++)
            mixer->buffer[mixer->channels.n * i + c][j] =
                (*mixer->sources[c])[i][j];
      else
        for (size_t i = 0; i < data->buffer_size; i++)
          for (size_t j = 0; j < 2; j++)
            mixer->buffer[mixer->channels.n * i + c][j] = 0.0f;
    }
    for (size_t i = 0; i < data->buffer_size; i++)
      for (size_t c = 0; c < mixer->channels.n; c++)
        for (size_t j = 0; j < 2; j++)
          if (mixer->port_data[j])
            (*mixer->port_data[j])[i] +=
                mixer->vols[c][j] * mixer->buffer[mixer->channels.n * i + c][j];
  }
}

static void buffer_size(void *_data, size_t buffer_size) {
  struct data *data = _data;
  data->buffer_size = buffer_size;
  vec_foreach(bus, data->buses) {
    bus->buffer = realloc(bus->buffer, buffer_size * sizeof *bus->buffer);
  }
  vec_foreach(mixer, data->mixers) {
    mixer->buffer = realloc(mixer->buffer, mixer->channels.n * buffer_size *
                                               sizeof *mixer->buffer);
  }
}
static void sample_rate(void *_data, double sample_rate) {
  struct data *data = _data;
  data->sample_rate = sample_rate;
}

RPC_DEFN(init) {
  struct data *data = rpc_data();
  rpc_init();
  struct port_info *port = core_ports(data->core);
  while (port) {
    rpc_reply("port_new", (*port));
    free(port->path);
    free(port->node);
    struct port_info *next = port->next;
    free(port);
    port = next;
  }
  vec_foreach(bus, data->buses) { rpc_reply("bus_new", bus); }
  vec_foreach(mixer, data->mixers) { rpc_reply("mixer_new", mixer); }
  rpc_return();
}

int main(int, char **) {
  int ret = EXIT_FAILURE;

  struct data data = {};

  vec_init(&data.buses);
  vec_init(&data.mixers);

  if (!(data.core = core_new(&data, (struct core_events){
                                        .port_new = port_new,
                                        .port_delete = port_delete,
                                        .process = process,
                                        .buffer_size = buffer_size,
                                        .sample_rate = sample_rate,
                                    }))) {
    printf("Failed to initialize audio\n");
    goto err_core;
  }
  if (!(data.srv = srv_new(&data))) {
    printf("Failed to initialize server\n");
    goto err_srv;
  }

  srv_reg(data.srv, init, "init");

  srv_reg(data.srv, bus_new, "bus_new");
  srv_reg(data.srv, bus_delete, "bus_delete");
  srv_reg(data.srv, bus_set_port, "bus_set_port");
  srv_reg(data.srv, bus_set_gain, "bus_set_gain");
  srv_reg(data.srv, bus_set_balance, "bus_set_balance");

  srv_reg(data.srv, mixer_new, "mixer_new");
  srv_reg(data.srv, mixer_delete, "mixer_delete");
  srv_reg(data.srv, mixer_set_port, "mixer_set_port");
  srv_reg(data.srv, mixer_set_master, "mixer_set_master");

  srv_reg(data.srv, channel_new, "channel_new");
  srv_reg(data.srv, channel_delete, "channel_delete");
  srv_reg(data.srv, channel_set_src, "channel_set_src");
  srv_reg(data.srv, channel_set_gain, "channel_set_gain");
  srv_reg(data.srv, channel_set_balance, "channel_set_balance");

  core_start(data.core);

  printf("Welcome to mixup!\n");

  while (!core_quit(data.core))
    srv_run(data.srv);

  core_stop(data.core);

  ret = EXIT_SUCCESS;

err_srv:
  srv_del(data.srv);

  vec_foreach(mixer, data.mixers) {
    for (size_t j = 0; j < 2; j++)
      free(mixer->port[j]);
    free(mixer->buffer);
    free(mixer);
  }
  vec_free(&data.mixers);

  vec_foreach(bus, data.buses) {
    for (size_t j = 0; j < 2; j++)
      free(bus->port[j]);
    free(bus->buffer);
    free(bus);
  }
  vec_free(&data.buses);

err_core:
  core_del(data.core);

  return ret;
}
