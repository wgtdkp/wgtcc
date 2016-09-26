//#include <math.h>

#define __CONCAT(name,r) name##r


#define __MATHCALL(function,suffix, args)	\
  __MATHDECL (double,function,suffix, args)

#define __MATHDECL(type, function,suffix, args) \
  __MATHDECL_1(type, function,suffix, args); \
  __MATHDECL_1(type, __CONCAT(__,function),suffix, args)

#define __MATHDECL_1(type, function,suffix, args) \
  extern type __MATH_PRECNAME(function,suffix) args

#define __MATH_PRECNAME(name,r)	__CONCAT(name,r)

__MATHCALL (acos,, (double __x));

int main() {
  return 0;
}
