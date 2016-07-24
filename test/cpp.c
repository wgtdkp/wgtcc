#define THREE (3)
#define PI  (THREE + (.1415926))
#define SUM(a, b) ((a) + (b##5))
float sum = THREE + SUM(PI, );

#define hash_hash # ## #
#define mkstr(a) # a
#define in_between(a) mkstr(a)
#define join(c, d) in_between(c hash_hash d)
char p[10] = join(x, y);
