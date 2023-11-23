#ifndef _SCANNER_FUNAPI_H_
#define _SCANNER_FUNAPI_H_
#ifdef __cplusplus
 extern "C" {
 #endif

#define DEFAULT(x)	= x

#define JPEG_DEFAULT_W        0		// default
#define JPEG_QUALITYSUPERB_W  0x80	// superb quality (100:1)
#define JPEG_QUALITYGOOD_W    0x0100	// good quality (75:1)
#define JPEG_QUALITYNORMAL_W  0x0200	// normal quality (50:1)
#define JPEG_QUALITYAVERAGE_W 0x0400	// average quality (25:1)
#define JPEG_QUALITYBAD_W     0x0800	// bad quality (10:1)

#define TIFF_DEFAULT_W        0
#define TIFF_PACKBITS_W       0x0100  // PACKBITS compression
#define TIFF_DEFLATE_W        0x0200  // DEFLATE compression (a.k.a. ZLIB compression)
#define TIFF_ADOBE_DEFLATE_W  0x0400  // ADOBE DEFLATE compression
#define TIFF_NONE_W           0x0800  // save without any compression
#define TIFF_CCITTFAX3_W      0x1000  // CCITT Group 3 fax encoding
#define TIFF_CCITTFAX4_W      0x2000  // CCITT Group 4 fax encoding
#define TIFF_LZW_W	      0x4000	// LZW compression
#define TIFF_JPEG_W	      0x8000	// JPEG compression
#define TIFF_LOGLUV_W	      0x10000	// LogLuv compression

typedef float				GCH_FLOAT;
typedef int				GCH_INT;
typedef bool 				GCH_BOOL;
/**字节（8位）*/
typedef unsigned char			GCH_BYTE;

/**指向GCH_BYTE类型的指针*/
typedef unsigned char *			GCH_LPBYTE;

/**指向GCH_BYTE类型常量的指针*/
typedef const unsigned char *		GCH_LPCBYTE;

/**无符号整型（16位）*/
typedef unsigned short          	GCH_WORD;

/**无符号整型（32位）*/
typedef unsigned int            	GCH_DWORD;

/**字符（8位) */
typedef char				GCH_CHAR;

/**指向8位字符的指针*/
typedef char *				GCH_LPSTR;

/**指向8位字符的指针*/
typedef const char *			GCH_LPCSTR;


/**
 * @brief 添加可用设备认证号。
 * @param[in]	szLicense 认证码。	
 * @return 成功返回0；失败返回-1。
 *
 */
GCH_BOOL gch_ApproveLicenseA(GCH_LPCSTR szLicense);

/**
 * @brief 初始化。
 * @return 成功返回0；失败返回-1。
 *
 */
GCH_INT gch_Init();

/**
 * @brief 获取设备列表。
 *
 * @param[in]	szSeperator	分隔符。	
  * @return 成功返回以szSeperator分隔的设备列表；无设备返回NULL。	
 *
 */
GCH_LPSTR gch_GetDevicesList(GCH_LPSTR szSeperator);

/**
 * @brief 打开设备。
 *
 * @param[in]	szDevice 设备名。	
 * @return 成功返回0；失败返回-1。
 *
 */
GCH_INT gch_OpenDevice(GCH_LPSTR szDevice);

/**
 * @brief 关闭设备。
 * @return 无返回。	
 *
 */
void gch_CloseDevice();

/**
 * @brief 设定扫描参数
 *
 * @param[in]	nCap	扫描参数。
 * @param[in]	value	需要设置的值。
 * @return 成功返回0；失败返回-1。
 *
 */
GCH_INT gch_SetCapability_INT(GCH_INT nCap, GCH_LPCSTR value);
/**
 * @brief 设定扫描参数
 *
 * @param[in]	nCap	扫描参数。
 * @param[in]	value	需要设置的值。
 * @return 成功返回0；失败返回-1。
 *
 */
GCH_INT gch_SetCapability_STR(GCH_LPCSTR nCap, GCH_LPCSTR value);

/**
 * @brief 开始扫描
 *
 * @return 返回扫描过程中的错误类型，0表示成功，可进行下一次扫描。
 *
 */
GCH_INT gch_StartScan();

/**
 * @brief 取消扫描
 *
 * @return 
 *
 */
void gch_EndScan();

/**
 * @brief 获取图像
 *
 * @param[in]	szFilename 保存的图像名。
 * @param[in]	flags 图像压缩设置。
 * @return 成功返回0；失败返回-1。
 *
 */
GCH_INT gch_CaptureImage(GCH_LPSTR szFilename, int flags DEFAULT(0));

/**
 * @brief 获取图像
 *
 * @param[out]	bytes_line 图像每一行的字节数。
 * @param[out]	width 图像宽。
 * @param[out]	height 图像高。
 * @param[out]	bpp 图像位深。
 * @return 返回图像字节流，失败返回NULL。
 *
 */
GCH_LPBYTE gch_CaptureImageMap(int &bytes_line, int &width, int &height, int &bpp);
#ifdef __cplusplus
}
#endif
#endif
