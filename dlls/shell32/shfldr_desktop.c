
/*
 *    Virtual Desktop Folder
 *
 *    Copyright 1997            Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "shlguid.h"

#include "pidl.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "shellfolder.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"
#include "winbase.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/* Undocumented functions from shdocvw */
extern HRESULT WINAPI IEParseDisplayNameWithBCW(DWORD codepage, LPCWSTR lpszDisplayName, LPBC pbc, LPITEMIDLIST *ppidl);

/***********************************************************************
*     Desktopfolder implementation
*/

typedef struct {
    IShellFolder2 IShellFolder2_iface;
    IPersistFolder2 IPersistFolder2_iface;
    IPersistFolder3     IPersistFolder3_iface;
    LONG ref;
    ISFHelper ISFHelper_iface;
    CHAR *m_pszPath;

    LPITEMIDLIST m_pidlLocation; /* Location in the shell namespace */
    DWORD        m_dwPathMode;
    DWORD        m_dwAttributes;
    const CLSID  *m_pCLSID;
    DWORD        m_dwDropEffectsMask;
    /* both paths are parsible from the desktop */
    LPWSTR sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */
    LPITEMIDLIST pidlRoot;  /* absolute pidl */

    UINT cfShellIDList;        /* clipboardformat for IDropTarget */
    BOOL fAcceptFmt;        /* flag for pending Drop */
} IDesktopFolderImpl;

static IDesktopFolderImpl *cached_sf;

static inline IDesktopFolderImpl *impl_from_IShellFolder2(IShellFolder2 *iface)
{
    return CONTAINING_RECORD(iface, IDesktopFolderImpl, IShellFolder2_iface);
}

static inline IDesktopFolderImpl *impl_from_IPersistFolder2( IPersistFolder2 *iface )
{
    return CONTAINING_RECORD(iface, IDesktopFolderImpl, IPersistFolder2_iface);
}

static inline IDesktopFolderImpl *impl_from_IPersistFolder3(IPersistFolder3 *iface)
{
    return CONTAINING_RECORD(iface, IDesktopFolderImpl, IPersistFolder3_iface);
}

static inline IDesktopFolderImpl * impl_from_ISFHelper(ISFHelper * iface)
{
    return CONTAINING_RECORD(iface, IDesktopFolderImpl, ISFHelper_iface);
}

static const shvheader desktop_header[] =
{
    { &FMTID_Storage, PID_STG_NAME, IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 15 },
    { &FMTID_Storage, PID_STG_SIZE, IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 10 },
    { &FMTID_Storage, PID_STG_STORAGETYPE, IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 10 },
    { &FMTID_Storage, PID_STG_WRITETIME, IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12 },
    { &FMTID_Storage, PID_STG_ATTRIBUTES, IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 5  },
};

/**************************************************************************
 *    ISF_Desktop_fnQueryInterface
 *
 */
static HRESULT WINAPI ISF_Desktop_fnQueryInterface(
                IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    if (!ppvObj) return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = &This->IShellFolder2_iface;
    }
    else if (IsEqualIID (riid, &IID_IPersist) ||
             IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = &This->IPersistFolder2_iface;
    }
    else if (IsEqualIID (riid, &IID_ISFHelper))
    {
        *ppvObj = &This->ISFHelper_iface;
    }

    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_Desktop_fnAddRef (IShellFolder2 * iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI ISF_Desktop_fnRelease (IShellFolder2 * iface)
{
    return 1; /* non-heap based object */
}

/**************************************************************************
 *    ISF_Desktop_fnParseDisplayName
 *
 * NOTES
 *    "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" and "" binds
 *    to MyComputer
 */
static HRESULT WINAPI ISF_Desktop_fnParseDisplayName (IShellFolder2 * iface,
                HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
                DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    PARSEDURLW urldata;
    HRESULT hr = S_OK;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           This, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!ppidl) return E_INVALIDARG;
    *ppidl = 0;

    if (!lpszDisplayName) return E_INVALIDARG;

    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    urldata.cbSize = sizeof(urldata);

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE ("-- element: %s\n", debugstr_w (szElement));
        SHCLSIDFromStringW (szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, &clsid);
    }
    else if (PathGetDriveNumberW (lpszDisplayName) >= 0)
    {
        /* it's a filesystem path with a drive. Let MyComputer/UnixDosFolder parse it */
        if (UNIXFS_is_rooted_at_desktop()) 
            pidlTemp = _ILCreateGuid(PT_GUID, &CLSID_UnixDosFolder);
        else
            pidlTemp = _ILCreateMyComputer ();
        szNext = lpszDisplayName;
    }
    else if (PathIsUNCW(lpszDisplayName))
    {
        pidlTemp = _ILCreateNetwork();
        szNext = lpszDisplayName;
    }
    else if( (pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName)) )
    {
        *ppidl = pidlTemp;
        return S_OK;
    }
    else if (SUCCEEDED(ParseURLW(lpszDisplayName, &urldata)))
    {
        if (urldata.nScheme == URL_SCHEME_SHELL) /* handle shell: urls */
        {
            TRACE ("-- shell url: %s\n", debugstr_w(urldata.pszSuffix));
            SHCLSIDFromStringW (urldata.pszSuffix+2, &clsid);
            pidlTemp = _ILCreateGuid (PT_GUID, &clsid);
        }
        else
            return IEParseDisplayNameWithBCW(CP_ACP,lpszDisplayName,pbc,ppidl);
    }
    else
    {
        /* it's a filesystem path on the desktop. Let a FSFolder parse it */

        if (*lpszDisplayName)
        {
            if (*lpszDisplayName == '/')
            {
                /* UNIX paths should be parsed by unixfs */
                IShellFolder *unixFS;
                hr = UnixFolder_Constructor(NULL, &IID_IShellFolder, (LPVOID*)&unixFS);
                if (SUCCEEDED(hr))
                {
                    hr = IShellFolder_ParseDisplayName(unixFS, NULL, NULL,
                            lpszDisplayName, NULL, &pidlTemp, NULL);
                    IShellFolder_Release(unixFS);
                }
            }
            else
            {
                /* build a complete path to create a simple pidl */
                WCHAR szPath[MAX_PATH];
                LPWSTR pathPtr;

                lstrcpynW(szPath, This->sPathTarget, MAX_PATH);
                pathPtr = PathAddBackslashW(szPath);
                if (pathPtr)
                {
                    lstrcpynW(pathPtr, lpszDisplayName, MAX_PATH - (pathPtr - szPath));
                    hr = _ILCreateFromPathW(szPath, &pidlTemp);
                }
                else
                {
                    /* should never reach here, but for completeness */
                    hr = E_NOT_SUFFICIENT_BUFFER;
                }
            }
        }
        else
            pidlTemp = _ILCreateMyComputer();

        szNext = NULL;
    }

    if (SUCCEEDED(hr) && pidlTemp)
    {
        if (szNext && *szNext)
        {
            hr = SHELL32_ParseNextElement(iface, hwndOwner, pbc,
                    &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
        }
        else
        {
            if (pdwAttributes && *pdwAttributes)
                hr = SHELL32_GetItemAttributes(iface, pidlTemp, pdwAttributes);
        }
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08x)\n", This, hr);

    return hr;
}

static void add_shell_namespace_extensions(IEnumIDListImpl *list, HKEY root)
{
    static const WCHAR Desktop_NameSpaceW[] = { 'S','O','F','T','W','A','R','E','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'E','x','p','l','o','r','e','r','\\','D','e','s','k','t','o','p','\\',
        'N','a','m','e','s','p','a','c','e','\0' };
    static const WCHAR clsidfmtW[] = {'C','L','S','I','D','\\','%','s','\\',
        'S','h','e','l','l','F','o','l','d','e','r',0};
    static const WCHAR attributesW[] = {'A','t','t','r','i','b','u','t','e','s',0};
    WCHAR guid[39], clsidkeyW[ARRAY_SIZE(clsidfmtW) + 39];
    DWORD size, i = 0;
    HKEY hkey;

    if (RegOpenKeyExW(root, Desktop_NameSpaceW, 0, KEY_READ, &hkey))
        return;

    size = ARRAY_SIZE(guid);
    while (!RegEnumKeyExW(hkey, i++, guid, &size, 0, NULL, NULL, NULL))
    {
        DWORD attributes, value_size = sizeof(attributes);

        /* Check if extension is configured as nonenumerable */
        sprintfW(clsidkeyW, clsidfmtW, guid);
        RegGetValueW(HKEY_CLASSES_ROOT, clsidkeyW, attributesW, RRF_RT_REG_DWORD | RRF_ZEROONFAILURE,
            NULL, &attributes, &value_size);

        if (!(attributes & SFGAO_NONENUMERATED))
            AddToEnumList(list, _ILCreateGuidFromStrW(guid));
        size = ARRAY_SIZE(guid);
    }

    RegCloseKey(hkey);
}

/**************************************************************************
 *  CreateDesktopEnumList()
 */
static BOOL CreateDesktopEnumList(IEnumIDListImpl *list, DWORD dwFlags)
{
    BOOL ret = TRUE;
    WCHAR szPath[MAX_PATH];

    TRACE("(%p)->(flags=0x%08x)\n", list, dwFlags);

    /* enumerate the root folders */
    if (dwFlags & SHCONTF_FOLDERS)
    {
        ret = AddToEnumList(list, _ILCreateMyComputer());
        add_shell_namespace_extensions(list, HKEY_LOCAL_MACHINE);
        add_shell_namespace_extensions(list, HKEY_CURRENT_USER);
    }

    /* enumerate the elements in %windir%\desktop */
    ret = ret && SHGetSpecialFolderPathW(0, szPath, CSIDL_DESKTOPDIRECTORY, FALSE);
    ret = ret && CreateFolderEnumList(list, szPath, dwFlags);

    return ret;
}

/**************************************************************************
 *        ISF_Desktop_fnEnumObjects
 */
static HRESULT WINAPI ISF_Desktop_fnEnumObjects (IShellFolder2 * iface,
                HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    IEnumIDListImpl *list;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n",
           This, hwndOwner, dwFlags, ppEnumIDList);

    if (!(list = IEnumIDList_Constructor()))
        return E_OUTOFMEMORY;
    CreateDesktopEnumList(list, dwFlags);
    *ppEnumIDList = &list->IEnumIDList_iface;

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
 *        ISF_Desktop_fnBindToObject
 */
static HRESULT WINAPI ISF_Desktop_fnBindToObject (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild( This->pidlRoot, This->sPathTarget, pidl, riid, ppvOut );
}

/**************************************************************************
 *    ISF_Desktop_fnBindToStorage
 */
static HRESULT WINAPI ISF_Desktop_fnBindToStorage (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     ISF_Desktop_fnCompareIDs
 */
static HRESULT WINAPI ISF_Desktop_fnCompareIDs (IShellFolder2 *iface,
                        LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    hr = SHELL32_CompareIDs(iface, lParam, pidl1, pidl2);
    TRACE ("-- 0x%08x\n", hr);
    return hr;
}

/**************************************************************************
 *    ISF_Desktop_fnCreateViewObject
 */
static HRESULT WINAPI ISF_Desktop_fnCreateViewObject (IShellFolder2 * iface,
                              HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n",
           This, hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (!ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IContextMenu))
    {
        WARN ("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IShellView))
    {
        pShellView = IShellView_Constructor ((IShellFolder *) iface);
        if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
        }
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
 *  ISF_Desktop_fnGetAttributesOf
 */
static HRESULT WINAPI ISF_Desktop_fnGetAttributesOf (IShellFolder2 * iface,
                UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    static const DWORD dwDesktopAttributes = 
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
    static const DWORD dwMyComputerAttributes = 
        SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET |
        SFGAO_DROPTARGET | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
           This, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;
    
    if(cidl == 0) {
        *rgfInOut &= dwDesktopAttributes; 
    } else {
        while (cidl > 0 && *apidl) {
            pdump (*apidl);
            if (_ILIsDesktop(*apidl)) { 
                *rgfInOut &= dwDesktopAttributes;
            } else if (_ILIsMyComputer(*apidl)) {
                *rgfInOut &= dwMyComputerAttributes;
            } else {
                SHELL32_GetItemAttributes(iface, *apidl, rgfInOut);
            }
            apidl++;
            cidl--;
        }
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08x\n", *rgfInOut);

    return S_OK;
}

/**************************************************************************
 *    ISF_Desktop_fnGetUIObjectOf
 *
 * PARAMETERS
 *  HWND           hwndOwner, //[in ] Parent window for any output
 *  UINT           cidl,      //[in ] array size
 *  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
 *  REFIID         riid,      //[in ] Requested Interface
 *  UINT*          prgfInOut, //[   ] reserved
 *  LPVOID*        ppvObject) //[out] Resulting Interface
 *
 */
static HRESULT WINAPI ISF_Desktop_fnGetUIObjectOf (IShellFolder2 * iface,
                HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
       This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu))
    {
        if (cidl > 0)
            return ItemMenu_Constructor((IShellFolder*)iface, This->pidlRoot, apidl, cidl, riid, ppvOut);
        else
            return BackgroundMenu_Constructor((IShellFolder*)iface, TRUE, riid, ppvOut);
    }
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) IDataObject_Constructor( hwndOwner,
                                                  This->pidlRoot, apidl, cidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1))
    {
        hr = IShellFolder2_QueryInterface (iface,
                                          &IID_IDropTarget, (LPVOID *) & pObj);
    }
    else if ((IsEqualIID(riid,&IID_IShellLinkW) ||
              IsEqualIID(riid,&IID_IShellLinkA)) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl, &pObj);
        SHFree (pidl);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08x\n", This, hr);
    return hr;
}

/**************************************************************************
 *    ISF_Desktop_fnGetDisplayNameOf
 *
 * NOTES
 *    special case: pidl = null gives desktop-name back
 */
static HRESULT WINAPI ISF_Desktop_fnGetDisplayNameOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr = S_OK;
    LPWSTR pszPath;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = CoTaskMemAlloc((MAX_PATH +1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if (_ILIsDesktop (pidl))
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
            (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            strcpyW(pszPath, This->sPathTarget);
        else
            HCR_GetClassNameW(&CLSID_ShellDesktop, pszPath, MAX_PATH);
    }
    else if (_ILIsPidlSimple (pidl))
    {
        GUID const *clsid;

        if ((clsid = _ILGetGUIDPointer (pidl)))
        {
            if ((GET_SHGDN_FOR (dwFlags) & (SHGDN_FORPARSING | SHGDN_FORADDRESSBAR)) == SHGDN_FORPARSING)
            {
                BOOL bWantsForParsing;

                /*
                 * We can only get a filesystem path from a shellfolder if the
                 *  value WantsFORPARSING in CLSID\\{...}\\shellfolder exists.
                 *
                 * Exception: The MyComputer folder doesn't have this key,
                 *   but any other filesystem backed folder it needs it.
                 */
                if (IsEqualIID (clsid, &CLSID_MyComputer))
                {
                    bWantsForParsing = TRUE;
                }
                else
                {
                    /* get the "WantsFORPARSING" flag from the registry */
                    static const WCHAR clsidW[] =
                     { 'C','L','S','I','D','\\',0 };
                    static const WCHAR shellfolderW[] =
                     { '\\','s','h','e','l','l','f','o','l','d','e','r',0 };
                    static const WCHAR wantsForParsingW[] =
                     { 'W','a','n','t','s','F','o','r','P','a','r','s','i','n',
                     'g',0 };
                    WCHAR szRegPath[100];
                    LONG r;

                    lstrcpyW (szRegPath, clsidW);
                    SHELL32_GUIDToStringW (clsid, &szRegPath[6]);
                    lstrcatW (szRegPath, shellfolderW);
                    r = SHGetValueW(HKEY_CLASSES_ROOT, szRegPath,
                                    wantsForParsingW, NULL, NULL, NULL);
                    if (r == ERROR_SUCCESS)
                        bWantsForParsing = TRUE;
                    else
                        bWantsForParsing = FALSE;
                }

                if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                     bWantsForParsing)
                {
                    /*
                     * we need the filesystem path to the destination folder.
                     * Only the folder itself can know it
                     */
                    hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags,
                                                        pszPath,
                                                        MAX_PATH);
                }
                else
                {
                    /* parsing name like ::{...} */
                    pszPath[0] = ':';
                    pszPath[1] = ':';
                    SHELL32_GUIDToStringW (clsid, &pszPath[2]);
                }
            }
            else
            {
                /* user friendly name */
                HCR_GetClassNameW (clsid, pszPath, MAX_PATH);
            }
        }
        else
        {
            int cLen = 0;

            /* file system folder or file rooted at the desktop */
            if ((GET_SHGDN_FOR(dwFlags) == SHGDN_FORPARSING) &&
                (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
            {
                lstrcpynW(pszPath, This->sPathTarget, MAX_PATH - 1);
                PathAddBackslashW(pszPath);
                cLen = lstrlenW(pszPath);
            }

            _ILSimpleGetTextW(pidl, pszPath + cLen, MAX_PATH - cLen);

            if (!_ILIsFolder(pidl))
                SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);
        }
    }
    else
    {
        /* a complex pidl, let the subfolder do the work */
        hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags,
                                            pszPath, MAX_PATH);
    }

    if (SUCCEEDED(hr))
    {
        /* Win9x always returns ANSI strings, NT always returns Unicode strings */
        if (GetVersion() & 0x80000000)
        {
            strRet->uType = STRRET_CSTR;
            if (!WideCharToMultiByte(CP_ACP, 0, pszPath, -1, strRet->u.cStr, MAX_PATH,
                                     NULL, NULL))
                strRet->u.cStr[0] = '\0';
            CoTaskMemFree(pszPath);
        }
        else
        {
            strRet->uType = STRRET_WSTR;
            strRet->u.pOleStr = pszPath;
        }
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s,0x%08x)\n", This,
    strRet->uType == STRRET_CSTR ? strRet->u.cStr :
    debugstr_w(strRet->u.pOleStr), hr);
    return hr;
}

/**************************************************************************
 *  ISF_Desktop_fnSetNameOf
 *  Changes the name of a file object or subfolder, possibly changing its item
 *  identifier in the process.
 *
 * PARAMETERS
 *  HWND          hwndOwner,  //[in ] Owner window for output
 *  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
 *  LPCOLESTR     lpszName,   //[in ] the items new display name
 *  DWORD         dwFlags,    //[in ] SHGNO formatting flags
 *  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
 */
static HRESULT WINAPI ISF_Desktop_fnSetNameOf (IShellFolder2 * iface,
                HWND hwndOwner, LPCITEMIDLIST pidl,    /* simple pidl */
                LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p) stub\n", This, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultSearchGUID(IShellFolder2 *iface, GUID *guid)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    TRACE("(%p)->(%p)\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnEnumSearches (IShellFolder2 *iface,
                IEnumExtraSearch ** ppenum)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)->(%p) stub\n", This, ppenum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumn(IShellFolder2 *iface, DWORD reserved, ULONG *sort, ULONG *display)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%#x, %p, %p)\n", This, reserved, sort, display);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumnState (
                IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%d %p)\n", This, iColumn, pcsFlags);

    if (!pcsFlags || iColumn >= ARRAY_SIZE(desktop_header))
        return E_INVALIDARG;

    *pcsFlags = desktop_header[iColumn].pcsFlags;

    return S_OK;
}

static HRESULT WINAPI ISF_Desktop_fnGetDetailsEx (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)->(%p %p %p) stub\n", This, pidl, pscid, pv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDetailsOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    HRESULT hr = S_OK;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= ARRAY_SIZE(desktop_header))
        return E_INVALIDARG;

    if (!pidl)
        return SHELL32_GetColumnDetails(desktop_header, iColumn, psd);

    /* the data from the pidl */
    psd->str.uType = STRRET_CSTR;
    switch (iColumn)
    {
    case 0:        /* name */
        hr = IShellFolder2_GetDisplayNameOf(iface, pidl,
                   SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
        break;
    case 1:        /* size */
        _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case 2:        /* type */
        _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case 3:        /* date */
        _ILGetFileDate (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case 4:        /* attributes */
        _ILGetFileAttributes (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    }

    return hr;
}

static HRESULT WINAPI ISF_Desktop_fnMapColumnToSCID(IShellFolder2 *iface, UINT column, SHCOLUMNID *scid)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%u %p)\n", This, column, scid);

    if (column >= ARRAY_SIZE(desktop_header))
        return E_INVALIDARG;

    return shellfolder_map_column_to_scid(desktop_header, column, scid);
}

static const IShellFolder2Vtbl vt_MCFldr_ShellFolder2 =
{
    ISF_Desktop_fnQueryInterface,
    ISF_Desktop_fnAddRef,
    ISF_Desktop_fnRelease,
    ISF_Desktop_fnParseDisplayName,
    ISF_Desktop_fnEnumObjects,
    ISF_Desktop_fnBindToObject,
    ISF_Desktop_fnBindToStorage,
    ISF_Desktop_fnCompareIDs,
    ISF_Desktop_fnCreateViewObject,
    ISF_Desktop_fnGetAttributesOf,
    ISF_Desktop_fnGetUIObjectOf,
    ISF_Desktop_fnGetDisplayNameOf,
    ISF_Desktop_fnSetNameOf,
    /* ShellFolder2 */
    ISF_Desktop_fnGetDefaultSearchGUID,
    ISF_Desktop_fnEnumSearches,
    ISF_Desktop_fnGetDefaultColumn,
    ISF_Desktop_fnGetDefaultColumnState,
    ISF_Desktop_fnGetDetailsEx,
    ISF_Desktop_fnGetDetailsOf,
    ISF_Desktop_fnMapColumnToSCID
};

/**************************************************************************
 *    IPersist
 */
static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnQueryInterface(
    IPersistFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObj);
}

static ULONG WINAPI ISF_Desktop_IPersistFolder2_fnAddRef(
    IPersistFolder2 *iface)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI ISF_Desktop_IPersistFolder2_fnRelease(
    IPersistFolder2 *iface)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnGetClassID(
    IPersistFolder2 *iface, CLSID *clsid)
{
    *clsid = CLSID_ShellDesktop;
    return S_OK;
}
static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnInitialize(
    IPersistFolder2 *iface, LPCITEMIDLIST pidl)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    FIXME ("(%p)->(%p) stub\n", This, pidl);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnGetCurFolder(
    IPersistFolder2 *iface, LPITEMIDLIST *ppidl)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    *ppidl = ILClone(This->pidlRoot);
    return S_OK;
}

static const IPersistFolder2Vtbl vt_IPersistFolder2 =
{
    ISF_Desktop_IPersistFolder2_fnQueryInterface,
    ISF_Desktop_IPersistFolder2_fnAddRef,
    ISF_Desktop_IPersistFolder2_fnRelease,
    ISF_Desktop_IPersistFolder2_fnGetClassID,
    ISF_Desktop_IPersistFolder2_fnInitialize,
    ISF_Desktop_IPersistFolder2_fnGetCurFolder
};

void release_desktop_folder(void)
{
    if (!cached_sf) return;
    SHFree(cached_sf->pidlRoot);
    SHFree(cached_sf->sPathTarget);
    LocalFree(cached_sf);
}

/*-------------------------------*/

static BOOL UNIXFS_get_unix_path(LPCWSTR pszDosPath, char *pszCanonicalPath)
{
    char *pPathTail, *pElement, *pCanonicalTail, szPath[FILENAME_MAX], *pszUnixPath, mb_path[FILENAME_MAX];
    BOOL has_failed = FALSE;
    WCHAR wszDrive[] = { '?', ':', '\\', 0 }, dospath[MAX_PATH], *dospath_end;
    int cDriveSymlinkLen;
    void *redir;

    TRACE("(pszDosPath=%s, pszCanonicalPath=%p)\n", debugstr_w(pszDosPath), pszCanonicalPath);

    if (!pszDosPath || pszDosPath[1] != ':')
        return FALSE;

    /* Get the canonicalized unix path corresponding to the drive letter. */
    wszDrive[0] = pszDosPath[0];
    pszUnixPath = wine_get_unix_file_name(wszDrive);
    if (!pszUnixPath) return FALSE;
    cDriveSymlinkLen = strlen(pszUnixPath);
    pElement = realpath(pszUnixPath, szPath);
    heap_free(pszUnixPath);
    if (!pElement) return FALSE;
    if (szPath[strlen(szPath)-1] != '/') strcat(szPath, "/");

    /* Append the part relative to the drive symbolic link target. */
    lstrcpyW(dospath, pszDosPath);
    dospath_end = dospath + lstrlenW(dospath);
    /* search for the most valid UNIX path possible, then append missing
     * path parts */
    Wow64DisableWow64FsRedirection(&redir);
    while(!(pszUnixPath = wine_get_unix_file_name(dospath))){
        if(has_failed){
            *dospath_end = '/';
            --dospath_end;
        }else
            has_failed = TRUE;
        while(*dospath_end != '\\' && *dospath_end != '/'){
            --dospath_end;
            if(dospath_end < dospath)
                break;
        }
        *dospath_end = '\0';
    }
    Wow64RevertWow64FsRedirection(redir);
    if(dospath_end < dospath)
        return FALSE;
    strcat(szPath, pszUnixPath + cDriveSymlinkLen);
    heap_free(pszUnixPath);

    if(has_failed && WideCharToMultiByte(CP_UNIXCP, 0, dospath_end + 1, -1,
                mb_path, FILENAME_MAX, NULL, NULL) > 0){
        strcat(szPath, "/");
        strcat(szPath, mb_path);
    }

    /* pCanonicalTail always points to the end of the canonical path constructed
     * thus far. pPathTail points to the still to be processed part of the input
     * path. pElement points to the path element currently investigated.
     */
    *pszCanonicalPath = '\0';
    pCanonicalTail = pszCanonicalPath;
    pPathTail = szPath;

    do {
        char cTemp;
            
        pElement = pPathTail;
        pPathTail = strchr(pPathTail+1, '/');
        if (!pPathTail) /* Last path element may not be terminated by '/'. */ 
            pPathTail = pElement + strlen(pElement);
        /* Temporarily terminate the current path element. Will be restored later. */
        cTemp = *pPathTail;
        *pPathTail = '\0';

        /* Skip "/." path elements */
        if (!strcmp("/.", pElement)) {
            *pPathTail = cTemp;
        } else if (!strcmp("/..", pElement)) {
            /* Remove last element in canonical path for "/.." elements, then skip. */
            char *pTemp = strrchr(pszCanonicalPath, '/');
            if (pTemp)
                pCanonicalTail = pTemp;
            *pCanonicalTail = '\0';
            *pPathTail = cTemp;
        } else {
            /* Directory or file. Copy to canonical path */
            if (pCanonicalTail - pszCanonicalPath + pPathTail - pElement + 1 > FILENAME_MAX)
                return FALSE;
                
            memcpy(pCanonicalTail, pElement, pPathTail - pElement + 1);
            pCanonicalTail += pPathTail - pElement;
            *pPathTail = cTemp;
        }
    } while (pPathTail[0] == '/');
   
    TRACE("--> %s\n", debugstr_a(pszCanonicalPath));
    
    return TRUE;
}

/*-------------------------------*/
static int UNIXFS_filename_from_shitemid(LPCITEMIDLIST pidl, char* pszPathElement) {
    FileStructW *pFileStructW = _ILGetFileStructW(pidl);
    int cLen = 0;

    if (pFileStructW) {
        cLen = WideCharToMultiByte(CP_UNIXCP, 0, pFileStructW->wszName, -1, pszPathElement,
            pszPathElement ? FILENAME_MAX : 0, 0, 0);
    } else {
        /* There might be pidls slipping in from shfldr_fs.c, which don't contain the
         * FileStructW field. In this case, we have to convert from CP_ACP to CP_UNIXCP. */
        char *pszText = _ILGetTextPointer(pidl);
        WCHAR *pwszPathElement = NULL;
        int cWideChars;
    
        cWideChars = MultiByteToWideChar(CP_ACP, 0, pszText, -1, NULL, 0);
        if (!cWideChars) goto cleanup;

        pwszPathElement = SHAlloc(cWideChars * sizeof(WCHAR));
        if (!pwszPathElement) goto cleanup;
    
        cWideChars = MultiByteToWideChar(CP_ACP, 0, pszText, -1, pwszPathElement, cWideChars);
        if (!cWideChars) goto cleanup; 

        cLen = WideCharToMultiByte(CP_UNIXCP, 0, pwszPathElement, -1, pszPathElement, 
            pszPathElement ? FILENAME_MAX : 0, 0, 0);

    cleanup:
        SHFree(pwszPathElement);
    }
        
    if (cLen) cLen--; /* Don't count terminating NUL! */
    return cLen;
}

#define PATHMODE_UNIX 0
#define PATHMODE_DOS  1

static HRESULT UNIXFS_initialize_target_folder(IDesktopFolderImpl *This, const char *szBasePath,
    LPCITEMIDLIST pidlSubFolder, DWORD dwAttributes)
{
    LPCITEMIDLIST current = pidlSubFolder;
    DWORD dwPathLen = strlen(szBasePath)+1;
    char *pNextDir;
    WCHAR *dos_name;

    /* Determine the path's length bytes */
    while (!_ILIsEmpty(current)) {
        dwPathLen += UNIXFS_filename_from_shitemid(current, NULL) + 1; /* For the '/' */
        current = ILGetNext(current);
    };

    /* Build the path and compute the attributes */
    This->m_dwAttributes = 
            dwAttributes|SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR|SFGAO_CANRENAME;
    This->m_pszPath = pNextDir = SHAlloc(dwPathLen);
    if (!This->m_pszPath) {
        WARN("SHAlloc failed!\n");
        return E_FAIL;
    }
    current = pidlSubFolder;
    strcpy(pNextDir, szBasePath);
    pNextDir += strlen(szBasePath);
    if (This->m_dwPathMode == PATHMODE_UNIX || IsEqualCLSID(&CLSID_MyDocuments, This->m_pCLSID))
        This->m_dwAttributes |= SFGAO_FILESYSTEM;
    while (!_ILIsEmpty(current)) {
        pNextDir += UNIXFS_filename_from_shitemid(current, pNextDir);
        *pNextDir++ = '/';
        current = ILGetNext(current);
    }
    *pNextDir='\0';

    if (!(This->m_dwAttributes & SFGAO_FILESYSTEM) &&
        ((dos_name = wine_get_dos_file_name(This->m_pszPath))))
    {
        This->m_dwAttributes |= SFGAO_FILESYSTEM;
        heap_free( dos_name );
    }

    return S_OK;
}

static HRESULT UNIXFS_delete_with_shfileop(IDesktopFolderImpl *This, UINT cidl, const LPCITEMIDLIST *apidl)
{
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPWSTR wszPathsList, wszListPos;
    SHFILEOPSTRUCTW op;
    HRESULT ret;
    UINT i;
    
    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    wszListPos = wszPathsList = heap_alloc(cidl*MAX_PATH*sizeof(WCHAR)+1);
    if (wszPathsList == NULL)
        return E_OUTOFMEMORY;
    for (i=0; i<cidl; i++) {
        LPWSTR wszDosPath;
        
        if (!_ILIsFolder(apidl[i]) && !_ILIsValue(apidl[i]))
            continue;
        if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelative))
        {
            heap_free(wszPathsList);
            return E_INVALIDARG;
        }
        wszDosPath = wine_get_dos_file_name(szAbsolute);
        if (wszDosPath == NULL || lstrlenW(wszDosPath) >= MAX_PATH)
        {
            heap_free(wszPathsList);
            heap_free(wszDosPath);
            return S_FALSE;
        }
        lstrcpyW(wszListPos, wszDosPath);
        wszListPos += lstrlenW(wszListPos)+1;
        heap_free(wszDosPath);
    }
    *wszListPos = 0;
    
    ZeroMemory(&op, sizeof(op));
    op.hwnd = GetActiveWindow();
    op.wFunc = FO_DELETE;
    op.pFrom = wszPathsList;
    op.fFlags = FOF_ALLOWUNDO;
    if (SHFileOperationW(&op))
    {
        WARN("SHFileOperationW failed\n");
        ret = E_FAIL;
    }
    else
        ret = S_OK;

    heap_free(wszPathsList);
    return ret;
}

static HRESULT UNIXFS_delete_with_syscalls(IDesktopFolderImpl *This, UINT cidl, const LPCITEMIDLIST *apidl)
{
    char szAbsolute[FILENAME_MAX], *pszRelative;
    static const WCHAR empty[] = {0};
    UINT i;
    
    if (!SHELL_ConfirmYesNoW(GetActiveWindow(), ASK_DELETE_SELECTED, empty))
        return S_OK;
    
    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    for (i=0; i<cidl; i++) {
        if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelative))
            return E_INVALIDARG;
        if (_ILIsFolder(apidl[i])) {
            if (rmdir(szAbsolute))
                return E_FAIL;
        } else if (_ILIsValue(apidl[i])) {
            if (unlink(szAbsolute))
                return E_FAIL;
        }
    }
    return S_OK;
}

static HRESULT UNIXFS_copy(LPCWSTR pwszDosSrc, LPCWSTR pwszDosDst)
{
    SHFILEOPSTRUCTW op;
    LPWSTR pwszSrc, pwszDst;
    HRESULT res = E_OUTOFMEMORY;
    UINT iSrcLen, iDstLen;

    if (!pwszDosSrc || !pwszDosDst)
        return E_FAIL;

    iSrcLen = lstrlenW(pwszDosSrc);
    iDstLen = lstrlenW(pwszDosDst);
    pwszSrc = heap_alloc((iSrcLen + 2) * sizeof(WCHAR));
    pwszDst = heap_alloc((iDstLen + 2) * sizeof(WCHAR));

    if (pwszSrc && pwszDst) {
        lstrcpyW(pwszSrc, pwszDosSrc);
        lstrcpyW(pwszDst, pwszDosDst);
        /* double null termination */
        pwszSrc[iSrcLen + 1] = 0;
        pwszDst[iDstLen + 1] = 0;

        ZeroMemory(&op, sizeof(op));
        op.hwnd = GetActiveWindow();
        op.wFunc = FO_COPY;
        op.pFrom = pwszSrc;
        op.pTo = pwszDst;
        op.fFlags = FOF_ALLOWUNDO;
        if (SHFileOperationW(&op))
        {
            WARN("SHFileOperationW failed\n");
            res = E_FAIL;
        }
        else
            res = S_OK;
    }

    heap_free(pwszSrc);
    heap_free(pwszDst);
    return res;
}

#define LEN_SHITEMID_FIXED_PART ((USHORT) \
    ( sizeof(USHORT)      /* SHITEMID's cb field. */ \
    + sizeof(PIDLTYPE)    /* PIDLDATA's type field. */ \
    + sizeof(FileStruct)  /* Well, the FileStruct. */ \
    - sizeof(char)        /* One char too much in FileStruct. */ \
    + sizeof(FileStructW) /* You name it. */ \
    - sizeof(WCHAR)       /* One WCHAR too much in FileStructW. */ \
    + sizeof(WORD) ))    

static const WCHAR wFileSystemBindData[] = {
    'F','i','l','e',' ','S','y','s','t','e','m',' ','B','i','n','d',' ','D','a','t','a',0};


static USHORT UNIXFS_shitemid_len_from_filename(
    const char *szPathElement, char **ppszPathElement, WCHAR **ppwszPathElement) 
{
    USHORT cbPidlLen = 0;
    WCHAR *pwszPathElement = NULL;
    char *pszPathElement = NULL;
    int cWideChars, cChars;

    /* There and Back Again: A Hobbit's Holiday. CP_UNIXCP might be some ANSI
     * codepage or it might be a real multi-byte encoding like utf-8. There is no
     * other way to figure out the length of the corresponding WCHAR and CP_ACP 
     * strings without actually doing the full CP_UNIXCP -> WCHAR -> CP_ACP cycle. */
    
    cWideChars = MultiByteToWideChar(CP_UNIXCP, 0, szPathElement, -1, NULL, 0);
    if (!cWideChars) goto cleanup;

    pwszPathElement = SHAlloc(cWideChars * sizeof(WCHAR));
    if (!pwszPathElement) goto cleanup;

    cWideChars = MultiByteToWideChar(CP_UNIXCP, 0, szPathElement, -1, pwszPathElement, cWideChars);
    if (!cWideChars) goto cleanup; 

    cChars = WideCharToMultiByte(CP_ACP, 0, pwszPathElement, -1, NULL, 0, 0, 0);
    if (!cChars) goto cleanup;

    pszPathElement = SHAlloc(cChars);
    if (!pszPathElement) goto cleanup;

    cChars = WideCharToMultiByte(CP_ACP, 0, pwszPathElement, -1, pszPathElement, cChars, 0, 0);
    if (!cChars) goto cleanup;

    /* (cChars & 0x1) is for the potential alignment byte */
    cbPidlLen = LEN_SHITEMID_FIXED_PART + cChars + (cChars & 0x1) + cWideChars * sizeof(WCHAR);
    
cleanup:
    if (cbPidlLen && ppszPathElement) 
        *ppszPathElement = pszPathElement;
    else 
        SHFree(pszPathElement);

    if (cbPidlLen && ppwszPathElement)
        *ppwszPathElement = pwszPathElement;
    else
        SHFree(pwszPathElement);

    return cbPidlLen;
}

static char* UNIXFS_build_shitemid(char *pszUnixPath, BOOL bMustExist, WIN32_FIND_DATAW *pFindData, void *pIDL) {
    LPPIDLDATA pIDLData;
    struct stat fileStat;
    WIN32_FIND_DATAW findData;
    char *pszComponentU, *pszComponentA;
    WCHAR *pwszComponentW;
    int cComponentULen, cComponentALen;
    USHORT cbLen;
    FileStructW *pFileStructW;
    WORD uOffsetW, *pOffsetW;

    TRACE("(pszUnixPath=%s, bMustExist=%s, pFindData=%p, pIDL=%p)\n",
            debugstr_a(pszUnixPath), bMustExist ? "T" : "F", pFindData, pIDL);

    if (pFindData)
        memcpy(&findData, pFindData, sizeof(WIN32_FIND_DATAW));
    else {
        memset(&findData, 0, sizeof(WIN32_FIND_DATAW));
        findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }

    /* We are only interested in regular files and directories. */
    if (stat(pszUnixPath, &fileStat)){
        if (bMustExist || errno != ENOENT)
            return NULL;
    } else {
        LARGE_INTEGER time;

        if (S_ISDIR(fileStat.st_mode))
            findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        else if (S_ISREG(fileStat.st_mode))
            findData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        else
            return NULL;

        findData.nFileSizeLow = (DWORD)fileStat.st_size;
        findData.nFileSizeHigh = fileStat.st_size >> 32;

        RtlSecondsSince1970ToTime(fileStat.st_mtime, &time);
        findData.ftLastWriteTime.dwLowDateTime = time.u.LowPart;
        findData.ftLastWriteTime.dwHighDateTime = time.u.HighPart;
        RtlSecondsSince1970ToTime(fileStat.st_atime, &time);
        findData.ftLastAccessTime.dwLowDateTime = time.u.LowPart;
        findData.ftLastAccessTime.dwHighDateTime = time.u.HighPart;
    }
    
    /* Compute the SHITEMID's length and wipe it. */
    pszComponentU = strrchr(pszUnixPath, '/') + 1;
    cComponentULen = strlen(pszComponentU);
    cbLen = UNIXFS_shitemid_len_from_filename(pszComponentU, &pszComponentA, &pwszComponentW);
    if (!cbLen) return NULL;
    memset(pIDL, 0, cbLen);
    ((LPSHITEMID)pIDL)->cb = cbLen;
    
    /* Set shell32's standard SHITEMID data fields. */
    pIDLData = _ILGetDataPointer(pIDL);
    pIDLData->type = (findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? PT_FOLDER : PT_VALUE;
    pIDLData->u.file.dwFileSize = findData.nFileSizeLow;
    FileTimeToDosDateTime(&findData.ftLastWriteTime, &pIDLData->u.file.uFileDate,
            &pIDLData->u.file.uFileTime);
    pIDLData->u.file.uFileAttribs = 0;
    pIDLData->u.file.uFileAttribs |= findData.dwFileAttributes;
    if (pszComponentU[0] == '.') pIDLData->u.file.uFileAttribs |=  FILE_ATTRIBUTE_HIDDEN;
    cComponentALen = lstrlenA(pszComponentA) + 1;
    memcpy(pIDLData->u.file.szNames, pszComponentA, cComponentALen);
    
    pFileStructW = (FileStructW*)(pIDLData->u.file.szNames + cComponentALen + (cComponentALen & 0x1));
    uOffsetW = (WORD)(((LPBYTE)pFileStructW) - ((LPBYTE)pIDL));
    pFileStructW->cbLen = cbLen - uOffsetW;
    FileTimeToDosDateTime(&findData.ftLastWriteTime, &pFileStructW->uCreationDate,
        &pFileStructW->uCreationTime);
    FileTimeToDosDateTime(&findData.ftLastAccessTime, &pFileStructW->uLastAccessDate,
        &pFileStructW->uLastAccessTime);
    lstrcpyW(pFileStructW->wszName, pwszComponentW);

    pOffsetW = (WORD*)(((LPBYTE)pIDL) + cbLen - sizeof(WORD));
    *pOffsetW = uOffsetW;
    
    SHFree(pszComponentA);
    SHFree(pwszComponentW);
    
    return pszComponentU + cComponentULen;
}

static HRESULT UNIXFS_path_to_pidl(IDesktopFolderImpl *pUnixFolder, LPBC pbc, const WCHAR *path,
        LPITEMIDLIST *ppidl) {
    LPITEMIDLIST pidl;
    int cPidlLen, cPathLen;
    char *pSlash, *pNextSlash, szCompletePath[FILENAME_MAX], *pNextPathElement, *pszAPath;
    WCHAR *pwszPath;
    WIN32_FIND_DATAW find_data;
    BOOL must_exist = TRUE;

    TRACE("pUnixFolder=%p, pbc=%p, path=%s, ppidl=%p\n", pUnixFolder, pbc, debugstr_w(path), ppidl);
   
    if (!ppidl || !path)
        return E_INVALIDARG;

    /* Build an absolute path and let pNextPathElement point to the interesting 
     * relative sub-path. We need the absolute path to call 'stat', but the pidl
     * will only contain the relative part.
     */
    if ((pUnixFolder->m_dwPathMode == PATHMODE_DOS) && (path[1] == ':')) 
    {
        /* Absolute dos path. Convert to unix */
        if (!UNIXFS_get_unix_path(path, szCompletePath))
            return E_FAIL;
        pNextPathElement = szCompletePath;
    } 
    else if ((pUnixFolder->m_dwPathMode == PATHMODE_UNIX) && (path[0] == '/')) 
    {
        /* Absolute unix path. Just convert to ANSI. */
        WideCharToMultiByte(CP_UNIXCP, 0, path, -1, szCompletePath, FILENAME_MAX, NULL, NULL); 
        pNextPathElement = szCompletePath;
    } 
    else 
    {
        /* Relative dos or unix path. Concat with this folder's path */
        int cBasePathLen = strlen(pUnixFolder->m_pszPath);
        memcpy(szCompletePath, pUnixFolder->m_pszPath, cBasePathLen);
        WideCharToMultiByte(CP_UNIXCP, 0, path, -1, szCompletePath + cBasePathLen, 
                            FILENAME_MAX - cBasePathLen, NULL, NULL);
        pNextPathElement = szCompletePath + cBasePathLen - 1;
        
        /* If in dos mode, replace '\' with '/' */
        if (pUnixFolder->m_dwPathMode == PATHMODE_DOS) {
            char *pBackslash = strchr(pNextPathElement, '\\');
            while (pBackslash) {
                *pBackslash = '/';
                pBackslash = strchr(pBackslash, '\\');
            }
        }
    }

    /* Special case for the root folder. */
    if (!strcmp(szCompletePath, "/")) {
        *ppidl = pidl = SHAlloc(sizeof(USHORT));
        if (!pidl) return E_FAIL;
        pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */
        return S_OK;
    }
    
    /* Remove trailing slash, if present */
    cPathLen = strlen(szCompletePath);
    if (szCompletePath[cPathLen-1] == '/') 
        szCompletePath[cPathLen-1] = '\0';

    if ((szCompletePath[0] != '/') || (pNextPathElement[0] != '/')) {
        ERR("szCompletePath: %s, pNextPathElement: %s\n", szCompletePath, pNextPathElement);
        return E_FAIL;
    }
    
    /* At this point, we have an absolute unix path in szCompletePath 
     * and the relative portion of it in pNextPathElement. Both starting with '/'
     * and _not_ terminated by a '/'. */
    TRACE("complete path: %s, relative path: %s\n", szCompletePath, pNextPathElement);
    
    /* Convert to CP_ACP and WCHAR */
    if (!UNIXFS_shitemid_len_from_filename(pNextPathElement, &pszAPath, &pwszPath))
        return E_FAIL;

    /* Compute the length of the complete ITEMIDLIST */
    cPidlLen = 0;
    pSlash = pszAPath;
    while (pSlash) {
        pNextSlash = strchr(pSlash+1, '/');
        cPidlLen += LEN_SHITEMID_FIXED_PART + /* Fixed part length plus potential alignment byte. */
            (pNextSlash ? (pNextSlash - pSlash) & 0x1 : lstrlenA(pSlash) & 0x1); 
        pSlash = pNextSlash;
    }

    /* The USHORT is for the ITEMIDLIST terminator. The NUL terminators for the sub-path-strings
     * are accounted for by the '/' separators, which are not stored in the SHITEMIDs. Above we
     * have ensured that the number of '/'s exactly matches the number of sub-path-strings. */
    cPidlLen += lstrlenA(pszAPath) + lstrlenW(pwszPath) * sizeof(WCHAR) + sizeof(USHORT);

    SHFree(pszAPath);
    SHFree(pwszPath);

    *ppidl = pidl = SHAlloc(cPidlLen);
    if (!pidl) return E_FAIL;

    if (pbc) {
        IUnknown *unk;
        IFileSystemBindData *fsb;
        HRESULT hr;

        hr = IBindCtx_GetObjectParam(pbc, (LPOLESTR)wFileSystemBindData, &unk);
        if (SUCCEEDED(hr)) {
            hr = IUnknown_QueryInterface(unk, &IID_IFileSystemBindData, (LPVOID*)&fsb);
            if (SUCCEEDED(hr)) {
                hr = IFileSystemBindData_GetFindData(fsb, &find_data);
                if (FAILED(hr))
                    memset(&find_data, 0, sizeof(WIN32_FIND_DATAW));

                must_exist = FALSE;
                IFileSystemBindData_Release(fsb);
            }
            IUnknown_Release(unk);
        }
    }

    /* Concatenate the SHITEMIDs of the sub-directories. */
    while (*pNextPathElement) {
        pSlash = strchr(pNextPathElement+1, '/');
        if (pSlash) *pSlash = '\0';
        pNextPathElement = UNIXFS_build_shitemid(szCompletePath, must_exist,
                must_exist&&!pSlash ? &find_data : NULL, pidl);
        if (pSlash) *pSlash = '/';
            
        if (!pNextPathElement) {
            SHFree(*ppidl);
            *ppidl = NULL;
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        pidl = ILGetNext(pidl);
    }
    pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */

    if ((char *)pidl-(char *)*ppidl+sizeof(USHORT) != cPidlLen) /* We've corrupted the heap :( */ 
        ERR("Computed length of pidl incorrect. Please report.\n");
    
    return S_OK;
}
/*------------------------------*/

static HRESULT WINAPI PersistFolder3_QueryInterface(IPersistFolder3* iface, REFIID riid,
    void** ppvObject)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI PersistFolder3_AddRef(IPersistFolder3* iface)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI PersistFolder3_Release(IPersistFolder3* iface)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI PersistFolder3_GetClassID(IPersistFolder3* iface, CLSID* pClassID)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    
    TRACE("(%p)->(%p)\n", This, pClassID);
    
    if (!pClassID)
        return E_INVALIDARG;

    *pClassID = *This->m_pCLSID;
    return S_OK;
}

static HRESULT WINAPI PersistFolder3_Initialize(IPersistFolder3* iface, LPCITEMIDLIST pidl)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    LPCITEMIDLIST current = pidl;
    char szBasePath[FILENAME_MAX] = "/";
    
    TRACE("(%p)->(%p)\n", This, pidl);

    /* Find the UnixFolderClass root */
    while (current->mkid.cb) {
        if ((_ILIsDrive(current) && IsEqualCLSID(This->m_pCLSID, &CLSID_ShellFSFolder)) ||
            (_ILIsSpecialFolder(current) && IsEqualCLSID(This->m_pCLSID, _ILGetGUIDPointer(current))))
        {
            break;
        }
        current = ILGetNext(current);
    }

    if (current->mkid.cb) {
        if (_ILIsDrive(current)) {
            WCHAR wszDrive[] = { '?', ':', '\\', 0 };
            wszDrive[0] = (WCHAR)*_ILGetTextPointer(current);
            if (!UNIXFS_get_unix_path(wszDrive, szBasePath))
                return E_FAIL;
        } else if (IsEqualIID(&CLSID_MyDocuments, _ILGetGUIDPointer(current))) {
            WCHAR wszMyDocumentsPath[MAX_PATH];
            if (!SHGetSpecialFolderPathW(0, wszMyDocumentsPath, CSIDL_PERSONAL, FALSE))
                return E_FAIL;
            PathAddBackslashW(wszMyDocumentsPath);
            if (!UNIXFS_get_unix_path(wszMyDocumentsPath, szBasePath))
                return E_FAIL;
        } 
        current = ILGetNext(current);
    } else if (_ILIsDesktop(pidl) || _ILIsValue(pidl) || _ILIsFolder(pidl)) {
        /* Path rooted at Desktop */
        WCHAR wszDesktopPath[MAX_PATH];
        if (!SHGetSpecialFolderPathW(0, wszDesktopPath, CSIDL_DESKTOPDIRECTORY, FALSE)) 
            return E_FAIL;
        PathAddBackslashW(wszDesktopPath);
        if (!UNIXFS_get_unix_path(wszDesktopPath, szBasePath))
            return E_FAIL;
        current = pidl;
    } else if (IsEqualCLSID(This->m_pCLSID, &CLSID_FolderShortcut)) {
        /* FolderShortcuts' Initialize method only sets the ITEMIDLIST, which
         * specifies the location in the shell namespace, but leaves the
         * target folder (m_pszPath) alone. See unit tests in tests/shlfolder.c */
        This->m_pidlLocation = ILClone(pidl);
        return S_OK;
    } else {
        ERR("Unknown pidl type!\n");
        pdump(pidl);
        return E_INVALIDARG;
    }
  
    This->m_pidlLocation = ILClone(pidl);
    return UNIXFS_initialize_target_folder(This, szBasePath, current, 0); 
}

static HRESULT WINAPI PersistFolder3_GetCurFolder(IPersistFolder3* iface, LPITEMIDLIST* ppidl)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    
    TRACE ("(iface=%p, ppidl=%p)\n", iface, ppidl);

    if (!ppidl)
        return E_POINTER;
    *ppidl = ILClone (This->m_pidlLocation);
    return S_OK;
}

static HRESULT WINAPI PersistFolder3_InitializeEx(IPersistFolder3 *iface, IBindCtx *pbc,
    LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    WCHAR wszTargetDosPath[MAX_PATH];
    char szTargetPath[FILENAME_MAX] = "";
    
    TRACE("(%p)->(%p %p %p)\n", This, pbc, pidlRoot, ppfti);

    /* If no PERSIST_FOLDER_TARGET_INFO is given InitializeEx is equivalent to Initialize. */
    if (!ppfti) 
        return IPersistFolder3_Initialize(iface, pidlRoot);

    if (ppfti->csidl != -1) {
        if (FAILED(SHGetFolderPathW(0, ppfti->csidl, NULL, 0, wszTargetDosPath)) ||
            !UNIXFS_get_unix_path(wszTargetDosPath, szTargetPath))
        {
            return E_FAIL;
        }
    } else if (*ppfti->szTargetParsingName) {
        lstrcpyW(wszTargetDosPath, ppfti->szTargetParsingName);
        PathAddBackslashW(wszTargetDosPath);
        if (!UNIXFS_get_unix_path(wszTargetDosPath, szTargetPath)) {
            return E_FAIL;
        }
    } else if (ppfti->pidlTargetFolder) {
        if (!SHGetPathFromIDListW(ppfti->pidlTargetFolder, wszTargetDosPath) ||
            !UNIXFS_get_unix_path(wszTargetDosPath, szTargetPath))
        {
            return E_FAIL;
        }
    } else {
        return E_FAIL;
    }

    This->m_pszPath = SHAlloc(lstrlenA(szTargetPath)+1);
    if (!This->m_pszPath) 
        return E_FAIL;
    lstrcpyA(This->m_pszPath, szTargetPath);
    This->m_pidlLocation = ILClone(pidlRoot);
    This->m_dwAttributes = (ppfti->dwAttributes != -1) ? ppfti->dwAttributes :
        (SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR|SFGAO_CANRENAME|SFGAO_FILESYSTEM);

    return S_OK;
}

static HRESULT WINAPI PersistFolder2_GetFolderTargetInfo(IPersistFolder3 *iface,
    PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    FIXME("(%p)->(%p): stub\n", This, ppfti);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistFolder3_GetFolderTargetInfo(IPersistFolder3 *iface,
    PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder3(iface);
    FIXME("(%p)->(%p): stub\n", This, ppfti);
    return E_NOTIMPL;
}

static const IPersistFolder3Vtbl PersistFolder3Vtbl = {
    PersistFolder3_QueryInterface,
    PersistFolder3_AddRef,
    PersistFolder3_Release,
    PersistFolder3_GetClassID,
    PersistFolder3_Initialize,
    PersistFolder3_GetCurFolder,
    PersistFolder3_InitializeEx,
    PersistFolder3_GetFolderTargetInfo
};

static HRESULT WINAPI SFHelper_QueryInterface(ISFHelper* iface, REFIID riid, void** ppvObject)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI SFHelper_AddRef(ISFHelper* iface)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI SFHelper_Release(ISFHelper* iface)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI SFHelper_GetUniqueName(ISFHelper* iface, LPWSTR pwszName, UINT uLen)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    IEnumIDList *pEnum;
    HRESULT hr;
    LPITEMIDLIST pidlElem;
    DWORD dwFetched;
    int i;
    WCHAR wszNewFolder[25];
    static const WCHAR wszFormat[] = { '%','s',' ','%','d',0 };
    MESSAGE("pwszName : %s\n",debugstr_w(pwszName));

    TRACE("(%p)->(%p %u)\n", This, pwszName, uLen);

    LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, ARRAY_SIZE(wszNewFolder));

    if (uLen < ARRAY_SIZE(wszNewFolder) + 3)
        return E_INVALIDARG;

    hr = IShellFolder2_EnumObjects(&This->IShellFolder2_iface, 0,
                                   SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN, &pEnum);
    if (SUCCEEDED(hr)) {
        lstrcpynW(pwszName, wszNewFolder, uLen);
        IEnumIDList_Reset(pEnum);
        i = 2;
        while ((IEnumIDList_Next(pEnum, 1, &pidlElem, &dwFetched) == S_OK) && (dwFetched == 1)) {
            WCHAR wszTemp[MAX_PATH];
            _ILSimpleGetTextW(pidlElem, wszTemp, MAX_PATH);
            if (!lstrcmpiW(wszTemp, pwszName)) {
                IEnumIDList_Reset(pEnum);
                snprintfW(pwszName, uLen, wszFormat, wszNewFolder, i++);
                if (i > 99) {
                    hr = E_FAIL;
                    break;
                }
            }
        }
        IEnumIDList_Release(pEnum);
    }
    return hr;
}

HRESULT WINAPI SFHelper_AddFolder(ISFHelper* iface, HWND hwnd, LPCWSTR pwszName,
    LPITEMIDLIST* ppidlOut)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    char szNewDir[FILENAME_MAX];
    int cBaseLen = 0;

    TRACE("(%p)->(%p %s %p)\n", This, hwnd, debugstr_w(pwszName), ppidlOut);

    if (ppidlOut)
        *ppidlOut = NULL;

    char *pNextDir = NULL;
    char unixTargetPath[FILENAME_MAX] = {0};

    if(!UNIXFS_get_unix_path(This-> sPathTarget,unixTargetPath))
        return E_FAIL;
    
    if(unixTargetPath[strlen(unixTargetPath)-1 ] != '/') strcat(unixTargetPath,"/");

    DWORD dPathLen = strlen(unixTargetPath) + 1;
    This ->m_pszPath = pNextDir = SHAlloc(dPathLen);

    if (!This->m_pszPath) {
        WARN("SHAlloc failed!\n");
        return E_FAIL;
    }

    strcpy(pNextDir, unixTargetPath);

    if (!This->m_pszPath)
         return E_FAIL;

    lstrcpynA(szNewDir, This->m_pszPath, FILENAME_MAX);
    cBaseLen = lstrlenA(szNewDir);
    WideCharToMultiByte(CP_UNIXCP, 0, pwszName, -1, szNewDir+cBaseLen, FILENAME_MAX-cBaseLen, 0, 0);
   
    if (mkdir(szNewDir, 0777)) {
        char szMessage[256 + FILENAME_MAX];
        char szCaption[256];

        LoadStringA(shell32_hInstance, IDS_CREATEFOLDER_DENIED, szCaption, ARRAY_SIZE(szCaption));
        sprintf(szMessage, szCaption, szNewDir);
        LoadStringA(shell32_hInstance, IDS_CREATEFOLDER_CAPTION, szCaption, ARRAY_SIZE(szCaption));
        MessageBoxA(hwnd, szMessage, szCaption, MB_OK | MB_ICONEXCLAMATION);

        return E_FAIL;
    } else { 
        LPITEMIDLIST pidlRelative;

        /* Inform the shell */
        if (SUCCEEDED(UNIXFS_path_to_pidl(This, NULL, pwszName, &pidlRelative))) {
            LPITEMIDLIST pidlAbsolute = ILCombine(This->m_pidlLocation, pidlRelative);
            if (ppidlOut)
                *ppidlOut = pidlRelative;
            else
                ILFree(pidlRelative);
            SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlAbsolute, NULL);
            ILFree(pidlAbsolute);
        } else return E_FAIL;
        return S_OK;
    }
}

static HRESULT WINAPI SFHelper_DeleteItems(ISFHelper* iface, UINT cidl, LPCITEMIDLIST* apidl)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPITEMIDLIST pidlAbsolute;
    HRESULT hr = S_OK;
    UINT i;
    struct stat st;
    
    TRACE("(%p)->(%d %p)\n", This, cidl, apidl);

    hr = UNIXFS_delete_with_shfileop(This, cidl, apidl);
    if (hr == S_FALSE)
        hr = UNIXFS_delete_with_syscalls(This, cidl, apidl);
    
    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    /* we need to manually send the notifies if the files doesn't exist */
    for (i=0; i<cidl; i++) {
        if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelative))
            continue;
        pidlAbsolute = ILCombine(This->m_pidlLocation, apidl[i]);
        if (stat(szAbsolute, &st))
        {
            if (_ILIsFolder(apidl[i])) {
                SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST, pidlAbsolute, NULL);
            } else if (_ILIsValue(apidl[i])) {
                SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlAbsolute, NULL);
            }
        }
        ILFree(pidlAbsolute);
    }
        
    return hr;
}

static HRESULT WINAPI SFHelper_CopyItems(ISFHelper* iface, IShellFolder *psfFrom,
    UINT cidl, LPCITEMIDLIST *apidl)
{
    IDesktopFolderImpl *This = impl_from_ISFHelper(iface);
    DWORD dwAttributes;
    UINT i;
    HRESULT hr;
    char szAbsoluteDst[FILENAME_MAX], *pszRelativeDst;
    
    TRACE("(%p)->(%p %d %p)\n", This, psfFrom, cidl, apidl);

    if (!psfFrom || !cidl || !apidl)
        return E_INVALIDARG;

    /* All source items have to be filesystem items. */
    dwAttributes = SFGAO_FILESYSTEM;
    hr = IShellFolder_GetAttributesOf(psfFrom, cidl, apidl, &dwAttributes);
    if (FAILED(hr) || !(dwAttributes & SFGAO_FILESYSTEM)) 
        return E_INVALIDARG;

    lstrcpyA(szAbsoluteDst, This->m_pszPath);
    pszRelativeDst = szAbsoluteDst + strlen(szAbsoluteDst);
    
    for (i=0; i<cidl; i++) {
        WCHAR wszSrc[MAX_PATH];
        char szSrc[FILENAME_MAX];
        STRRET strret;
        HRESULT res;
        WCHAR *pwszDosSrc, *pwszDosDst;

        /* Build the unix path of the current source item. */
        if (FAILED(IShellFolder_GetDisplayNameOf(psfFrom, apidl[i], SHGDN_FORPARSING, &strret)))
            return E_FAIL;
        if (FAILED(StrRetToBufW(&strret, apidl[i], wszSrc, MAX_PATH)))
            return E_FAIL;
        if (!UNIXFS_get_unix_path(wszSrc, szSrc)) 
            return E_FAIL;

        /* Build the unix path of the current destination item */
        UNIXFS_filename_from_shitemid(apidl[i], pszRelativeDst);

        pwszDosSrc = wine_get_dos_file_name(szSrc);
        pwszDosDst = wine_get_dos_file_name(szAbsoluteDst);

        if (pwszDosSrc && pwszDosDst)
            res = UNIXFS_copy(pwszDosSrc, pwszDosDst);
        else
            res = E_OUTOFMEMORY;

        heap_free(pwszDosSrc);
        heap_free(pwszDosDst);

        if (res != S_OK)
            return res;
    }
    return S_OK;
}

static const ISFHelperVtbl SFHelperVtbl = {
    SFHelper_QueryInterface,
    SFHelper_AddRef,
    SFHelper_Release,
    SFHelper_GetUniqueName,
    SFHelper_AddFolder,
    SFHelper_DeleteItems,
    SFHelper_CopyItems
};

/**************************************************************************
 *    ISF_Desktop_Constructor
 */
HRESULT WINAPI ISF_Desktop_Constructor (
                IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    WCHAR szMyPath[MAX_PATH];

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!cached_sf)
    {
        IDesktopFolderImpl *sf;

        if (!SHGetSpecialFolderPathW( 0, szMyPath, CSIDL_DESKTOPDIRECTORY, TRUE ))
            return E_UNEXPECTED;

        sf = LocalAlloc( LMEM_ZEROINIT, sizeof (IDesktopFolderImpl) );
        if (!sf)
            return E_OUTOFMEMORY;

        sf->ref = 1;
        sf->IShellFolder2_iface.lpVtbl = &vt_MCFldr_ShellFolder2;
        sf->IPersistFolder2_iface.lpVtbl = &vt_IPersistFolder2;
        sf->IPersistFolder3_iface.lpVtbl = &PersistFolder3Vtbl;
        sf->ISFHelper_iface.lpVtbl = &SFHelperVtbl;
        sf->pidlRoot = _ILCreateDesktop();    /* my qualified pidl */
        sf->sPathTarget = SHAlloc( (lstrlenW(szMyPath) + 1)*sizeof(WCHAR) );
        lstrcpyW( sf->sPathTarget, szMyPath );

        if (InterlockedCompareExchangePointer((void *)&cached_sf, sf, NULL) != NULL)
        {
            /* some other thread already been here */
            SHFree( sf->pidlRoot );
            SHFree( sf->sPathTarget );
            LocalFree( sf );
        }
    }

    return IShellFolder2_QueryInterface( &cached_sf->IShellFolder2_iface, riid, ppv );
}
