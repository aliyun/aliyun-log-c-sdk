#ifndef LIBAOS_FSTACK_H
#define LIBAOS_FSTACK_H

#include "aos_define.h"

AOS_CPP_START

typedef void (*aos_func1_pt)(void*);
typedef void (*aos_func2_pt)();
typedef int (*aos_func3_pt)(void*);
typedef int (*aos_func4_pt)();

typedef union aos_func_u {
    aos_func1_pt func1;
    aos_func2_pt func2;
    aos_func3_pt func3;
    aos_func4_pt func4;
} aos_func_u;

typedef struct aos_fstack_item_t {
    void *data;
    aos_func_u func;
    int order;
} aos_fstack_item_t;

aos_array_header_t *aos_fstack_create(aos_pool_t *p, int size);

aos_fstack_item_t *aos_fstack_pop(aos_array_header_t *fstack);

void aos_fstack_destory(aos_array_header_t *fstack);

void aos_fstack_push(aos_array_header_t *fstack, void *data, aos_func_u func, int order);

AOS_CPP_END

#endif
