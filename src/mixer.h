#ifndef __MIXUP_MIXER_H__
#define __MIXUP_MIXER_H__

#include "mixup.h"
#include "rpc.h"

struct mixer {
  struct port_data *port_data[2];
  float2 *vols;

  char *port[2];

  vec(struct src) sources;
  double master, *gain, *balance;
};

RPC_DECL(mixer_new);
RPC_DECL(mixer_delete);
RPC_DECL(mixer_set_port);
RPC_DECL(mixer_set_gain);
RPC_DECL(mixer_set_balance);

#endif
