#include <hypertracer.h>
#include "string.h"

void HTString_free(struct HTString *str) {
    if(str->free_func) {
        str->free_func(str->free_param);
    }
}
