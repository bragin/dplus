/*
 * File: icon.c
 *
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif  /* _WIN32 */

/* The icon's Windows resource number.
 * Keep this value consistent with dillo.rc */
#define IDI_MAIN_ICON 101

#include "icon.h"

/*
 * Return the program icon as a character string.
 * The specific implementation varies across platforms.
 */
char *a_Icon_load(void)
{
#ifdef _WIN32
   HMODULE hModule = GetModuleHandle(NULL);  /* running process */
   return (char*)LoadImage(hModule, MAKEINTRESOURCE(IDI_MAIN_ICON),
                           IMAGE_ICON, 16, 16, 0);
#else /* _WIN32 */
   return (char*)0;
#endif /* _WIN32 */
}
