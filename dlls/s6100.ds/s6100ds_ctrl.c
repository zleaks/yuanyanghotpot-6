/*
 * Copyright 2000 Corel Corporation
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "s6100ds.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_CONTROL/DAT_CAPABILITY/MSG_GET */
TW_UINT16 SANE_CapabilityGet (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT */
TW_UINT16 SANE_CapabilityGetCurrent (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT */
TW_UINT16 SANE_CapabilityGetDefault (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_QUERYSUPPORT */
TW_UINT16 SANE_CapabilityQuerySupport (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_RESET */
TW_UINT16 SANE_CapabilityReset (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_SET */
TW_UINT16 SANE_CapabilitySet (pTW_IDENTITY pOrigin, 
                               TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT */
TW_UINT16 SANE_ProcessEvent (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_NOTDSEVENT;
    pTW_EVENT pEvent = (pTW_EVENT) pData;
    MSG *pMsg = pEvent->pEvent;
    pEvent->TWMessage = MSG_XFERREADY;
    TRACE("DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT  msg 0x%x, wParam 0x%lx\n", pMsg->message, pMsg->wParam);
    if (activeDS.twCC == TWCC_SUCCESS) {
        TRACE("twRC = TWRC_DSEVENT\n");
        twRC = TWRC_DSEVENT;
    }
    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER */
TW_UINT16 SANE_PendingXfersEndXfer (pTW_IDENTITY pOrigin, 
                                     TW_MEMREF pData)
{
    TW_UINT16 ret = -1;
    TW_UINT16 rc = TWRC_SUCCESS;
    TRACE("DG_CONTROL/DAT_PENDINGXFERSENDXFER/MSG_ENDXFER\n");
    ret = s6100_startScan();
    if (ret == 0) {
        rc = TWRC_SUCCESS;
    } else {
        rc = TWRC_FAILURE;
    }
    s6100_endScan();
    return rc;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_SET */
TW_UINT16 SANE_SetupFileXfer2Set(pTW_IDENTITY pOrigin, TW_MEMREF pData) {
    pTW_SETUPFILEXFER pSetupFileXfer = (pTW_SETUPFILEXFER)pData;
    if (pData == 0) {
        return TWRC_FAILURE;
    }
    TRACE("DG_CONTROL/DAT_SETUPFILEXFER/MSG_SET\n");
    strcpy(activeDS.FileXfer.FileName, pSetupFileXfer->FileName);
    activeDS.FileXfer.Format = pSetupFileXfer->Format;
    TRACE("Image Name : %s, Format : %d\n", pSetupFileXfer->FileName, pSetupFileXfer->Format);
    activeDS.twCC = TWRC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_GET */
TW_UINT16 SANE_SetupFileXfer2Get(pTW_IDENTITY pOrigin, TW_MEMREF pData) {
    pTW_SETUPFILEXFER pSetupFileXfer = (pTW_SETUPFILEXFER)pData;
    if (pData == 0) {
        return TWRC_FAILURE;
    }
    TRACE("DG_CONTROL/DAT_SETUPFILEXFER/MSG_GET\n");
    strcpy(pSetupFileXfer->FileName, activeDS.FileXfer.FileName);
    pSetupFileXfer->Format = activeDS.FileXfer.Format;
    activeDS.twCC = TWRC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_STATUS/MSG_GET */
TW_UINT16 SANE_GetDSStatus (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    pTW_STATUS pSourceStatus = (pTW_STATUS) pData;

    TRACE ("DG_CONTROL/DAT_STATUS/MSG_GET\n");
    pSourceStatus->ConditionCode = activeDS.twCC;
    /* Reset the condition code */
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS */
TW_UINT16 SANE_DisableDSUserInterface (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS\n");
    activeDS.currentState = 4;
    twRC = TWRC_SUCCESS;
    activeDS.twCC = TWCC_SUCCESS;

    return twRC;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS */
TW_UINT16 SANE_EnableDSUserInterface (pTW_IDENTITY pOrigin,
                                       TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_USERINTERFACE pUserInterface = (pTW_USERINTERFACE) pData;

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS\n");
	twRC = s6100_EnableDSUserInterface(pUserInterface);
    return twRC;
}

//////////////////////////// He Nan 20200814 //////////////////////////////////////////

TW_UINT16 s6100_EnableDSUserInterface (pTW_USERINTERFACE pUserInterface)
{
	BOOL rc;
    TW_UINT16 twRC = TWRC_SUCCESS;
	if (pUserInterface->ShowUI)
	{
        activeDS.twCC = TWCC_BUMMER;
		rc = s6100_showUI(pUserInterface->hParent);
		TRACE("s6100_showUI:  ret = %d \n", rc);
        if (!rc) {
            twRC = TWRC_FAILURE;
        }
	}
	else
	{
        SANE_Notify(MSG_XFERREADY);
		TRACE("%s:  pUserInterface->ShowUI = %d \n", __func__, pUserInterface->ShowUI);
	}

	return twRC;
}

//////////////////////////////////////////////////////////////////////////////////////
