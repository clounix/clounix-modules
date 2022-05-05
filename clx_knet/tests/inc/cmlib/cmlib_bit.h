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

/* FILE NAME:  cmlib_bit.h
 * PURPOSE:
 *  this file is used to provide bitwise operations to other users.
 * NOTES:
 *  it contains operations as below:
 *      1. and
 *      2. or
 *      3. xor
 *      4. compare
 *      5. invert
 *      6. rotate
 *      7. reverse
 *      8. get index of set-bit
 *      9. check bit
 *      10. set
 *      11. clear
 *
 *
 *
 */
#ifndef CMLIB_BIT_H
#define CMLIB_BIT_H
/* INCLUDE FILE DECLARATIONS
 */

#include <cmlib/cmlib.h>

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */

/* FUNCTION NAME: _CMLIB_BITN_OP
 * PURPOSE:
 *      this macro is used to do bit operation for bit-8/16/32 operands.
 * INPUT:
 *      data  -- the operand
 *      op    -- operation
 *      bit_n -- the operation bit no, 0~data bit width.
 * OUTPUT:
 *      None.
 * RETURN:
 *      the operation result.
 * NOTES:
 *
 */
#define _CMLIB_BITN_OP(data, op, bit_n) \
    ((data) op (1U << (bit_n)))

/* FUNCTION NAME: _CMLIB_BIT64_OP
 * PURPOSE:
 *      this macro is used to do bit operation for bit-64 operands.
 * INPUT:
 *      data  -- the operand
 *      op    -- operation
 *      bit_n -- the operation bit no, 0~63.
 * OUTPUT:
 *      None.
 * RETURN:
 *      the operation result.
 * NOTES:
 *
 */
#define _CMLIB_BIT64_OP(data, op, bit_n) \
    ((data) op (1ULL << (bit_n)))


/* FUNCTION NAME: CMLIB_BIT_AND
 * PURPOSE:
 *      "and" two data of 8/16/32-bit and save the result in dest of 8/16/32-bit
 * INPUT:
 *      data1  -- the first operand of 8/16/32-bit
 *      data2  -- the second operand of 8/16/32-bit
 * OUTPUT:
 *      dest   -- the result will be saved in this data of 8/16/32-bit
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_AND(data1, data2, dest) \
    ((dest) = (data1) & (data2))

/* FUNCTION NAME: CMLIB_BIT_OR
 * PURPOSE:
 *      "or" two data of 8/16/32-bit and save the result in dest of 8/16/32-bit
 * INPUT:
 *      data1  -- the first operand of 8/16/32-bit
 *      data2  -- the second operand of 8/16/32-bit
 * OUTPUT:
 *      dest   -- the result will be saved in this data of 8/16/32-bit
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_OR(data1, data2, dest) \
    ((dest) = (data1) | (data2))

/* FUNCTION NAME: CMLIB_BIT_XOR
 * PURPOSE:
 *      "xor" two data of 8/16/32-bit and save the result in dest of 8/16/32-bit
 * INPUT:
 *      data1  -- the first operand of 8/16/32-bit
 *      data2  -- the second operand of 8/16/32-bit
 * OUTPUT:
 *      dest   -- the result will be saved in this data of 8/16/32-bit
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_XOR(data1, data2, dest) \
    ((dest) = (data1) ^ (data2))

/* FUNCTION NAME: CMLIB_BIT_CMP
 * PURPOSE:
 *      compare two data of 8/16/32-bit if they are the same and save the
 *      result in dest of 32-bit
 * INPUT:
 *      data1  -- the first operand of 8/16/32-bit
 *      data2  -- the second operand of 8/16/32-bit
 * OUTPUT:
 *      dest   -- the result will be saved in this data of 32-bit
 *                  0  : two operands are the same
 *                  1  : two operands are not the same.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_CMP(data1, data2, dest) \
    ((dest) = ((data1) - (data2)) ? 1 : 0)

/* FUNCTION NAME: CMLIB_BIT64_CMP
 * PURPOSE:
 *      compare two data of 64-bit if they are the same and save the
 *      result in dest of 32-bit
 * INPUT:
 *      data1  -- the first operand of 64-bit
 *      data2  -- the second operand of 64-bit
 * OUTPUT:
 *      dest   -- the result will be saved in this data of 32-bit
 *                  0  : two operands are the same
 *                  1  : two operands are not the same.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define CMLIB_BIT64_CMP(data1, data2, dest) \
    ((dest) = ((data1) - (data2)) ? 1 : 0)
#else
#define CMLIB_BIT64_CMP(data1, data2, dest)                               \
    do                                                                    \
    {                                                                     \
        (dest) = (data1).ui64[UI64_LSW] - (data2).ui64[UI64_LSW];         \
        (dest) |= (data1).ui64[UI64_MSW] - (data2).ui64[UI64_MSW];        \
        (dest) = (dest) ? 1 : 0;                                          \
    }while(0)
#endif

/* shift */


/* FUNCTION NAME: CMLIB_BIT8_SHF_L
 * PURPOSE:
 *      it is used to shift left data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 8-bit
 *      digit     -- shift digit, it is 0~7
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 8-bit.
 */
#define CMLIB_BIT8_SHF_L(data, digit, dest) \
    (dest) = (data) << ((digit)& 7)

/* FUNCTION NAME: CMLIB_BIT16_SHF_L
 * PURPOSE:
 *      it is used to shift left data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 16-bit
 *      digit     -- shift digit, it is 0~15
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 16-bit.
 */
#define CMLIB_BIT16_SHF_L(data, digit, dest) \
    (dest) = (data) << ((digit)& 15)

/* FUNCTION NAME: CMLIB_BIT32_SHF_L
 * PURPOSE:
 *      it is used to shift left data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 32-bit
 *      digit     -- shift digit, it is 0~31
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 32-bit.
 */
#define CMLIB_BIT32_SHF_L(data, digit, dest) \
    (dest) = (data) << ((digit)& 31)


/* FUNCTION NAME: CMLIB_BIT64_SHF_L
 * PURPOSE:
 *      it is used to shift left data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 64-bit
 *      digit     -- shift digit, it is 0~63
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 64-bit.
 */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define CMLIB_BIT64_SHF_L(data, digit, dest) \
    (dest) = (data) << ((digit)& 63)
#else
#define CMLIB_BIT64_SHF_L(data, digit, dest)                              \
    do                                                                    \
    {                                                                     \
        UI32_T _digit = (digit) & 63;                                     \
        if(_digit > 32)                                                   \
        {                                                                 \
            _digit &= 31;                                                 \
            (dest).ui64[UI64_MSW] = ((data).ui64[UI64_LSW] << _digit);    \
            (dest).ui64[UI64_LSW] = 0;                                    \
        }                                                                 \
        else if(_digit == 32)                                             \
        {                                                                 \
            (dest).ui64[UI64_MSW] = (data).ui64[UI64_LSW];                \
            (dest).ui64[UI64_LSW] = 0;                                    \
        }                                                                 \
        else if(0 != _digit)                                              \
        {                                                                 \
            (dest).ui64[UI64_MSW] = ((data).ui64[UI64_MSW] << _digit)     \
                                | ((data).ui64[UI64_LSW] >> (32-_digit)); \
            (dest).ui64[UI64_LSW] = ((data).ui64[UI64_LSW] << _digit);    \
        }                                                                 \
        else                                                              \
        {                                                                 \
            (dest).ui64[UI64_LSW] = (data).ui64[UI64_LSW];                \
            (dest).ui64[UI64_MSW] = (data).ui64[UI64_MSW];                \
        }                                                                 \
    }while(0)
#endif

/* FUNCTION NAME: CMLIB_BIT8_SHF_R
 * PURPOSE:
 *      it is used to shift right data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 8-bit
 *      digit     -- shift digit, it is 0~7
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 8-bit.
 */
#define CMLIB_BIT8_SHF_R(data, digit, dest) \
    (dest) = (data) >> ((digit)& 7)

/* FUNCTION NAME: CMLIB_BIT16_SHF_R
 * PURPOSE:
 *      it is used to shift right data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 16-bit
 *      digit     -- shift digit, it is 0~15
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 16-bit.
 */
#define CMLIB_BIT16_SHF_R(data, digit, dest) \
    (dest) = (data) >> ((digit)& 15)

/* FUNCTION NAME: CMLIB_BIT32_SHF_R
 * PURPOSE:
 *      it is used to shift right data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 32-bit
 *      digit     -- shift digit, it is 0~31
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 32-bit.
 */
#define CMLIB_BIT32_SHF_R(data, digit, dest) \
    (dest) = (data) >> ((digit)& 31)

/* FUNCTION NAME: CMLIB_BIT64_SHF_R
 * PURPOSE:
 *      it is used to shift right data with digit and save the result in dest.
 * INPUT:
 *      data      -- the data of 64-bit
 *      digit     -- shift digit, it is 0~63
 * OUTPUT:
 *      dest      -- the shift result
 * RETURN:
 *      None.
 * NOTES:
 *      the operand should be unsigned 64-bit.
 */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define CMLIB_BIT64_SHF_R(data, digit, dest) \
    (dest) = (data) >> ((digit)& 63)
#else
#define CMLIB_BIT64_SHF_R(data, digit, dest)                              \
    do                                                                    \
    {                                                                     \
        UI32_T _digit = (digit) & 63;                                     \
        if(_digit > 32)                                                   \
        {                                                                 \
            _digit &= 31;                                                 \
            (dest).ui64[UI64_LSW] = ((data).ui64[UI64_MSW] >> _digit);    \
            (dest).ui64[UI64_MSW] = 0;                                    \
        }                                                                 \
        else if(_digit == 32)                                             \
        {                                                                 \
            (dest).ui64[UI64_LSW] = (data).ui64[UI64_MSW];                \
            (dest).ui64[UI64_MSW] = 0;                                    \
        }                                                                 \
        else if(0 != _digit)                                              \
        {                                                                 \
            (dest).ui64[UI64_LSW] = ((data).ui64[UI64_LSW] >> _digit)     \
                                | ((data).ui64[UI64_MSW] << (32-_digit)); \
            (dest).ui64[UI64_MSW] = ((data).ui64[UI64_MSW] >> _digit);    \
        }                                                                 \
        else                                                              \
        {                                                                 \
            (dest).ui64[UI64_LSW] = (data).ui64[UI64_LSW];                \
            (dest).ui64[UI64_MSW] = (data).ui64[UI64_MSW];                \
        }                                                                 \
    }while(0)
#endif



/* rotate */

#define _CMLIB_BIT_N_ROT_L(data, digit, bit_width, dest)       \
    ((dest) = (((data) << ((digit) & ((bit_width) - 1))) |     \
    ((data) >> (((bit_width) - ((digit) & ((bit_width) - 1)))  \
    & ((bit_width) - 1)))))


#define _CMLIB_BIT_N_ROT_R(data, digit, bit_width, dest)       \
    ((dest) = (((data) >> ((digit) & ((bit_width) - 1))) |     \
    ((data) << (((bit_width) - ((digit) & ((bit_width) - 1)))  \
    & ((bit_width) - 1)))))



/* FUNCTION NAME: CMLIB_BIT8_ROT_L
 * PURPOSE:
 *      rotate left a data of 8-bit.
 * INPUT:
 *      data      -- the data of 8-bit
 *      digit     -- rotate digit, it is 0~7
 * OUTPUT:
 *      dest      -- the rotate result
 * RETURN:
 *      the rotated data of 8-bit
 * NOTES:
 *      the operand should be unsigned 8-bit.
 */
#define CMLIB_BIT8_ROT_L(data, digit, dest)  _CMLIB_BIT_N_ROT_L(data, digit, 8, dest)

/* FUNCTION NAME: CMLIB_BIT16_ROT_L
 * PURPOSE:
 *      rotate left a data of 16-bit.
 * INPUT:
 *      data      -- the data of 16-bit
 *      digit     -- rotate digit, it is 0~15
 * OUTPUT:
 *      dest      -- the rotate result
 * RETURN:
 *      the rotated data of 16-bit
 * NOTES:
 *      the operand should be unsigned 16-bit.
 */
#define CMLIB_BIT16_ROT_L(data, digit, dest) _CMLIB_BIT_N_ROT_L(data, digit, 16, dest)

/* FUNCTION NAME: CMLIB_BIT32_ROT_L
 * PURPOSE:
 *      rotate left a data of 32-bit.
 * INPUT:
 *      data      -- the data of 32-bit
 *      digit     -- rotate digit, it is 0~31
 * OUTPUT:
 *      dest      -- the rotate result
 * RETURN:
 *      the rotated data of 32-bit
 * NOTES:
 *      the operand should be unsigned 32-bit.
 */
#define CMLIB_BIT32_ROT_L(data, digit, dest) _CMLIB_BIT_N_ROT_L(data, digit, 32, dest)

/* FUNCTION NAME: CMLIB_BIT8_ROT_R
 * PURPOSE:
 *      rotate right a data of 8-bit.
 * INPUT:
 *      data      -- the data of 8-bit
 *      digit     -- rotate digit, it is 0~7
 * OUTPUT:
 *      dest      -- the rotate result
 * RETURN:
 *      the rotated data of 8-bit
 * NOTES:
 *
 */
#define CMLIB_BIT8_ROT_R(data, digit, dest)  _CMLIB_BIT_N_ROT_R(data, digit, 8, dest)

/* FUNCTION NAME: CMLIB_BIT16_ROT_R
 * PURPOSE:
 *      rotate right a data of 16-bit.
 * INPUT:
 *      data      -- the data of 16-bit
 *      digit     -- rotate digit, it is 0~15
 * OUTPUT:
 *      dest      -- the rotate result
 * RETURN:
 *      the rotated data of 16-bit
 * NOTES:
 *
 */
#define CMLIB_BIT16_ROT_R(data, digit, dest) _CMLIB_BIT_N_ROT_R(data, digit, 16, dest)

/* FUNCTION NAME: CMLIB_BIT32_ROT_R
 * PURPOSE:
 *      rotate right a data of 32-bit.
 * INPUT:
 *      data      -- the data of 32-bit
 *      digit     -- rotate digit, it is 0~31
 * OUTPUT:
 *      dest      -- the rotate result
 * RETURN:
 *      the rotated data of 32-bit
 * NOTES:
 *
 */
#define CMLIB_BIT32_ROT_R(data, digit, dest) _CMLIB_BIT_N_ROT_R(data, digit, 32, dest)

/* FUNCTION NAME: CMLIB_BIT_INVERT
 * PURPOSE:
 *      invert a data of 8/16/32-bit and save the result in dest of 8/16/32-bit
 * INPUT:
 *      data  -- the data of 8/16/32-bit
 * OUTPUT:
 *      dest  -- the result will be saved in the dest data of 8/16/32-bit
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_INVERT(data, dest) ((dest) = ~(data))

/* FUNCTION NAME: CMLIB_BIT_CHECK
 * PURPOSE:
 *      check the (bit_n)th bit if is set.
 * INPUT:
 *      data  -- 8/16/32-bit data
 *      bit_n -- the (bit_n)th bit will be check, 0~7/15/31
 * OUTPUT:
 *      None.
 * RETURN:
 *      0 -- the (bit_n)th bit is 0
 *      1 -- the (bit_n)th bit is 1
 * NOTES:
 *
 */
#define CMLIB_BIT_CHECK(data, bit_n) \
    (_CMLIB_BITN_OP(data, &, bit_n) ? 1 : 0)

/* FUNCTION NAME: CMLIB_BIT_SET
 * PURPOSE:
 *      set the (bit_n)th bit to 1
 * INPUT:
 *      data  -- 8/16/32-bit data
 *      bit_n -- the (bit_n)th bit will be set, 0~7/15/31
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_SET(data, bit_n) \
    _CMLIB_BITN_OP(data, |=, bit_n)

/* FUNCTION NAME: CMLIB_BIT_CLR
 * PURPOSE:
 *      set the (bit_n)th bit to 0
 * INPUT:
 *      data  -- 8/16/32-bit data
 *      bit_n -- the (bit_n)th bit will be set, 0~7/15/31
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#define CMLIB_BIT_CLR(data, bit_n) \
    _CMLIB_BITN_OP(data, &=~, bit_n)

/* FUNCTION NAME: CMLIB_BIT64_CHECK
 * PURPOSE:
 *      check the (bit_n)th bit if is set.
 * INPUT:
 *      data  -- 64-bit data
 *      bit_n -- the (bit_n)th bit will be check, 0~63
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define CMLIB_BIT64_CHECK(data, bit_n) \
    (_CMLIB_BIT64_OP(data, &, bit_n) ? 1 : 0)
#else
#define CMLIB_BIT64_CHECK(data, bit_n) \
    (_CMLIB_BITN_OP((data).ui64[(((bit_n)/32)==0)?UI64_LSW:UI64_MSW], &, (bit_n)%32) ? 1 : 0)
#endif

/* FUNCTION NAME: CMLIB_BIT64_SET
 * PURPOSE:
 *      set the (bit_n)th bit to 1
 * INPUT:
 *      data  -- 64-bit data
 *      bit_n -- the (bit_n)th bit will be set, 0~63
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define CMLIB_BIT64_SET(data, bit_n) \
    _CMLIB_BIT64_OP(data, |=, bit_n)
#else
#define CMLIB_BIT64_SET(data, bit_n) \
    _CMLIB_BITN_OP((data).ui64[(((bit_n)/32)==0)?UI64_LSW:UI64_MSW], |=, (bit_n)%32)
#endif

/* FUNCTION NAME: CMLIB_BIT64_CLR
 * PURPOSE:
 *      set the (bit_n)th bit to 0
 * INPUT:
 *      data  -- 64-bit data
 *      bit_n -- the (bit_n)th bit will be set, 0~63
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define CMLIB_BIT64_CLR(data, bit_n) \
    _CMLIB_BIT64_OP(data, &=~, bit_n)
#else
#define CMLIB_BIT64_CLR(data, bit_n) \
    _CMLIB_BITN_OP((data).ui64[(((bit_n)/32)==0)?UI64_LSW:UI64_MSW], &=~, (bit_n)%32)
#endif


/* FUNCTION NAME: CMLIB_BIT_FOREACH_SETBIT
 * PURPOSE:
 *      get each set-bit index of 8/16/32-bitdata. the result will be return in
 *      ptr_index.
 * INPUT:
 *      data      -- 8/16/32-bit data to get set-bit index.
 * OUTPUT:
 *      ptr_index -- I32_T pointer, the index of set-bit. 0~31.
 * RETURN:
 *      None.
 * NOTES:
 *      This macro will not check ptr_index if it null pointer.
 * please make sure ptr_index is not null pointer.
 *      The ptr_index should be a I32_T pointer, please notice.
 *      eg:
 *          UI32_T data = 0x7;
 *          I32_T i = 0;
 *          CMLIB_BIT_FOREACH_SETBIT(data, &i)
 *          {
 *              osal_printf("i=%d\n", i);
 *          }
 *      output:
 *          i=0
 *          i=1
 *          i=2
 */
#define CMLIB_BIT_FOREACH_SETBIT(data, ptr_index)                            \
    *(ptr_index) = -1;                                                       \
    while(CLX_E_OK==cmlib_bit_getIdxOfSetBit((UI32_T)(data), (ptr_index)))

/* FUNCTION NAME: CMLIB_BIT64_FOREACH_SETBIT
 * PURPOSE:
 *      get each set-bit index of 64-bit data. the result will be return in
 *      ptr_index.
 * INPUT:
 *      data      -- 64-bit data to get set-bit index.
 * OUTPUT:
 *      ptr_index -- I32_T pointer, the index of set-bit. 0~63.
 * RETURN:
 *      None.
 * NOTES:
 *      This macro will not check ptr_index if it null pointer.
 * please make sure ptr_index is not null pointer.
 *      The ptr_index should be a I32_T pointer, please notice.
 *      eg:
 *          UI64_T data = 0x7;
 *          I32_T i = 0;
 *          CMLIB_BIT64_FOREACH_SETBIT(data, &i)
 *          {
 *              osal_printf("i=%d\n", i);
 *          }
 *      output:
 *          i=0
 *          i=1
 *          i=2
 */
#define CMLIB_BIT64_FOREACH_SETBIT(data, ptr_index)                          \
    *(ptr_index) = -1;                                                       \
    while(CLX_E_OK==cmlib_bit64_getIdxOfSetBit((UI64_T)(data), (ptr_index)))

/* FUNCTION NAME: CMLIB_BIT8_ARRAY_FOREACH_SETBIT
 * PURPOSE:
 *      get each set-bit index of 8-bit data array. the result will be return in
 *      ptr_index.
 * INPUT:
 *      ptr_data  -- 8-bit data array start address
 *      size      -- array size
 * OUTPUT:
 *      ptr_index -- I32_T pointer, the index of set-bit.
 * RETURN:
 *      None.
 * NOTES:
 *      This macro will not check ptr_index if it null pointer.
 * please make sure ptr_index is not null pointer.
 *      The ptr_index should be a I32_T pointer, please notice.
 *      eg:
 *          UI8_T data[2] = {0x7, 0x7};
 *          I32_T i = 0;
 *          CMLIB_BIT8_ARRAY_FOREACH_SETBIT(data, 2, &i)
 *          {
 *              osal_printf("i=%d\n", i);
 *          }
 *      output:
 *          i=0
 *          i=1
 *          i=2
 *          i=8
 *          i=9
 *          i=10
 */
#define CMLIB_BIT8_ARRAY_FOREACH_SETBIT(ptr_data, size, ptr_index)                  \
    *(ptr_index) = -1;                                                              \
    while(CLX_E_OK==cmlib_bit8_getIdxOfSetBitOfArray((ptr_data), (size), (ptr_index)))

/* FUNCTION NAME: CMLIB_BIT32_UI32_BYTE0
 * PURPOSE:
 *      get the first byte from UI32 data
 * INPUT:
 *      ui32_data  -- ui32 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the first LSB byte.
 * NOTES:
 *
 */
#define CMLIB_BIT32_UI32_BYTE0(ui32_data)          ((ui32_data) & 0x000000FF)

/* FUNCTION NAME: CMLIB_BIT32_UI32_BYTE1
 * PURPOSE:
 *      get the second byte from UI32 data
 * INPUT:
 *      ui32_data  -- ui32 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the second LSB byte.
 * NOTES:
 *
 */
#define CMLIB_BIT32_UI32_BYTE1(ui32_data)          (((ui32_data) >> 8) & 0x000000FF)

/* FUNCTION NAME: CMLIB_BIT32_UI32_BYTE2
 * PURPOSE:
 *      get the third byte from UI32 data
 * INPUT:
 *      ui32_data  -- ui32 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the third LSB byte.
 * NOTES:
 *
 */
#define CMLIB_BIT32_UI32_BYTE2(ui32_data)          (((ui32_data) >> 16) & 0x000000FF)

/* FUNCTION NAME: CMLIB_BIT32_UI32_BYTE3
 * PURPOSE:
 *      get the fourth byte from UI32 data
 * INPUT:
 *      ui32_data  -- ui32 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the fourth LSB byte.
 * NOTES:
 *
 */
#define CMLIB_BIT32_UI32_BYTE3(ui32_data)          (((ui32_data) >> 24) & 0x000000FF)

/* FUNCTION NAME: CMLIB_BIT32_UI32_LOW
 * PURPOSE:
 *      get the lower two bytes from UI32 data
 * INPUT:
 *      ui32_data  -- ui32 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the two LSB bytes.
 * NOTES:
 *
 */
#define CMLIB_BIT32_UI32_LOW(ui32_data)            ((ui32_data) & 0x0000FFFF)

/* FUNCTION NAME: CMLIB_BIT32_UI32_HIGH
 * PURPOSE:
 *      get the higher two bytes from UI32 data
 * INPUT:
 *      ui32_data  -- ui32 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the two MSB bytes.
 * NOTES:
 *
 */
#define CMLIB_BIT32_UI32_HIGH(ui32_data)           (((ui32_data) >> 16) & 0x0000FFFF)

/* FUNCTION NAME: CMLIB_BIT16_UI16_LOW
 * PURPOSE:
 *      get the lower byte from UI16 data
 * INPUT:
 *      ui16_data  -- ui16 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the LSB byte.
 * NOTES:
 *
 */
#define CMLIB_BIT16_UI16_LOW(ui16_data)            ((ui16_data) & 0x00FF)

/* FUNCTION NAME: CMLIB_BIT16_UI16_HIGH
 * PURPOSE:
 *      get the higher byte from UI16 data
 * INPUT:
 *      ui16_data  -- ui16 data
 * OUTPUT:
 *      None.
 * RETURN:
 *      the MSB byte.
 * NOTES:
 *
 */
#define CMLIB_BIT16_UI16_HIGH(ui16_data)           (((ui16_data) >> 8) & 0x00FF)


/* DATA TYPE DECLARATIONS
 */
/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */


/* FUNCTION NAME: cmlib_bit_getIdxOfSetBit
 * PURPOSE:
 *      get the first set-bit index of an 8-bit data starting from
 *      (*ptr_index + 1) bit(include) to MSB.
 * INPUT:
 *      data      -- the data of 8/16/32-bit.
 *      ptr_index -- the previous bit postion of start check bit. -1~30.
 * OUTPUT:
 *      ptr_index -- the index of first set-bit. 0~31.
 * RETURN:
 *      CLX_E_OK              -- check success
 *      CLX_E_BAD_PARAMETER   -- the parameter is invalid
 *      CLX_E_ENTRY_NOT_FOUND -- there is not set-bit from start_bit to MSB.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit_getIdxOfSetBit(
    UI32_T data,
    I32_T  *ptr_index );


/* FUNCTION NAME: cmlib_bit64_getIdxOfSetBit
 * PURPOSE:
 *      get the first set-bit index of 64-bit data starting from
 *      (*ptr_index + 1) bit(include) to MSB.
 * INPUT:
 *      data      -- the data of 64-bit.
 *      start_bit -- the start bit of the data, -1~62
 * OUTPUT:
 *      ptr_index -- the index of first set-bit. 0~63.
 * RETURN:
 *      CLX_E_OK              -- check success
 *      CLX_E_BAD_PARAMETER   -- the parameter is invalid
 *      CLX_E_ENTRY_NOT_FOUND -- not set-bit from (*ptr_index + 1) bit to MSB.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit64_getIdxOfSetBit(
    UI64_T data,
    I32_T  *ptr_index );

/* FUNCTION NAME: cmlib_bit8_andOfArray
 * PURPOSE:
 *      "and" two 8-bit arrays and save the result in dest array.
 * INPUT:
 *      ptr_data1 -- the first array of 8-bit
 *      ptr_data2 -- the second array of 8-bit
 *      length    -- array lenght
 * OUTPUT:
 *      ptr_dest  -- the dest array which is used to save the "and" result
 * RETURN:
 *      CLX_E_OK             -- check success
 *      CLX_E_BAD_PARAMETER  -- the parameter is invalid
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_andOfArray(
    UI8_T  *ptr_data1,
    UI8_T  *ptr_data2,
    UI32_T length,
    UI8_T  *ptr_dest );

/* FUNCTION NAME: cmlib_bit8_orOfArray
 * PURPOSE:
 *      "or" two 8-bit arrays and save the result in dest array.
 * INPUT:
 *      ptr_data1 -- the first array of 8-bit
 *      ptr_data2 -- the second array of 8-bit
 *      length    -- array lenght
 * OUTPUT:
 *      ptr_dest  -- the dest array which is used to save the "or" result
 * RETURN:
 *      CLX_E_OK             -- check success
 *      CLX_E_BAD_PARAMETER  -- the parameter is invalid
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_orOfArray(
    UI8_T  *ptr_data1,
    UI8_T  *ptr_data2,
    UI32_T length,
    UI8_T  *ptr_dest );

/* FUNCTION NAME: cmlib_bit8_xorOfArray
 * PURPOSE:
 *      "xor" two 8-bit arrays and save the result in dest array.
 * INPUT:
 *      ptr_data1 -- the first array of 8-bit
 *      ptr_data2 -- the second array of 8-bit
 *      length    -- array lenght
 * OUTPUT:
 *      ptr_dest  -- the dest array which is used to save the "xor" result
 * RETURN:
 *      CLX_E_OK             -- check success
 *      CLX_E_BAD_PARAMETER  -- the parameter is invalid
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_xorOfArray(
    UI8_T  *ptr_data1,
    UI8_T  *ptr_data2,
    UI32_T length,
    UI8_T  *ptr_dest );

/* FUNCTION NAME: cmlib_bit8_cmpOfArray
 * PURPOSE:
 *      cmpare two 8-bit arrays if they are the same and save the result in
 *      dest.
 * INPUT:
 *      ptr_data1 -- the first array of 8-bit
 *      ptr_data2 -- the second array of 8-bit
 *      length    -- array length
 * OUTPUT:
 *      ptr_cmp_ret  -- the compare result
 *                      0: two arrays of 8-bit are the same
 *                      1: two arrays of 8-bit are not the same
 * RETURN:
 *      CLX_E_OK             -- check success
 *      CLX_E_BAD_PARAMETER  -- the parameter is invalid
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_cmpOfArray(
    UI8_T  *ptr_data1,
    UI8_T  *ptr_data2,
    UI32_T length,
    I32_T  *ptr_cmp_ret );

/* FUNCTION NAME: cmlib_bit8_invertOfArray
 * PURPOSE:
 *      invert an array of 8-bit and save the result in dest array.
 * INPUT:
 *      ptr_data -- the array of 8-bit
 *      length   -- array lenght
 * OUTPUT:
 *      ptr_dest -- the dest array is used to save inverted data
 * RETURN:
 *      CLX_E_OK             -- check success
 *      CLX_E_BAD_PARAMETER  -- the parameter is invalid
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_invertOfArray(
    UI8_T  *ptr_data,
    UI32_T length,
    UI8_T  *ptr_dest );

/* FUNCTION NAME: cmlib_bit8_getIdxOfSetBitOfArray
 * PURPOSE:
 *      get the first set-bit index of an array of 8-bit starting from start_bit
 * INPUT:
 *      ptr_data  -- the 8-bit array
 *      length    -- array length
 *      ptr_index -- the previous bit postion of start check bit,
 *                      (0~(length*8-1))-1
 * OUTPUT:
 *      ptr_index -- the index of first set-bit.
 *                      >=0: the index
 * RETURN:
 *      CLX_E_OK              -- check success
 *      CLX_E_BAD_PARAMETER   -- the parameter is invalid
 *      CLX_E_ENTRY_NOT_FOUND -- no set-bit from start bit to the last bit.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_getIdxOfSetBitOfArray(
    const UI8_T  *ptr_data,
    UI32_T       length,
    I32_T        *ptr_index );


/* FUNCTION NAME: cmlib_bit8_checkBitOfArray
 * PURPOSE:
 *      check one bit of an array of 8-bit if it is set
 * INPUT:
 *      ptr_data  -- the array of 8-bit
 *      length    -- the array length
 *      bit_n     -- the checked bit, 0~(length*8-1)
 * OUTPUT:
 *      ptr_ret   -- the check result
 *                   0: the bit is not set
 *                   1: the bit is set
 * RETURN:
 *      CLX_E_OK             -- check success
 *      CLX_E_BAD_PARAMETER  -- the parameter is invalid
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_checkBitOfArray(
    UI8_T  *ptr_data,
    UI32_T length,
    UI32_T bit_n,
    I32_T  *ptr_ret );


/* FUNCTION NAME: cmlib_bit8_setRangeOfArray
 * PURPOSE:
 *      set a range of bit of an array of 8-bit to 1
 * INPUT:
 *      ptr_data  -- the array of 8-bit
 *      length    -- the array length
 *      start_bit -- the start bit of the array to set, 0~(length*8-1)
 *      bit_cnt   -- the count of bit will be set, start_bit+bit_cnt <= length*8
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- check success
 *      CLX_E_BAD_PARAMETER -- ptr_data is NULL or length == 0 or start_bit
 *                             >= length*8-1 or bit_cnt == 0 or start_bit+bit_cnt
 *                             >= length*8
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_setRangeOfArray(
    UI8_T  *ptr_data,
    UI32_T length,
    UI32_T start_bit,
    UI32_T bit_cnt );


/* FUNCTION NAME: cmlib_bit8_clrRangeOfArray
 * PURPOSE:
 *      set a range of bit of an array of 8-bit to 0
 * INPUT:
 *      ptr_data  -- the array of 8-bit
 *      length    -- the array length
 *      start_bit -- the start bit of the array to clear, 0~(length*8-1)
 *      bit_cnt   -- the count of bit will be cleared,start_bit+bit_cnt<=ength*8
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- check success
 *      CLX_E_BAD_PARAMETER -- ptr_data is NULL or length == 0 or start_bit
 *                             >=length*8-1 or bit_cnt == 0 or start_bit+bit_cnt
 *                             >=length*8
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit8_clrRangeOfArray(
    UI8_T  *ptr_data,
    UI32_T length,
    UI32_T start_bit,
    UI32_T bit_cnt );


/* FUNCTION NAME: cmlib_bit8_rev
 * PURPOSE:
 *     it is used to reverse bit8 operand
 * INPUT:
 *     bit8_data  -- the data will be reverse
 * OUTPUT:
 *     None.
 * RETURN:
 *     0-0xFF -- the reverse data.
 * NOTES:
 *
 */
UI8_T
cmlib_bit8_rev(
    UI8_T bit8_data );

/* FUNCTION NAME: cmlib_bit16_rev
 * PURPOSE:
 *     it is used to reverse bit16 operand
 * INPUT:
 *     bit16_data  -- the data will be reverse
 * OUTPUT:
 *     None.
 * RETURN:
 *     0-0xFFFF -- the reverse data.
 * NOTES:
 *
 */
UI16_T
cmlib_bit16_rev(
    UI16_T bit16_data );

/* FUNCTION NAME: cmlib_bit32_rev
 * PURPOSE:
 *     it is used to reverse bit32 operand
 * INPUT:
 *     bit32_data  -- the data will be reverse
 * OUTPUT:
 *     None.
 * RETURN:
 *     0-0xFFFFFFFF -- the reverse data.
 * NOTES:
 *
 */
UI32_T
cmlib_bit32_rev(
    UI32_T bit32_data );

/* FUNCTION NAME: cmlib_bit64_rev
 * PURPOSE:
 *     it is used to reverse bit64 operand
 * INPUT:
 *     bit64_data  -- the data will be reverse
 * OUTPUT:
 *     None.
 * RETURN:
 *     0-0xFFFFFFFFFFFFFFFF -- the reverse data.
 * NOTES:
 *
 */
UI64_T
cmlib_bit64_rev(
    UI64_T bit64_data );

/* FUNCTION NAME: cmlib_bit64_divUi32
 * PURPOSE:
 *     it is used to do 64 bits division by 32 bits.
 * INPUT:
 *      dividend   -- dividend operator
 *      divisor    -- divisor operator
 * OUTPUT:
 *      ptr_result -- save the devision result

 * RETURN:
 *     0-0xFFFFFFFFFFFFFFFF -- the reverse data.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit64_divUi32(
    UI64_T dividend,
    UI32_T divisor,
    UI64_T * ptr_result);

/* FUNCTION NAME: cmlib_bit64_divUi32
 * PURPOSE:
 *     it is used to do 64 bits division by 64 bits.
 * INPUT:
 *      dividend   -- dividend operator
 *      divisor    -- divisor operator
 * OUTPUT:
 *      ptr_result -- save the devision result

 * RETURN:
 *     0-0xFFFFFFFFFFFFFFFF -- the reverse data.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_bit64_divUi64(
    UI64_T dividend,
    UI64_T divisor,
    UI64_T * ptr_result);

#endif /* End of CMLIB_BIT_H */

