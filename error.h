#ifndef _WGTCC_ERROR_H_
#define _WGTCC_ERROR_H_

class Coordinate;


void Error(const Coordinate& coord, const char* format, ...);

#endif
