#pragma once

#include <stdlib.h>

/*
** Valid type ids for NLS objects
*/
typedef enum __lc_type_id_t {
	_LC_CAR = 1,
	_LC_LOCALE = 2,
	_LC_CHARMAP = 3,
	_LC_CTYPE = 4,
	_LC_COLLATE = 5,
	_LC_NUMERIC = 6,
	_LC_MONETARY = 7,
	_LC_TIME = 8,
	_LC_MESSAGES = 9
} __lc_type_id_t;


typedef struct {
	__lc_type_id_t	type_id;
	unsigned int	magic;
	unsigned short	major_ver;
	unsigned short	minor_ver;
	size_t	size;
} _LC_object_t;