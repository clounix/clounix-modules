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

/* FILE NAME:  osali.h
 * PURPOSE:
 *  osali.h provide an OS abstration layer's API internal defines and macros.
 *
 * NOTES:
 *
 */

#ifndef OSALI_H
#define OSALI_H

#include <clx_error.h>
#include <clx_types.h>

/* if define this macro, the mem statistic will enable */
#define OSAL_EN_MEM_CHK

/* Log */
#define OSAL_DEBUG_LOG_ENABLE               (1)
#define OSAL_WARNING_LOG_ENABLE             (2)

/* Memory */
#define OSAL_MEM_BEGIN_SIGNATURE            (0xDEAD)
#define OSAL_MEM_END_SIGNATURE              (0xBEEF)

#define OSAL_MEM_LARGE_MEM_SIZE             (4 * 1024 * 1024)   /* 4M, alloc from kmalloc or vmalloc */

/* Thread */
#define OSAL_THREAD_NAME_LEN                (16)
#define OSAL_THREAD_DFT_NAME                ("Unknown")
#define OSAL_THREAD_HIGHEST_PRIORITY        (99)

/* Semaphore */
#define OSAL_SEMA_NAME_LEN                  (16)
#define OSAL_SEMA_DFT_NAME                  ("Unknown")
#define OSAL_SEMA_MAX_CNT                   (1)

/* Spinlock */
#define OSAL_SPIN_NAME_LEN                  (16)
#define OSAL_SPIN_DFT_NAME                  ("Unknown")

#define OSAL_PRN_BUF_SZ                     (8192)
#define OSAL_TICKS_PER_SEC                  (1000000)

#define OSAL_IS_INITED()                    (_osal_is_inited)

typedef enum
{
    OSAL_MEM_COOKIE_KMALLOC = 0, /* use kmalloc */
    OSAL_MEM_COOKIE_VMALLOC,     /* use vmalloc */
    OSAL_MEM_COOKIE_MALLOC,      /* use malloc  */
    OSAL_MEM_COOKIE_LAST
} OSAL_MEM_COOKIE_T;

#endif /* end of OSALI_H */

