#define _GNU_SOURCE // LOL
#include "stdlib.h"
#include <X11/Xauth.h>
#include "lsb_output.h"

#include <dlfcn.h>

#undef XauGetBestAuthByAddr
static Xauth *(*funcptr) (unsigned int , unsigned int , const char * , unsigned int , const char * , int , char * * , const int * ) = 0;

int __lsb_check_params = 1; // LOL

Xauth * XauGetBestAuthByAddr (unsigned int arg0 , unsigned int arg1 , const char * arg2 , unsigned int arg3 , const char * arg4 , int arg5 , char * * arg6 , const int * arg7 )
{
	int reset_flag = __lsb_check_params;
	Xauth * ret_value  ;
    printf("%s, %d\n", __FILE__, __LINE__);
	__lsb_output(4, "Invoking wrapper for XauGetBestAuthByAddr()");
    printf("%s, %d\n", __FILE__, __LINE__);
	if(!funcptr)
    {
		funcptr = dlsym(RTLD_NEXT, "XauGetBestAuthByAddr");
        printf("%s, %d\n", __FILE__, __LINE__);
    }
	if(!funcptr)
    {
        printf("%s, %d\n", __FILE__, __LINE__);
		__lsb_output(-1, "Failed to load XauGetBestAuthByAddr. Probably the library was loaded using dlopen, we don't support this at the moment.");
		exit(1);
	}
	if(__lsb_check_params)
	{
        printf("edited!");
        exit(0);
		// __lsb_check_params=0;
		// __lsb_output(4, "XauGetBestAuthByAddr() - validating");
		// validate_NULL_TYPETYPE(  arg0, "XauGetBestAuthByAddr - arg0");
		// validate_NULL_TYPETYPE(  arg1, "XauGetBestAuthByAddr - arg1");
		// validate_Rdaddress( arg2, "XauGetBestAuthByAddr - arg2");
		// validate_NULL_TYPETYPE(  arg2, "XauGetBestAuthByAddr - arg2");
		// validate_NULL_TYPETYPE(  arg3, "XauGetBestAuthByAddr - arg3");
		// validate_Rdaddress( arg4, "XauGetBestAuthByAddr - arg4");
		// validate_NULL_TYPETYPE(  arg4, "XauGetBestAuthByAddr - arg4");
		// validate_NULL_TYPETYPE(  arg5, "XauGetBestAuthByAddr - arg5");
		// validate_RWaddress( arg6, "XauGetBestAuthByAddr - arg6");
		// validate_NULL_TYPETYPE(  arg6, "XauGetBestAuthByAddr - arg6");
		// validate_Rdaddress( arg7, "XauGetBestAuthByAddr - arg7");
		// validate_NULL_TYPETYPE(  arg7, "XauGetBestAuthByAddr - arg7");
	}
	ret_value = funcptr(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	__lsb_check_params = reset_flag;
	return ret_value;
}