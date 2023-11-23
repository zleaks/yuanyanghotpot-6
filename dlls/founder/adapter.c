#include <stdarg.h>
#include <stdbool.h>
#include <windows.h>

#include "winbase.h"
#include "windef.h"
#include "winerror.h"

#include "wine/debug.h"

#include <dlfcn.h>

static void *handle;

WINE_DEFAULT_DEBUG_CHANNEL(founder);

typedef unsigned int DWORD;
typedef int LONG;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef void *HANDLE;
typedef char *LPTSTR;
typedef void *LPVOID;
typedef const char *LPCSTR;
typedef char TCHAR;
typedef double DOUBLE;

#define IDB_BUTTON_LOGIN 111
#define IDC_EDIT 4000

BOOL ParamUI(void) { return true; }

LONG get_param_num(DWORD nCap) { return nCap; }

LPCSTR get_param_str(DWORD nCap, DWORD nData) {
  if (nCap == 257) {
    switch (nData) {
    case 0:
      return "Lineart";
    case 1:
      return "Gray";
    case 2:
      return "Color";
    }
  }

  if (nCap == 258) {
    return "Automatic";
  }

  if (nCap == 259) {
    if (nData)
      return "ADF Front";
    else
      return "ADF Duplex";
  }

  if (nCap == 260 || nCap == 261) {
    static char str[16] = {0};
    sprintf(str, "%d", nData);
    return str;
  }
  printf("NOT Implement!");
  return NULL;
}

BOOL myinit(void) {
  //    handle = dlopen("/usr/lib/libwmgsdk[x86].so", RTLD_LOCAL | RTLD_NOW |
  //    RTLD_DEEPBIND);
  handle = dlopen("/usr/lib/libscanimagesdk[x86].so",
                  RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);
  TRACE("--myinit handle=%p \n", handle);
  return handle != 0 ? TRUE : FALSE;
}

void myfini(void) {
  if (handle) {
    dlclose(handle);
  }
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv) {
  TRACE("%p,%x,%p\n", hInstDLL, fdwReason, lpv);
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    return myinit();
  case DLL_PROCESS_DETACH:
    myfini();
    break;
  }
  return TRUE;
}

BOOL WINAPI ApproveLicenseA(LPCSTR szLicense) {
  int (*pfn_init)(void)  = NULL;
  int ret_init=0;
  BOOL (*pfn)(LPCSTR) = NULL;
  pfn_init = (int(*)(void))dlsym(handle,"gch_Init");
  ret_init=pfn_init();
  TRACE("--gch_init pfn_init=%p ret=%d\n", pfn_init,ret_init);
  pfn = (BOOL(*)(LPCSTR))dlsym(handle, "gch_ApproveLicenseA");
//  pfn = (BOOL(*)(LPCSTR))dlsym(handle, "wmg_approvelicenseA");
  ret_init=pfn(szLicense);
  TRACE("--ApproveLicenseA pfn=%p ret=%d\n", pfn,ret_init);
  return ret_init;
}

BOOL WINAPI ApproveLicenseW(LPCWSTR szLicense) {
  BOOL (*pfn)(LPCSTR) = NULL;
  pfn = (BOOL(*)(LPCSTR))dlsym(handle, "gch_ApproveLicenseW");
  TRACE("--ApproveLicenseW pfn=%p \n", pfn);
  return pfn(szLicense);
}

BOOL WINAPI CheckDeviceLicenseA(LPCSTR szDevice) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI CheckDeviceLicenseW(LPCWSTR szDevice) {
  printf("Not Implement!\n");
  return 0;
}

LPCSTR WINAPI GetAllDevicesA(LPCSTR szSeperator) {
  LPCSTR (*pfn)(LPCSTR) = NULL;
  LPCSTR *devlist = NULL ;
  pfn = (LPCSTR(*)(LPCSTR))dlsym(handle, "gch_GetDevicesList");
  devlist=pfn(szSeperator);
  TRACE("--GetAllDevicesA pfn=%p devlist=%s \n", pfn,devlist);
  return devlist;
}

LPCWSTR WINAPI GetAllDevicesW(LPCWSTR szSeperator) {
  LPCWSTR (*pfn)(LPCWSTR) = NULL;
  pfn = (LPCWSTR(*)(LPCWSTR))dlsym(handle, "gch_GetDevicesList");
  TRACE("--GetAllDevicesW pfn=%p \n", pfn);
  return pfn(szSeperator);
}

BOOL WINAPI OpenDeviceA(LPCSTR szDevice) {
  BOOL (*pfn)(LPCSTR) = NULL;
  LONG ret;
  pfn = (BOOL(*)(LPCSTR))dlsym(handle, "gch_OpenDevice");
  ret=pfn(szDevice); 
  TRACE("--OpenDeviceA pfn=%p device=%s ret=%ld\n", pfn,szDevice,ret);
  return 1-ret;
}

BOOL WINAPI OpenDeviceW(LPCWSTR szDevice) {
  BOOL (*pfn)(LPCWSTR) = NULL;
  pfn = (BOOL(*)(LPCWSTR))dlsym(handle, "gch_OpenDevice");
  TRACE("--OpenDeviceW pfn=%p \n", pfn);
  return pfn(szDevice);
}

BOOL WINAPI CloseDevice(void) {
  BOOL (*pfn)(void) = NULL;
  pfn = (BOOL(*)(void))dlsym(handle, "gch_CloseDevice");
  TRACE("--CloseDevice pfn=%p \n", pfn);
  return pfn();
}

DWORD WINAPI GetSystemError(void) {
  printf("Not Implement!\n");
  return 0;
}

DWORD WINAPI GetDeviceError(void) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetupDevice(void) { return ParamUI(); }

BOOL WINAPI StartScan(BOOL bUI) {
  BOOL (*pfn)(void) = NULL;
  LONG num;
  if (bUI)
    if (!ParamUI())
      return false;
  pfn = (BOOL(*)(void))dlsym(handle, "gch_StartScan");
  num= pfn();
  TRACE("--StartScan pfn=%p,ret=%ld \n", pfn,num);
  return num;
}

BOOL WINAPI EndScan(void) {
  BOOL (*pfn)(void) = NULL;
  pfn = (BOOL(*)(void))dlsym(handle, "gch_EndScan");
  TRACE("--EndScan pfn=%p \n", pfn);
  return pfn();
}

BOOL WINAPI HaltScan(void) {
  printf("Not Implement!\n");
  return 0;
}

LONG WINAPI CaptureImageA(LPCSTR szFilename) {
  LONG (*pfn)(LPCSTR) = NULL;
  LONG  num;
  pfn = (LONG(*)(LPCSTR))dlsym(handle, "gch_CaptureImage");
  num=pfn(szFilename);
  TRACE("--CaptureImageA pfn=%p %s %ld \n", pfn,szFilename,num);
  return -num;
}

LONG WINAPI CaptureImageW(LPCWSTR szFilename) {
  LONG (*pfn)(LPCWSTR) = NULL;
  pfn = (LONG(*)(LPCWSTR))dlsym(handle, "gch_CaptureImage");
  TRACE("--CaptureImageW pfn=%p \n", pfn);
  return pfn(szFilename);
}

// WMG_INT wmg_SetCapability_INT(WMG_INT nCap, WMG_LPCSTR value)
BOOL WINAPI SetCapability(DWORD nCap, DWORD nData, DWORD nType) {
  LONG param_num = get_param_num(nCap);
  LPCSTR param_str = get_param_str(nCap, nData);
  BOOL (*pfn)(LONG, LPCSTR) = NULL;
  pfn = (BOOL(*)(LONG, LPCSTR))dlsym(handle, "gch_SetCapability_INT");
  return pfn(param_num, param_str);
}

BOOL WINAPI SetCapabilityBOOL(DWORD nCap, BOOL bValue) {
  LONG param_num = get_param_num(nCap);
  LPCSTR param_str = get_param_str(nCap, bValue);
  BOOL (*pfn)(LONG, LPCSTR) = NULL;
  pfn = (BOOL(*)(LONG, LPCSTR))dlsym(handle, "gch_SetCapability_INT");
  return pfn(param_num, param_str);
}

BOOL WINAPI SetCapabilityUINT8(DWORD nCap, DWORD uValue) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetCapabilityUINT16(DWORD nCap, DWORD uValue) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetCapabilityUINT32(DWORD nCap, DWORD uValue) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetCapabilityINT8(DWORD nCap, LONG uValue) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetCapabilityINT16(DWORD nCap, LONG uValue) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetCapabilityINT32(DWORD nCap, LONG uValue) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI SetCapabilityFIX32(DWORD nCap, DOUBLE lfValue) {
  LONG param_num = get_param_num(nCap);
  LPCSTR param_str = get_param_str(nCap, lfValue);
  BOOL (*pfn)(LONG, LPCSTR) = NULL;
  pfn = (BOOL(*)(LONG, LPCSTR))dlsym(handle, "gch_SetCapability_INT");
  return pfn(param_num, param_str);
}

DWORD WINAPI GetCapability(DWORD nCap) {
  printf("Not Implement!\n");
  return 0;
}

DOUBLE WINAPI GetCapabilityFIX32(DWORD nCap) {
  printf("Not Implement!\n");
  return 0;
}

LPCSTR WINAPI GetCapabilityStringA(DWORD nCap) {
  printf("Not Implement!\n");
  return 0;
}

LPCWSTR WINAPI GetCapabilityStringW(DWORD nCap) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI EnableMsgBoxA(BOOL bEnable, LPCSTR szPath) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI EnableMsgBoxW(BOOL bEnable, LPCWSTR szPath) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI CaptureImageWithBarcodeA(LPCSTR szFilename) {
  printf("Not Implement!\n");
  return 0;
}

BOOL WINAPI CaptureImageWithBarcodeW(LPCWSTR szFilename) {
  printf("Not Implement!\n");
  return 0;
}

LONG WINAPI GetBarcodeCount() {
  printf("Not Implement!\n");
  return 0;
}

DWORD WINAPI GetBarcodeType(INT nItem) {
  printf("Not Implement!\n");
  return 0;
}

LPCSTR WINAPI GetBarcodeStringA(INT nItem) {
  printf("Not Implement!\n");
  return 0;
}

LPCWSTR WINAPI GetBarcodeStringW(INT nItem) {
  printf("Not Implement!\n");
  return 0;
}

LONG WINAPI ReleaseBarcode() {
  printf("Not Implement!\n");
  return 0;
}

BOOL GetTex(HWND hWnd, char key[10]) {
  HWND h;
  char temp[255] = {0};
  h = GetDlgItem(hWnd, IDC_EDIT);
  Sleep(10);
  SendMessageA(h, WM_GETTEXT, 255, (LPARAM)temp);
  if (!strcmp(key, temp)) {
    return TRUE;
  } else
    return FALSE;
}

static LRESULT WINAPI Founder_WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam) {
  char param[10] = "";
  int wmId, wmEvent;
  switch (msg) {
  case WM_COMMAND:
    wmId = LOWORD(wParam);
    wmEvent = HIWORD(wParam);
    if (wmId == IDB_BUTTON_LOGIN) {
      GetTex(hWnd, param);
    }
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_CREATE: {
    HWND Reset_Button = CreateWindowA(
        "button", "提交", WS_CHILD | WS_VISIBLE, 565, 20, 75, 20, hWnd,
        (HMENU)IDB_BUTTON_LOGIN, ((LPCREATESTRUCTW)lParam)->hInstance, NULL);

    HWND edit_mode =
        CreateWindowA("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 120, 20,
                      120, 20, hWnd, NULL, NULL, NULL);
    HWND edit_x_resolution =
        CreateWindowA("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 325, 20,
                      120, 20, hWnd, NULL, NULL, NULL);
    HWND edit_y_resolution =
        CreateWindowA("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 325, 20,
                      120, 20, hWnd, NULL, NULL, NULL);
    HWND hStatic1;
    hStatic1 = CreateWindowA(
        "static", "色彩模式：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 20, 90,
        20, hWnd, (HMENU)1, ((LPCREATESTRUCTW)lParam)->hInstance, NULL);
    HWND hStatic2;
    hStatic2 = CreateWindowA(
        "static", "x轴像素：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 540, 45,
        30, hWnd, (HMENU)2, ((LPCREATESTRUCTW)lParam)->hInstance, NULL);
    HWND hStatic3;
    hStatic3 = CreateWindowA(
        "static", "y轴像素：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 250, 20, 65,
        20, hWnd, (HMENU)3, ((LPCREATESTRUCTW)lParam)->hInstance, NULL);
  }
  case WM_PAINT:
    break;
  default:
    break;
  }
  return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline,
                   int show) {
  static const WCHAR className[] = {'F', 'o', 'u', 'n', 'd', 'e', 'r', 0};
  static const WCHAR winName[] = {'F', 'o', 'u', 'n', 'd', 'e', 'r', 0};
  MSG msg;
  WNDCLASSEXW class;
  ZeroMemory(&class, sizeof(class));
  class.cbSize = sizeof(class);
  class.hInstance = hInstance;
  class.lpfnWndProc = Founder_WndProc;
  class.lpszClassName = className;
  class.hCursor = NULL;
  class.lpszMenuName = NULL;
  class.hIcon = NULL;

  if (!RegisterClassExW(&class))
    return FALSE;

  HWND hWnd = CreateWindowW(className, winName, WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
  ShowWindow(hWnd, SW_NORMAL);
  UpdateWindow(hWnd);
  return msg.wParam;
}
