#ifndef __MIXUP_MIXER_H__
#define __MIXUP_MIXER_H__

#include "mixup.h"

struct mixer {
  struct port_data *port_data[2];
  float2 *vols;

  char *port[2];

  vec(struct src) sources;
  double master, *gain, *balance;
};

void mixer_new(void *);
void mixer_delete(void *);
void mixer_set_port(void *);
void mixer_set_gain(void *);
void mixer_set_balance(void *);

#endif
