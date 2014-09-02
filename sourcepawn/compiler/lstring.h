/* prototypes for strlcpy() and strlcat() */

#include <stddef.h>

#if defined __WATCOMC__ && __WATCOMC__ >= 1240
  /* OpenWatcom introduced BSD "safe string functions" with version 1.4 */
  #define HAVE_SAFESTR
#endif

#if !defined HAVE_SAFESTR

#if defined(__cplusplus)
# define EXPORT extern "C"
#else
# define EXPORT
#endif

EXPORT size_t
strlcpy(char *dst, const char *src, size_t siz);

EXPORT size_t
strlcat(char *dst, const char *src, size_t siz);

#undef EXPORT

#endif
