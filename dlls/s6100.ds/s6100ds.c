/*
 * SANE.DS functions
 *
 * Copyright 2000 Shi Quan He <shiquan@cyberdude.com>
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

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "s6100ds.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

DSMENTRYPROC SANE_dsmentry;

#ifdef SONAME_LIBSANE

HINSTANCE SANE_instance;

static void * libsane_handle;
static void * x86sdk_handle;
TW_INT8 strpicexname[32];


static void close_libsane(void *h)
{
    if (h)
        dlclose(h);
}

static void *open_libsane(void)
{
    static void *h;

    h = dlopen(SONAME_LIBSANE, RTLD_GLOBAL | RTLD_NOW);
    if (!h)
    {
        WARN("dlopen(%s) failed\n", SONAME_LIBSANE);
        return NULL;
    }

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(h, #f)) == NULL) { \
        close_libsane(h); \
        ERR("Could not dlsym %s\n", #f); \
        return NULL; \
    }

    LOAD_FUNCPTR(sane_init)
    LOAD_FUNCPTR(sane_exit)
    LOAD_FUNCPTR(sane_get_devices)
    LOAD_FUNCPTR(sane_open)
    LOAD_FUNCPTR(sane_close)
    LOAD_FUNCPTR(sane_get_option_descriptor)
    LOAD_FUNCPTR(sane_control_option)
    LOAD_FUNCPTR(sane_get_parameters)
    LOAD_FUNCPTR(sane_start)
    LOAD_FUNCPTR(sane_read)
    LOAD_FUNCPTR(sane_cancel)
    LOAD_FUNCPTR(sane_set_io_mode)
    LOAD_FUNCPTR(sane_get_select_fd)
    LOAD_FUNCPTR(sane_strstatus)
#undef LOAD_FUNCPTR

    return h;
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TW_UINT16 twRC = TWRC_FAILURE;

    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH: {
	    SANE_Int version_code;
            twRC = s6100_init();
	    if(twRC == TWRC_FAILURE)
	    {
		WARN("dlopen(%s) failed\n", S6100_SDK_PATH);
	    }
            libsane_handle = open_libsane();
            if (! libsane_handle)
                return FALSE;

	    psane_init (&version_code, NULL);
	    SANE_instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
	}
        case DLL_PROCESS_DETACH:
            twRC = s6100_close();
            if (lpvReserved) break;
            TRACE("calling sane_exit()\n");
	        psane_exit ();
            close_libsane(libsane_handle);
            break;
    }

    return TRUE;
}

static TW_UINT16 SANE_GetIdentity( pTW_IDENTITY, pTW_IDENTITY);
static TW_UINT16 SANE_OpenDS( pTW_IDENTITY, pTW_IDENTITY);

#endif /* SONAME_LIBSANE */

//////////////////////////// He Nan 20200803 //////////////////////////////////////////

TW_UINT16 s6100_init(void)
{
	TW_INT16 ret = -1;
    TW_INT32 (*pfn_init)(void)  = NULL;    

    x86sdk_handle = dlopen(S6100_SDK_PATH, RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);	
    TRACE("x86sdk_handle = %p \n", x86sdk_handle);

    pfn_init = (TW_INT32 (*)(void))dlsym(x86sdk_handle,"gch_Init");
    ret = pfn_init();
    TRACE("--gch_init pfn_init = %p ret = %d\n", pfn_init,ret);

	s6100_addLicense(S6100_SDK_LICENSE);
	s6100_addLicense(KS8130_SDK_LICENSE);
	
    return TWRC_SUCCESS;
}

TW_UINT16 s6100_close(void)
{
    if (x86sdk_handle != NULL) 
    {
        dlclose(x86sdk_handle);
    }
    return TWRC_SUCCESS;
}

TW_UINT16 s6100_openDevice(void)
{
    //TW_INT32 (*pfn_init)(void)  = NULL;    
    TW_INT16 ret = -1;
    TW_INT8 (*pfn)(const TW_INT8 *) = NULL;
    TW_INT8 * device_list = NULL;

    device_list = s6100_getDevices(";");
    pfn = (TW_INT8 (*)(const TW_INT8 *))dlsym(x86sdk_handle, "gch_OpenDevice");
    ret = pfn(S6100_SDK_DEVICE);
    //ret = pfn(KS8130_SDK_DEVICE);
    TRACE("--OpenDevice pfn = %p device_list = %s ret = %d\n", pfn, device_list, ret);

	strcpy(strpicexname, ".tif");
    return TWRC_SUCCESS;
}

TW_UINT16 s6100_addLicense(const TW_INT8 * license)
{
    TW_INT16 ret = -1;
    TW_INT8 (*pfn)(const TW_INT8 *) = NULL;

    pfn = (TW_INT8 (*)(const TW_INT8 *))dlsym(x86sdk_handle, "gch_ApproveLicenseA");
    ret = pfn(license);
	TRACE("--license = %s, pfn = %p, ret = %d\n", license, pfn, ret);

	return TWRC_SUCCESS;
}

TW_INT8 * s6100_getDevices(const TW_INT8 * szSeperator)
{
    TW_INT8 * (*pfn)(const TW_INT8 *) = NULL;
    static TW_INT8 * devlist = NULL;

    pfn = (TW_INT8 *(*)(const TW_INT8 *))dlsym(x86sdk_handle, "gch_GetDevicesList");
    devlist=pfn(szSeperator);
    TRACE("--GetAllDevicesA pfn = %p devlist = %s \n", pfn,devlist);
    return devlist;
}

TW_UINT16 s6100_closeDevice(void)
{
    void (*pfn)(void) = NULL;

    pfn = (void(*)(void))dlsym(x86sdk_handle, "gch_CloseDevice");
    TRACE("--CloseDevice pfn = %p \n", pfn);
    pfn();
    return TWRC_SUCCESS;
}

TW_UINT16 s6100_startScan(void)
{
    TW_INT32 (*pfn)(void) = NULL;
    TW_INT32 ret;
    
    pfn = (TW_INT32 (*)(void))dlsym(x86sdk_handle, "gch_StartScan");
    ret = pfn();
    TRACE("--StartScan pfn = %p,ret = %d \n", pfn, ret);
    return ret;
}

TW_UINT16 s6100_endScan(void)
{
    void (*pfn)(void) = NULL;

    pfn = (void(*)(void))dlsym(x86sdk_handle, "gch_EndScan");
    TRACE("--EndScan  pfn = %p \n", pfn);
    pfn();
    return TWRC_SUCCESS;
}

TW_UINT16 s6100_captureImage(const TW_INT8 * szFilename)
{
    TW_INT32 (*pfn)(const TW_INT8 *) = NULL;
    TW_INT16 ret;
    
    pfn = (TW_INT32 (*)(const TW_INT8 *))dlsym(x86sdk_handle, "gch_CaptureImage");
    ret = pfn(szFilename);
    TRACE("--CaptureImage pfn = %p,ret = %d \n", pfn, ret);
    //return TWRC_SUCCESS;
    return ret;
}

TW_UINT16 s6100_setCapability(const TW_INT8 * nCap, const TW_INT8 * value)
{
	TW_INT32 (*pfn)(LPCSTR, LPCSTR) = NULL;
    TW_INT32 ret;

    pfn = (TW_INT32 (*)(LPCSTR, LPCSTR))dlsym(x86sdk_handle, "gch_SetCapability_STR");
    ret = pfn(nCap, value);
    //TRACE("--SetCapability pfn = %p,ret = %d \n", pfn, ret);

    return ret;
}

void s6100_makePicture(void)
{
    TW_UINT16 ret = -1;
    uid_t userid;
    struct passwd * pwd;
    static TW_UINT32 count = 0;
    TW_INT8 strpath[512];

    bzero(strpath, sizeof(strpath));
    userid = getuid();
    TRACE("userid is %d\n", userid);
    pwd = getpwuid(userid);
    TRACE("username is %s\nuserdir = %s\n", pwd->pw_name, pwd->pw_dir);

	if (strcmp(strpicexname, ".bmp") && strcmp(strpicexname, ".jpg") && strcmp(strpicexname, ".tif") != 0)
	{
		strcpy(strpicexname, ".tif");
	}
	
    sprintf(strpath, "%s/文档/%04d%s", pwd->pw_dir, ++count, strpicexname);
    TRACE("strpath = %s \n", strpath);
    ret = s6100_captureImage(strpath);
    TRACE("%s:  ret = %d \n", __func__, ret);
    return;
}

void s6100_scanimage(void)
{
    TW_UINT16 ret = -1;
    while (true) {
        ret = s6100_startScan();
        if(ret == 0){
            s6100_makePicture(); 
        } else {
            break;
        }
    }
    s6100_endScan();
    return;	
}


//////////////////////////////////////////////////////////////////////////////////////

static TW_UINT16 SANE_SetEntryPoint (pTW_IDENTITY pOrigin, TW_MEMREF pData);

static TW_UINT16 SANE_SourceControlHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
	case DAT_IDENTITY:
	    switch (MSG)
	    {
            case MSG_CLOSEDS:			
                twRC = s6100_endScan();
                twRC = s6100_closeDevice();
                break;
            case MSG_OPENDS:		     
                twRC = SANE_OpenDS( pOrigin, (pTW_IDENTITY)pData);
                break;
            case MSG_GET:
                twRC = SANE_GetIdentity( pOrigin, (pTW_IDENTITY)pData);
                break;
	    }
	    break;

    case DAT_CAPABILITY:
        if (MSG == MSG_SET) {
            twRC = SANE_CapabilitySet(pOrigin, pData);
        } else {
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_CAPBADOPERATION;
            FIXME("unrecognized operation triplet\n");
        }
        break;
        
    case DAT_ENTRYPOINT:
        if (MSG == MSG_SET)
            twRC = SANE_SetEntryPoint (pOrigin, pData);
        else
        {
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_CAPBADOPERATION;
            FIXME("unrecognized operation triplet\n");
        }
        break;

    case DAT_EVENT:
        if (MSG == MSG_PROCESSEVENT)
            twRC = SANE_ProcessEvent (pOrigin, pData);
        else
        {
            activeDS.twCC = TWCC_CAPBADOPERATION;
            twRC = TWRC_FAILURE;
        }
        break;

    case DAT_PENDINGXFERS:
        switch (MSG)
        {
            case MSG_ENDXFER:
                twRC = SANE_PendingXfersEndXfer (pOrigin, pData);
                break;
            default:
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
        }
        break;
    case DAT_SETUPFILEXFER:
        if (MSG == MSG_GET) {
            twRC = SANE_SetupFileXfer2Get(pOrigin, (pTW_SETUPFILEXFER)pData);
        } else if (MSG == MSG_SET) {
            twRC = SANE_SetupFileXfer2Set(pOrigin, (pTW_SETUPFILEXFER)pData);
        }
        break;

    case DAT_STATUS:
        if (MSG == MSG_GET)
            twRC = SANE_GetDSStatus (pOrigin, pData);
        else
        {
            activeDS.twCC = TWCC_CAPBADOPERATION;
            twRC = TWRC_FAILURE;
        }
        break;

    case DAT_USERINTERFACE:
        switch (MSG)
        {
            case MSG_DISABLEDS:
                twRC = SANE_DisableDSUserInterface (pOrigin, pData);
                break;
            case MSG_ENABLEDS:
                twRC = SANE_EnableDSUserInterface (pOrigin, pData);
                break;
            default:
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
                break;
        }
        break;
    default:
    WARN("code unsupported: %d\n", DAT);
        activeDS.twCC = TWCC_CAPUNSUPPORTED;
        twRC = TWRC_FAILURE;
        break;
    }

    return twRC;
}


static TW_UINT16 SANE_ImageGroupHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
        case DAT_IMAGEFILEXFER:
            if (MSG == MSG_GET) {
                twRC = SANE_ImageFileXferGet(pOrigin, pData);
            } else {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;
        
        case DAT_IMAGENATIVEXFER:
            if (MSG == MSG_GET) {
                twRC = SANE_ImageNativeXferGet(pOrigin, pData);
            } else {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;
            
        default:
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_CAPUNSUPPORTED;
            WARN("unsupported DG type %d\n", DAT);
            break;
    }
    return twRC;
}

/* Main entry point for the TWAIN library */
TW_UINT16 WINAPI
DS_Entry ( pTW_IDENTITY pOrigin,
           TW_UINT32    DG,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;  /* Return Code */

    TRACE("(DG=%d DAT=%d MSG=%d)\n", DG, DAT, MSG);

    switch (DG)
    {
        case DG_CONTROL:
            twRC = SANE_SourceControlHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_IMAGE:
            twRC = SANE_ImageGroupHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_AUDIO:
            WARN("Audio group of controls not implemented yet.\n");
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_CAPUNSUPPORTED;
            break;
        default:
            activeDS.twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
    }

    return twRC;
}

void SANE_Notify (TW_UINT16 message)
{
    activeDS.twCC = TWCC_SUCCESS;
}

/* DG_CONTROL/DAT_ENTRYPOINT/MSG_SET */
TW_UINT16 SANE_SetEntryPoint (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_ENTRYPOINT *entry = (TW_ENTRYPOINT*)pData;

    SANE_dsmentry = entry->DSM_Entry;

    return TWRC_SUCCESS;
}

#ifdef SONAME_LIBSANE
/* Sane returns device names that are longer than the 32 bytes allowed
   by TWAIN.  However, it colon separates them, and the last bit is
   the most interesting.  So we use the last bit, and add a signature
   to ensure uniqueness */
static void copy_s6100_short_name(const char *in, char *out, size_t outsize)
{
    const char *p;
    int  signature = 0;

    if (strlen(in) <= outsize - 1)
    {
        strcpy(out, in);
        return;
    }

    for (p = in; *p; p++)
        signature += *p;

    p = strrchr(in, ':');
    if (!p)
        p = in;
    else
        p++;

    if (strlen(p) > outsize - 7 - 1)
        p += strlen(p) - (outsize - 7 - 1);

    strcpy(out, p);
    sprintf(out + strlen(out), "(%04X)", signature % 0x10000);

}

static TW_UINT16
SANE_GetIdentity( pTW_IDENTITY pOrigin, pTW_IDENTITY self)
{
    TW_INT8 * device_list = NULL;
    TRACE("[%s](%d): %s \n",__FILE__, __LINE__, __func__);
	
    device_list = s6100_getDevices(";");
    TRACE("--SANE_GetIdentity device_list = %s\n", device_list);
    self->ProtocolMajor = TWON_PROTOCOLMAJOR;
    self->ProtocolMinor = TWON_PROTOCOLMINOR;
    self->SupportedGroups = DG_CONTROL | DG_IMAGE | DF_DS2;
    copy_s6100_short_name(S6100_SDK_DEVICE, self->ProductName, sizeof(self->ProductName) - 1);
    //copy_s6100_short_name(KS8130_SDK_DEVICE, self->ProductName, sizeof(self->ProductName) - 1);
    lstrcpynA (self->Manufacturer, "", sizeof(self->Manufacturer) - 1);
    lstrcpynA (self->ProductFamily, "", sizeof(self->ProductFamily) - 1);
    
    return TWRC_SUCCESS;
}

static TW_UINT16 SANE_OpenDS( pTW_IDENTITY pOrigin, pTW_IDENTITY self) {
	TW_UINT16 twRC = TWRC_SUCCESS;
	TRACE("[%s](%d): %s \n",__FILE__, __LINE__, __func__);
	twRC = s6100_openDevice();
	TRACE("s6100_openDevice: ret = %d \n", twRC);
    activeDS.currentState = 4;
    activeDS.identity.Id = self->Id;
    activeDS.appIdentity = *pOrigin;
    return TWRC_SUCCESS;
}
#endif
