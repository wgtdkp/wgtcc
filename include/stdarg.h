#ifndef _WGTCC_STDARG_H_
#define _WGTCC_STDARG_H_

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void* overflow_arg_area;
    void* reg_save_area;
} va_list[1];



#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list;

#endif