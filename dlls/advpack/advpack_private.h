/*
 * Advpack private header
 *
 * Copyright 2006 James Hawkins
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __ADVPACK_PRIVATE_H
#define __ADVPACK_PRIVATE_H

HRESULT do_ocx_reg(HMODULE hocx, BOOL do_reg) DECLSPEC_HIDDEN;
LPWSTR get_parameter(LPWSTR *params, WCHAR separator) DECLSPEC_HIDDEN;
void set_ldids(HINF hInf, LPCWSTR pszInstallSection, LPCWSTR pszWorkingDir) DECLSPEC_HIDDEN;

HRESULT launch_exe(LPCWSTR cmd, LPCWSTR dir, HANDLE *phEXE) DECLSPEC_HIDDEN;

#endif /* __ADVPACK_PRIVATE_H */
