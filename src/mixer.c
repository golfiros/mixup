#include "mixer.h"
#include "core.h"
#include "input.h"
#include "vector.h"
#include <string.h>

#define RPC_EXTRACT_TYPES                                                      \
  struct mixer * : _rpc_extract_obj, struct channel * : _rpc_extract_obj
#define RPC_PRINT_TYPES                                                        \
  struct mixer * : print_mixer, struct channel * : print_channel
#include "rpc.h"

void print_mixer(FILE *fp, va_list *ap) {
  struct mixer *mixer = va_arg(*ap, typeof(mixer));
  fprintf(
      fp,
      "{\"id\":\"%p\",\"port\":[\"%s\",\"%s\"],\"master\":%f,\"channels\":[",
      mixer, mixer->port[0], mixer->port[1], mixer->master);
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
  fprintf(fp, "{\"id\":\"%p\",\"src\":\"%p\",\"gain\":%f,\"balance\":%f}",
          channel, channel->src, channel->gain, channel->balance);
}

static inline void _channel_update_vol(struct mixer *mixer, size_t c) {
  const struct channel *channel = mixer->channels.data[c];
  mixer->vols[c][0] = DB_TO_AMP(mixer->master) * DB_TO_AMP(channel->gain) *
                      PAN_POWL(channel->balance);
  mixer->vols[c][1] = DB_TO_AMP(mixer->master) * DB_TO_AMP(channel->gain) *
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
  for (size_t i = 0; i < 2; i++)
    free(mixer->port[i]);
  free(mixer->buffer);
  free(mixer);

  vec_delete(&data->mixers, vec_idx(data->mixers, == mixer));
}
RPC_DEFN(mixer_delete, (struct mixer *, mixer)) {
  struct data *data = rpc_data();
  core_cbk(data->core, impl_mixer_delete, mixer);
  rpc_relay("mixer_delete", ((void *)mixer));
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
RPC_DEFN(mixer_set_port, (struct mixer *, mixer), (long, idx), (char *, path)) {
  struct data *data = rpc_data();
  if (!strcmp(path, mixer->port[idx])) {
    free(path);
    rpc_return();
  }
  core_cbk(data->core, impl_mixer_set_port, mixer, idx, path);
  rpc_return();
}

CBK_DEFN(impl_mixer_update_all, (struct mixer *, mixer)) {
  for (size_t c = 0; c < mixer->channels.n; c++)
    _channel_update_vol(mixer, c);
}
RPC_DEFN(mixer_set_master, (struct mixer *, mixer), (double, gain)) {
  struct data *data = rpc_data();
  mixer->master = fmax(gain, MIN_GAIN);
  core_cbk(data->core, impl_mixer_update_all, mixer);
  rpc_relay("mixer_set_master", ((void *)mixer), mixer->master);
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
  _channel_update_vol(mixer, mixer->channels.n - 1);
}
RPC_DEFN(channel_new, (struct mixer *, mixer)) {
  struct data *data = rpc_data();
  struct channel *channel = malloc(sizeof *channel);
  *channel = (typeof(*channel)){.mixer = mixer};
  core_cbk(data->core, impl_channel_new, channel);
  rpc_relay("channel_new", mixer, channel);
  rpc_return(channel);
}

CBK_DEFN(impl_channel_delete, (struct channel *, channel)) {
  struct data *data = cbk_data();
  struct mixer *mixer = channel->mixer;
  size_t idx = vec_idx(mixer->channels, == channel);
  vec_delete(&mixer->channels, idx);
  for (size_t c = idx; c < mixer->channels.n; c++)
    _channel_update_vol(mixer, c);
  free(channel);
}
RPC_DEFN(channel_delete, (struct channel *, channel)) {
  struct data *data = rpc_data();
  struct mixer *mixer = channel->mixer;
  core_cbk(data->core, impl_channel_delete, channel);
  rpc_relay("channel_delete", mixer, ((void *)channel));
  rpc_return();
}

CBK_DEFN(impl_channel_set_src, (struct channel *, channel)) {
  struct data *data = cbk_data();
  struct mixer *mixer = channel->mixer;
  size_t c = vec_idx(mixer->channels, == channel);
  if (vec_idx(data->inputs, == channel->src) < data->inputs.n) {
    struct input *input = channel->src;
    mixer->sources[c] = &input->buffer;
  }
}
RPC_DEFN(channel_set_src, (struct channel *, channel), (void *, src)) {
  struct data *data = rpc_data();
  channel->src = src;
  core_cbk(data->core, impl_channel_set_src, channel);
  rpc_relay("channel_set_src", ((void *)channel), src);
  rpc_return();
}

CBK_DEFN(impl_channel_update_vol, (struct channel *, channel)) {
  struct mixer *mixer = channel->mixer;
  _channel_update_vol(mixer, vec_idx(mixer->channels, == channel));
}
RPC_DEFN(channel_set_gain, (struct channel *, channel), (double, gain)) {
  struct data *data = rpc_data();
  channel->gain = fmaxf(gain, MIN_GAIN);
  core_cbk(data->core, impl_channel_update_vol, channel);
  rpc_relay("channel_set_gain", ((void *)channel), channel->gain);
  if (gain != channel->gain)
    rpc_return(channel->gain);
  else
    rpc_return();
}
RPC_DEFN(channel_set_balance, (struct channel *, channel), (double, balance)) {
  struct data *data = rpc_data();
  channel->balance = fmax(-MAX_BAL, fmin(MAX_BAL, balance));
  core_cbk(data->core, impl_channel_update_vol, channel);
  rpc_relay("channel_set_gain", ((void *)channel), channel->gain);
  if (balance != channel->balance)
    rpc_return(channel->balance);
  else
    rpc_return();
}
