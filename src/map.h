#ifndef __MIXUP_MAP_H__
#define __MIXUP_MAP_H__

#include "vector.h"

#define map(keytype, valuetype)                                                \
  struct {                                                                     \
    vec(struct {                                                               \
      keytype key;                                                             \
      valuetype val;                                                           \
    }) _vec;                                                                   \
    int(_cmp)(const void *, const void *);                                     \
  }

#define map_init(map, cmp)                                                     \
  vec_init((map)._vec);                                                        \
  (map)._cmp = (cmp);

#endif // !__MIXUP_MAP_H__
