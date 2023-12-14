#include "string.h"
#include <hypertracer.h>

void HTString_free(struct HTString *str) {
    if (str->free_func) {
        str->free_func(str->free_param);
    }
}
