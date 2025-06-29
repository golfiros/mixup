#include "mixer.h"
#include "core.h"
#include "input.h"
#include "map.h"
#include "vector.h"
#include <string.h>

#define RPC_PRINT_TYPES                                                        \
  struct mixer * : print_mixer, struct channel * : print_channel
#include "server.h"

void print_mixer(FILE *fp, va_list *ap) {
  struct mixer *mixer = va_arg(*ap, typeof(mixer));
  fprintf(fp,
          "{\"id\":\"%s\",\"name\":\"%s\",\"port\":[\"%s\",\"%s\"],\"master\":%"
          "f,\"channels\":[",
          mixer->id, mixer->name, mixer->port[0], mixer->port[1],
          mixer->master);
  for (size_t c = 0; c < mixer->channels.n; c++) {
    struct channel *channel = mixer->channels.data[c];
    if (c)
      fprintf(fp, ",");
    fprint_arg(fp, channel);
  }
  fprintf(fp, "]}");
}

void print_channel(FILE *fp, va_list *ap) {
  struct channel *channel = va_arg(*ap, typeof(channel));
  fprintf(fp,
          "{\"id\":\"%s\",\"name\":\"%s\",\"src\":\"%s\",\"gain\":%f,"
          "\"balance\":%f,\"mute\":%s}",
          channel->id, channel->name, channel->src, channel->gain,
          channel->balance, channel->mute ? "true" : "false");
}

static inline void _update_vol(struct mixer *mixer, size_t c) {
  const struct channel *channel = mixer->channels.data[c];
  mixer->vols[c][0] = channel->mute ? 0.0f
                                    : DB_TO_AMP(mixer->master) *
                                          DB_TO_AMP(channel->gain) *
                                          PAN_POWL(channel->balance);
  mixer->vols[c][1] = channel->mute ? 0.0f
                                    : DB_TO_AMP(mixer->master) *
                                          DB_TO_AMP(channel->gain) *
                                          PAN_POWR(channel->balance);
}

#define PATH_NONE ""

CBK_DEFN(impl_mixer_new, (struct mixer *, mixer)) {
  struct data *data = cbk_data();
  vec_push(&data->mixers, mixer);
}
RPC_DEFN(mixer_new) {
  struct data *data = rpc_data();
  struct mixer *mixer = malloc(sizeof *mixer);
  *mixer = (typeof(*mixer)){
      .id = map_insert(data->map,
                       &(struct obj){
                           .type = MIXUP_MIXER,
                           .ptr = mixer,
                       }),
      .name = strdup("# mixer"),
      .port = {strdup(PATH_NONE), strdup(PATH_NONE)},

      .buffer = malloc(0),
  };
  vec_init(&mixer->channels);
  core_cbk(data->core, impl_mixer_new, mixer);
  rpc_relay("mixer_new", mixer);
  rpc_return(mixer);
}

CBK_DEFN(impl_mixer_delete, (struct mixer *, mixer)) {
  struct data *data = cbk_data();
  for (size_t j = 0; j < 2; j++) {
    struct port *port = core_get_port(data->core, mixer->port[j]);
    if (port)
      port_disconnect(port);
  }
  vec_free(&mixer->channels);
  free(mixer->id);
  for (size_t i = 0; i < 2; i++)
    free(mixer->port[i]);
  free(mixer->buffer);
  free(mixer);

  vec_delete(&data->mixers, vec_idx(data->mixers, == mixer));
}
RPC_DEFN(mixer_delete, (char *, id)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_MIXER) {
    free(id);
    rpc_err(-32602, "Provided object is not a mixer");
  }
  struct mixer *mixer = obj->ptr;
  core_cbk(data->core, impl_mixer_delete, mixer);
  rpc_relay("mixer_delete", id);
  map_remove(data->map, id);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_mixer_set_index, (struct mixer *, mixer), (size_t, idx)) {
  struct data *data = cbk_data();
  vec_delete(&data->mixers, vec_idx(data->mixers, == mixer));
  vec_insert(&data->mixers, idx, mixer);
}
RPC_DEFN(mixer_set_index, (char *, id), (long, idx)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_MIXER) {
    free(id);
    rpc_err(-32602, "Provided object is not a mixer");
  }
  struct mixer *mixer = obj->ptr;
  core_cbk(data->core, impl_mixer_set_index, mixer, idx);
  rpc_relay("mixer_set_index", id, idx);
  free(id);
  rpc_return();
}

RPC_DEFN(mixer_set_name, (char *, id), (char *, name)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_MIXER) {
    free(id);
    rpc_err(-32602, "Provided object is not a mixer");
  }
  struct mixer *mixer = obj->ptr;
  free(mixer->name);
  mixer->name = name;
  rpc_relay("mixer_set_name", id, name);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_mixer_set_port, (struct mixer *, mixer), (size_t, idx),
         (char *, path)) {
  struct data *data = cbk_data();

  if (strlen(mixer->port[idx])) {
    struct port *port = core_get_port(data->core, mixer->port[idx]);
    if (port) {
      port_disconnect(port);
      mixer->port_data[idx] = nullptr;
    }
  }
  free(mixer->port[idx]);
  struct port *port = core_get_port(data->core, path);
  if (port) {
    port_connect(port);
    mixer->port_data[idx] = port_buf(port);
  }
  mixer->port[idx] = path;
}
RPC_DEFN(mixer_set_port, (char *, id), (long, idx), (char *, path)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_MIXER) {
    free(path);
    free(id);
    rpc_err(-32602, "Provided object is not a mixer");
  }
  struct mixer *mixer = obj->ptr;
  if (!strcmp(path, mixer->port[idx])) {
    free(path);
    free(id);
    rpc_return();
  }
  core_cbk(data->core, impl_mixer_set_port, mixer, idx, path);
  rpc_relay("mixer_set_port", id, idx, path);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_mixer_update_all, (struct mixer *, mixer)) {
  for (size_t c = 0; c < mixer->channels.n; c++)
    _update_vol(mixer, c);
}
RPC_DEFN(mixer_set_master, (char *, id), (double, gain)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_MIXER) {
    free(id);
    rpc_err(-32602, "Provided object is not a mixer");
  }
  struct mixer *mixer = obj->ptr;
  mixer->master = fmax(gain, MIN_GAIN);
  core_cbk(data->core, impl_mixer_update_all, mixer);
  rpc_relay("mixer_set_master", id, mixer->master);
  free(id);
  if (gain != mixer->master)
    rpc_return(mixer->master);
  else
    rpc_return();
}

CBK_DEFN(impl_channel_new, (struct channel *, channel)) {
  struct data *data = cbk_data();
  struct mixer *mixer = channel->mixer;
  vec_push(&mixer->channels, channel);
  mixer->vols = realloc(mixer->vols, mixer->channels.n * sizeof *mixer->vols);
  mixer->sources =
      realloc(mixer->sources, mixer->channels.n * sizeof *mixer->sources);
  mixer->sources[mixer->channels.n - 1] = nullptr;
  if (data->buffer_size)
    mixer->buffer =
        realloc(mixer->buffer,
                mixer->channels.n * data->buffer_size * sizeof *mixer->buffer);
  _update_vol(mixer, mixer->channels.n - 1);
}
RPC_DEFN(channel_new, (char *, id)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_MIXER) {
    free(id);
    rpc_err(-32602, "Provided object is not a mixer");
  }
  struct mixer *mixer = obj->ptr;
  struct channel *channel = malloc(sizeof *channel);
  *channel = (typeof(*channel)){
      .id = map_insert(data->map,
                       &(struct obj){
                           .type = MIXUP_CHANNEL,
                           .ptr = channel,
                       }),
      .name = strdup("# channel"),
      .src = strdup(MAP_ID_NULL),
      .mute = true,

      .mixer = mixer,
  };
  core_cbk(data->core, impl_channel_new, channel);
  rpc_relay("channel_new", id, channel);
  free(id);
  rpc_return(channel);
}

CBK_DEFN(impl_channel_delete, (struct channel *, channel)) {
  struct mixer *mixer = channel->mixer;
  size_t idx = vec_idx(mixer->channels, == channel);
  vec_delete(&mixer->channels, idx);
  for (size_t c = idx; c < mixer->channels.n; c++)
    _update_vol(mixer, c);
  free(channel->id);
  free(channel);
}
RPC_DEFN(channel_delete, (char *, id)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_CHANNEL) {
    free(id);
    rpc_err(-32602, "Provided object is not a channel");
  }
  struct channel *channel = obj->ptr;
  core_cbk(data->core, impl_channel_delete, channel);
  rpc_relay("channel_delete", id);
  map_remove(data->map, id);
  free(id);
  rpc_return();
}

RPC_DEFN(channel_set_name, (char *, id), (char *, name)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_CHANNEL) {
    free(id);
    rpc_err(-32602, "Provided object is not a channel");
  }
  struct channel *channel = obj->ptr;
  free(channel->name);
  channel->name = name;
  rpc_relay("channel_set_name", id, name);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_channel_set_src_none, (struct channel *, channel)) {
  struct mixer *mixer = channel->mixer;
  size_t c = vec_idx(mixer->channels, == channel);
  mixer->sources[c] = nullptr;
}
CBK_DEFN(impl_channel_set_src_input, (struct channel *, channel),
         (struct input *, input)) {
  struct mixer *mixer = channel->mixer;
  size_t c = vec_idx(mixer->channels, == channel);
  mixer->sources[c] = &input->buffer;
}
RPC_DEFN(channel_set_src, (char *, id), (char *, src_id)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_CHANNEL) {
    free(id);
    free(src_id);
    rpc_err(-32602, "Provided object is not a channel");
  }
  struct channel *channel = obj->ptr;
  struct obj *src = map_get(data->map, src_id);
  if (!src)
    core_cbk(data->core, impl_channel_set_src_none, channel);
  else if (src->type == MIXUP_INPUT)
    core_cbk(data->core, impl_channel_set_src_input, channel, src->ptr);
  free(channel->src);
  channel->src = src_id;
  rpc_relay("channel_set_src", id, src_id);
  free(id);
  rpc_return();
}

CBK_DEFN(impl_channel_update_vol, (struct channel *, channel)) {
  struct mixer *mixer = channel->mixer;
  _update_vol(mixer, vec_idx(mixer->channels, == channel));
}
RPC_DEFN(channel_set_gain, (char *, id), (double, gain)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_CHANNEL) {
    free(id);
    rpc_err(-32602, "Provided object is not a channel");
  }
  struct channel *channel = obj->ptr;
  channel->gain = fmaxf(gain, MIN_GAIN);
  core_cbk(data->core, impl_channel_update_vol, channel);
  rpc_relay("channel_set_gain", id, channel->gain);
  free(id);
  if (gain != channel->gain)
    rpc_return(channel->gain);
  else
    rpc_return();
}
RPC_DEFN(channel_set_balance, (char *, id), (double, balance)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_CHANNEL) {
    free(id);
    rpc_err(-32602, "Provided object is not a channel");
  }
  struct channel *channel = obj->ptr;
  channel->balance = fmax(-MAX_BAL, fmin(MAX_BAL, balance));
  core_cbk(data->core, impl_channel_update_vol, channel);
  rpc_relay("channel_set_gain", id, channel->gain);
  free(id);
  if (balance != channel->balance)
    rpc_return(channel->balance);
  else
    rpc_return();
}
RPC_DEFN(channel_set_mute, (char *, id), (bool, mute)) {
  struct data *data = rpc_data();
  struct obj *obj = map_get(data->map, id);
  if (!obj || obj->type != MIXUP_CHANNEL) {
    free(id);
    rpc_err(-32602, "Provided object is not a channel");
  }
  struct channel *channel = obj->ptr;
  channel->mute = mute;
  core_cbk(data->core, impl_channel_update_vol, channel);
  rpc_relay("channel_set_mute", id, mute);
  rpc_return();
}
