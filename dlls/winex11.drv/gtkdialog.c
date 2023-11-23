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
WINE_DEFAULT_DEBUG_CHANNEL(gtkdlg);
static LPWSTR UnixPathToDosPath(LPCSTR unixPath )
{
	typedef LPWSTR(*CDECL WGDFN)(LPCSTR);
	LPWSTR(*CDECL pWineGetDosFileName)(LPCSTR) = NULL;
	pWineGetDosFileName = (WGDFN)GetProcAddress( GetModuleHandleA("KERNEL32"),"wine_get_dos_file_name");
	if (pWineGetDosFileName == NULL)
	{
		ERR("Failed to get pointer of wine_get_dos_file_name");
		return NULL;
	}
	 return pWineGetDosFileName(unixPath);

}

#if 0

//add by xiaoya
static char *heap_strdupWtoA(LPCWSTR str)
{
	char *ret = NULL;
	if(str){
                UINT size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
		ret = heap_alloc(size*sizeof(char));
		if(ret)
			WideCharToMultiByte(CP_UTF8, 0, str, -1, ret, size, NULL, NULL);
	}
	return ret;
}
//end by xiaoya

BOOL GTK_OnOpenMultipleFiles(GSList *filename_list, OPENFILENAMEW *ofn)
{
    GSList *iterator;
    LPWSTR  filename_pW, filename_dirW, filename_longW ;
    WCHAR lpstrTemp[MAX_PATH] ;
    LPWSTR file_dir_tmp[255];
    UINT nSizePath,sizeUsed = 0;
    
    ofn->lpstrFile[0] = '\0';
    ofn->nMaxFile = 2599;
    filename_dirW = UnixPathToDosPath(filename_list->data);
    PathRemoveFileSpecW(filename_dirW);
    nSizePath = lstrlenW(filename_dirW);
    
    memset(lpstrTemp,0,sizeof(WCHAR)*MAX_PATH);

    for(iterator = filename_list; iterator; iterator= iterator->next)
   {		
	TRACE("cccxxxyyy----------------- file list %s\n",iterator->data);
	filename_pW = UnixPathToDosPath(iterator->data);
	
	lstrcpyW(file_dir_tmp, filename_pW);
    	PathRemoveFileSpecW(file_dir_tmp);
	if( filename_dirW != NULL && file_dir_tmp != NULL && lstrcmpW(filename_dirW, file_dir_tmp))
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK,"please select the same directory!!!");
		gtk_window_set_title(GTK_WINDOW(dialog), "Error");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		while(gtk_events_pending())
			gtk_main_iteration();

		return FALSE;	
	}

	filename_longW = PathFindFileNameW(filename_pW);

	UINT nstrCharCount = 0 ;
	sizeUsed++;

	while( nstrCharCount < lstrlenW(filename_longW))
	{
	    lpstrTemp[sizeUsed++] =*(filename_longW+nstrCharCount);
	    nstrCharCount++;
	}
   }
	sizeUsed+=2;

    lstrcpyW(ofn->lpstrFile,filename_dirW);

    memcpy(ofn->lpstrFile + nSizePath,lpstrTemp,sizeUsed*sizeof(WCHAR));

    ofn->nFileOffset = nSizePath+1;
    ofn->nFileExtension = 0;
    heap_free(lpstrTemp);  
    return TRUE;
}

BOOL GTK_GetOpenFileNameW(OPENFILENAMEW *ofn)
{
		//use os filemanager
again:	
    TRACE("ccxxyy------select Linux style \n");
	TRACE("%p (%p)\n", ofn,ofn->hwndOwner);
	   
    // gtk_init(0,0);
    GtkWindow* wrapperGTKWin=X11DRV_GetGTKWindowHandle(ofn->hwndOwner);
    TRACE("YYYXXXRRR---> get foreign x11 wrapper=%p\n",wrapperGTKWin);

    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new( "打开",wrapperGTKWin ,GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,NULL);

    if (ofn->Flags & OFN_PATHMUSTEXIST)
    {
	TRACE("cccxxxyyy----------------\n");
	gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER(dialog), TRUE);
    }
		
    if(gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT)
    {
 
	LPWSTR filename_pW, lpszTemp;
	GSList *filename_list;
	WCHAR *temp;
	DWORD  index;
	ofn->nMaxFile = 2599;
			
	filename_list = gtk_file_chooser_get_filenames( GTK_FILE_CHOOSER(dialog));
	if (g_slist_length(filename_list) >1 )
	{
	    UINT ret ;
	    ret = GTK_OnOpenMultipleFiles(filename_list, ofn);
			
	    gtk_widget_destroy(dialog);
	    while(gtk_events_pending())
		gtk_main_iteration();
			
	    if(!ret)
		goto again;
	    else
		return ret; 
				
	}
			
	filename_pW = UnixPathToDosPath( filename_list->data);
	TRACE("ccxxyy------select one file---- %s\n", debugstr_w(filename_pW));

	lstrcpyW(ofn->lpstrFile, filename_pW);
	ofn->lpstrFile[lstrlenW(ofn->lpstrFile)+1] =0;
	
	lpszTemp = PathFindFileNameW(filename_pW);
	ofn->lpstrFileTitle = lpszTemp;
	temp = strstrW(ofn->lpstrFile, lpszTemp);
	index = lstrlenW(ofn->lpstrFile) -lstrlenW(temp);
	ofn->nFileOffset = index;
	TRACE("ccxxyy-----------%d   %s %s\n",ofn->nFileOffset, debugstr_w(ofn->lpstrFile));


	lpszTemp = PathFindExtensionW(filename_pW);
	ofn->lpstrDefExt= lpszTemp;

	ofn->nFileExtension = (*lpszTemp) ? index+1 : 0;

	gtk_widget_destroy(dialog);
			
	while(gtk_events_pending())
	{
	    gtk_main_iteration();
	}

	return TRUE;

    }else { 
	
	gtk_widget_destroy(dialog);
	while(gtk_events_pending())
	{
	    gtk_main_iteration();
	}
	return FALSE;
    }
}

BOOL GTK_GetSaveFileNameW(LPOPENFILENAMEW ofn)
{
    static const char default_file_name_baseW[] = {'U','n','t','i','t','l','e','d','.','%','s',0};
    char defaultFileName[255];

    // gtk_init(0,0);
    GtkWindow* wrapperGTKWin=X11DRV_GetGTKWindowHandle(ofn->hwndOwner);
    TRACE("YYYXXXRRR---> get foreign x11 wrapper=%p\n",wrapperGTKWin);

    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new( "保存",wrapperGTKWin ,GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,NULL);
    sprintf(defaultFileName, default_file_name_baseW ,heap_strdupWtoA(ofn->lpstrDefExt));
    if(ofn->lpstrInitialDir)
    {
    	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), heap_strdupWtoA(ofn->lpstrInitialDir));
        TRACE("ccxxyy--------default folder %s %s\n",debugstr_w(ofn->lpstrInitialDir), heap_strdupWtoA(ofn->lpstrInitialDir));		
    }
    if(ofn->lpstrFile==NULL) 
    {
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defaultFileName);
    }else{
	TRACE("ccxxyy--------- %s\n",heap_strdupWtoA(ofn->lpstrFile));

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),heap_strdupWtoA(ofn->lpstrFile));
    }	
		
    if(gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT)
    {
	LPCSTR filename_p;
	LPWSTR filename_pW, lpszTemp;
	WCHAR *temp;
	DWORD index;

	filename_p = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog));
	filename_pW = UnixPathToDosPath( filename_p);
	lstrcpyW(ofn->lpstrFile, filename_pW);
	ofn->nMaxFile = 2599;
	ofn->lpstrFile[lstrlenW(ofn->lpstrFile)+1] ='\0';
			
	lpszTemp = PathFindFileNameW(filename_pW);
	ofn->lpstrFileTitle= lpszTemp ;
	temp =	strstrW(ofn->lpstrFile, lpszTemp);
        index = lstrlenW(ofn->lpstrFile) -lstrlenW(temp)+1;
	ofn->nFileOffset = index;


	lpszTemp = PathFindExtensionW(filename_pW);
	ofn->lpstrDefExt = lpszTemp;
	ofn->nFileExtension = (*lpszTemp) ? index+1 : 0;

			
	gtk_widget_destroy( dialog);
	while(gtk_events_pending())
	{
	    gtk_main_iteration();
	}

	return TRUE;
    }else{

        gtk_widget_destroy(dialog);
	while(gtk_events_pending())
	{
	    gtk_main_iteration();
	}
	return FALSE;
    }

}
#endif

//add by xiaoya
GTK_SHBrowseForFolderW(LPBROWSEINFOW lpbi,LPCWSTR path)
{
    if(!path){
        return;
    }
    // LPITEMIDLIST lpid;
    // gtk_init(0,0);
    GtkWidget *dialog;
    dialog =  gtk_file_chooser_dialog_new("select folder",NULL,GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,NULL); 
    if(gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT) 
    {
	
	LPCSTR folderPath;
	LPCWSTR folderPathW;
	folderPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	TRACE("cccxxxyyy-------save directory is (%s)\n",folderPath);
        folderPathW = UnixPathToDosPath(folderPath);
        if(folderPathW){
            lstrcpyW(path, folderPathW);
            HeapFree( GetProcessHeap(), 0, folderPathW );
        }
        // lpid = SHSimpleIDListFromPathW(folderPathW);
	// pdump(lpid);

        gtk_widget_destroy(dialog);
	while (gtk_events_pending())
	    gtk_main_iteration();
	// return lpid;
    return;
    
    }else{
    
    	gtk_widget_destroy(dialog);
	while (gtk_events_pending())
	{
	    gtk_main_iteration();
	}
	// return NULL;
    return;
    }   
}
//endby xiaoya
