#pragma once

#include <git2.h>

#ifdef LIBGIT2_VER_MINOR
#define CHECK_LIBGIT2_VERSION(MAJOR, MINOR)                                    \
  ((LIBGIT2_VER_MAJOR == (MAJOR) && LIBGIT2_VER_MINOR >= (MINOR)) ||           \
   LIBGIT2_VER_MAJOR > (MAJOR))
#else /* ! defined(LIBGIT2_VER_MINOR) */
#define CHECK_LIBGIT2_VERSION(MAJOR, MINOR) 0
#endif
