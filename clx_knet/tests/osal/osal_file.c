/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Hangzhou Clounix Technology Limited. (C) 2013-2021
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
*  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*******************************************************************************/

/* FILE NAME:  osal_file.c
 * PURPOSE:
 *  Provide an OS abstration layer's file API for different OS
 * NOTES:
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <osal/osal_file.h>

typedef struct OSAL_FILE_CB
{
    FILE            *ptr_file_cb;
} OSAL_FILE_CB_T;

OSAL_FILE *
osal_openFile(
    C8_T            *ptr_filename)
{
    OSAL_FILE_CB_T  *ptr_cb = NULL;

    ptr_cb = (OSAL_FILE_CB_T *)osal_alloc(sizeof(OSAL_FILE_CB_T));
    if (NULL == ptr_cb)
    {
        return NULL;
    }

    ptr_cb->ptr_file_cb = fopen(ptr_filename, "a+");
    if (NULL == ptr_cb->ptr_file_cb)
    {
        osal_free(ptr_cb);
        return NULL;
    }

    return (OSAL_FILE *)ptr_cb;
}

void
osal_closeFile(
    OSAL_FILE       *ptr_file)
{
    OSAL_FILE_CB_T  *ptr_cb = (OSAL_FILE_CB_T *)ptr_file;

    if (NULL == ptr_cb)
    {
        return ;
    }

    fclose(ptr_cb->ptr_file_cb);

    osal_free(ptr_cb);
}

I32_T
osal_writeFile(
    OSAL_FILE       *ptr_file,
    const C8_T      *ptr_buf,
    I32_T           count)
{
    I32_T           write_len = 0;
    OSAL_FILE_CB_T  *ptr_cb = (OSAL_FILE_CB_T *)ptr_file;

    if (NULL == ptr_file || NULL == ptr_buf || 0 >= count)
    {
        return 0;
    }

    write_len = fwrite(ptr_buf, sizeof(I8_T), count, ptr_cb->ptr_file_cb);

    return write_len;
}

I32_T
osal_readFile(
    OSAL_FILE       *ptr_file,
    C8_T            *ptr_buf,
    I32_T           count)
{
    I32_T           read_len = 0;
    OSAL_FILE_CB_T  *ptr_cb = (OSAL_FILE_CB_T *)ptr_file;

    if (NULL == ptr_file || NULL == ptr_buf || 0 >= count)
    {
        return 0;
    }

    read_len = fread(ptr_buf, 1, count, ptr_cb->ptr_file_cb);

    return read_len;
}

CLX_ERROR_NO_T
osal_removeFile(
    C8_T            *ptr_filename)
{
    if (0 == remove(ptr_filename))
    {
        return CLX_E_OK;
    }

    return CLX_E_OTHERS;
}

BOOL_T
osal_isExistedFile(
    C8_T            *ptr_filename)
{
    if (0 == access(ptr_filename, 0))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL_T
osal_renameFile(
    C8_T            *ptr_oldname,
    C8_T            *ptr_newname)
{
    if (0 == rename(ptr_oldname, ptr_newname))
    {
        return TRUE;
    }

    return FALSE;
}

