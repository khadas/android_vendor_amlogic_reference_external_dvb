#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief iconv functions
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2014-03-18: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <stdlib.h>
#include <am_iconv.h>
#include <am_debug.h>

UConverter* (*am_ucnv_open_ptr)(const char *converterName, UErrorCode *err);
void (*am_ucnv_close_ptr)(UConverter * converter);
void (*am_ucnv_convertEx_ptr)(UConverter *targetCnv, UConverter *sourceCnv,
		char **target, const char *targetLimit,
		const char **source, const char *sourceLimit,
		UChar *pivotStart, UChar **pivotSource,
		UChar **pivotTarget, const UChar *pivotLimit,
		UBool reset, UBool flush,
		UErrorCode *pErrorCode);
void (*am_u_setDataDirectory_ptr)(const char *directory);
void (*am_u_init_ptr)(long *status);

void
am_ucnv_dlink(void)
{
	static void* handle = NULL;

	if(handle == NULL) {
		handle = dlopen("libicuuc.so", RTLD_LAZY);
		setenv("ICU_DATA", "/system/usr/icu", 1);
	}
	assert(handle);

#define LOAD_UCNV_SYMBOL(name, post)\
	if(!am_##name##_ptr)\
		am_##name##_ptr = dlsym(handle, #name post);
#define LOAD_UCNV_SYMBOLS(post)\
	LOAD_UCNV_SYMBOL(ucnv_open, post)\
	LOAD_UCNV_SYMBOL(ucnv_close, post)\
	LOAD_UCNV_SYMBOL(ucnv_convertEx, post)\
	LOAD_UCNV_SYMBOL(u_setDataDirectory, post)\
	LOAD_UCNV_SYMBOL(u_init, post)

#define CHECK_LOAD_SYMBOL(name)\
	if(!am_##name##_ptr){\
		AM_DEBUG(1, #name" not found. ucnv init fail.");}
#define CHECK_LOAD_SYMBOLS()\
	CHECK_LOAD_SYMBOL(ucnv_open)\
	CHECK_LOAD_SYMBOL(ucnv_close)\
	CHECK_LOAD_SYMBOL(ucnv_convertEx)\
	CHECK_LOAD_SYMBOL(u_setDataDirectory)\
	CHECK_LOAD_SYMBOL(u_init)

	LOAD_UCNV_SYMBOLS("")
	LOAD_UCNV_SYMBOLS("_48")
	LOAD_UCNV_SYMBOLS("_51")
	LOAD_UCNV_SYMBOLS("_53")
	LOAD_UCNV_SYMBOLS("_55")
	LOAD_UCNV_SYMBOLS("_56")

	CHECK_LOAD_SYMBOLS()

	if (am_u_setDataDirectory_ptr)
		am_u_setDataDirectory_ptr("/system/usr/icu");
	if (am_u_init_ptr) {
		long status = 0;
		am_u_init_ptr(&status);
		if (status > 0) {
			AM_DEBUG(1, "icu init fail. [%ld]", status);
		}
	}
}

