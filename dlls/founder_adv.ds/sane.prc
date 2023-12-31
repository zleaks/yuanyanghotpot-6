/*
 * Resources for the SANE backend
 *
 * Copyright 2006 CodeWeavers, Aric Stewart
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

#include "resource.h"

#pragma makedep po

/*LANGUAGE LANG_ENGLISH, SUBLANG_DEFAULT*/
LANGUAGE LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED 


STRINGTABLE
{
 0 ""
 1 "#msgctxt#unit: pixels#px"
 2 "#msgctxt#unit: bits#b"
 3 "#msgctxt#unit: millimeters#mm"
 4 "#msgctxt#unit: dots/inch#dpi"
 5 "#msgctxt#unit: percent#%"
 6 "#msgctxt#unit: microseconds#us"
}

IDD_DIALOG1 DIALOG 0, 0, 300, 250
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | DS_CENTER | DS_SETFOREGROUND
CAPTION "Scanning"
FONT 8, "MS Shell Dlg"
BEGIN
    LTEXT   "SCANNING... Please Wait",IDC_STATIC,5,19,176,8, SS_CENTER
END

IDD_APPCFG DIALOG  0, 0, 361, 244
STYLE WS_CHILD | WS_DISABLED
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,50,179,50,14
    LTEXT           "Please input property value",IDC_STATIC,10,96,200,8
    COMBOBOX        IDC_COMBO_para,50,31,79,30,CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_SORT | WS_VSCROLL | WS_TABSTOP
    EDITTEXT        IDC_EDIT_value,50,53,79,14,ES_AUTOHSCROLL 
END

IDD_DLLCFG DIALOG  0, 0, 361, 244
STYLE WS_CHILD | WS_DISABLED
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "确定",ID_SET_OK,227,215,50,14
    PUSHBUTTON      "取消",ID_SET_CANCEL,292,215,50,14
    GROUPBOX        "",IDC_STATIC,14,13,151,133
    LTEXT           "扫描模式：",IDC_STATIC,23,27,62,12
    COMBOBOX        IDC_COMBO1,93,27,62,16,CBS_DROPDOWNLIST | WS_VSCROLL
    LTEXT           "分辨率（dpi）：",IDC_STATIC,23,52,62,12
    COMBOBOX        IDC_COMBO2,93,52,62,16,CBS_DROPDOWNLIST | WS_VSCROLL 
    LTEXT           "扫描来源：",IDC_STATIC,23,77,62,12
    COMBOBOX        IDC_COMBO3,93,77,62,16,CBS_DROPDOWNLIST | WS_VSCROLL 
    LTEXT           "图片旋转：",IDC_STATIC,23,102,62,12
    COMBOBOX        IDC_COMBO4,93,102,62,16,CBS_DROPDOWNLIST | WS_VSCROLL 
    LTEXT           "自动翻转：",IDC_STATIC,23,127,62,12
    COMBOBOX        IDC_COMBO5,93,127,62,16,CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED
    LTEXT           "图片格式",IDC_STATIC,23,159,62,12
    COMBOBOX        IDC_COMBO6,93,159,62,16,CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED
    GROUPBOX        "",IDC_STATIC,169,13,173,133
    LTEXT           "亮度：",IDC_STATIC,177,24,62,12
    LTEXT           "对比度：",IDC_STATIC,177,40,62,12
    LTEXT           "纸张大小（宽）：",IDC_STATIC,177,74,84,12
    LTEXT           "纸张大小（高）：",IDC_STATIC,177,90,84,12
    CONTROL         "自动裁切",IDC_CHECK1,"Button",BS_AUTOCHECKBOX | WS_TABSTOP | WS_DISABLED,177,59,46,10
    CONTROL         "去除空白页",IDC_CHECK2,"Button",BS_AUTOCHECKBOX | WS_TABSTOP | WS_DISABLED,177,111,54,10
    LTEXT           "去除空白页阈值设置：",IDC_STATIC,177,127,84,10,WS_DISABLED
    GROUPBOX        "",IDC_STATIC,14,148,151,31
    EDITTEXT        IDC_EDIT1,272,24,62,12,ES_RIGHT | ES_AUTOHSCROLL | WS_DISABLED
    EDITTEXT        IDC_EDIT2,272,40,62,12,ES_RIGHT | ES_AUTOHSCROLL | WS_DISABLED
    EDITTEXT        IDC_EDIT3,272,74,62,12,ES_RIGHT | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED
    EDITTEXT        IDC_EDIT4,272,90,62,12,ES_RIGHT | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED
    EDITTEXT        IDC_EDIT5,272,126,62,12,ES_RIGHT | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED
    GROUPBOX        "",IDC_STATIC,169,148,173,31
    LTEXT           "许可证号：",IDC_STATIC,177,159,39,12,WS_DISABLED
    EDITTEXT        IDC_EDIT6,224,159,110,12,ES_RIGHT | ES_AUTOHSCROLL | WS_DISABLED
END

LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL

IDI_WINECFG ICON s6100dsui.ico


