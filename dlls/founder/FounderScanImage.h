#ifndef __FOUNDER_SCAN_IMAGE_LIBRARY__
#define __FOUNDER_SCAN_IMAGE_LIBRARY__

#ifndef _UNICODE
#define ApproveLicense ApproveLicenseA
#define OpenDevice OpenDeviceA
#define CaptureImage CaptureImageA
#define GetCapabilityString GetCapabilityStringA
#define CheckDeviceLicense CheckDeviceLicenseA
#define GetAllDevices GetAllDevicesA
#define EnableMsgBox EnableMsgBoxA
#define CaptureImageWithBarcode CaptureImageWithBarcodeA
#define GetBarcodeString GetBarcodeStringA
#else
#define ApproveLicense ApproveLicenseW
#define OpenDevice OpenDeviceW
#define CaptureImage CaptureImageW
#define GetCapabilityString GetCapabilityStringW
#define CheckDeviceLicense CheckDeviceLicenseW
#define GetAllDevices GetAllDevicesW
#define EnableMsgBox EnableMsgBoxW
#define CaptureImageWithBarcode CaptureImageWithBarcodeW
#define GetBarcodeString GetBarcodeStringW
#endif

BOOL WINAPI ApproveLicenseA(LPCSTR szLicense);
BOOL WINAPI ApproveLicenseW(LPCWSTR szLicense);
BOOL WINAPI CheckDeviceLicenseA(LPCSTR szDevice);
BOOL WINAPI CheckDeviceLicenseW(LPCWSTR szDevice);
LPCSTR WINAPI GetAllDevicesA(LPCSTR szSeperator);
LPCWSTR WINAPI GetAllDevicesW(LPCWSTR szSeperator);
BOOL WINAPI OpenDeviceA(LPCSTR szDevice);
BOOL WINAPI OpenDeviceW(LPCWSTR szDevice);
BOOL WINAPI CloseDevice();
DWORD WINAPI GetSystemError();
DWORD WINAPI GetDeviceError();
BOOL WINAPI SetupDevice();
BOOL WINAPI StartScan(BOOL bUI);
BOOL WINAPI EndScan();
BOOL WINAPI HaltScan();
LONG WINAPI CaptureImageA(LPCSTR szFilename);
LONG WINAPI CaptureImageW(LPCWSTR szFilename);

BOOL WINAPI SetCapability(DWORD nCap,DWORD nData,DWORD nType);
BOOL WINAPI SetCapabilityBOOL(DWORD nCap,BOOL bValue);
BOOL WINAPI SetCapabilityUINT8(DWORD nCap,DWORD uValue);
BOOL WINAPI SetCapabilityUINT16(DWORD nCap,DWORD uValue);
BOOL WINAPI SetCapabilityUINT32(DWORD nCap,DWORD uValue);
BOOL WINAPI SetCapabilityINT8(DWORD nCap,LONG uValue);
BOOL WINAPI SetCapabilityINT16(DWORD nCap,LONG uValue);
BOOL WINAPI SetCapabilityINT32(DWORD nCap,LONG uValue);
BOOL WINAPI SetCapabilityFIX32(DWORD nCap,DOUBLE lfValue);
DWORD WINAPI GetCapability(DWORD nCap);
DOUBLE WINAPI GetCapabilityFIX32(DWORD nCap);
LPCSTR WINAPI GetCapabilityStringA(DWORD nCap);
LPCWSTR WINAPI GetCapabilityStringW(DWORD nCap);

BOOL WINAPI EnableMsgBoxA(BOOL bEnable,LPCSTR szPath);
BOOL WINAPI EnableMsgBoxW(BOOL bEnable,LPCWSTR szPath);

BOOL WINAPI CaptureImageWithBarcodeA(LPCSTR szFilename);
BOOL WINAPI CaptureImageWithBarcodeW(LPCWSTR szFilename);
LONG WINAPI GetBarcodeCount();
DWORD WINAPI GetBarcodeType(INT nItem);
LPCSTR WINAPI GetBarcodeStringA(INT nItem);
LPCWSTR WINAPI GetBarcodeStringW(INT nItem);
LONG WINAPI ReleaseBarcode();

#endif