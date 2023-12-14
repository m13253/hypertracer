#include "string.h"
#include <hypertracer.h>

void htString_free(struct htString *str) {
    if (str->free_func) {
        str->free_func(str->free_param);
    }
}
