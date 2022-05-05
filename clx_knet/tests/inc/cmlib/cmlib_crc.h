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

/* FILE NAME:  cmlib_crc.h
 * PURPOSE:
 *  this file is used to provide crc32 calculation operations to other users.
 * NOTES:
 *  it contains operations as below:
 *      1. calculate crc32
 *
 *
 *
 */
#ifndef CMLIB_CRC_H
#define CMLIB_CRC_H

/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME: cmlib_crc32
 * PURPOSE:
 *      it is used to calculate crc32 of a buffer of data.
 * INPUT:
 *      ptr_data    -- buffer start address
 *      byte_len    -- buffer length
 *      ptr_crc     -- input crc base (initialized to 0 when first call)
 * OUTPUT:
 *      ptr_crc     -- the crc result
 * RETURN:
 *      CLX_E_OK            -- calculate success
 *      CLX_E_BAD_PARAMETER -- there is NULL pointer parameter.
 * NOTES:
 *      "123456789" crc is 0xCBF43926
 *      in packet, the crc is filled like: 26 39 f4 cb
 */
CLX_ERROR_NO_T
cmlib_crc32(
    const UI8_T     *ptr_data,
    const UI32_T    byte_len,
    UI32_T          *ptr_crc);

/* FUNCTION NAME: cmlib_crc32_word
 * PURPOSE:
 *      it is used to calculate crc32 of a buffer of data.
 * INPUT:
 *      ptr_data    -- buffer start address
 *      word_len    -- buffer length
 * OUTPUT:
 *      ptr_crc     -- the crc result
 * RETURN:
 *      CLX_E_OK            -- calculate success
 *      CLX_E_BAD_PARAMETER -- there is NULL pointer parameter.
 * NOTES:
 *      Efuse crc check needs this function for word crc calculation
 */
CLX_ERROR_NO_T
cmlib_crc32_word(
    const UI32_T    *ptr_data,
    const UI32_T    word_len,
    UI32_T          *ptr_crc);

#endif /* End of CMLIB_CRC_H */
