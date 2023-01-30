#define _GNU_SOURCE // LOL
#include "stdlib.h"
#include <X11/Xauth.h>
#include "lsb_output.h"

#include <X11/Xauth.h>
#include <X11/Xos.h>

#include <dlfcn.h>

static
binaryEqual (a, b, len)
register char	*a, *b;
register int	len;
{
    while (len--)
	if (*a++ != *b++)
	    return 0;
    return 1;
}

#undef XauGetBestAuthByAddr
static Xauth *(*funcptr) (unsigned int , unsigned int , const char * , unsigned int , const char * , int , char * * , const int * ) = 0;

int __lsb_check_params = 1; // LOL

// Xauth * XauGetBestAuthByAddr (unsigned int arg0 , unsigned int arg1 , const char * arg2 , unsigned int arg3 , const char * arg4 , int arg5 , char * * arg6 , const int * arg7 )
// {
// 	int reset_flag = __lsb_check_params;
// 	Xauth * ret_value  ;
//     printf("%s, %d\n", __FILE__, __LINE__);
// 	__lsb_output(4, "Invoking wrapper for XauGetBestAuthByAddr()");
//     printf("%s, %d\n", __FILE__, __LINE__);
// 	if(!funcptr)
//     {
// 		funcptr = dlsym(RTLD_NEXT, "XauGetBestAuthByAddr");
//         printf("%s, %d\n", __FILE__, __LINE__);
//     }
// 	if(!funcptr)
//     {
//         printf("%s, %d\n", __FILE__, __LINE__);
// 		__lsb_output(-1, "Failed to load XauGetBestAuthByAddr. Probably the library was loaded using dlopen, we don't support this at the moment.");
// 		exit(1);
// 	}
// 	if(__lsb_check_params)
// 	{
//         printf("edited!");
//         exit(0);
// 		// __lsb_check_params=0;
// 		// __lsb_output(4, "XauGetBestAuthByAddr() - validating");
// 		// validate_NULL_TYPETYPE(  arg0, "XauGetBestAuthByAddr - arg0");
// 		// validate_NULL_TYPETYPE(  arg1, "XauGetBestAuthByAddr - arg1");
// 		// validate_Rdaddress( arg2, "XauGetBestAuthByAddr - arg2");
// 		// validate_NULL_TYPETYPE(  arg2, "XauGetBestAuthByAddr - arg2");
// 		// validate_NULL_TYPETYPE(  arg3, "XauGetBestAuthByAddr - arg3");
// 		// validate_Rdaddress( arg4, "XauGetBestAuthByAddr - arg4");
// 		// validate_NULL_TYPETYPE(  arg4, "XauGetBestAuthByAddr - arg4");
// 		// validate_NULL_TYPETYPE(  arg5, "XauGetBestAuthByAddr - arg5");
// 		// validate_RWaddress( arg6, "XauGetBestAuthByAddr - arg6");
// 		// validate_NULL_TYPETYPE(  arg6, "XauGetBestAuthByAddr - arg6");
// 		// validate_Rdaddress( arg7, "XauGetBestAuthByAddr - arg7");
// 		// validate_NULL_TYPETYPE(  arg7, "XauGetBestAuthByAddr - arg7");
// 	}
// 	ret_value = funcptr(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
// 	__lsb_check_params = reset_flag;
// 	return ret_value;
// }

void
XauDisposeAuth(Xauth *auth)
{
    if (auth) {
	if (auth->address) (void) free (auth->address);
	if (auth->number) (void) free (auth->number);
	if (auth->name) (void) free (auth->name);
	if (auth->data) (void) free (auth->data);
	free ((char *) auth);
    }
    return;
}

static
read_short (shortp, file)
unsigned short	*shortp;
FILE		*file;
{
    unsigned char   file_short[2];

    if (fread ((char *) file_short, (int) sizeof (file_short), 1, file) != 1)
	return 0;
    *shortp = file_short[0] * 256 + file_short[1];
    return 1;
}

static
read_counted_string (
unsigned short	*countp,
char	**stringp,
FILE	*file)
{
    unsigned short  len;
    char	    *data;

    if (read_short (&len, file) == 0)
	return 0;
    if (len == 0) {
	data = 0;
    } else {
    	data = malloc ((unsigned) len);
    	if (!data)
	    return 0;
    	if (fread (data, (int) sizeof (char), (int) len, file) != len) {
	    free (data);
	    return 0;
    	}
    }
    *stringp = data;
    *countp = len;
    return 1;
}

Xauth *
XauReadAuth (FILE	*auth_file)
{
    Xauth   local;
    Xauth   *ret;
    // char    *malloc ();

    if (read_short (&local.family, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.address_length, &local.address, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.number_length, &local.number, auth_file) == 0) {
	if (local.address) free (local.address);
	return 0;
    }
    if (read_counted_string (&local.name_length, &local.name, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	return 0;
    }
    if (read_counted_string (&local.data_length, &local.data, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	return 0;
    }
    ret = (Xauth *) malloc (sizeof (Xauth));
    if (!ret) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	if (local.data) free (local.data);
	return 0;
    }
    *ret = local;
    return ret;
}

char *
XauFileName ()
{
    char    *name = malloc (128);
    // char    *strcat (), *strcpy ();
    static char	*buf = NULL;
    static int	bsize;
    int	    size;

    if (name = getenv ("XAUTHORITY"))
    {
	    return name;
    }
    else if (name = getenv ("HOME"))
    {
        size = strlen (name) + strlen(".Xauthority") + 2;
        if (size > bsize)
        {
            if (buf)
            free (buf);
            buf = malloc ((unsigned) size);
            if (!buf)
            return 0;
            bsize = size;
        }
        strcpy (buf, name);
        strcat (buf, "/.Xauthority" + (name[1] == '\0' ? 1 : 0));
        return buf;
    }
    return 0;
}

Xauth *
XauGetBestAuthByAddr(
    unsigned int family, 
    unsigned int address_length, 
    const char *address,
	unsigned int number_length, 
    const char *number,
	int	types_length, 
    char **types, 
    const int *type_lengths)

{
    FILE    *auth_file;
    char    *auth_name;
    Xauth   *entry;
    Xauth   *best;
    int	    best_type;
    int	    type;

    printf("%s, %s, %d\n", __func__, __FILE__, __LINE__);

    auth_name = XauFileName();
    if (!auth_name)
	    return 0;
    printf("%s, %s, %d\n", __func__, __FILE__, __LINE__);
    if (access (auth_name, R_OK) != 0)		/* checks REAL id */
	    return 0;
    printf("%s, %s, %d\n", __func__, __FILE__, __LINE__);
    auth_file = fopen (auth_name, "r");
    if (!auth_file)
	    return 0;
    printf("%s, %s, %d\n", __func__, __FILE__, __LINE__);
    best = 0;
    best_type = types_length;
    for (;;)
    {
        entry = XauReadAuth (auth_file);
        if (!entry)
            break;
        /*
        * Match when:
        *   either family or entry->family are FamilyWild or
        *    family and entry->family are the same
        *  and
        *   either address or entry->address are empty or
        *    address and entry->address are the same
        *  and
        *   either number or entry->number are empty or
        *    number and entry->number are the same
        *  and
        *   name matches one of the specified names, or no names
        *    were specified
        */

        if ((family == FamilyWild || entry->family == FamilyWild ||
            (entry->family == family &&
            address_length == entry->address_length &&
            binaryEqual (entry->address, address, (int)address_length))) &&
            (number_length == 0 || entry->number_length == 0 ||
            (number_length == entry->number_length &&
            binaryEqual (entry->number, number, (int)number_length))))
        {
            if (best_type == 0)
            {
            best = entry;
            break;
            }
            for (type = 0; type < best_type; type++)
            if (type_lengths[type] == entry->name_length &&
                !(strncmp (types[type], entry->name, entry->name_length)))
            {
                break;
            }
            if (type < best_type)
            {
            if (best)
                XauDisposeAuth (best);
            best = entry;
            best_type = type;
            if (type == 0)
                break;
            continue;
            }
        }
        XauDisposeAuth (entry);
    }
    printf("%s, %s, %d\n", __func__, __FILE__, __LINE__);
    (void) fclose (auth_file);
    return best;
}