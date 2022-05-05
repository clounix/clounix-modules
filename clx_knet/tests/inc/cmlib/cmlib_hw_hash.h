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

/* FILE NAME:  cmlib_hw_hash.h
 * PURPOSE:
 *  Provide hardware hash algorithm to each HAL modules for hash access.
 *
 *
 * NOTES:
 *
 *
 */

#ifndef CMLIB_HW_HASH_H
#define CMLIB_HW_HASH_H
#include <clx_error.h>
#include <clx_types.h>

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:   cmlib_hw_hash
 * PURPOSE:
 *      cmlib_hw_hash() is the generic function to calculate out hash index.
 *
 * INPUT:
 *      buk_num       -- The bulk number of this time hash calculation.
 *                       Normally, this buk_num will be table-size/2/4 due
 *                       to hardware design.
 *      ptr_key_blk   -- The key buffer pointer that store 16 bits key array.
 *      blk_num       -- The key buffer array number.
 *      hash_config   -- The hash configuration of this hash table.
 * OUTPUT:
 *      ptr_hasl_val  -- The result of this hash entry index.
 * RETURN:
 *      CLX_E_OK      -- Successfully perform a hash calculation.
 *      CLX_E_OTHERS  -- Fail to complete a hash calculation.
 *
 * NOTES:
 *      The return hash index is not exactly the same as hash table index.
 *      Caller should choose one of buckets to translate it as table address.
 *
 */
CLX_ERROR_NO_T
cmlib_hw_hash(
    const UI32_T buk_num,
    const UI16_T *ptr_key_blk,
    const UI32_T blk_num,
    const UI32_T hash_config,
    UI32_T *ptr_hash_val);

#endif     /* #ifndef CMLIB_HW_HASH_H */

