#ifndef _WGTCC_TEST_H_
#define _WGTCC_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>


#define expect(a, b)                            			\
if ((a) != (b)) {                                   	\
		printf("error:%s:%s:%d: failed, %d != %d\n",     	\
				__FILE__, __func__, __LINE__, (a), (b));			\
};

#define expect_float(a, b)                          	\
if ((a) != (b)) {                                   	\
		printf("error:%s:%s:%d: failed, %f != %f\n",     	\
				__FILE__, __func__, __LINE__, (a), (b)); 			\
};

#define expect_string(a, b)                         							\
if (sizeof(a) != sizeof(b) || memcmp(a, b, sizeof(a)) != 0) {  		\
		printf("error:%s:%s:%d: failed, %d != %d\n",    							\
				__FILE__, __func__, __LINE__, sizeof(a), sizeof(b)); 			\
};    


#endif
