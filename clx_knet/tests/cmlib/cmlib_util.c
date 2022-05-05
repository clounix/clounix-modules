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

/* FILE NAME:  cmlib_util.c
 * PURPOSE:
 *   1. To provide API for port bitmap operation
 *
 * NOTES:
 *
 *
 *
 */
 /* INCLUDE FILE DECLARATIONS
  */
#include <cmlib/cmlib_util.h>

/* NAMING CONSTANT DECLARATIONS
*/
/* MACRO FUNCTION DECLARATIONS
*/
/* DATA TYPE DECLARATIONS
*/
/* GLOBAL VARIABLE DECLARATIONS
*/
/* LOCAL SUBPROGRAM DECLARATIONS
*/
/* STATIC VARIABLE DECLARATIONS
*/

/* EXPORTED SUBPROGRAM BODIES
*/
UI32_T
cmlib_util_getFcidPrefix(
    const UI32_T mask)
{
    UI32_T temp_mask = mask << 8;
    UI32_T prefix = 0;
    UI32_T i = 0;

    do{
        if (temp_mask & 0x80000000)
        {
            prefix++;
            temp_mask = temp_mask << 1;
        }
        else
        {
            break;
        }
        i++;
    } while (i < 24);

    return prefix;
}

UI32_T
cmlib_util_getIpv4Prefix(
    const UI32_T mask)
{
    UI32_T temp_mask = mask;
    UI32_T prefix = 0;
    UI32_T i = 0;

    do{
        if (temp_mask & 0x80000000)
        {
            prefix++;
            temp_mask = temp_mask << 1;
        }
        else
        {
            break;
        }
        i++;
    } while (i < 32);

    return prefix;
}

UI32_T
cmlib_util_getIpv6Prefix(
    const UI8_T *ptr_mask)
{
    UI32_T prefix = 0;
    UI32_T i = 0;
    UI8_T value;

    do{
        if (0xFF == ptr_mask[i])
        {
            prefix += 8;
        }
        else
        {
            break;
        }
        i++;
    } while (i < sizeof(CLX_IPV6_T));

    if (i < sizeof(CLX_IPV6_T))
    {
        value = ptr_mask[i];
        while (value)
        {
            prefix++;
            value <<= 1;
        }
    }

    return prefix;
}

void
cmlib_util_getIpv4Str(
    const CLX_IPV4_T *ptr_ipv4,
    C8_T *ptr_str)
{
    CMLIB_UTIL_IPV4_TO_STR(ptr_str, *ptr_ipv4);
}

void
cmlib_util_getIpv6Str(
    const CLX_IPV6_T *ptr_ipv6,
    C8_T *ptr_str)
{
    UI32_T idx = 0, next = 0, last = 16;
    UI32_T cont_zero = 0;

    while (idx < last)
    {
        if ((0 == cont_zero) && (0 == (*ptr_ipv6)[idx]) && (0 == (*ptr_ipv6)[idx + 1]))
        {
            next = idx + 2;

            while (next < last)
            {
                if (((*ptr_ipv6)[next]) || ((*ptr_ipv6)[next + 1]))
                {
                    osal_snprintf(
                            ptr_str + osal_strlen(ptr_str),
                            CMLIB_UTIL_IPV6_STR_SIZE - osal_strlen(ptr_str),
                            "%s", (cont_zero) ? (":") : (":0"));
                    break;
                }

                if (0 == cont_zero)
                {
                    cont_zero = 1;
                }
                next += 2;
            }

            if (next == last)
            {

                osal_snprintf(
                        ptr_str + osal_strlen(ptr_str),
                        CMLIB_UTIL_IPV6_STR_SIZE - osal_strlen(ptr_str),
                        "%s", (cont_zero) ? ("::") : (":0"));
            }

            idx = next;
        }
        else
        {
            if (idx)
            {
                osal_snprintf(
                    ptr_str + osal_strlen(ptr_str),
                    CMLIB_UTIL_IPV6_STR_SIZE - osal_strlen(ptr_str),
                    ":");
            }

            if ((*ptr_ipv6)[idx])
            {
                osal_snprintf(
                    ptr_str + osal_strlen(ptr_str),
                    CMLIB_UTIL_IPV6_STR_SIZE - osal_strlen(ptr_str),
                    "%0x%02x", (*ptr_ipv6)[idx], (*ptr_ipv6)[idx + 1]);
            }
            else
            {
                osal_snprintf(
                    ptr_str + osal_strlen(ptr_str),
                    CMLIB_UTIL_IPV6_STR_SIZE - osal_strlen(ptr_str),
                    "%0x", (*ptr_ipv6)[idx + 1]);
            }

            idx += 2;
        }
    }
}

void
cmlib_util_getIpAddrStr(
    const CLX_IP_ADDR_T *ptr_ip_addr,
    C8_T *ptr_str)
{
    if (TRUE == ptr_ip_addr->ipv4)
    {
        cmlib_util_getIpv4Str(&ptr_ip_addr->ip_addr.ipv4_addr, ptr_str);
    }
    else
    {
        cmlib_util_getIpv6Str(&ptr_ip_addr->ip_addr.ipv6_addr, ptr_str);
    }
}

void
cmlib_util_getIpNetworkStr(
    const CLX_L3_IP_NETWORK_ADDR_T *ptr_network_addr,
    C8_T *ptr_str)
{
    UI32_T prefix = 0;

    if (TRUE == ptr_network_addr->ipv4)
    {
        cmlib_util_getIpv4Str(&ptr_network_addr->ip_addr.ipv4_addr, ptr_str);
        prefix = cmlib_util_getIpv4Prefix(ptr_network_addr->ip_mask.ipv4_addr);
    }
    else
    {
        cmlib_util_getIpv6Str(&ptr_network_addr->ip_addr.ipv6_addr, ptr_str);
        prefix = cmlib_util_getIpv6Prefix(ptr_network_addr->ip_mask.ipv6_addr);
    }

    osal_snprintf(
        ptr_str + osal_strlen(ptr_str),
        CMLIB_UTIL_IP_NETWORK_STR_SIZE - osal_strlen(ptr_str),
        "/%d", prefix);
}

/* FUNCTION NAME:   cmlib_util_popcount
 * PURPOSE:
 *      cmlib_util_popcount() is a function to count number of one in a word.
 *
 * INPUT:
 *      count      -- The specified word.
 * OUTPUT:
 *      None
 * RETURN:
 *      UI32_T   -- Return number of one in a word.
 *
 * NOTES:
 *
 */
UI32_T
cmlib_util_popcount(
    UI32_T count)
{
    count = ((count >> 1) & 0x55555555) + (count & 0x55555555);
    count = ((count >> 2) & 0x33333333) + (count & 0x33333333);
    count = ((count >> 4) + count) & 0x0F0F0F0F;
    count = ((count >> 8) + count);
    count = ((count >> 16) + count) & 0xFF;
    return count;
}

/* FUNCTION NAME:   cmlib_util_findFirstZero
 * PURPOSE:
 *      cmlib_util_findFirstZero() is a function to get first zero bit from LSB.
 *
 * INPUT:
 *      ui32_map  -- The specified word.
 * OUTPUT:
 *      None
 * RETURN:
 *      UI32_T   -- Return available bit in a word.
 *
 * NOTES:
 *
 */
UI32_T
cmlib_util_findFirstZero(
    UI32_T ui32_map)
{
    UI32_T shift_count;
    static const C8_T lookup_table[] =
    {
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /*   0 ..  15 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,  /*  16 ..  31 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /*  32 ..  47 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,  /*  48 ..  63 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /*  64 ..  79 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,  /*  80 ..  95 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /*  96 .. 111 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7,  /* 112 .. 127 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /* 128 .. 143 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,  /* 144 .. 159 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /* 160 .. 175 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,  /* 176 .. 191 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /* 192 .. 207 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,  /* 208 .. 223 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,  /* 224 .. 239 */
        0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 8   /* 240 .. 254, 255 is invalid */
    };

    for (shift_count = 0; shift_count < 32; shift_count += 8)
    {
        if ((ui32_map & 0xff) != 0xff)
        {
            break;
        }
        ui32_map >>= 8;
    }

    return lookup_table[ui32_map & 0xff] + shift_count;
}
