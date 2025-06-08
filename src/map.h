#ifndef __MIXUP_MAP_H__
#define __MIXUP_MAP_H__

#include <stddef.h>

#define MAP_ID_NULL "00000000-0000-0000-0000-000000000000"

struct map *map_new(size_t);
void map_delete(struct map *);

char *map_insert(struct map *, const void *);

// void *map_add(struct map *, const char *, const void *);
void *map_get(struct map *, const char *);
void map_remove(struct map *, const char *);

#endif // !__MIXUP_MAP_H__
