#include "test.h"
#include <uchar.h>
#include <wchar.h>


int main(void)
{
    char arr[] = "ab" "cd";
    //char arr2[] = "ab" u8"cd";
    char arr3[] = "ab" u"cd";
    //char arr3[] = "ab" U"cd";
    //char arr3[] = "ab" L"cd";

    char16_t arr16[] = u"ab" u"cd";
    //char16_t arr162[] = u"ab" U"cd";
    //char16_t arr162[] = u"ab" L"cd";
    //char16_t arr162[] = u"ab" u8"cd";
    char16_t arr162[] = u"ab" "cd";

    char32_t arr32[] = U"ab" U"cd";
    //char32_t arr322[] = U"ab" L"cd";
    char32_t arr323[] = U"ab" "cd";

    wchar_t arrw[] = L"ab" L"ab";    
    return 0;
}