#ifndef __TOOL_H__
#define __TOOL_H__

#ifndef IF_RES_FAIL_RETURN
#define IF_RES_FAIL_RETURN if(res == EXIT_FAILURE)\
{\
    return EXIT_FAILURE;\
}
#endif

#define STRING_TRUE_LEN(buf_max) (buf_max - 1)

#endif
