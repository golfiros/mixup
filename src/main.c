#include "bus.h"
#include "mixup.h"
#include "registry.h"

#define RPC_PRINT_TYPES struct bus * : print_bus, struct port : print_port
#include "rpc.h"

static int impl_buffer_size(struct spa_loop *, bool, uint32_t,
                            const void *_args, size_t, void *_data) {
  struct data *data = _data;
  size_t buffer_size = *(typeof(&buffer_size))_args;

  data->buffer_size = buffer_size;
  vec_foreach(bus, data->buses) {
    bus->buffer = realloc(bus->buffer, buffer_size * sizeof *bus->buffer);
  }
  data->event &= ~MIXUP_EVENT_BUFFER_SIZE;

  return 0;
}
static int impl_sample_rate(struct spa_loop *, bool, uint32_t,
                            const void *_args, size_t, void *_data) {
  struct data *data = _data;
  double sample_rate = *(typeof(&sample_rate))_args;

  data->sample_rate = sample_rate;

  data->event &= ~MIXUP_EVENT_SAMPLE_RATE;
  return 0;
}
static void filter_process(void *_data, struct spa_io_position *pos) {
  struct data *data = _data;
  size_t buffer_size = pos->clock.duration;
  vec_foreach(port, data->ports) {
    if (port.buf) {
      *port.buf = pw_filter_get_dsp_buffer(port.buf, buffer_size);
      if (!port.input)
        for (size_t t = 0; t < buffer_size; t++)
          (*port.buf)[t] = 0.0f;
    }
  }
  float *out[2];
  for (size_t i = 0; i < 2; i++) {
    out[i] = pw_filter_get_dsp_buffer(data->output[i], buffer_size);
    if (out[i])
      for (size_t t = 0; t < buffer_size; t++)
        out[i][t] = 0.0;
  }

  if (buffer_size != data->buffer_size) {
    data->event = data->event | MIXUP_EVENT_BUFFER_SIZE;
    pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop), impl_buffer_size,
                   SPA_ID_INVALID, &buffer_size, sizeof buffer_size, false,
                   data);
  }
  double sample_rate = pos->clock.rate.denom;
  sample_rate /= pos->clock.rate.num;
  if (sample_rate != data->sample_rate) {
    data->event = data->event | MIXUP_EVENT_SAMPLE_RATE;
    pw_loop_invoke(pw_thread_loop_get_loop(data->thread_loop), impl_sample_rate,
                   SPA_ID_INVALID, &sample_rate, sizeof sample_rate, false,
                   data);
  }
  if (data->event)
    return;
  vec_foreach(bus, data->buses) {
    if (bus->port_data[0])
      for (size_t i = 0; i < buffer_size; i++)
        bus->buffer[i][0] = (*bus->port_data[0])[i];
    else
      for (size_t i = 0; i < buffer_size; i++)
        bus->buffer[i][0] = 0;
    if (bus->port_data[1])
      for (size_t i = 0; i < buffer_size; i++)
        bus->buffer[i][1] = (*bus->port_data[1])[i];
    else
      for (size_t i = 0; i < buffer_size; i++)
        bus->buffer[i][1] = bus->buffer[i][0];
    for (size_t i = 0; i < buffer_size; i++)
      for (size_t j = 0; j < 2; j++)
        bus->buffer[i][j] *= bus->vol[j];
    for (size_t i = 0; i < 2; i++)
      if (out[i])
        for (size_t t = 0; t < buffer_size; t++)
          out[i][t] += bus->buffer[t][i];
  }
}
static const struct pw_filter_events filter_events = {
    PW_VERSION_FILTER_EVENTS,
    .process = filter_process,
};

RPC_DEFN(init) {
  struct data *data = rpc_data();
  rpc_init();
  pw_thread_loop_lock(data->thread_loop);
  vec_foreach(port, data->ports) { rpc_reply("port_new", port); }
  pw_thread_loop_unlock(data->thread_loop);
  vec_foreach(bus, data->buses) { rpc_reply("bus_new", bus); }
  rpc_return();
}

void loop_quit(void *_data, int) {
  struct data *data = _data;
  data->quit = true;
}

int main(int, char **) {
  int ret = EXIT_FAILURE;

  struct data data = {};
  spa_zero(data.listener);

  vec_init(&data.nodes);
  vec_init(&data.ports);
  vec_init(&data.buses);

  pw_init(nullptr, nullptr);
  if (!(data.thread_loop = pw_thread_loop_new(MIXUP, nullptr))) {
    printf("Failed to initialize pipewire\n");
    goto err_loop;
  }

  pw_thread_loop_lock(data.thread_loop);

  struct pw_loop *loop = pw_thread_loop_get_loop(data.thread_loop);
  pw_loop_add_signal(loop, SIGINT, loop_quit, &data);
  pw_loop_add_signal(loop, SIGTERM, loop_quit, &data);

  if (!(data.filter = pw_filter_new_simple(
            loop, MIXUP,
            pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                              "Filter", PW_KEY_MEDIA_ROLE, "DSP", nullptr),
            &filter_events, &data))) {
    printf("Failed to allocate pipewire filter\n");
    goto err_filter;
  }

  for (size_t i = 0; i < 2; i++)
    data.output[i] = pw_filter_add_port(
        data.filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
        pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                          nullptr),
        nullptr, 0);

  if (pw_filter_connect(data.filter, PW_FILTER_FLAG_RT_PROCESS, nullptr, 0) <
          0 ||
      !(data.registry = pw_core_get_registry(pw_filter_get_core(data.filter),
                                             PW_VERSION_REGISTRY, 0))) {
    printf("Failed to connect to pipewire\n");
    goto err_conn;
  }
  if (pw_registry_add_listener(data.registry, &data.listener, &registry_events,
                               &data) < 0) {
    printf("Failed to add registry listener\n");
    goto err_list;
  }

  data.srv = srv_new();
  if (!data.srv) {
    printf("Failed to initialize server");
    goto err_srv;
  }

  srv_reg(data.srv, init, "init", &data);

  srv_reg(data.srv, bus_new, "bus_new", &data);
  srv_reg(data.srv, bus_delete, "bus_delete", &data);
  srv_reg(data.srv, bus_set_port, "bus_set_port", &data);
  srv_reg(data.srv, bus_set_gain, "bus_set_gain", &data);
  srv_reg(data.srv, bus_set_balance, "bus_set_balance", &data);

  pw_thread_loop_start(data.thread_loop);
  pw_thread_loop_unlock(data.thread_loop);

  while ((data.filter_id = pw_filter_get_node_id(data.filter)) ==
         SPA_ID_INVALID)
    ;

  printf("Welcome to mixup!\n");

  while (!data.quit)
    srv_run(data.srv);
  pw_thread_loop_stop(data.thread_loop);

  ret = EXIT_SUCCESS;

err_srv:
  srv_del(data.srv);

  vec_foreach(bus, data.buses) {
    for (size_t j = 0; j < 2; j++)
      free(bus->port[j]);
    free(bus->buffer);
    free(bus);
  }
  vec_free(&data.buses);

  vec_foreach(port, data.ports) {
    while (port.conn)
      port_disconnect(&port);
    free(port.path);
  }
  vec_free(&data.ports);

  vec_foreach(node, data.nodes) { free(node.name); }
  vec_free(&data.nodes);
  spa_hook_remove(&data.listener);
err_list:
  pw_proxy_destroy((struct pw_proxy *)data.registry);
err_conn:
  pw_filter_destroy(data.filter);
err_filter:
  pw_thread_loop_destroy(data.thread_loop);
err_loop:
  return ret;
}
