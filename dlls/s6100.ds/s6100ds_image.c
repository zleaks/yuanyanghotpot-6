/*
 * Copyright 2000 Corel Corporation
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

#include "config.h"

#include <stdarg.h>

#include "s6100ds.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_IMAGE/DAT_IMAGEINFO/MSG_GET */
TW_UINT16 SANE_ImageInfoGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    return TWRC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GET */
TW_UINT16 SANE_ImageLayoutGet (pTW_IDENTITY pOrigin,
                                TW_MEMREF pData)
{
    return TWCC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GETDEFAULT */
TW_UINT16 SANE_ImageLayoutGetDefault (pTW_IDENTITY pOrigin, 
                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");
    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_RESET */
TW_UINT16 SANE_ImageLayoutReset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_SET */
TW_UINT16 SANE_ImageLayoutSet (pTW_IDENTITY pOrigin,
                                TW_MEMREF pData)
{
    return TWRC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET */
TW_UINT16 SANE_ImageMemXferGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    return TWRC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGEFILEXFER/MSG_GET */
TW_UINT16 SANE_ImageFileXferGet(pTW_IDENTITY pOrigin, TW_MEMREF pData) {
    TRACE("DG_IMAGE/DAT_IMAGEFILEXFER/MSG_GET\n");
    s6100_scanimage();
    return TWRC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET */
TW_UINT16 SANE_ImageNativeXferGet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    TRACE("DG_IMAGE/DAT_ImageNativeXfer/MSG_GET\n");
    s6100_scanimage();
    return TWRC_SUCCESS;
}
