#include "input.h"
#include "core.h"
#include "map.h"
#include "vector.h"
#include <string.h>

#define INPUT_MIN_GAIN -24.0
#define INPUT_MAX_GAIN 24.0

#define RPC_EXTRACT_TYPES struct input * : _rpc_extract_obj
#define RPC_PRINT_TYPES struct input * : print_input
#include "server.h"

void print_input(FILE *fp, va_list *ap) {
  struct input *input = va_arg(*ap, typeof(input));
  fprintf(
      fp, "{\"id\":\"%s\",\"port\":[\"%s\",\"%s\"],\"gain\":%f,\"balance\":%f}",
      input->id, input->port[0], input->port[1], input->gain, input->balance);
}

static inline void _update_vol(struct input *input) {
  input->vol[0] = DB_TO_AMP(input->gain) * PAN_LINL(input->balance);
  input->vol[1] = DB_TO_AMP(input->gain) * PAN_LINR(input->balance);
}

#define PATH_NONE ""

CBK_DEFN(impl_input_new, (struct input *, input)) {
  struct data *data = cbk_data();
  if (data->buffer_size)
    input->buffer =
        realloc(input->buffer, data->buffer_size * sizeof *input->buffer);
  vec_push(&data->inputs, input);
  _update_vol(input);
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
      .port = {strdup(PATH_NONE), strdup(PATH_NONE)},

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
