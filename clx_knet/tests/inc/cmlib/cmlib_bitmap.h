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

/* FILE NAME:  cmlib_bitmap.h
 * PURPOSE:
 *  this file is used to provide independent portlist macro.
 * NOTES:
 *  it contains operations as below:
 *      1. CMLIB_BITMAP_CLR
 *      2. CMLIB_BITMAP_SET
 *      3. CMLIB_BITMAP_ADD
 *      4. CMLIB_BITMAP_DEL
 *      5. CMLIB_BITMAP_AND
 *      6. CMLIB_BITMAP_INV
 *
 */
#ifndef CMLIB_BITMAP_H
#define CMLIB_BITMAP_H
/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>
#include <cmlib/cmlib_util.h>
#include <cmlib/cmlib.h>

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */
#define CMLIB_BITMAP_BIT_ADD(bitmap, bit) (((bitmap)[(bit)/32]) |= (1U << ((bit)%32)))
#define CMLIB_BITMAP_BIT_DEL(bitmap, bit) (((bitmap)[(bit)/32]) &= ~(1U << ((bit)%32)))
#define CMLIB_BITMAP_BIT_CHK(bitmap, bit) ((((bitmap)[(bit)/32] & (1U << ((bit)%32)))) != 0)

#define CMLIB_BITMAP_BIT_FOREACH(bitmap, bit, word) \
        for(bit=0; bit<(word * 32); bit++) \
            if(CMLIB_BITMAP_BIT_CHK(bitmap, bit))

#define CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, bitwise_operator) \
        do {\
            UI32_T i; \
            for(i=0; i<word; i++) \
            { \
                (((bitmap_a)[i]) bitwise_operator ((bitmap_b)[i])); \
            } \
        } while(0)

#define CMLIB_BITMAP_SET(bitmap_a, bitmap_b, word)    CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, =)
#define CMLIB_BITMAP_ADD(bitmap_a, bitmap_b, word)    CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, |=)
#define CMLIB_BITMAP_DEL(bitmap_a, bitmap_b, word)    CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, &= ~)
#define CMLIB_BITMAP_AND(bitmap_a, bitmap_b, word)    CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, &=)
#define CMLIB_BITMAP_INV(bitmap_a, bitmap_b, word)    CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, = ~)
#define CMLIB_BITMAP_OR(bitmap_a, bitmap_b, word)     CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, |=)
#define CMLIB_BITMAP_XOR(bitmap_a, bitmap_b, word)    CMLIB_BITMAP_OPERATION(bitmap_a, bitmap_b, word, ^=)
#define CMLIB_BITMAP_COUNT(bitmap, count, word) \
        do {\
            UI32_T i; \
            count = 0; \
            for(i=0; i<word; i++) \
            { \
                count += cmlib_util_popcount((bitmap)[i]); \
            } \
        } while (0)

#define CMLIB_BITMAP_CLEAR(bitmap, word)              osal_memset(bitmap, 0, word * 4)
#define CMLIB_BITMAP_EMPTY(bitmap, word)              cmlib_bitmap_empty(bitmap, word)
#define CMLIB_BITMAP_EQUAL(bitmap_a, bitmap_b, word)  (!osal_memcmp(bitmap_a, bitmap_b, (word * 4)))

/* DATA TYPE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:   cmlib_bitmap_empty
 * PURPOSE:
 *      cmlib_bitmap_empty() is a function to check if ptr_bitmap is empty.
 *
 * INPUT:
 *      ptr_bitmap   -- The specified bitmap pointer.
 *      word            -- The size of specified bitmap.
 * OUTPUT:
 *      None
 * RETURN:
 *      UI32_T       -- return 1 or 0. 1: empty, 0: not empty.
 *
 * NOTES:
 *      None
 *
 */
UI32_T
cmlib_bitmap_empty(
    const   UI32_T  *ptr_bitmap,
    const   UI32_T   word   );


 /* FUNCTION NAME:    cmlib_bitmap_getFreeIdx
  * PURPOSE:
  *      cmlib_bitmap_getFreeIdx() is a function to check if ptr_bitmap is empty.
  *
  * INPUT:
  *      ptr_bitmap   -- The specified bitmap pointer.
  *      word            -- The size of specified bitmap.
  * OUTPUT:
  *      ptr_idx         -- Found pointer index.
  * RETURN:
  *      CLX_E_OK              -- get the free index success.
  *      CLX_E_ENTRY_NOT_FOUND -- get the free index failed.
  *
  * NOTES:
  *      None
  *
  */
CLX_ERROR_NO_T
cmlib_bitmap_getFreeIdx(
    const UI32_T   *ptr_bitmap,
    const UI32_T   word,
          UI32_T  *ptr_idx);

#endif /* End of CMLIB_BITMAP_H */
