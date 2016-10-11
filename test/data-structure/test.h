#ifndef _TEST_H_
#define _TEST_H_

#include <stdio.h>

#define EXPECT(a, b)  {                         \
  if ((a) != (b)) {                             \
    fprintf(stderr, "%s:%d: error: %d != %d\n", \
      __func__, __LINE__, (a), (b));            \
  }                                             \
}

#endif
