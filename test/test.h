#ifndef _WGTCC_TEST_H_
#define _WGTCC_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>


#define expect(a, b)                                                \
    if ((a) != (b)) {                                               \
        printf("%s:%s:%d: failed\n", __FILE__, __func__, __LINE__); \
    };

#endif
