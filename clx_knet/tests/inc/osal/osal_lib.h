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

/* FILE NAME:  osal_lib.h
 * PURPOSE:
 *      Provide platform dependent librauroray porting layer.
 *
 * NOTES:
 *
 */

#ifndef OSAL_LIB_H
#define OSAL_LIB_H

/* INCLUDE FILE DECLARATIONS
 */

#include <clx_error.h>
#include <clx_types.h>
#include <stdarg.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */
#define OSAL_VA_START(ap, last)        va_start(ap, last)
#define OSAL_VA_END(ap)                va_end(ap)
#define OSAL_VA_ARG(ap, type)          va_arg(ap, type)

/* For FT and SQA verification, OSAL_EN_ASSERT should be defined. */
/* #define OSAL_EN_ASSERT */

#ifdef OSAL_EN_ASSERT
#define OSAL_ASSERT(express)           ((express) ? 0 : osal_assert(#express, __FILE__, __LINE__))
#else
#define OSAL_ASSERT(express)
#endif

/* DATA TYPE DECLARATIONS
 */
typedef va_list OSAL_VA_LIST;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:  osal_memset
 * PURPOSE:
 *  OS abstration API to set the first (num) bytes of the block of memory
 *  pointed by (ptr) to the specified (value).
 * INPUT:
 *      ptr_mem -- point of the memory to fill
 *      value   -- value to be set (cast to unsigned char)
 *      num     -- number of bytes to copy.
 * OUTPUT:
 *      None
 * RETURN:
 *      Pointer to memory is returned
 * NOTES:
 *      None
 */
void *
osal_memset(
    void            *ptr_mem,
    const I32_T     value,
    const UI32_T    num);

/* FUNCTION NAME:  osal_memcpy
 * PURPOSE:
 *  OS abstration API to copy from source address to destination address with
 *  given number of bytes.
 * INPUT:
 *      ptr_dst -- point of the destination memory to copy to
 *      ptr_src -- point of the source memory to copy from
 *      num     -- number of bytes to copy.
 * OUTPUT:
 *      None
 * RETURN:
 *      Destination address is returned
 * NOTES:
 *      None
 */
void *
osal_memcpy(
    void            *ptr_dst,
    const void      *ptr_src,
    const UI32_T    num);

/* FUNCTION NAME:  osal_memcmp
 * PURPOSE:
 *  OS abstration API to compare between 2 block of memory with given number of
 *  bytes.
 * INPUT:
 *      ptr_mem1    -- point of the memory to compare.
 *      ptr_mem2    -- point of the memory to compare.
 *      num         -- number of bytes to compare.
 * OUTPUT:
 *      None
 * RETURN:
 *      0                -- contents of both memory blocks are equal.
 *      positive integer -- not match, ptr_mem1's first byte is greater than
 *                              ptr_mem2's first byte.
 *      negative integer -- not match, ptr_mem1's first byte is less than
 *                              ptr_mem2's first byte.
 *
 * NOTES:
 *      None
 */
I32_T
osal_memcmp(
    const void      *ptr_mem1,
    const void      *ptr_mem2,
    const UI32_T    num);

/* FUNCTION NAME:  osal_strncpy
 * PURPOSE:
 *  OS abstration API to copy (num) characters from (ptr_src) to (ptr_dst). If the
 *  end of the source C string (which is signaled by a null-character) is found
 *  before num characters have been copied, destination is padded with zeros
 *  until a total of num characters have been written to it.
 * INPUT:
 *      ptr_dst -- Pointer to the destination array where the content is to be copied.
 *      ptr_src -- C string to be copied.
 *      num     -- Maximum number of characters to be copied from source.
 * OUTPUT:
 *      None
 * RETURN:
 *      Destination address is returned
 * NOTES:
 *      If there is no null byte among the frist (num)  bytes of (ptr_src),
 *  the (ptr_dst) will not be null terminated.
 */
C8_T *
osal_strncpy(
    C8_T            *ptr_dst,
    const C8_T      *ptr_src,
    const UI32_T    num);

/* FUNCTION NAME:  osal_strncmp
 * PURPOSE:
 *  OS abstration API to compare the first (num) characters of (ptr_str1) and
 *  (ptr_str2).
 * INPUT:
 *      ptr_str1    -- C string to be compared.
 *      ptr_str2    -- C string to be compared.
 *      num         -- Maximum number of characters to compare.
 * OUTPUT:
 *      None
 * RETURN:
 *      0                -- contents of both memory blocks are equal.
 *      positive integer -- not match, ptr_str1 is greater than ptr_str2.
 *      negative integer -- not match, ptr_str1 is less than ptr_str2.
 * NOTES:
 *      None
 */
I32_T
osal_strncmp(
    const C8_T      *ptr_str1,
    const C8_T      *ptr_str2,
    const UI32_T    num);

/* FUNCTION NAME:  osal_strlen
 * PURPOSE:
 *  OS abstration API to returns the length of the C string str.
 * INPUT:
 *      ptr_str -- C string to return length.
 * OUTPUT:
 *      None
 * RETURN:
 *      length of string
 * NOTES:
 *      None
 */
UI32_T
osal_strlen(
    const C8_T      *ptr_str);

/* FUNCTION NAME: osal_strcat
 * PURPOSE:
 *      it appends the <ptr_src> string to the <ptr_dest> string, owverwriting
 *  the null byte('\0') at the end of dest, and then adds a terminating null
 *  byte. The <ptr_dest> string must have enough space for the result.
 * INPUT:
 *      ptr_dest  -- the dest string will save the result, it must have enough space.
 *      ptr_src   -- the src string will be appended to the dest string.
 * OUTPUT:
 *      None
 * RETURN:
 *      Non NULL -- the result string pointer, it points to ptr_dest as same.
 *      NULL     -- there is NULL pointer.
 * NOTES:
 *      the dest string must have at least space:
 *      osal_strlen(ptr_dest) + osal_strlen(ptr_src) + 1
 */
C8_T *
osal_strcat(
    C8_T            *ptr_dest,
    const C8_T      *ptr_src);

/* FUNCTION NAME: osal_strncat
 * PURPOSE:
 *      it appends the <ptr_src> string to the <ptr_dest> string at most <num>
 *  chars,  the null byte('\0') at the end of dest will be overwirtten, and will
 *  add a '\0' to the end of the result. The <ptr_dest> string must have enough
 *  space for the result.
 * INPUT:
 *      ptr_dest  -- the dest string will save the result, it must have enough
 *                  space.
 *      ptr_src   -- the src string will be appended to the dest string.
 *      num       -- the max number of chars from ptr_src will be appended.
 * OUTPUT:
 *      None
 * RETURN:
 *      Non NULL -- the result string pointer, it points to ptr_dest as same.
 *      NULL     -- there is NULL pointer.
 * NOTES:
 *      the dest string must have at least space:
 *      osal_strlen(ptr_dest) + n + 1
 */
C8_T *
osal_strncat(
    C8_T            *ptr_dest,
    const C8_T      *ptr_src,
    UI32_T          num);

/* FUNCTION NAME:  osal_printf
 * PURPOSE:
 *  OS abstration API to print the string to standard output.
 * INPUT:
 *          fmt     -- %[flags][width][.precision][length]specifier
 *
 *              specifier   | output
 *              ------------+---------------------------------------------------
 *              d or i      | Signed decimal integer
 *              u           | Unsigned decimal integer
 *              o or O      | Unsigned octal
 *              x           | Unsigned hexadecimal integer
 *              X           | Unsigned hexadecimal integer (uppercase)
 *              f           | Decimal floating point
 *              c           | Character
 *              s           | String of characters
 *              n           | Nothing printed.
 *              %           | A % followed by another % character will write a
 *                          | single % to the stream.
 *
 *              flags       | description
 *              ------------+---------------------------------------------------
 *              -           | Left-justify within the given field width
 *              +           | Forces to preceed the result with a plus or minus
 *                          | sign
 *              (space)     | If no sign is going to be written, a blank space
 *                          | is inserted before the value.
 *              #           | Used with o, x or X specifiers the value is
 *                          | preceeded with 0, 0x or 0x respectively for values
 *                          | different than zero.
 *              0           | Left-pads the number with zeroes (0) instead of
 *                          | spaces when padding is specified
 *
 *              width       | description
 *              ------------+---------------------------------------------------
 *              (number)    | Minimum number of characters to be printed.
 *              *           | The width is not specified in the format string,
 *                          | but as an additional integer value argument
 *                          | preceding the argument that has to be formatted.
 *
 *              .precision  | description
 *              ------------+---------------------------------------------------
 *              .number        | For integer specifiers (d, i, o, u, x, X):
 *                          | precision specifies the minimum number of digits
 *                          | to be written. If the value to be written is
 *                          | shorter than this number, the result is padded
 *                          | with leading zeros. The value is not truncated
 *                          | even if the result is longer. A precision of 0
 *                          | means that no character is written for the value 0
 *                          |
 *                          | For a, A, e, E, f and F specifiers:
 *                          | this is the number of digits to be printed after
 *                          | the decimal point (by default, this is 6).
 *                          |
 *                          | For g and G specifiers:
 *                          | This is the maximum number of significant digits
 *                          | to be printed.
 *                          | For s: this is the maximum number of characters to
 *                          | be printed. By default all characters are printed
 *                          | until the ending null character is encountered.
 *                          | If the period is specified without an explicit
 *                          | value for precision, 0 is assumed.
 *
 *                          |           specifiers
 *                          +---------------+-----------------------+-----------
 *              length      | d i           | u o x X               |
 *              ------------+---------------+-----------------------+-----------
 *              h           | short int     | unsigned short int    |
 *              l           | long int      | unsigned long int     |
 * OUTPUT:
 *      None
 * RETURN:
 *      On success, the total number of characters written is returned.
 *
 *      If a writing error occurs, the error indicator (ferror) is set and a
 *      negative number is returned.
 * NOTES:
 *      It could refer to the standard C's printf.
 */
void
osal_printf(
    const C8_T      *ptr_fmt,
    ...);

/* FUNCTION NAME:  osal_snprintf
 * PURPOSE:
 *  OS abstration API to composes a string with the same text that would be
 *  printed.
 * INPUT:
 *      ptr_str -- pointer to the buffer
 *      length -- The length of the buffer
 *      ptr_fmt -- C string that contains a format string that follows
 *                     the same specifications as format in osal_printf.
 * OUTPUT:
 *      None
 * RETURN:
 *      On success, the total number of characters written is returned.
 *
 *      If a writing error occurs, the error indicator (ferror) is set and a
 *      negative number is returned.
 * NOTES:
 *      None
 */
I32_T
osal_snprintf(
    C8_T            *ptr_str,
    const UI32_T    length,
    const C8_T      *ptr_fmt,
    ... );

/* FUNCTION NAME:  osal_vsnprintf
 * PURPOSE:
 *  OS abstration API to composes a string with the same text that would be
 *  printed.
 * INPUT:
 *      ptr_str -- pointer to the buffer
 *      length  -- The length of the buffer
 *      ptr_fmt -- C string that contains a format string that follows
 *                     the same specifications as format in osal_printf.
 *      ap      -- The pointer ro the first variable
 * OUTPUT:
 *      None
 * RETURN:
 *      On success, the total number of characters written is returned.
 *
 *      If a writing error occurs, the error indicator (ferror) is set and a
 *      negative number is returned.
 * NOTES:
 *      None
 */
I32_T
osal_vsnprintf(
    C8_T            *ptr_str,
    const UI32_T    length,
    const C8_T      *ptr_fmt,
    OSAL_VA_LIST    ap);


/* FUNCTION NAME: osal_srand
 * PURPOSE:
 *      it is used to set random seed
 * INPUT:
 *      seed -- random seed
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
void
osal_srand(
    const UI32_T    seed);

/* FUNCTION NAME: osal_rand
 * PURPOSE:
 *      it is used to get a random UI32_T number
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      random number
 * NOTES:
 *      None
 */
UI32_T
osal_rand(
    void);

/* FUNCTION NAME:  osal_assert
 * PURPOSE:
 *  OS abstration API to assert.
 * INPUT:
 *      ptr_express     -- express to evaluate, false to assert
 *      prt_file        -- filename of assert point
 *      line            -- line number of assert point
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
UI32_T
osal_assert (
    const C8_T      *ptr_express,
    const C8_T      *prt_file,
    const UI32_T    line);

#if defined (CLX_EN_COMPILER_SUPPORT_LONG_LONG)
/* FUNCTION NAME:  osal_64bits_divUi32
 * PURPOSE:
 * OS abstration API to do 64 bits division by 32 bits.
 * INPUT:
 *      dividend    --  dividend operator
 *      divisor      --  divisor operator
 * OUTPUT:
 *      ptr_result -- save the devision result
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_64bits_divUi32(
    UI64_T          dividend,
    UI32_T          divisor,
    UI64_T          *ptr_result);
#endif

#endif /* End of OSAL_LIB_H */

