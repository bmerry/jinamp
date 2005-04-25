#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>

#if !HAVE_MEMSET_H

void *memset(void *s, int c, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        ((unsigned char *) s)[i] = (unsigned char) c;
}

#endif /* HAVE_MEMSET_H */
