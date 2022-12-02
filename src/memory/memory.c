#include <string.h>
#include <memory.h>

void *memmem(const void *haystack_start, size_t haystack_len,
             const void *needle_start, size_t needle_len)
{
    const void *haystack = NULL;
    if (needle_len == 0)
        return (void *) haystack_start;
    if (haystack_len < needle_len)
        return NULL;
    for (size_t i = 0; i < haystack_len - needle_len; i++) {
        if (((unsigned char *)(haystack_start + i))[0] == ((unsigned char *)needle_start)[0]) {
            haystack = haystack_start + i;
            haystack_len -= i;
            break;
        }
    }
    if (!haystack || haystack_len < needle_len)
        return NULL;
    for (size_t i = 0; i < haystack_len - needle_len; i++) {
        if (!memcmp(haystack + i, needle_start, needle_len))
            return (void *) haystack + i;
    }
    return NULL;
}
