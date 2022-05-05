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

/* FILE NAME:  cmlib_port.h
 * PURPOSE:
 *  this file is used to provide independent portlist macro.
 * NOTES:
 *  it contains operations as below:
 *      1. CMLIB_PORT_ADD
 *      2. CMLIB_PORT_DEL
 *      3. CMLIB_PORT_CHK
 *      4. CMLIB_PORT_FOREACH
 *      5. CMLIB_PORT_BITMAP_CLR
 *      6. CMLIB_PORT_BITMAP_SET
 *      7. CMLIB_PORT_BITMAP_ADD
 *      8. CMLIB_PORT_BITMAP_DEL
 *      9. CMLIB_PORT_BITMAP_AND
 *      10. CMLIB_PORT_BITMAP_INV
 *      11. CMLIB_PORT_BITMAP_EMPTY
 *      12. CMLIB_PORT_BITMAP_EQUAL
 *
 */
#ifndef CMLIB_PORT_H
#define CMLIB_PORT_H
/* INCLUDE FILE DECLARATIONS
 */

#include <clx_port.h>
#include <clx_types.h>
#include <cmlib/cmlib_util.h>

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */
#define CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b, bitwise_operator) \
        do { \
            ((bitmap_a)[0]) bitwise_operator ((bitmap_b)[0]); \
            ((bitmap_a)[1]) bitwise_operator ((bitmap_b)[1]); \
            ((bitmap_a)[2]) bitwise_operator ((bitmap_b)[2]); \
            ((bitmap_a)[3]) bitwise_operator ((bitmap_b)[3]); \
            ((bitmap_a)[4]) bitwise_operator ((bitmap_b)[4]); \
            ((bitmap_a)[5]) bitwise_operator ((bitmap_b)[5]); \
            ((bitmap_a)[6]) bitwise_operator ((bitmap_b)[6]); \
            ((bitmap_a)[7]) bitwise_operator ((bitmap_b)[7]); \
            ((bitmap_a)[8]) bitwise_operator ((bitmap_b)[8]); \
        } while(0)

#define CMLIB_PORT_BITMAP_SET(bitmap_a, bitmap_b)    CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b, =)
#define CMLIB_PORT_BITMAP_ADD(bitmap_a, bitmap_b)    CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b, |=)
#define CMLIB_PORT_BITMAP_DEL(bitmap_a, bitmap_b)    CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b, &= ~)
#define CMLIB_PORT_BITMAP_AND(bitmap_a, bitmap_b)    CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b, &=)
#define CMLIB_PORT_BITMAP_OR(bitmap_a, bitmap_b)     CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b,  |=)
#define CMLIB_PORT_BITMAP_XOR(bitmap_a, bitmap_b)    CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b,  ^=)
#define CMLIB_PORT_BITMAP_INV(bitmap_a, bitmap_b)    CMLIB_PORT_BITMAP_OPERATION(bitmap_a, bitmap_b, = ~)
#define CMLIB_PORT_BITMAP_COUNT(bitmap, count) \
        do  { \
                count = cmlib_util_popcount((bitmap)[0]) + \
                        cmlib_util_popcount((bitmap)[1]) + \
                        cmlib_util_popcount((bitmap)[2]) + \
                        cmlib_util_popcount((bitmap)[3]) + \
                        cmlib_util_popcount((bitmap)[4]) + \
                        cmlib_util_popcount((bitmap)[5]) + \
                        cmlib_util_popcount((bitmap)[6]) + \
                        cmlib_util_popcount((bitmap)[7]) + \
                        cmlib_util_popcount((bitmap)[8]); \
        } while (0)


/* DATA TYPE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

#endif /* End of CMLIB_PORT_H */
