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

/* FILE NAME:  cmlib_bit.c
 * PURPOSE:
 *  {1. What is covered in this file - function and scope.}
 *  {2. Related documents or hardware information}
 * NOTES:
 *
 *
 *
 */
/* INCLUDE FILE DECLARATIONS
 */

#include <cmlib/cmlib_bit.h>
#include <osal/osal.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
/* GLOBAL VARIABLE DECLARATIONS
 */
//DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_bit.c");
/* LOCAL SUBPROGRAM DECLARATIONS
 */
/* STATIC VARIABLE DECLARATIONS
 */
/* EXPORTED SUBPROGRAM BODIES
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
    UI32_T  data,
    I32_T   *ptr_index )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    I32_T  start_bit = 0;
    UI32_T i = 0;
    /* BODY */
    HAL_CHECK_PTR(ptr_index);

    start_bit = (*ptr_index) + 1;

    for(i=start_bit;i<32;i++)
    {
        if(CMLIB_BIT_CHECK(data, i))
        {
            *ptr_index = i;
            return CLX_E_OK;
        }
    }
    *ptr_index = -1;
    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of cmlib_bit_getIdxOfSetBit */

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
    I32_T  *ptr_index )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    I32_T  start_bit = 0;
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_index);

    start_bit = (*ptr_index) + 1;

    for(i=start_bit;i<64;i++)
    {
        if(CMLIB_BIT64_CHECK(data, i))
        {
            *ptr_index = i;
            return CLX_E_OK;
        }
    }
    *ptr_index = -1;
    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of cmlib_bit64_getIdxOfSetBit */

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
    UI8_T  *ptr_dest )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data1);
    HAL_CHECK_PTR(ptr_data2);
    HAL_CHECK_PTR(ptr_dest);
    if(length == 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=0;i<length;i++)
    {
        ptr_dest[i] = ptr_data1[i] & ptr_data2[i];
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_andOfArray */

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
    UI8_T  *ptr_dest )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data1);
    HAL_CHECK_PTR(ptr_data2);
    HAL_CHECK_PTR(ptr_dest);
    if(length == 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=0;i<length;i++)
    {
        ptr_dest[i] = ptr_data1[i] | ptr_data2[i];
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_orOfArray */

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
    UI8_T  *ptr_dest )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data1);
    HAL_CHECK_PTR(ptr_data2);
    HAL_CHECK_PTR(ptr_dest);
    if(0 == length)
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=0;i<length;i++)
    {
        ptr_dest[i] = ptr_data1[i] ^ ptr_data2[i];
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_xorOfArray */

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
    I32_T  *ptr_cmp_ret )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data1);
    HAL_CHECK_PTR(ptr_data2);
    HAL_CHECK_PTR(ptr_cmp_ret);
    if(0 == length)
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=0;i<length;i++)
    {
        if(ptr_data1[i] != ptr_data2[i])
        {
            *ptr_cmp_ret = 1;
            return CLX_E_OK;
        }
    }
    *ptr_cmp_ret = 0;
    return CLX_E_OK;
}   /* End of cmlib_bit8_cmpOfArray */

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
    UI8_T  *ptr_dest )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;
    /* BODY */
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(ptr_dest);
    if(0 == length)
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=0;i<length;i++)
    {
        ptr_dest[i] = ~ptr_data[i];
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_invertOfArray */

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
    I32_T        *ptr_index )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;
    I32_T start_bit = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(ptr_index);
    if(0 == length)
    {
        return CLX_E_BAD_PARAMETER;
    }
    start_bit = (*ptr_index) + 1;
    if(start_bit >= (I32_T)length * 8 || start_bit < 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=start_bit;i<8*length;i++)
    {
        if(ptr_data[i/8] & (1U << (i%8)))
        {
            *ptr_index = i;
            return CLX_E_OK;
        }
    }

    *ptr_index = -1;
    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of cmlib_bit8_getIdxOfSetBitOfArray */

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
    I32_T  *ptr_ret)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(ptr_ret);
    if(length == 0 || bit_n >= length * 8)
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(ptr_data[bit_n/8] & (1U << (bit_n%8)))
    {
        *ptr_ret = 1;
    }
    else
    {
        *ptr_ret = 0;
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_checkBitOfArray */

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
    UI32_T bit_cnt)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data);
    if((0 == length)
        || (0 == bit_cnt)
        || (start_bit+bit_cnt > length * 8))
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=start_bit;i<start_bit+bit_cnt;i++)
    {
        ptr_data[i/8] |= (1U << (i%8));
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_setRangeOfArray */

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
    UI8_T *ptr_data,
    UI32_T length,
    UI32_T start_bit,
    UI32_T bit_cnt)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_data);
    if((0 == length)
        || (0 == bit_cnt)
        || (start_bit+bit_cnt > length * 8))
    {
        return CLX_E_BAD_PARAMETER;
    }

    for(i=start_bit;i<start_bit+bit_cnt;i++)
    {
        ptr_data[i/8] &= ~(1U << (i%8));
    }

    return CLX_E_OK;
}   /* End of cmlib_bit8_setRangeOfArray */

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
    UI8_T bit8_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    bit8_data = (((bit8_data & 0xAA) >> 1) | ((bit8_data & 0x55) << 1));
    bit8_data = (((bit8_data & 0xCC) >> 2) | ((bit8_data & 0x33) << 2));
    bit8_data = (((bit8_data & 0xF0) >> 4) | ((bit8_data & 0x0F) << 4));

    return bit8_data;
}   /* End of cmlib_bit8_rev */

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
    UI16_T bit16_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    bit16_data = (((bit16_data & 0xAAAA) >> 1) | ((bit16_data & 0x5555) << 1));
    bit16_data = (((bit16_data & 0xCCCC) >> 2) | ((bit16_data & 0x3333) << 2));
    bit16_data = (((bit16_data & 0xF0F0) >> 4) | ((bit16_data & 0x0F0F) << 4));
    bit16_data = (((bit16_data & 0xFF00) >> 8) | ((bit16_data & 0x00FF) << 8));

    return bit16_data;
}   /* End of cmlib_bit16_rev */

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
    UI32_T bit32_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    bit32_data = (((bit32_data & 0xAAAAAAAA) >> 1) | ((bit32_data & 0x55555555) << 1));
    bit32_data = (((bit32_data & 0xCCCCCCCC) >> 2) | ((bit32_data & 0x33333333) << 2));
    bit32_data = (((bit32_data & 0xF0F0F0F0) >> 4) | ((bit32_data & 0x0F0F0F0F) << 4));
    bit32_data = (((bit32_data & 0xFF00FF00) >> 8) | ((bit32_data & 0x00FF00FF) << 8));

    return (bit32_data >> 16) | (bit32_data << 16);
}   /* End of cmlib_bit32_rev */


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
    UI64_T bit64_data)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
    bit64_data = (((bit64_data & 0xAAAAAAAAAAAAAAAAULL) >> 1)
                    | ((bit64_data & 0x5555555555555555ULL) << 1));
    bit64_data = (((bit64_data & 0xCCCCCCCCCCCCCCCCULL) >> 2)
                    | ((bit64_data & 0x3333333333333333ULL) << 2));
    bit64_data = (((bit64_data & 0xF0F0F0F0F0F0F0F0ULL) >> 4)
                    | ((bit64_data & 0x0F0F0F0F0F0F0F0FULL) << 4));
    bit64_data = (((bit64_data & 0xFF00FF00FF00FF00ULL) >> 8)
                    | ((bit64_data & 0x00FF00FF00FF00FFULL) << 8));
    bit64_data = (((bit64_data & 0xFFFF0000FFFF0000ULL) >> 16)
                    | ((bit64_data & 0x0000FFFF0000FFFFULL) << 16));
    return (bit64_data >> 32) | (bit64_data << 32);
#else
    UI32_T tmp = bit64_data.ui64[UI64_MSW];
    bit64_data.ui64[UI64_MSW] = cmlib_bit32_rev(bit64_data.ui64[UI64_LSW]);
    bit64_data.ui64[UI64_LSW] = cmlib_bit32_rev(tmp);
    return bit64_data;
#endif
}   /* End of cmlib_bit64_rev */

CLX_ERROR_NO_T
cmlib_bit64_divUi32(
    UI64_T dividend,
    UI32_T divisor,
    UI64_T * ptr_result)
{
#ifdef CLX_EN_COMPILER_SUPPORT_LONG_LONG
    return osal_64bits_divUi32(dividend, divisor, ptr_result);
#else
    UI64_T divisor_64;
    UI64_ASSIGN(divisor_64, 0, divisor);
    return cmlib_bit64_divUi64(dividend, divisor_64, ptr_result);
#endif
}

CLX_ERROR_NO_T
cmlib_bit64_divUi64(
    UI64_T dividend,
    UI64_T divisor,
    UI64_T * ptr_result)
{
    UI64_T temp1, temp2, temp3, div_tmp, rs;
    UI32_T dst_bit = 0;
    UI32_T src_bit = 0;
    UI32_T rs_bit = 0;

    HAL_CHECK_PTR(ptr_result);
    if((0 == UI64_HI(divisor)) && (0 == UI64_LOW(divisor)))
    {
        return CLX_E_BAD_PARAMETER;
    }

#ifdef CLX_EN_COMPILER_SUPPORT_LONG_LONG
    UI64_SET(*ptr_result, 0);
#else
    ptr_result->ui64[UI64_MSW] = 0;
    ptr_result->ui64[UI64_LSW] = 0;
#endif

    UI64_SET(temp1, divisor);
    UI64_SET(temp2, divisor);
    while((0 != UI64_HI(temp1)) || (0 != UI64_LOW(temp1)))
    {
        CMLIB_BIT64_SHF_R(temp2, 1, temp1);
        UI64_SET(temp2, temp1);
        src_bit ++;
    }

    UI64_SET(div_tmp, dividend);
    UI64_ASSIGN(rs, 0, 0);

    while(1)
    {
        if((UI64_HI(divisor) > UI64_HI(div_tmp)) ||
            ((UI64_HI(divisor) == UI64_HI(div_tmp)) &&
             (UI64_LOW(divisor) > UI64_LOW(div_tmp))))
        {
            return CLX_E_OK;
        }

        UI64_SET(temp1, div_tmp);
        UI64_SET(temp2, div_tmp);
        while((0 != UI64_HI(temp1)) || (0 != UI64_LOW(temp1)))
        {
            CMLIB_BIT64_SHF_R(temp2, 1, temp1);
            UI64_SET(temp2, temp1);
            dst_bit ++;
        }

        if (src_bit == dst_bit)
        {
            UI64_ADD_UI32(rs, 1);
#ifdef CLX_EN_COMPILER_SUPPORT_LONG_LONG
            UI64_SET(*ptr_result, rs);
#else
            ptr_result->ui64[UI64_MSW] = UI64_HI(rs);
            ptr_result->ui64[UI64_LSW] = UI64_LOW(rs);
#endif
            return CLX_E_OK;
        }
        else
        {
            CMLIB_BIT64_SHF_L(divisor, (dst_bit - src_bit), temp1);

            if((UI64_HI(temp1) > UI64_HI(div_tmp)) ||
                ((UI64_HI(temp1) == UI64_HI(div_tmp)) &&
                 (UI64_LOW(temp1) > UI64_LOW(div_tmp))))
            {
                rs_bit = dst_bit - src_bit - 1;
                CMLIB_BIT64_SHF_L(divisor, rs_bit, temp1);
            }
            else
            {
                rs_bit = dst_bit - src_bit;
            }

            UI64_ASSIGN(temp2, 0, 1);
            CMLIB_BIT64_SHF_L(temp2, rs_bit, temp3);

            UI64_ADD_UI64(rs, temp3);
#ifdef CLX_EN_COMPILER_SUPPORT_LONG_LONG
            UI64_SET(*ptr_result, rs);
#else
            ptr_result->ui64[UI64_MSW] = UI64_HI(rs);
            ptr_result->ui64[UI64_LSW] = UI64_LOW(rs);
#endif
            UI64_SUB_UI64(div_tmp, temp1);
            dst_bit = 0;
        }
    }

    return CLX_E_OK;
}


/* LOCAL SUBPROGRAM BODIES
 */

