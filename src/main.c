#include "core.h"
#include "input.h"
#include "map.h"
#include "mixer.h"
#include "mixup.h"
#include <math.h>
#include <string.h>

#define RPC_PRINT_TYPES                                                        \
  struct input * : print_input,                                                \
                   struct mixer * : print_mixer,                               \
                                    struct port_info : print_port
#include "server.h"

static void port_new(void *_data, struct port_info info) {
  struct data *data = _data;
  vec_foreach(input, data->inputs) {
    for (size_t i = 0; i < 2; i++)
      if (!strcmp(info.path, input->port[i])) {
        struct port *port = core_get_port(data->core, info.path);
        port_connect(port);
        input->port_data[i] = port_buf(port);
      }
  }
  vec_foreach(mixer, data->mixers) {
    for (size_t i = 0; i < 2; i++)
      if (!strcmp(info.path, mixer->port[i])) {
        struct port *port = core_get_port(data->core, info.path);
        port_connect(port);
        mixer->port_data[i] = port_buf(port);
      }
  }
  rpc_broadcast(data->srv, "port_new", info);
}

static void port_delete(void *_data, const char *path) {
  struct data *data = _data;
  vec_foreach(input, data->inputs) {
    for (size_t i = 0; i < 2; i++)
      if (!strcmp(path, input->port[i]))
        input->port_data[i] = nullptr;
  }
  vec_foreach(mixer, data->mixers) {
    for (size_t i = 0; i < 2; i++)
      if (!strcmp(path, mixer->port[i]))
        mixer->port_data[i] = nullptr;
  }
  rpc_broadcast(data->srv, "port_delete", path);
}

static void process(void *_data) {
  struct data *data = _data;
  vec_foreach(input, data->inputs) {
    if (input->port_data[0] && *input->port_data[0])
      for (size_t i = 0; i < data->buffer_size; i++)
        input->buffer[i][0] = (*input->port_data[0])[i];
    else
      for (size_t t = 0; t < data->buffer_size; t++)
        input->buffer[t][0] = 0.0f;
    if (input->port_data[1] && *input->port_data[1])
      for (size_t t = 0; t < data->buffer_size; t++)
        input->buffer[t][1] = (*input->port_data[1])[t];
    else
      for (size_t t = 0; t < data->buffer_size; t++)
        input->buffer[t][1] = input->buffer[t][0];
    for (size_t t = 0; t < data->buffer_size; t++) {
      float2 y = {input->buffer[t][0], input->buffer[t][1]}, *p0, *p1;
      for (size_t s = 0; s < INPUT_EQ_STAGES; s++) {
        float2 x = {y[0], y[1]};

        p0 = input->eq_buffer[s], p1 = p0 + 1;
        for (size_t j = 0; j < 2; j++)
          y[j] = input->eq_coeffs[s].b[0] * x[j];
        for (size_t j = 0; j < 2; j++)
          y[j] = fmaf(input->eq_coeffs[s].b[1], (*p0)[j], y[j]);
        for (size_t j = 0; j < 2; j++)
          y[j] = fmaf(input->eq_coeffs[s].b[2], (*p1)[j], y[j]);
        for (size_t j = 0; j < 2; j++)
          (*p1)[j] = (*p0)[j], (*p0)[j] = x[j];

        p0 = input->eq_buffer[s + 1], p1 = p0 + 1;
        for (size_t j = 0; j < 2; j++)
          y[j] = fmaf(-input->eq_coeffs[s].a[0], (*p0)[j], y[j]);
        for (size_t j = 0; j < 2; j++)
          y[j] = fmaf(-input->eq_coeffs[s].a[1], (*p1)[j], y[j]);
      }
      for (size_t j = 0; j < 2; j++)
        (*p1)[j] = (*p0)[j], input->buffer[t][j] = (*p0)[j] = y[j];
    }
    for (size_t t = 0; t < data->buffer_size; t++)
      for (size_t j = 0; j < 2; j++)
        input->buffer[t][j] *= input->vol[j];
  }
  vec_foreach(mixer, data->mixers) {
    for (size_t c = 0; c < mixer->channels.n; c++) {
      if (mixer->sources[c])
        for (size_t t = 0; t < data->buffer_size; t++)
          for (size_t j = 0; j < 2; j++)
            mixer->buffer[mixer->channels.n * t + c][j] =
                (*mixer->sources[c])[t][j];
      else
        for (size_t t = 0; t < data->buffer_size; t++)
          for (size_t j = 0; j < 2; j++)
            mixer->buffer[mixer->channels.n * t + c][j] = 0.0f;
    }
    for (size_t t = 0; t < data->buffer_size; t++)
      for (size_t c = 0; c < mixer->channels.n; c++)
        for (size_t j = 0; j < 2; j++)
          if (mixer->port_data[j] && *mixer->port_data[j])
            (*mixer->port_data[j])[t] = fmaf(
                mixer->vols[c][j], mixer->buffer[mixer->channels.n * t + c][j],
                (*mixer->port_data[j])[t]);
  }
}

static void buffer_size(void *_data, size_t buffer_size) {
  struct data *data = _data;
  data->buffer_size = buffer_size;
  vec_foreach(input, data->inputs) {
    input->buffer = realloc(input->buffer, buffer_size * sizeof *input->buffer);
  }
  vec_foreach(mixer, data->mixers) {
    mixer->buffer = realloc(mixer->buffer, mixer->channels.n * buffer_size *
                                               sizeof *mixer->buffer);
  }
}
static void sample_rate(void *_data, double sample_rate) {
  struct data *data = _data;
  data->sample_rate = sample_rate;
  if (sample_rate)
    vec_foreach(input, data->inputs) {
      for (size_t i = 0; i < INPUT_EQ_STAGES; i++)
        input_update_eq(input, i, sample_rate);
    }
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
  vec_foreach(input, data->inputs) { rpc_reply("input_new", input); }
  vec_foreach(mixer, data->mixers) { rpc_reply("mixer_new", mixer); }
  rpc_reply("done");
  rpc_return();
}

int main(int, char **) {
  int ret = EXIT_FAILURE;

  struct data data = {};

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

  srv_reg(data.srv, input_new, "input_new");
  srv_reg(data.srv, input_delete, "input_delete");
  srv_reg(data.srv, input_set_index, "input_set_index");
  srv_reg(data.srv, input_set_name, "input_set_name");
  srv_reg(data.srv, input_set_port, "input_set_port");
  srv_reg(data.srv, input_set_gain, "input_set_gain");
  srv_reg(data.srv, input_set_balance, "input_set_balance");
  srv_reg(data.srv, input_set_eq_freq, "input_set_eq_freq");
  srv_reg(data.srv, input_set_eq_quality, "input_set_eq_quality");
  srv_reg(data.srv, input_set_eq_gain, "input_set_eq_gain");

  srv_reg(data.srv, mixer_new, "mixer_new");
  srv_reg(data.srv, mixer_delete, "mixer_delete");
  srv_reg(data.srv, mixer_set_index, "mixer_set_index");
  srv_reg(data.srv, mixer_set_name, "mixer_set_name");
  srv_reg(data.srv, mixer_set_port, "mixer_set_port");
  srv_reg(data.srv, mixer_set_master, "mixer_set_master");

  srv_reg(data.srv, channel_new, "channel_new");
  srv_reg(data.srv, channel_delete, "channel_delete");
  srv_reg(data.srv, channel_set_index, "channel_set_index");
  srv_reg(data.srv, channel_set_name, "channel_set_name");
  srv_reg(data.srv, channel_set_src, "channel_set_src");
  srv_reg(data.srv, channel_set_gain, "channel_set_gain");
  srv_reg(data.srv, channel_set_balance, "channel_set_balance");
  srv_reg(data.srv, channel_set_mute, "channel_set_mute");

  data.map = map_new(sizeof(struct obj));

  vec_init(&data.inputs);
  vec_init(&data.mixers);

  core_start(data.core);

  printf("Welcome to mixup!\n");

  while (!core_quit(data.core))
    srv_run(data.srv);

  core_stop(data.core);

  ret = EXIT_SUCCESS;

err_srv:
  srv_del(data.srv);

  vec_foreach(mixer, data.mixers) {
    for (size_t i = 0; i < 2; i++) {
      mixer->port_data[i] = nullptr;
      free(mixer->port[i]);
    }
    free(mixer->buffer);
    free(mixer);
  }
  vec_free(&data.mixers);

  vec_foreach(input, data.inputs) {
    for (size_t i = 0; i < 2; i++) {
      input->port_data[i] = nullptr;
      free(input->port[i]);
    }
    free(input->buffer);
    free(input);
  }
  vec_free(&data.inputs);

  map_delete(data.map);

err_core:
  core_del(data.core);

  return ret;
}
