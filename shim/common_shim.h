/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020-2021, Dolby Laboratories

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/

#ifndef __COMMON_SHIM_H_
#define __COMMON_SHIM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#if defined(HAVE_DLADDR)
 #include <dlfcn.h>
#elif defined(HAVE_WINAPI)
 #include <Windows.h>
#else
 #error dladdr not implemented
#endif


#include <assert.h>

static inline void *
get_proc_address (void *lib, const char *name)
{
  assert (lib);
  assert (name);

#if defined(HAVE_DLADDR)
  return dlsym (lib, name);
#elif defined(HAVE_WINAPI)
  return GetProcAddress((HMODULE)lib, name);
#endif
}

static inline void *
open_dynamic_lib (const char *name)
{
#if defined(HAVE_DLADDR)
  void *lib = dlopen (name, RTLD_NOW | RTLD_LOCAL);
#elif defined(HAVE_WINAPI)
  void *lib = LoadLibrary(name);
#endif

  if (!lib)
    return 0;

  return lib;
}

#endif // __COMMON_SHIM_H_
