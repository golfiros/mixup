#ifndef __MIXUP_BUS_H__
#define __MIXUP_BUS_H__

#include "mixup.h"
#include <stdio.h>

#define INPUT_EQ_STAGES 5

struct input {
  char *id;
  char *name;
  char *port[2];
  double gain, balance;
  struct {
    double freq, quality, gain;
  } eq[INPUT_EQ_STAGES];

  float **port_data[2];
  float2 *buffer;
  float2 vol;
  struct {
    float a[2], b[3];
  } eq_coeffs[INPUT_EQ_STAGES];
  float2 eq_buffer[INPUT_EQ_STAGES + 1][2];
};

void print_input(FILE *, va_list *);

void input_update_eq(struct input *, size_t, double);

// RPCs
void input_new(void *);
void input_delete(void *);
void input_set_index(void *);
void input_set_name(void *);
void input_set_port(void *);
void input_set_gain(void *);
void input_set_balance(void *);
void input_set_eq_freq(void *);
void input_set_eq_quality(void *);
void input_set_eq_gain(void *);

#endif
