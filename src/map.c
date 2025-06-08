#include "map.h"

#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

typedef unsigned _BitInt(128) u128;
static_assert(sizeof(u128) == sizeof(uuid_t));

struct map {
  struct {
    u128 val;
    bool full;
  } *keys;
  char *data;
  size_t n, m, s;
};

struct map *map_new(size_t size) {
  struct map *map = malloc(sizeof *map);
  *map = (typeof(*map)){
      .keys = malloc(sizeof *map->keys),
      .data = malloc(size),
      .m = 1,
      .s = size,
  };
  map->data[0] = 0;
  return map;
}

void map_delete(struct map *map) {
  if (map) {
    free(map->keys);
    free(map->data);
  }
  free(map);
}

static inline size_t _find(struct map *map, u128 uuid) {
  size_t i = uuid % map->m;
  while (map->keys[i].full && map->keys[i].val != uuid)
    i = (i + 1) % map->m;
  return i;
}

static inline void _set(struct map *map, u128 uuid, const void *data) {
  size_t i = _find(map, uuid);
  if (map->keys[i].full)
    memcpy(map->data + i * map->s, data, map->s);
  else {
    if (2 * ++map->n >= map->m) {
      struct map new = {
          .keys = malloc(2 * map->m * sizeof *new.keys),
          .data = malloc(2 * map->m * map->s),
          .m = 2 * map->m,
          .s = map->s,
      };
      for (size_t i = 0; i < new.m; i++)
        new.keys[i] = (typeof(new.keys[i])){};
      for (size_t i = 0; i < map->m; i++)
        if (map->keys[i].full)
          _set(&new, map->keys[i].val, map->data + i * map->s);
      free(map->keys);
      free(map->data);
      *map = new;
      i = _find(map, uuid);
    }
    map->keys[i] = (typeof(map->keys[i])){
        .val = uuid,
        .full = true,
    };
    memcpy(map->data + i * map->s, data, map->s);
  }
}

char *map_insert(struct map *map, const void *data) {
  if (!map)
    return nullptr;
  u128 uuid;
  do
    uuid_generate_random((unsigned char *)&uuid);
  while (map->keys[_find(map, uuid)].full);

  _set(map, uuid, data);

  char *uuid_s = strdup(MAP_ID_NULL);
  uuid_unparse_lower((unsigned char *)&uuid, uuid_s);
  return uuid_s;
}

void *map_get(struct map *map, const char *uuid_s) {
  if (!map)
    return nullptr;
  u128 uuid;
  uuid_parse(uuid_s, (unsigned char *)&uuid);

  size_t i = _find(map, uuid);
  if (map->keys[i].full)
    return map->data + i * map->s;
  else
    return nullptr;
}

void map_remove(struct map *map, const char *uuid_s) {
  if (!map)
    return;
  u128 uuid;
  uuid_parse(uuid_s, (unsigned char *)&uuid);

  size_t i = _find(map, uuid);
  if (!map->keys[i].full)
    return;
  for (size_t j = (i + 1) % map->m; map->keys[j].full; j = (j + 1) % map->m) {
    size_t k = map->keys[j].val % map->m;
    if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
      map->keys[i] = map->keys[j];
      memcpy(map->data + i * map->s, map->data + j * map->s, map->s);
      i = j;
    }
  }
  map->keys[i].full = false;
}
