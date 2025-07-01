#include "input.h"
#include "core.h"
#include "map.h"
#include "vector.h"
#include <string.h>

#define INPUT_MIN_GAIN -24.0
#define INPUT_MAX_GAIN 24.0
#define EQ_MIN_FREQ 20.0
#define EQ_MAX_FREQ 20000.0
#define EQ_MIN_GAIN -18.0
#define EQ_MAX_GAIN 18.0

#define RPC_EXTRACT_TYPES struct input * : _rpc_extract_obj
#define RPC_PRINT_TYPES struct input * : print_input
#include "server.h"

void print_input(FILE *fp, va_list *ap) {
  struct input *input = va_arg(*ap, typeof(input));
  fprintf(fp,
          "{\"id\":\"%s\",\"name\":\"%s\",\"port\":[\"%s\",\"%s\"],\"gain\":%f,"
          "\"balance\":%f,\"eq\":[",
          input->id, input->name, input->port[0], input->port[1], input->gain,
          input->balance);
  for (size_t i = 0; i < INPUT_EQ_STAGES; i++) {
    if (i)
      fprintf(fp, ",");
    fprintf(fp, "{\"freq\":%f,\"quality\":%f,\"gain\":%f}", input->eq[i].freq,
            input->eq[i].quality, input->eq[i].gain);
  }
  fprintf(fp, "]}");
}

static inline void _update_vol(struct input *input) {
  input->vol[0] = DB_TO_AMP(input->gain) * PAN_LINL(input->balance);
  input->vol[1] = DB_TO_AMP(input->gain) * PAN_LINR(input->balance);
}
void input_update_eq(struct input *input, size_t stage, double rate) {
  // from the Audio EQ cookbook https://www.w3.org/TR/audio-eq-cookbook/
  double A = DB_TO_AMP(0.5 * input->eq[stage].gain);
  double w0 = 2.0 * M_PI * input->eq[stage].freq / rate;
  double alpha = 0.5 * sin(w0) / input->eq[stage].quality, cc = cos(w0);
  double a[3], b[3];
  if (!stage) // high pass, H(s) = s2 / (s2 + s / Q + 1)
    a[0] = 1.0 + alpha, a[1] = -2.0 * cc, a[2] = 1.0 - alpha, b[1] = -(1 + cc),
    b[0] = b[2] = -0.5 * b[1];
  else if (stage ==
           INPUT_EQ_STAGES - 1) // low pass, H(s) = 1 / (s2 + s / Q + 1)
    a[0] = 1.0 + alpha, a[1] = -2.0 * cc, a[2] = 1.0 - alpha, b[1] = 1 - cc,
    b[0] = b[2] = 0.5 * b[1];
  else // EQ, H(s) = (s2 + As / Q + 1) / (s2 + s / AQ + 2)
    a[0] = 1.0 + alpha / A, a[1] = -2.0 * cc, a[2] = 1.0 - alpha / A,
    b[0] = 1 + alpha * A, b[1] = a[1], b[2] = 1 - alpha * A;
  for (size_t i = 0; i < 3; i++) {
    if (i)
      input->eq_coeffs[stage].a[i - 1] = a[i] / a[0];
    input->eq_coeffs[stage].b[i] = b[i] / a[0];
  }
}

#define PATH_NONE ""

CBK_DEFN(impl_input_new, (struct input *, input)) {
  struct data *data = cbk_data();
  if (data->buffer_size)
    input->buffer =
        realloc(input->buffer, data->buffer_size * sizeof *input->buffer);
  vec_push(&data->inputs, input);
  _update_vol(input);
  if (data->sample_rate)
    for (size_t i = 0; i < INPUT_EQ_STAGES; i++)
      input_update_eq(input, i, data->sample_rate);
}
RPC_DEFN(input_new) {
  struct data *data = rpc_data();
  struct input *input = malloc(sizeof *input);
  *input = (typeof(*input)){
      .id = map_insert(data->map,
                       &(struct obj){
                           .type = MIXUP_INPUT,
                           .ptr = input,
                       }),
      .name = strdup("# input"),
      .port = {strdup(PATH_NONE), strdup(PATH_NONE)},
      .eq =
          {
              {.freq = EQ_MIN_FREQ, .quality = sqrt(0.5)},
              {.freq = 200.0, .quality = sqrt(0.5)},
              {.freq = 1000.0, .quality = sqrt(0.5)},
              {.freq = 5000.0, .quality = sqrt(0.5)},
              {.freq = EQ_MAX_FREQ, .quality = sqrt(0.5)},
          },

      .buffer = malloc(0),
  };
  core_cbk(data->core, impl_input_new, input);
  rpc_relay("input_new", input);
  rpc_return(input);
}

CBK_DEFN(impl_input_delete, (struct input *, input)) {
  struct data *data = cbk_data();
  for (size_t j = 0; j < 2; j++) {
    struct port *port = core_get_port(data->core, input->port[j]);
    if (port)
      port_disconnect(port);
  }
  free(input->id);
  for (size_t i = 0; i < 2; i++)
    free(input->port[i]);
  free(input->buffer);
  free(input);

  vec_delete(&data->inputs, vec_idx(data->inputs, == input));
}
RPC_DEFN(input_delete, (char *, id)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  core_cbk(data->core, impl_input_delete, input);
  rpc_relay("input_delete", id);
  map_remove(data->map, id);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_input_set_index, (struct input *, input), (size_t, idx)) {
  struct data *data = cbk_data();
  vec_delete(&data->inputs, vec_idx(data->inputs, == input));
  vec_insert(&data->inputs, idx, input);
}
RPC_DEFN(input_set_index, (char *, id), (long, idx)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  core_cbk(data->core, impl_input_set_index, input, idx);
  rpc_relay("input_set_index", id, idx);
  free(id);
  rpc_return();
}

RPC_DEFN(input_set_name, (char *, id), (char *, name)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  free(input->name);
  input->name = name;
  rpc_relay("input_set_name", id, name);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_input_set_port, (struct input *, input), (size_t, idx),
         (char *, path)) {
  struct data *data = cbk_data();

  if (strlen(input->port[idx])) {
    struct port *port = core_get_port(data->core, input->port[idx]);
    if (port) {
      port_disconnect(port);
      input->port_data[idx] = nullptr;
    }
  }
  free(input->port[idx]);
  struct port *port = core_get_port(data->core, path);
  if (port) {
    port_connect(port);
    input->port_data[idx] = port_buf(port);
  }
  input->port[idx] = path;
}
RPC_DEFN(input_set_port, (char *, id), (long, idx), (char *, path)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(path);
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  if (!strcmp(path, input->port[idx])) {
    free(path);
    free(id);
    rpc_return();
  }
  core_cbk(data->core, impl_input_set_port, input, idx, path);
  rpc_relay("input_set_port", id, idx, path);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_input_update_vol, (struct input *, input)) { _update_vol(input); }
RPC_DEFN(input_set_gain, (char *, id), (double, gain)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  input->gain = fmin(fmax(gain, INPUT_MIN_GAIN), INPUT_MAX_GAIN);
  core_cbk(data->core, impl_input_update_vol, input);
  rpc_relay("input_set_gain", id, input->gain);
  free(id);
  if (gain != input->gain)
    rpc_return(input->gain);
  else
    rpc_return();
}
RPC_DEFN(input_set_balance, (char *, id), (double, balance)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  input->balance = fmax(-MAX_BAL, fmin(MAX_BAL, balance));
  core_cbk(data->core, impl_input_update_vol, input);
  rpc_relay("input_set_balance", id, input->balance);
  free(id);
  if (balance != input->balance)
    rpc_return(input->balance);
  else
    rpc_return();
}

CBK_DEFN(impl_input_update_eq, (struct input *, input), (size_t, stage)) {
  struct data *data = cbk_data();
  if (data->sample_rate)
    input_update_eq(input, stage, data->sample_rate);
}
RPC_DEFN(input_set_eq_freq, (char *, id), (long, stage), (double, freq)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  input->eq[stage].freq = fmax(EQ_MIN_FREQ, fmin(EQ_MAX_FREQ, freq));
  core_cbk(data->core, impl_input_update_eq, input, stage);
  rpc_relay("input_set_eq_freq", id, stage, input->eq[stage].freq);
  free(id);
  if (freq != input->eq[stage].freq)
    rpc_return(input->eq[stage].freq);
  else
    rpc_return();
}
RPC_DEFN(input_set_eq_quality, (char *, id), (long, stage), (double, quality)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  input->eq[stage].quality = fmax(0.1, fmin(10.0, quality));
  core_cbk(data->core, impl_input_update_eq, input, stage);
  rpc_relay("input_set_eq_quality", id, stage, input->eq[stage].quality);
  free(id);
  if (quality != input->eq[stage].quality)
    rpc_return(input->eq[stage].quality);
  else
    rpc_return();
}
RPC_DEFN(input_set_eq_gain, (char *, id), (long, stage), (double, gain)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_INPUT) {
    free(id);
    rpc_err(-32602, "Provided object is not an input");
  }
  struct input *input = obj->ptr;
  input->eq[stage].gain = fmax(EQ_MIN_GAIN, fmin(EQ_MAX_GAIN, gain));
  core_cbk(data->core, impl_input_update_eq, input, stage);
  rpc_relay("input_set_eq_gain", id, stage, input->eq[stage].gain);
  free(id);
  if (gain != input->eq[stage].gain)
    rpc_return(input->eq[stage].gain);
  else
    rpc_return();
}
