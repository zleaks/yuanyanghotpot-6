#ifndef GTKDLG_H
#define GTKDLG_H
int get_FileManage_Style(DWORD flags);
void Gtk_GetPofn(OPENFILENAMEW** psg_ofn);
BOOL Gtk_GetOpenFileNameW_wrapper(OPENFILENAMEW* psg_ofn);
BOOL Gtk_GetSaveFileNameW_wrapper(OPENFILENAMEW* psg_ofn);
BOOL WINAPI Gtk_GetOpenFileNameW(void* p);
BOOL WINAPI Gtk_GetSaveFileNameW(void* p);
void Gtk_FreePofn(OPENFILENAMEW* psg_ofn);
OPENFILENAMEW* Gtk_Get_OPENFILENAMEW_From_OPENFILENAMEA(OPENFILENAMEA* p_ofn);
BOOL Gtk_Get_OPENFILENAMEA_From_OPENFILENAMEW(OPENFILENAMEA* p_ofn,OPENFILENAMEW* p_ofnw);

#endif
