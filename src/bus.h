#ifndef __MIXUP_BUS_H__
#define __MIXUP_BUS_H__

#include "mixup.h"
#include "rpc.h"

struct bus {
  char *port[2];
  double gain, balance;

  float **port_data[2];
  float2 *buffer;
  float2 vol;
};

RPC_DECL(bus_new);
RPC_DECL(bus_delete);
RPC_DECL(bus_set_port);
RPC_DECL(bus_set_gain);
RPC_DECL(bus_set_balance);

#endif
