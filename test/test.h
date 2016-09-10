#ifndef _WGTCC_TEST_H_
#define _WGTCC_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

#define fail(msg)																	\
fprintf(stderr, "error:%s:%s:%d: failed, %s\n", 	\
		__FILE__, __func__, __LINE__, (msg));					\

#define expect(a, b)                            						\
if ((a) != (b)) {                                   				\
		fprintf(stderr, "error:%s:%s:%d: failed, %d != %d\n",  	\
				__FILE__, __func__, __LINE__, (a), (b));						\
};

#define expectl expect
#define expectf expect_float
#define expectd expect_float

#define expect_float(a, b)                          				\
if ((a) != (b)) {                                   				\
		fprintf(stderr, "error:%s:%s:%d: failed, %f != %f\n",  	\
				__FILE__, __func__, __LINE__, (a), (b)); 						\
};

#define expect_string(a, b)                         				\
if (strcmp((a), (b))) {  																		\
		fprintf(stderr, "error:%s:%s:%d: failed, %s != %s\n",  	\
				__FILE__, __func__, __LINE__, (a), (b)); 						\
};    


#endif
