#ifndef __MIXUP_BUS_H__
#define __MIXUP_BUS_H__

#include "mixup.h"
#include <stdio.h>

struct bus {
  char *port[2];
  double gain, balance;

  float **port_data[2];
  float2 *buffer;
  float2 vol;
};

void print_bus(FILE *, va_list *);

// RPCs
void bus_new(void *);
void bus_delete(void *);
void bus_set_port(void *);
void bus_set_gain(void *);
void bus_set_balance(void *);

#endif
