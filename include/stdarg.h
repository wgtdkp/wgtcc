#ifndef _WGTCC_STDARG_H_
#define _WGTCC_STDARG_H_

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void* overflow_arg_area;
    void* reg_save_area;
} va_list[1];


#define va_start(ap, last) __builtin_va_start(&ap[0], &last)
#define va_arg(ap, type) *(type*)__builtin_va_arg(&ap[0], (type*)0)
#define va_end(ap) 1
#define va_copy(des, src) ((des)[0] = (src)[0])


// Workaround to load stdio.h properly
#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list;

#endif