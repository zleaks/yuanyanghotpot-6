#ifndef GTKDRV_MAIN_C
#define GTKDRV_MAIN_C

#include "config.h"
#include "wine/port.h"

#include "x11drv.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/unicode.h"
#include "commdlg.h"
#include "shlwapi.h"
#include "shlobj.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

//static GtkWindow* gparent=NULL;
//static OPENFILENAMEW* pofn=NULL;
#define MAX_FILETER  200
typedef struct wapper_OPENFILENAMEW {
	OPENFILENAMEW* pofn;
	BOOL  *bresult;
	//WCHAR *pfilter[MAX_FILETER];
	WCHAR **pfilter;
}wapper_OPENFILENAMEW;

static BOOL get_ignoreCase_pattern(char**ppignoreCase_pattern, unsigned int  ignore_size,char* pattern,unsigned int  size )
{
	//bin-->[Bb][Ii][Nn]
	//需要4倍空间？
	char*ignoreCase_pattern=*ppignoreCase_pattern;

	int j=0;
	int i=0;
	char tmp;
	char convertTmp;
	for(i=0;i<size;i++){
		if(j>=ignore_size)
			return FALSE;
		tmp=pattern[i];
		if(!isalpha(tmp)){//不是字母，直接复制
		  ignoreCase_pattern[j++]=tmp;
		  continue;
		}
		if (islower(tmp)) 
			convertTmp = toupper(tmp);
		else
			convertTmp = tolower(tmp);
		
		ignoreCase_pattern[j++]='[';
		ignoreCase_pattern[j++]=tmp;			
		ignoreCase_pattern[j++]=convertTmp;
		ignoreCase_pattern[j++]=']';
	}
	TRACE("(%s)==>(%s)\n",(pattern),(ignoreCase_pattern));


	return TRUE;

}
static BOOL set_OPENFILENAMEW_parameter(GtkFileFilter *filter,wapper_OPENFILENAMEW* wapper)
{
  OPENFILENAMEW* pofn=wapper->pofn;
  
   TRACE("11(%d)(%d)(%d)\n",pofn->nFilterIndex,pofn->nFileOffset,pofn->nFileExtension);
   //计算文件名offset
   	WCHAR* w_file_path=PathFindFileNameW(pofn->lpstrFile);
	pofn->nFileOffset=w_file_path-pofn->lpstrFile;
   
   //计算扩展名offset
   WCHAR * tmp=pofn->lpstrFile+strlenW(pofn->lpstrFile);
   
   while(tmp!=pofn->lpstrFile){
		if(*(--tmp)==(WCHAR)('.')){
			tmp++;
			break;
		}
   }
   pofn->nFileExtension=tmp-pofn->lpstrFile;


	//获取当前fileter索引
   WCHAR *pfiltername=wapper->pfilter[0];
   while(pfiltername){//如果存在filter
		const char *filter_name=gtk_file_filter_get_name (filter);
		if(!filter_name){
			ERR("filter_name err!\n");
			break;
		}
		
		WCHAR fileter_name_buf[MAX_PATH];
		memset(fileter_name_buf,0,sizeof(fileter_name_buf));
		int len = MultiByteToWideChar(CP_UTF8, 0, filter_name, -1, NULL, 0);
		MultiByteToWideChar(CP_UTF8, 0, filter_name, -1, fileter_name_buf, len);
		
		for(int i=0;i<MAX_FILETER;i+=2){
			TRACE("(%s)(%s)\n",debugstr_w(wapper->pfilter[i]),debugstr_w(wapper->pfilter[i+1]));
			if(!(wapper->pfilter[i]))
			  break;
			if(!strcmpW(fileter_name_buf,wapper->pfilter[i])){//名字相同
				pofn->nFilterIndex=i/2+1;
				break;
			}
		}
		break;
   	}

  TRACE("(%d)(%d)(%d)\n",pofn->nFilterIndex,pofn->nFileOffset,pofn->nFileExtension);

  return TRUE;
}

static LPWSTR get_first_ext_from_spec(LPWSTR buf, LPCWSTR spec)
{
    WCHAR *endpos, *ext;

    lstrcpyW(buf, spec);
    if( (endpos = StrChrW(buf, ';')) )
        *endpos = '\0';

    ext = PathFindExtensionW(buf);
    if(StrChrW(ext, '*'))
        return NULL;

    return ext;
}



static BOOL check_ext_from_spec(LPWSTR ext_checked, LPCWSTR spec)
{//spec形如：tif;jpg;g3f

	TRACE("(%s)----(%s)\n",debugstr_w(spec),debugstr_w(ext_checked));
	int size=(lstrlenW(spec)+1)*sizeof(WCHAR);
	WCHAR *buf=HeapAlloc( GetProcessHeap(), 0, size);
	if(!buf)
		return NULL;
    lstrcpyW(buf, spec);
	WCHAR* p=buf;
	WCHAR *endpos;
	WCHAR *ext;
	int jump_len=0;
	while(*p){		
		TRACE("(%s)\n",debugstr_w(p));
	    if( endpos = StrChrW(p, ';')){//分号处设置空字符
	        *endpos = '\0';
	    }
		ext = PathFindExtensionW(p);
		//if(StrChrW(ext, '*'))
			//return NULL;
		if(!lstrcmpiW(ext_checked, ext)){//对比是否找到
			HeapFree(GetProcessHeap(), 0,buf);
			return TRUE;
		}

		p+=lstrlenW(p);
		if(endpos)
			p++;
	}	
	HeapFree(GetProcessHeap(), 0,buf);
    return FALSE;
}



static BOOL get_fullname (GtkFileFilter *filter,wapper_OPENFILENAMEW* wapper)
{
  const char *name;
  char **patterns;
  char *pattern_list;
  WCHAR newextbuf[MAX_PATH]={0};
  WCHAR* newext=NULL;
  WCHAR *pfiltername=wapper->pfilter[0];
  unsigned int len=0;
  OPENFILENAMEW* pofn=wapper->pofn;
  
  WCHAR *ext_file = PathFindExtensionW(pofn->lpstrFile);//获取文件后缀
  if(pfiltername){//如果存在filter
	  
  	  name=gtk_file_filter_get_name (filter);
	  /*len = WideCharToMultiByte( CP_UTF8, 0, lpstrPos, -1, NULL, 0, NULL, NULL );
	  if (!(tmp=HeapAlloc( GetProcessHeap(), 0, len ))){
		  ERR("HeapAlloc error!\n");
		  return;
	  }
	  WideCharToMultiByte( CP_UTF8, 0, lpstrPos, -1, tmp, len, NULL, NULL );

	  len = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
	  WCHAR *ret = heap_alloc(len * sizeof(WCHAR));
	  if(!ret){
		  ERR("HeapAlloc error!\n");
		  return;
	  }*/
	  
	  WCHAR *newext_tmp=newextbuf;
	  memset(newext_tmp,0,sizeof(newextbuf));
	  
	  len = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
	  MultiByteToWideChar(CP_UTF8, 0, name, -1, newext_tmp, len);
	  for(int i=0;i<MAX_FILETER;i+=2){
	  	  TRACE("(%s)(%s)\n",debugstr_w(wapper->pfilter[i]),debugstr_w(wapper->pfilter[i+1]));
	  	  if(!(wapper->pfilter[i])){
		  	break;
	  	  }
		  if(!strcmpW(newext_tmp,wapper->pfilter[i])){//名字相同，拷贝后缀
		  		if(check_ext_from_spec(ext_file,wapper->pfilter[i+1]))//检查文件已有后缀是否存在于filter列表中
					return TRUE;
				memset(newext_tmp,0,sizeof(newextbuf));
				newext = get_first_ext_from_spec(newext_tmp,wapper->pfilter[i+1]);
				break;
		  }
	  }
  }
  if(!(newext&&*(newext))){//没有找到，添加默认
	  if((pofn->lpstrDefExt)&&(*((pofn->lpstrDefExt)))){
	  		  newext=newextbuf;		  
		  	  memset(newext,0,sizeof(newextbuf));
			  lstrcpyW(newext, L".");
			  lstrcatW(newext, pofn->lpstrDefExt);
	  }
  }
  if(!(newext&&*(newext))){
	TRACE("not find ext!\n");
	return TRUE;
  }
  TRACE("(%s)(%s)\n",debugstr_w(pofn->lpstrFile),debugstr_w(newext));
  if(newext&&(*newext))
  {
	  if(lstrcmpW(ext_file, newext))
		  lstrcatW(pofn->lpstrFile, newext);
  }
  TRACE("(%s)\n",debugstr_w(pofn->lpstrFile));
  return TRUE;
}



static void m_gtk_realize(GtkWidget *widget, gpointer user)
{
	gtk_widget_set_window(widget,(GdkWindow *)user);
}
static void* get_gtk_window_handle(HWND hwnd)
{
    TRACE("(%p)(%p)\n", hwnd,GetDesktopWindow());		
	HWND  parent_hwnd=NULL;
	Window root_return;
	Window parent_return;
	Window *children_return;
	unsigned int nchildren_return;
	GtkWidget *gtk=NULL;
	struct   x11drv_win_data* data=NULL;
	int xstatus;
	int err_num=0;
	do{
		data=get_win_data(hwnd);
		if((data&&data->display&&data->whole_window)){
			break;
		}
		if(data)
	      release_win_data(data);
		ERR("get_win_data err(%d)!\n",++err_num );
		hwnd=GetParent(hwnd);
		//hwnd=GetWindow(GetDesktopWindow(),GW_CHILD);
		if(!hwnd){
			ERR("get_win_data final err!\n" );
			return NULL;
		}
	}while(1);

	TRACE( "win %p/%lx window %s whole %s client %s\n",
		   hwnd, data->whole_window, wine_dbgstr_rect( &data->window_rect ),
		   wine_dbgstr_rect( &data->whole_rect ), wine_dbgstr_rect( &data->client_rect ));
	GdkDisplay *ddp=gdk_x11_lookup_xdisplay (data->display);
	if(!(ddp)){
		ERR("ddp error!(%x)(%x)\n",data,data->display);
		goto END;
	}
	TRACE("ddp(%x)\n",ddp);

	GdkWindow *dw=gdk_x11_window_foreign_new_for_display (ddp,data->whole_window);
	if(!(dw)){
		ERR("dw error!\n");
		goto END;
	}
	gdk_window_focus(dw, GDK_CURRENT_TIME);
	TRACE("dw(%p)\n",dw);
	gtk=gtk_widget_new(GTK_TYPE_WINDOW,NULL);
	if(!(gtk)){
		ERR("gtk error!\n");
		goto END;
	}
	g_signal_connect(gtk,"realize",G_CALLBACK(m_gtk_realize),dw);
	gtk_widget_set_has_window(gtk,TRUE);
	gtk_widget_realize(gtk);
	if(is_window_rect_full_screen(&data->window_rect)){
		reset_clipping_window();
		TRACE("(reset_clipping_window)\n");
	}
END:
	if(data)
	  release_win_data(data);
	return  GTK_WINDOW(gtk);
}

static void
dialog_response_callback (GtkDialog              *dialog,
                          gint                    response_id,
                          wapper_OPENFILENAMEW*	 wapper)
{

  OPENFILENAMEW* pofn=wapper->pofn;
  BOOL* bresult=wapper->bresult;
  *bresult=FALSE;

  gtk_widget_hide(dialog);
  if (response_id == GTK_RESPONSE_ACCEPT){
		char *filenametmp;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		filenametmp = gtk_file_chooser_get_filename (chooser);
		if(filenametmp){
		   WCHAR * tmp=wine_get_dos_file_name(filenametmp);
		   g_free (filenametmp);		   
		   memset(pofn->lpstrFile,0,sizeof(WCHAR)*(pofn->nMaxFile));
		   strcpyW(pofn->lpstrFile,tmp);
		   TRACE("11(%s)\n",debugstr_w(pofn->lpstrFile));
		   HeapFree( GetProcessHeap(), 0, tmp);
		   GtkFileFilter *filter=gtk_file_chooser_get_filter(chooser);
		   if(GTK_FILE_CHOOSER_ACTION_SAVE==gtk_file_chooser_get_action(chooser)){
			   get_fullname(filter,wapper);
		   }
		   set_OPENFILENAMEW_parameter(filter,wapper);
		   *bresult=TRUE;
		}
  
   }
  gtk_widget_destroy (dialog);
  

  return ;

}
static void init_dialog(wapper_OPENFILENAMEW* wrapper,GtkWidget       *dialog)
{
	TRACE("begin!\n");
	GtkFileFilter *filter;
	char * tmp;
	
	char * ignore_tmp;
	unsigned int len=0;
	unsigned int filter_index=0;
	OPENFILENAMEW* pofn;
	pofn=wrapper->pofn;
	char **pfilter=&(wrapper->pfilter[0]);
	if(!pofn){
		ERR("pofn error!\n");
		return;
	}
	//初始化过滤条件
	if(pofn->lpstrFilter)
	{
	  LPCWSTR lpstrPos = pofn->lpstrFilter;
	  //TRACE("(%s)",debugstr_w(pofn->lpstrFilter));
	  for(;;)
	  {

	  	if(filter_index>=MAX_FILETER){
			ERR("too many filters(%d)!\n",filter_index);
			break;
	  	}
		/* filter is a list...	title\0ext\0......\0\0
		 * Set the combo item text to the title and the item data
		 *	to the ext
		 */
		LPCWSTR lpstrDisplay;
		LPWSTR lpstrExt;
	
		/* Get the title */
		if(! *lpstrPos) break;	  /* end */
		
		filter = gtk_file_filter_new ();
        len = WideCharToMultiByte( CP_UTF8, 0, lpstrPos, -1, NULL, 0, NULL, NULL );
        if (!(tmp=HeapAlloc( GetProcessHeap(), 0, len ))){
			ERR("HeapAlloc error!\n");
			return;
        }
        WideCharToMultiByte( CP_UTF8, 0, lpstrPos, -1, tmp, len, NULL, NULL );
		gtk_file_filter_set_name (filter, tmp);

		pfilter[filter_index]=lpstrPos;
		TRACE("filter_name(%s)(%s)\n",debugstr_w(lpstrPos),tmp);

		HeapFree(GetProcessHeap(), 0,tmp);
		
		lpstrPos += lstrlenW(lpstrPos) + 1;//+1跳过空字符


        len = WideCharToMultiByte( CP_UTF8, 0, lpstrPos, -1, NULL, 0, NULL, NULL );
        if (!(tmp=HeapAlloc( GetProcessHeap(), 0, len ))){
			ERR("HeapAlloc error!\n");
			return;
        }
        WideCharToMultiByte( CP_UTF8, 0, lpstrPos, -1, tmp, len, NULL, NULL );
		
		pfilter[filter_index+1]=lpstrPos;
		//此处需要增加大小写模式
		if (!(ignore_tmp=HeapAlloc( GetProcessHeap(), 0, len*4 ))){//4倍大小
			ERR("HeapAlloc error!\n");
			return;
        }
		memset(ignore_tmp,0,len*4);
		if(!get_ignoreCase_pattern(&ignore_tmp,len*4,tmp,len)){
			ERR("get_ignoreCase_pattern error!\n");
			return;
		}

		TRACE("filter_pattern(%s)==>(%s)=>(%d)\n",debugstr_w(lpstrPos),tmp,filter_index);
		TRACE("filter_patternlist");
		char* tmppattern=ignore_tmp;
	    while(*tmppattern){
			char *endpos;
			if( (endpos = strchr(tmppattern, ';')) )//找到一个pattern
				*endpos = '\0';
	
			if(strcmp(tmppattern, "*.*"))//匹配所有
			  gtk_file_filter_add_pattern (filter, tmppattern);
			else
			  gtk_file_filter_add_pattern (filter, "*");
			
			TRACE("(%s)",(tmppattern));
			
			tmppattern+=strlen(tmppattern);
			if(endpos)
				tmppattern+=1;
		}
		TRACE("\n");
		HeapFree(GetProcessHeap(), 0,tmp);
		HeapFree(GetProcessHeap(), 0,ignore_tmp);
		
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
		if(0==filter_index){
			gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
		}
		filter_index+=2;
		/* Add the item at the end of the combo */
		//SendMessageW(fodInfos->DlgInfos.hwndFileTypeCB, CB_SETITEMDATA, nFilters - 1, (LPARAM)lpstrExt);
		/* malformed filters are added anyway... */
		
		lpstrPos += lstrlenW(lpstrPos) + 1;
		if (!*lpstrPos) break;
	  }
	}
	/* Make this filter the default */
	if(OFN_OVERWRITEPROMPT==pofn->Flags){
		gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
		TRACE("set overwrite_confirmation!\n");
	}
	
	if(*(pofn->lpstrFile)){

        len = WideCharToMultiByte( CP_UTF8, 0, pofn->lpstrFile, -1, NULL, 0, NULL, NULL );
        if (!(tmp=HeapAlloc( GetProcessHeap(), 0, len ))){
			ERR("HeapAlloc error!\n");
			return;
        }
        WideCharToMultiByte( CP_UTF8, 0, pofn->lpstrFile, -1, tmp, len, NULL, NULL );
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog), tmp);		
		TRACE("default_name(%s)(%s)\n",debugstr_w(pofn->lpstrFile),tmp);
		HeapFree(GetProcessHeap(), 0,tmp);
		
	}
	
	//默认后缀
	if((pofn->lpstrDefExt)&&(*(pofn->lpstrDefExt))){
		TRACE("default_ext(%s)\n",debugstr_w(pofn->lpstrDefExt));
	}

}
static BOOL  Gtk_file_chooser_dialog(wapper_OPENFILENAMEW* pwapper)

{
	BOOL bret=FALSE;
	GtkWidget *dialog;
	GtkWindow *gparent;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;
	gparent= get_gtk_window_handle(pwapper->pofn->hwndOwner);

	dialog = gtk_file_chooser_dialog_new ("打开",gparent,action,"_取消(C)", GTK_RESPONSE_CANCEL,"_打开(O)",GTK_RESPONSE_ACCEPT,  NULL);
	init_dialog(pwapper,dialog);

	g_signal_connect (dialog, "response",
					  G_CALLBACK (dialog_response_callback), pwapper);
	g_signal_connect (dialog, "destroy",
					  G_CALLBACK (gtk_main_quit), NULL);
	
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_transient_for  (GTK_WINDOW (dialog),	GTK_WINDOW(gparent));
	gtk_widget_show(dialog);
	return TRUE;

}


static BOOL Gtk_file_chooser_dialog_main (wapper_OPENFILENAMEW* pwapper)
{
     //gtk_init(NULL,NULL);
	 Gtk_file_chooser_dialog(pwapper);
	 gtk_main();
     return TRUE;
}



BOOL CDECL GTKDRV_Gtk_GetOpenFileNameW(void*p1)
{
	OPENFILENAMEW*  pofn=(OPENFILENAMEW*)p1;
	//gparent= get_gtk_window_handle(pofnw->hwndOwner);
	TRACE("(%p)\n",pofn);
	BOOL bret=FALSE;

	wapper_OPENFILENAMEW   wapper;
	memset(&wapper,0,sizeof(wapper));
	wapper.pofn=pofn;
	wapper.bresult=&bret;

	wapper.pfilter=(WCHAR**)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR*)*MAX_FILETER);
	memset((WCHAR*)(wapper.pfilter),0,sizeof(WCHAR*)*MAX_FILETER);
	Gtk_file_chooser_dialog_main(&wapper);	
	HeapFree(GetProcessHeap(), 0,(WCHAR*)(wapper.pfilter));
	if(bret){
		
		TRACE( "(%s %d)\n", debugstr_w(pofn->lpstrFile), bret );
	}
	
	TRACE("(%d)\n",bret);
	return bret;
}




static void  Gtk_file_chooser_save_dialog(wapper_OPENFILENAMEW* pwapper)

{

	GtkWidget *dialog;
	GtkWindow* gparent;
	gint res;


	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gparent= get_gtk_window_handle(pwapper->pofn->hwndOwner);

	dialog = gtk_file_chooser_dialog_new ("另存为",gparent,action,"_取消(C)", GTK_RESPONSE_CANCEL,"_保存(S)",GTK_RESPONSE_ACCEPT,  NULL);
	init_dialog(pwapper,dialog);
	g_signal_connect (dialog, "response",
					  G_CALLBACK (dialog_response_callback), pwapper);
	g_signal_connect (dialog, "destroy",
					  G_CALLBACK (gtk_main_quit), NULL);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_transient_for  (GTK_WINDOW (dialog),
		       gparent);
	gtk_widget_show(dialog);



}

static BOOL Gtk_file_chooser_save_dialog_main (wapper_OPENFILENAMEW* pwapper)
{

     //BOOL bret=FALSE;
     //gtk_init(NULL,NULL);
	 Gtk_file_chooser_save_dialog(pwapper);
	 gtk_main();
	 
     return TRUE;
}

/****************************************************************************/

BOOL  CDECL GTKDRV_Gtk_GetSaveFileNameW(void* p1)
{
	OPENFILENAMEW* pofn=(OPENFILENAMEW*)p1;
	TRACE("(%p)\n",pofn);
	BOOL bret=FALSE;	
	wapper_OPENFILENAMEW   wapper;
	memset(&wapper,0,sizeof(wapper));
	wapper.pofn=pofn;
	wapper.bresult=&bret;


	wapper.pfilter=(WCHAR**)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR*)*MAX_FILETER);
	memset((WCHAR*)(wapper.pfilter),0,sizeof(WCHAR*)*MAX_FILETER);
	
	Gtk_file_chooser_save_dialog_main(&wapper);
	HeapFree(GetProcessHeap(), 0,(WCHAR*)(wapper.pfilter));
	if(bret){		
		TRACE( "(%s)(%d)\n", debugstr_w(pofn->lpstrFile), bret );
	}
	TRACE("(%d)\n",bret);
	return bret;
}






#endif
