#include "aos_fstack.h"

aos_array_header_t *aos_fstack_create(aos_pool_t *p, int size)
{
    return apr_array_make(p, size, sizeof(aos_fstack_item_t));
}

void aos_fstack_push(aos_array_header_t *fstack, void *data, aos_func_u func, int order)
{
    aos_fstack_item_t *item;

    item = (aos_fstack_item_t*)apr_array_push(fstack);
    item->data = data;
    item->func = func;
    item->order = order;
}

aos_fstack_item_t *aos_fstack_pop(aos_array_header_t *fstack)
{
    aos_fstack_item_t *item;    
    
    item = (aos_fstack_item_t*)apr_array_pop(fstack);
    if (item == NULL) {
        return NULL;
    }

    switch (item->order) {
        case 1:
            item->func.func1(item->data);
            break;
        case 2:
            item->func.func2();
            break;
        case 3:
            item->func.func3(item->data);
            break;
        case 4:
            item->func.func4();
            break;
        default:
            break;
    }
    
    return item;
}

void aos_fstack_destory(aos_array_header_t *fstack)
{
    while (aos_fstack_pop(fstack) != NULL);
}
