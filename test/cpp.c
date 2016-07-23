#define THREE (3)
#define PI  (THREE + (.1415926))
#define SUM(a, b) ((a) + (b##5))
float sum = THREE + SUM(PI, .1415926);
