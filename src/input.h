#ifndef __MIXUP_BUS_H__
#define __MIXUP_BUS_H__

#include "mixup.h"
#include <stdio.h>

struct input {
  char *id;
  char *name;
  char *port[2];
  double gain, balance;

  float **port_data[2];
  float2 *buffer;
  float2 vol;
};

void print_input(FILE *, va_list *);

// RPCs
void input_new(void *);
void input_delete(void *);
void input_set_index(void *);
void input_set_name(void *);
void input_set_port(void *);
void input_set_gain(void *);
void input_set_balance(void *);

#endif
