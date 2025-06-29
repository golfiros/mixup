#ifndef __MIXUP_VECTOR_H__
#define __MIXUP_VECTOR_H__

#include <stdlib.h>

#define vec(typename)                                                          \
  struct {                                                                     \
    size_t n, _m;                                                              \
    typename *data;                                                            \
  }

#define vec_init(vec)                                                          \
  ({                                                                           \
    typeof(*(vec)) *_vec = (vec);                                              \
    *_vec = (typeof(*_vec)){                                                   \
        .n = 0,                                                                \
        ._m = 0,                                                               \
        .data = malloc(0),                                                     \
    };                                                                         \
  })

#define vec_free(vec)                                                          \
  ({                                                                           \
    typeof(*(vec)) *_vec = (vec);                                              \
    free(_vec->data);                                                          \
  });

#define vec_insert(vec, idx, item)                                             \
  ({                                                                           \
    typeof(*(vec)) *_vec = (vec);                                              \
    ptrdiff_t _idx = (idx);                                                    \
    typeof(item) _item = item;                                                 \
                                                                               \
    bool ret = true;                                                           \
                                                                               \
    if (_vec->n == _vec->_m) {                                                 \
      void *ptr = realloc(_vec->data, (_vec->_m = (3 * (_vec->_m + 1)) / 2) *  \
                                          sizeof *_vec->data);                 \
      if (!ptr)                                                                \
        ret = false;                                                           \
      else                                                                     \
        _vec->data = ptr;                                                      \
    }                                                                          \
                                                                               \
    if (ret) {                                                                 \
      for (ptrdiff_t i = _vec->n++; i > (ptrdiff_t)_idx; i--)                  \
        _vec->data[i] = _vec->data[i - 1];                                     \
      _vec->data[_idx] = _item;                                                \
    }                                                                          \
    ret;                                                                       \
  })
#define vec_push(vec, item) vec_insert((vec), (vec)->n, (item))

#define vec_delete(vec, idx)                                                   \
  ({                                                                           \
    typeof(*(vec)) *_vec = (vec);                                              \
    size_t _idx = (idx);                                                       \
    typeof(*_vec->data) item = _vec->data[_idx];                               \
    _vec->n--;                                                                 \
    for (size_t i = _idx; i < _vec->n; i++)                                    \
      _vec->data[i] = _vec->data[i + 1];                                       \
    item;                                                                      \
  })
#define vec_pop(vec) vec_delete((vec), (vec)->n);

#define vec_idx(vec, cond)                                                     \
  ({                                                                           \
    typeof(vec) _vec = (vec);                                                  \
    size_t _idx;                                                               \
    for (_idx = 0; _idx < _vec.n; _idx++)                                      \
      if (_vec.data[_idx] cond)                                                \
        break;                                                                 \
    _idx;                                                                      \
  })

#define vec_qsort(vec, cmp)                                                    \
  qsort((vec)->data, (vec)->n, sizeof *(vec)->data, (cmp))

#define vec_bsearch_ref(vec, cmp, ...)                                         \
  bsearch(&(typeof(*(vec).data)){__VA_ARGS__}, (vec).data, (vec).n,            \
          sizeof *(vec).data, (cmp))
#define vec_bsearch(vec, cmp, ...)                                             \
  ({                                                                           \
    typeof(*(vec).data) *item =                                                \
        bsearch(&(typeof(*(vec).data)){__VA_ARGS__}, (vec).data, (vec).n,      \
                sizeof *(vec).data, (cmp));                                    \
    *item;                                                                     \
  })

#define vec_foreach_ref(item, vec)                                             \
  for (typeof((vec).data)(item) = (vec).data; (item) < (vec).data + (vec).n;   \
       (item)++)
#define vec_foreach(item, vec)                                                 \
  for (typeof(*(vec).data) *_iter = (vec).data, (item); ({                     \
         bool cont = _iter < (vec).data + (vec).n;                             \
         if (cont)                                                             \
           (item) = *_iter;                                                    \
         cont;                                                                 \
       });                                                                     \
       _iter++)

#endif
