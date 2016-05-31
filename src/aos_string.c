#include "aos_string.h"

typedef int (*aos_is_char_pt)(char c);

static void aos_strip_str_func(aos_string_t *str, aos_is_char_pt func);

char *aos_pstrdup(aos_pool_t *p, const aos_string_t *s)
{
    return apr_pstrndup(p, s->data, s->len);
}

static void aos_strip_str_func(aos_string_t *str, aos_is_char_pt func)
{
    char *data = str->data;
    int len = str->len;
    int offset = 0;

    if (len == 0) return;
    
    while (len > 0 && func(data[len - 1])) {
        --len;
    }
    
    for (; offset < len && func(data[offset]); ++offset) {
        // empty;
    }

    str->data = data + offset;
    str->len = len - offset;
}

void aos_unquote_str(aos_string_t *str)
{
    aos_strip_str_func(str, aos_is_quote);
}

void aos_strip_space(aos_string_t *str)
{
    aos_strip_str_func(str, aos_is_space);
}

void aos_trip_space_and_cntrl(aos_string_t *str)
{
    aos_strip_str_func(str, aos_is_space_or_cntrl);
}

int aos_ends_with(const aos_string_t *str, const aos_string_t *suffix)
{
    if (!str || !suffix) {
        return 0;
    }

    return (str->len >= suffix->len) && strncmp(str->data + str->len - suffix->len, suffix->data, suffix->len) == 0;
}
