#ifndef _WGTCC_TEST_H_
#define _WGTCC_TEST_H_

#define expect(a, b)                                                            \
    if ((a) != (b)) {                                                           \
        fprintf(stderr, "%s:%s:%d: failed\n", __FILE__, __func__, __LINE__);    \
    };

#endif
