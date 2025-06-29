#ifndef __MIXUP_MIXER_H__
#define __MIXUP_MIXER_H__

#include "mixup.h"

struct mixer {
  char *id;
  char *name;
  char *port[2];
  double master;
  vec(struct channel *) channels;

  float **port_data[2];
  float2 *buffer;
  float2 ***sources;
  float2 *vols;
};

void print_mixer(FILE *, va_list *);

struct channel {
  char *id;
  char *name;
  char *src;
  double gain, balance;
  bool mute;

  struct mixer *mixer;
  float2 **buffer;
};

void print_channel(FILE *, va_list *);

// RPCs
void mixer_new(void *);
void mixer_delete(void *);
void mixer_set_index(void *);
void mixer_set_name(void *);
void mixer_set_port(void *);
void mixer_set_master(void *);

void channel_new(void *);
void channel_delete(void *);
void channel_set_index(void *);
void channel_set_name(void *);
void channel_set_src(void *);
void channel_set_gain(void *);
void channel_set_balance(void *);
void channel_set_mute(void *);

#endif
