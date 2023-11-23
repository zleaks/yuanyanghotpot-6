#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"
#include "commdlg.h"
#include "dlgs.h"
#include "cdlg.h"
#include "cderr.h"
#include "shellapi.h"
#include "shlobj.h"
#include "filedlgbrowser.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);
#ifdef GTK_DIALOG


 static WCHAR *heap_strdupAtoW(const char *str)
 {
	 WCHAR *ret;
	 INT len;
 
	 if (!str)
		 return NULL;
 
	 len = MultiByteToWideChar(CP_ACP, 0, str, -1, 0, 0);
	 ret = heap_alloc(len * sizeof(WCHAR));
	 MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
 
	 return ret;
 }

 OPENFILENAMEW* Gtk_Get_OPENFILENAMEW_From_OPENFILENAMEA(OPENFILENAMEA* p_ofn)
{
	if(!p_ofn)
		return NULL;
	OPENFILENAMEW* p_ofnw=heap_alloc(sizeof(OPENFILENAMEW));
	if(!p_ofnw){
		ERR("alloc err!\n");
		return NULL;
	}
	*p_ofnw=*(OPENFILENAMEW*)p_ofn;
	

    p_ofnw->lpstrInitialDir = heap_strdupAtoW(p_ofn->lpstrInitialDir);
    p_ofnw->lpstrDefExt = heap_strdupAtoW(p_ofn->lpstrDefExt);
    p_ofnw->lpstrTitle = heap_strdupAtoW(p_ofn->lpstrTitle);
	p_ofnw->lpstrFile=NULL;
	p_ofnw->lpstrFilter=NULL;
	p_ofnw->lpstrCustomFilter=NULL;
    //if (p_ofn->lpstrFile&&(*(p_ofn->lpstrFile)))
    if (p_ofn->lpstrFile)
    {
    	TRACE("(%s)(%d)\n",p_ofn->lpstrFile,p_ofn->nMaxFile);//p_ofn->nMaxFile代表字符数
		const int alloc_len=p_ofn->nMaxFile;

        p_ofnw->lpstrFile = heap_alloc(alloc_len * sizeof(WCHAR));		
		memset(p_ofnw->lpstrFile,0,sizeof(alloc_len * sizeof(WCHAR)));
        p_ofnw->nMaxFile = alloc_len;

        int len = MultiByteToWideChar(CP_ACP, 0, p_ofn->lpstrFile, -1, NULL, 0);//计算字符数
		if(len<=p_ofnw->nMaxFile){
        	MultiByteToWideChar(CP_ACP, 0, p_ofn->lpstrFile, -1, p_ofnw->lpstrFile, len);
		}
		
		TRACE("(%d)(%d)(%d)\n",len,p_ofn->nMaxFile,p_ofnw->nMaxFile);		
    }

    if (p_ofn->lpstrFilter)
    {
        LPCSTR s;
        int n;

        /* filter is a list...  title\0ext\0......\0\0 */
        s = p_ofn->lpstrFilter;
        while (*s) s = s+strlen(s)+1;
        s++;
        n = s - p_ofn->lpstrFilter;
        int len = MultiByteToWideChar(CP_ACP, 0, p_ofn->lpstrFilter, n, NULL, 0);
        p_ofnw->lpstrFilter = heap_alloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, p_ofn->lpstrFilter, n, (WCHAR *)p_ofnw->lpstrFilter, len);
    }

    /* convert lpstrCustomFilter */
    if (p_ofn->lpstrCustomFilter)
    {
        int n, len;
        LPCSTR s;

        /* customfilter contains a pair of strings...  title\0ext\0 */
        s = p_ofn->lpstrCustomFilter;
        if (*s) s = s+strlen(s)+1;
        if (*s) s = s+strlen(s)+1;
        n = s - p_ofn->lpstrCustomFilter;
        len = MultiByteToWideChar(CP_ACP, 0, p_ofn->lpstrCustomFilter, n, NULL, 0);
        p_ofnw->lpstrCustomFilter = heap_alloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, p_ofn->lpstrCustomFilter, n, p_ofn->lpstrCustomFilter, len);
    }

	return p_ofnw;

}
BOOL Gtk_Get_OPENFILENAMEA_From_OPENFILENAMEW(OPENFILENAMEA* p_ofn,OPENFILENAMEW* p_ofnw)
{
	//目前只实现文件路径的复制
	if(!p_ofn||!p_ofnw){
		return FALSE;
	}
	if(0==p_ofnw->lpstrFile[0])//无效路径，返回
		return FALSE;
	
	DWORD lenW = lstrlenW(p_ofnw->lpstrFile) + 1;
	DWORD lenA = WideCharToMultiByte( CP_ACP, 0, p_ofnw->lpstrFile, lenW, NULL, 0, NULL, NULL );
	if(lenW>p_ofn->nMaxFile){
		ERR("(%d)(%d)!\n",lenW,p_ofn->nMaxFile);
		return FALSE;
	}
	memset(p_ofn->lpstrFile,0,lenA);
	WideCharToMultiByte( CP_ACP, 0, p_ofnw->lpstrFile, lenW,
						 p_ofn->lpstrFile, lenA, NULL, NULL );
	return TRUE;
}

/*add by xiaoya get fileMeange style form registry*/
 int get_FileManage_Style(DWORD flags)
{
   static int s_styleID=-1;
   if(s_styleID>0)
     return s_styleID;
   const char *dialog_style = getenv( "DIALOG_STYLE" );
   if(dialog_style&&!strcmp(dialog_style,"GTK_STYLE")){
	 if((flags&OFN_ENABLETEMPLATEHANDLE)||(flags&OFN_ENABLETEMPLATE)||(flags&OFN_ENABLEHOOK )){//进一步过滤判断
		 s_styleID= FILESTYLE_WINE;
	 }
	 else
         s_styleID= FILESTYLE_GTK;
   }
   else
   	s_styleID= FILESTYLE_WINE;
   return s_styleID;
   
   #if 0
   
   volatile  int tt=0;
   const WCHAR szKeyStyle[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','F','i','l','e','S','t','y','l','e',0}; 
   WCHAR key_name[]={'D','i','a','l','o','g','S','t','y','l','e',0};

   const WCHAR szDrives[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','D','r','i','v','e','s',0}; 

   int res;
   HKEY style_key;
   TRACE("styleID(%d),key_name(%s)\n",s_styleID,debugstr_w(key_name));

   if(s_styleID>0)
     return s_styleID;
   #ifdef  __DIALOGSTYLE_WINE__
    s_styleID= FILESTYLE_WINE;
   #else
	s_styleID= FILESTYLE_GTK;
   #endif
	
    res=RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"Software\\Wine\\FileStyle",0,KEY_READ|KEY_WOW64_64KEY, &style_key);
	TRACE("res(%d)\n",res);
	if(ERROR_SUCCESS!=res){
	    res=RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"Software\\Wine\\FileStyle",0,KEY_READ|KEY_WOW64_32KEY, &style_key);
		TRACE("res(%d)\n",res);
	}
	if(ERROR_SUCCESS==res){
		int data=0;
		DWORD data_size = sizeof(data);
		
		res=RegQueryValueExW(style_key, key_name, NULL, NULL,(BYTE *)&data, &data_size);
		if((ERROR_SUCCESS==res)&&(data>0)){
		   if(1==data){
				s_styleID= FILESTYLE_WINE;
		   }
		   else if(2==data){
				s_styleID= FILESTYLE_GTK;
		   }
		   else;	
    	}
		tt=123;
		TRACE("res=%d,styleID=%d\n",res,s_styleID);
	}
	else{
		tt=456;
		TRACE("style_key is NULL:res(%d)\n",res);
	}
	//添加
	if((flags&OFN_ENABLETEMPLATEHANDLE)||(flags&OFN_ENABLETEMPLATE)||(flags&OFN_ENABLEHOOK )){
		s_styleID= FILESTYLE_WINE;
	}
	TRACE("FILESTYLE(%d),tt(%d)\n",s_styleID,tt);
	return s_styleID;

	#endif

}
 void debug_OPENFILENAMEW(OPENFILENAMEW *This)
{
	 /*typedef struct tagOFNW {
		 DWORD		 lStructSize;//当前结构体占用字节大小
		 HWND		 hwndOwner;//窗口句柄
		 HINSTANCE	 hInstance;
		 LPCWSTR	 lpstrFilter;//过滤条件，for example, ".TXT;.DOC;.BAK"
		 LPWSTR 	 lpstrCustomFilter;//用户定义的过滤条件，为空即可
		 DWORD		 nMaxCustFilter;
		 DWORD		 nFilterIndex;
		 LPWSTR 	 lpstrFile;//默认名字
		 DWORD		 nMaxFile;//lpstrFile的大小
		 LPWSTR 	 lpstrFileTitle;//The file name and extension (without path information) of the selected file
		 DWORD		 nMaxFileTitle;//lpstrFileTitle的大小
		 LPCWSTR	 lpstrInitialDir;//初始化目录，先不管了
		 LPCWSTR	 lpstrTitle;//对话框标题
		 DWORD		 Flags;//添加OFN_OVERWRITEPROMPT标志，提示覆盖
		 WORD		 nFileOffset;//The zero-based offset, in characters, from the beginning of the path to the file name in the string pointed to by lpstrFile.
		 WORD		 nFileExtension;//The zero-based offset, in characters, from the beginning of the path to the file name extension in the string pointed to by lpstrFile
		 LPCWSTR	 lpstrDefExt;//默认后缀，当用户输入的文件名不带后缀时，加上次此后缀
		 LPARAM 	 lCustData;//lpfnHook的参数
		 LPOFNHOOKPROC	 lpfnHook;//A pointer to a hook procedure
		 LPCWSTR	 lpTemplateName;
		 void			*pvReserved;
		 DWORD			 dwReserved;
		 DWORD			 FlagsEx;
	 } OPENFILENAMEW,*LPOPENFILENAMEW;
	 */

	 TRACE("(%s)(%s)(%s)(%s)(%s)(%s)(%s)(%d)\n", 
	 debugstr_w(This->lpstrFilter),debugstr_w(This->lpstrCustomFilter),
	 debugstr_w(This->lpstrFile),debugstr_w(This->lpstrFileTitle),
	 debugstr_w(This->lpstrInitialDir),debugstr_w(This->lpstrTitle),
	 debugstr_w(This->lpstrDefExt),This->nFilterIndex);
 }

 

static OPENFILENAMEW sg_ofn={ sizeof(OPENFILENAMEW),
				 NULL, /*hInst*/0, 0, NULL, 0, 0, NULL,
				 0, NULL, 0, NULL, 0,
				 OFN_SHOWHELP | OFN_HIDEREADONLY | OFN_ENABLESIZING,
						 0, 0, NULL, 0, NULL };



 WCHAR *strcpyW( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

static BOOL gtk_unix_path_to_win_path( WCHAR* src,unsigned int  max_len)
{
    WCHAR * tmp;
	int len = WideCharToMultiByte( CP_UTF8, 0, src, max_len, NULL, 0, NULL, NULL );
	char strtmp[MAX_PATH*sizeof(WCHAR)];
	memset(strtmp,0,sizeof(strtmp));

	WideCharToMultiByte( CP_UTF8, 0, src, max_len, strtmp, len, NULL, NULL );

	TRACE("GTK_DIALOG:(%s)\n",strtmp);
	
    tmp=wine_get_dos_file_name(strtmp);
	memset(src,0,sizeof(WCHAR)*max_len);
	strcpyW(src, tmp);
    HeapFree(GetProcessHeap(), 0, tmp);
    return TRUE;

}



 void Gtk_GetPofn(OPENFILENAMEW** psg_ofn)
{
	TRACE("(%p)\n",*psg_ofn);
	//*psg_ofn=&sg_ofn;
	*psg_ofn=(OPENFILENAMEW*)HeapAlloc(GetProcessHeap(), 0, sizeof(OPENFILENAMEW));
	memset(*psg_ofn,0,sizeof(OPENFILENAMEW));

	
	(*psg_ofn)->lpstrFile=(OPENFILENAMEW*)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*MAX_PATH);
	memset((*psg_ofn)->lpstrFile,0,sizeof(WCHAR)*MAX_PATH);
	(*psg_ofn)->nMaxFile = MAX_PATH;

	
	(*psg_ofn)->lpstrDefExt=(LPWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*MAX_PATH);
	memset((*psg_ofn)->lpstrDefExt,0,sizeof(WCHAR)*MAX_PATH);

}

  void Gtk_FreePofn(OPENFILENAMEW* psg_ofn)
 {
	 TRACE("(%p)\n",psg_ofn);
	 //*psg_ofn=&sg_ofn;
	 if(!psg_ofn)
	 	return;
	 if(psg_ofn->lpstrFilter)
	   HeapFree(GetProcessHeap(), 0,psg_ofn->lpstrFilter);
	 if(psg_ofn->lpstrDefExt)
	   HeapFree(GetProcessHeap(), 0,psg_ofn->lpstrDefExt);
	 if(psg_ofn->lpstrFile)
	   HeapFree(GetProcessHeap(), 0,psg_ofn->lpstrFile);

	 if(psg_ofn->lpstrInitialDir)
	   HeapFree(GetProcessHeap(), 0,psg_ofn->lpstrInitialDir);
	 if(psg_ofn->lpstrTitle)
	   HeapFree(GetProcessHeap(), 0,psg_ofn->lpstrTitle);
	 if(psg_ofn->lpstrCustomFilter)
	   HeapFree(GetProcessHeap(), 0,psg_ofn->lpstrCustomFilter);


	 HeapFree(GetProcessHeap(), 0,psg_ofn);

 }

 BOOL Gtk_GetOpenFileNameW_wrapper(OPENFILENAMEW* psg_ofn)
{

	if(!psg_ofn)
		return FALSE;
	return Gtk_GetOpenFileNameW(psg_ofn);


}
 BOOL Gtk_GetSaveFileNameW_wrapper(OPENFILENAMEW* psg_ofn)
{

	if(!psg_ofn)
		return FALSE;
	return Gtk_GetSaveFileNameW(psg_ofn);

}
#endif


