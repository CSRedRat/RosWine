/*
 *
 * Copyright 2011 Alistair Leslie-Hughes
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "shellapi.h"
#include "mscoree.h"
#include "corhdr.h"
#include "metahost.h"
#include "cordebug.h"
#include "wine/list.h"
#include "mscoree_private.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

static inline CorDebug *impl_from_ICorDebug( ICorDebug *iface )
{
    return CONTAINING_RECORD(iface, CorDebug, ICorDebug_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI CorDebug_QueryInterface(ICorDebug *iface, REFIID riid, void **ppvObject)
{
    CorDebug *This = impl_from_ICorDebug( iface );

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorDebug ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->ICorDebug_iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorDebug_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CorDebug_AddRef(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI CorDebug_Release(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref=%u\n", This, ref);

    if (ref == 0)
    {
        if(This->runtimehost)
            ICLRRuntimeHost_Release(This->runtimehost);

        if(This->pCallback)
            ICorDebugManagedCallback2_Release(This->pCallback2);

        if(This->pCallback)
            ICorDebugManagedCallback_Release(This->pCallback);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ICorDebug methods ***/
static HRESULT WINAPI CorDebug_Initialize(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p\n", This);
    return S_OK;
}

static HRESULT WINAPI CorDebug_Terminate(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_SetManagedHandler(ICorDebug *iface, ICorDebugManagedCallback *pCallback)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    HRESULT hr;
    ICorDebugManagedCallback2 *pCallback2;

    TRACE("%p (%p)\n", This, pCallback);

    if(!pCallback)
        return E_INVALIDARG;

    hr = ICorDebugManagedCallback_QueryInterface(pCallback, &IID_ICorDebugManagedCallback2, (void**)&pCallback2);
    if(hr == S_OK)
    {
        if(This->pCallback2)
            ICorDebugManagedCallback2_Release(This->pCallback2);

        if(This->pCallback)
            ICorDebugManagedCallback_Release(This->pCallback);

        This->pCallback = pCallback;
        This->pCallback2 = pCallback2;

        ICorDebugManagedCallback_AddRef(This->pCallback);
    }

    return hr;
}

static HRESULT WINAPI CorDebug_SetUnmanagedHandler(ICorDebug *iface, ICorDebugUnmanagedCallback *pCallback)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %p\n", This, pCallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_CreateProcess(ICorDebug *iface, LPCWSTR lpApplicationName,
            LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
            LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
            DWORD dwCreationFlags, PVOID lpEnvironment,LPCWSTR lpCurrentDirectory,
            LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
            CorDebugCreateProcessFlags debuggingFlags, ICorDebugProcess **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %s %s %p %p %d %d %p %s %p %p %d %p\n", This, debugstr_w(lpApplicationName),
            debugstr_w(lpCommandLine), lpProcessAttributes, lpThreadAttributes,
            bInheritHandles, dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory),
            lpStartupInfo, lpProcessInformation, debuggingFlags, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_DebugActiveProcess(ICorDebug *iface, DWORD id, BOOL win32Attach,
            ICorDebugProcess **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %d %d %p\n", This, id, win32Attach, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_EnumerateProcesses( ICorDebug *iface, ICorDebugProcessEnum **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %p\n", This, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_GetProcess(ICorDebug *iface, DWORD dwProcessId, ICorDebugProcess **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %d %p\n", This, dwProcessId, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_CanLaunchOrAttach(ICorDebug *iface, DWORD dwProcessId,
            BOOL win32DebuggingEnabled)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %d %d\n", This, dwProcessId, win32DebuggingEnabled);
    return E_NOTIMPL;
}

static const struct ICorDebugVtbl cordebug_vtbl =
{
    CorDebug_QueryInterface,
    CorDebug_AddRef,
    CorDebug_Release,
    CorDebug_Initialize,
    CorDebug_Terminate,
    CorDebug_SetManagedHandler,
    CorDebug_SetUnmanagedHandler,
    CorDebug_CreateProcess,
    CorDebug_DebugActiveProcess,
    CorDebug_EnumerateProcesses,
    CorDebug_GetProcess,
    CorDebug_CanLaunchOrAttach
};

HRESULT CorDebug_Create(ICLRRuntimeHost *runtimehost, IUnknown** ppUnk)
{
    CorDebug *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return E_OUTOFMEMORY;

    This->ICorDebug_iface.lpVtbl = &cordebug_vtbl;
    This->ref = 1;
    This->pCallback = NULL;
    This->pCallback2 = NULL;
    This->runtimehost = runtimehost;

    if(This->runtimehost)
        ICLRRuntimeHost_AddRef(This->runtimehost);

    *ppUnk = (IUnknown*)This;

    return S_OK;
}