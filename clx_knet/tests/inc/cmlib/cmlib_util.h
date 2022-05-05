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

/* FILE NAME:  cmlib_util.h
 * PURPOSE:
 *   1. provide common macro for other modules.
 * NOTES:
 *
 *
 *
 */
#ifndef CMLIB_UTIL_H
#define CMLIB_UTIL_H
/* INCLUDE FILE DECLARATIONS
 */
#include <osal/osal.h>
typedef struct CLX_L3_IP_NETWORK_ADDR_S
{
    CLX_IP_T    ip_addr;                        /* IP address. For default route, set all to zero.*/
    CLX_IP_T    ip_mask;                        /* IP address mask. For default route, set all to zero.*/
    BOOL_T      ipv4;                           /* This IP network address is IPV4 address or not.*/
} CLX_L3_IP_NETWORK_ADDR_T;

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */
#define CMLIB_UTIL_MAC_STR_SIZE         (15)
#define CMLIB_UTIL_MAC_TO_STR(__buf__,__mac__)  \
                    osal_snprintf(__buf__,CMLIB_UTIL_MAC_STR_SIZE,"%02X%02X-%02X%02X-%02X%02X", \
                        (__mac__)[0],(__mac__)[1],(__mac__)[2],(__mac__)[3],(__mac__)[4],(__mac__)[5])

#define CMLIB_UTIL_IPV4_STR_SIZE        (16)
#define CMLIB_UTIL_IPV4_TO_STR(__buf__,__ipv4__)    \
                        osal_snprintf(__buf__,CMLIB_UTIL_IPV4_STR_SIZE,"%d.%d.%d.%d",   \
                        ((__ipv4__)&0xFF000000)>>24,((__ipv4__)&0x00FF0000)>>16,    \
                        ((__ipv4__)&0x0000FF00)>>8, ((__ipv4__)&0x000000FF))

#define CMLIB_UTIL_IPV6_STR_SIZE        (40)
#define CMLIB_UTIL_IPV6_TO_STR(__buf__,__ipv6__)    \
                        osal_snprintf(__buf__,CMLIB_UTIL_IPV6_STR_SIZE, \
                        "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",  \
                        (__ipv6__)[0],  (__ipv6__)[1],  (__ipv6__)[2],  (__ipv6__)[3],    \
                        (__ipv6__)[4],  (__ipv6__)[5],  (__ipv6__)[6],  (__ipv6__)[7],    \
                        (__ipv6__)[8],  (__ipv6__)[9],  (__ipv6__)[10], (__ipv6__)[11],  \
                        (__ipv6__)[12], (__ipv6__)[13], (__ipv6__)[14], (__ipv6__)[15])

#define CMLIB_UTIL_IP_ADDR_STR_SIZE     (CMLIB_UTIL_IPV6_STR_SIZE)
#define CMLIB_UTIL_IP_NETWORK_STR_SIZE  (CMLIB_UTIL_IPV6_STR_SIZE)

#define CMLIB_UTIL_ENDIAN_SWAP32(__val__)           \
        ({                                          \
            UI32_T __val = (__val__);               \
            __val = ((__val & 0xFF00FF00) >>  8) |  \
                    ((__val & 0x00FF00FF) <<  8);   \
            __val = ((__val & 0xFFFF0000) >> 16) |  \
                    ((__val & 0x0000FFFF) << 16);   \
            __val;                                  \
        })

/* DATA TYPE DECLARATIONS
 */
/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
UI32_T
cmlib_util_getFcidPrefix(
    const UI32_T mask);

UI32_T
cmlib_util_getIpv4Prefix(
    const UI32_T mask);

UI32_T
cmlib_util_getIpv6Prefix(
    const UI8_T *ptr_mask);

void
cmlib_util_getIpv4Str(
    const CLX_IPV4_T *ptr_ipv4,
    C8_T *ptr_str);

void
cmlib_util_getIpv6Str(
    const CLX_IPV6_T *ptr_ipv6,
    C8_T *ptr_str);

void
cmlib_util_getIpAddrStr(
    const CLX_IP_ADDR_T *ptr_ip_addr,
    C8_T *ptr_str);

void
cmlib_util_getIpNetworkStr(
    const CLX_L3_IP_NETWORK_ADDR_T *ptr_network_addr,
    C8_T *ptr_str);

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
 *      None
 *
 */
UI32_T
cmlib_util_popcount(
    UI32_T count);

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
    UI32_T ui32_map);

#endif /* End of CMLIB_UTIL_H */

