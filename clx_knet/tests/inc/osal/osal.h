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

/* FILE NAME:  osal.h
 * PURPOSE:
 *  osal.h provide an OS abstration layer's API for different OS. The APIs
 *  include task/thread, semaphore, time, memory, string and C library.
 * NOTES:
 *
 */

#ifndef OSAL_H
#define OSAL_H

/* INCLUDE FILE DECLARATIONS
 */

#include <clx_error.h>
#include <clx_types.h>
#include <osal/osal_lib.h>
#include <clx_knl.h>

extern UI32_T verbosity;

#define DIAG_SET_MODULE_INFO(__module_id__, __file_name__)
#define HAL_RUN_CHIP_MODE           (0x0)

#define HAL_RUN_PCIE_BYPASS_MODE    (0x3)
#define HAL_CHECK_MIN_MAX_RANGE(__value__, __min__, __max__) do             \
    {                                                                       \
        if ( ((__value__) > (__max__))  ||                                  \
             ((__value__) < (__min__)) )                                    \
        {                                                                   \
            DIAG_PRINT(HAL_DBG_WARN, "invalid "#__value__"=%u, range=%u-%u,"\
            " rc=%d\n", __value__, (UI32_T)__min__, (UI32_T)__max__,        \
            CLX_E_BAD_PARAMETER);                                           \
            return  CLX_E_BAD_PARAMETER;                                    \
        }                                                                   \
    } while (0)

#define HAL_CHECK_PTR(__ptr__) do                                           \
    {                                                                       \
        if (NULL == (__ptr__))                                              \
        {                                                                   \
            osal_printf("%s is null pointer\n",  #__ptr__);                 \
            return  CLX_E_BAD_PARAMETER;                                    \
        }                                                                   \
    } while (0)

#define HAL_CHECK_PTR(__ptr__) do                                           \
    {                                                                       \
        if (NULL == (__ptr__))                                              \
        {                                                                   \
            DIAG_PRINT(HAL_DBG_WARN, #__ptr__" is null pointer, rc=%d\n",   \
            CLX_E_BAD_PARAMETER);                                           \
            return  CLX_E_BAD_PARAMETER;                                    \
        }                                                                   \
    } while (0)

#define HAL_CHECK_ENUM_RANGE(__value__, __max__) do                         \
    {                                                                       \
        if ((__value__) >= (__max__))                                       \
        {                                                                   \
            DIAG_PRINT(HAL_DBG_WARN, "invalid "#__value__"=%u, range=0-%u," \
            " rc=%d\n", __value__, ((__max__) - 1), CLX_E_BAD_PARAMETER);   \
            return  CLX_E_BAD_PARAMETER;                                    \
        }                                                                   \
    } while (0)

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

typedef struct  OSAL_TM_S
{
    UI32_T  year;   /* year,   20XX */
    UI32_T  month;  /* month,  1~12 */
    UI32_T  day;    /* day,    1~31 */
    UI32_T  hour;   /* hour,   0~23 */
    UI32_T  min;    /* minute, 0~59 */
    UI32_T  sec;    /* second, 0~59 */
} OSAL_TM_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:  osal_init
 * PURPOSE:
 *      OS abstration API to initialize osal module.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_init(
    void);

/* FUNCTION NAME:  osal_deinit
 * PURPOSE:
 *      Deinitialize the OSAL module
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 */
CLX_ERROR_NO_T
osal_deinit(
    void);

/* FUNCTION NAME:  osal_alloc
 * PURPOSE:
 *      OS abstration API to allocate memory.
 * INPUT:
 *      size    -- size of memory to be allocate
 * OUTPUT:
 *      None
 * RETURN:
 *      Point to memory
 * NOTES:
 *      None
 */
void *
osal_alloc(
    const UI32_T size);

/* FUNCTION NAME:  osal_free
 * PURPOSE:
 *      OS abstration API to free allocated memory.
 * INPUT:
 *      ptr_mem -- point of the memory to be freed.
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
void
osal_free(
    const void *ptr_mem);

/* FUNCTION NAME:  osal_initRunThread
 * PURPOSE:
 *      OS abstration API to init the running thread's attribute
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
void
osal_initRunThread(
    void);

/* FUNCTION NAME:  osal_createThread
 * PURPOSE:
 *      OS abstration API to create thread.
 * INPUT:
 *      ptr_thread_name -- Point to the string for name of thread
 *      stack_size      -- size of stack
 *      priority        -- thread priority (Highest : 99, Lowest : 1)
 *      function        -- function point of thread
 *      ptr_arg         -- Point to agrument for callback function
 * OUTPUT:
 *      ptr_thread_id   -- pointer to thread ID
 * RETURN:
 *      CLX_E_OK        -- Successfully create the thread
 *      CLX_E_OTHERS    -- Fail to create the thread.
 * NOTES:
 *      The proper way to invoke osal_createThread is
 *      1. Caller define a CLX_THREAD_ID_T thread_id,
 *      2. Invoke with thread_id's address, i.e. osal_createThread(&thread_id).
 */
CLX_ERROR_NO_T
osal_createThread (
    const C8_T          *ptr_thread_name,
    const UI32_T        stack_size,
    const UI32_T        priority,
    void                (function)(void*),
    void                *ptr_arg,
    CLX_THREAD_ID_T     *ptr_thread_id);

/* FUNCTION NAME:  osal_stopThread
 * PURPOSE:
 *      OS abstration API to destroy thread.
 * INPUT:
 *      ptr_thread_id   -- thread ID.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully destroy the thread
 * NOTES:
 *      Similar with osal_createThread, when invoke osal_stopThread(),
 *      the caller should pass the thread_id's address.
 */
CLX_ERROR_NO_T
osal_stopThread(
    CLX_THREAD_ID_T     *ptr_thread_id);

/* FUNCTION NAME:  osal_destroyThread
 * PURPOSE:
 *      OS abstration API to destroy thread.
 * INPUT:
 *      ptr_thread_id   -- thread ID.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully destroy the thread
 * NOTES:
 *      Similar with osal_createThread, when invoke osal_destroyThread(),
 *      the caller should pass the thread_id's address.
 */
CLX_ERROR_NO_T
osal_destroyThread(
    CLX_THREAD_ID_T     *ptr_thread_id);

/* FUNCTION NAME:  osal_isRunThread
 * PURPOSE:
 *      OS abstration API to check if the thread is in run state.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OTHERS -- the thread is not in run state
 *      CLX_E_OK     -- the thread is in run state
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_isRunThread(
    void);

/* FUNCTION NAME:  osal_exitRunThread
 * PURPOSE:
 *      OS abstration API to release the OS resource
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
void
osal_exitRunThread(
    void);

/* FUNCTION NAME:  osal_sleepThread
 * PURPOSE:
 *      OS abstration API to suspend the current thread for microseconds.
 * INPUT:
 *      usecond       -- microseconds to suspend
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully suspend the thread
 *      CLX_E_OTHERS  -- Fail to suspend
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_sleepThread(
    const UI32_T usecond);

/* FUNCTION NAME:  osal_delayUs
 * PURPOSE:
 *      OS abstration API to delay the current thread for microseconds.
 * INPUT:
 *      usecond       -- microseconds to delay
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully delay the thread
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_delayUs(
    const UI32_T usecond);

/* FUNCTION NAME:  osal_createSemaphore
 * PURPOSE:
 *      OS abstration API to create semaphore.
 * INPUT:
 *      *ptr_sema_name    -- pointer to the string of semaphore name
 *      sema_count        -- the init value of semaphore
 *                           CLX_SEMAPHORE_BINARY: this means the semaphore is as
 *                             mutex for protecting critical section
 *                           CLX_SEMAPHORE_SYNC: this means the semaphore is as
 *                             signal for syncing.
 * OUTPUT:
 *      *ptr_semaphore_id -- Pointer to semaphore ID
 * RETURN:
 *      CLX_E_OK      -- Successfully create the semaphore.
 *      CLX_E_OTHERS  -- Fail to create the semaphore.
 * NOTES:
 *      The proper way to invoke osal_createSemaphore is
 *      1. Caller define a CLX_SEMAPHORE_ID_T id,
 *      2. Invoke with id's address, i.e. osal_createSemaphore(&id).
 */
CLX_ERROR_NO_T
osal_createSemaphore(
    const C8_T          *ptr_sema_name,
    const UI32_T        sema_count,
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id);

/* FUNCTION NAME:  osal_destroySemaphore
 * PURPOSE:
 *      OS abstration API to destroy semaphore.
 * INPUT:
 *      ptr_semaphore_id    -- Pointer to semaphore ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK            -- Successfully destory the semaphore.
 *      CLX_E_OTHERS        -- Fail to destory the semaphore.
 * NOTES:
 *      Similar with osal_createSemaphore, when invoke osal_destroySemaphore(),
 *      the caller should pass the semaphore_id's address.
 */
CLX_ERROR_NO_T
osal_destroySemaphore(
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id);

/* FUNCTION NAME:  osal_takeSemaphore
 * PURPOSE:
 *      OS abstration API to take semaphore.
 * INPUT:
 *      ptr_semaphore_id    -- Pointer to semaphore ID
 *      time_out            -- Time out before waiting semaphore in usec.
 *                             Wait forever. (0xFFFFFFFF)
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK            -- Successfully take the semaphore.
 *      CLX_E_OTHERS        -- Timeout.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_takeSemaphore(
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id,
    UI32_T              time_out);

/* FUNCTION NAME:  osal_giveSemaphore
 * PURPOSE:
 *      OS abstration API to give semaphore.
 * INPUT:
 *      ptr_semaphore_id    -- Pointer to semaphore ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK            -- Successfully give the semaphore.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_giveSemaphore(
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id);

/* FUNCTION NAME:  osal_createIsrLock
 * PURPOSE:
 *    OS abstration API to create IsrLock.
 * INPUT:
 *      ptr_isrlock_name -- pointer to the string of IsrLock name
 * OUTPUT:
 *      ptr_isrlock_id -- Pointer to IsrLock ID
 * RETURN:
 *      CLX_E_OK    -- Successfully create IsrLock.
 *      CLX_E_OTHERS  -- Fail to create IsrLock.
 * NOTES:
 *      IsrLock can protect shared resources between interrupt context switch
 *      and thread context switch.
 *      IsrLock support single-core CPU and multi-core CPU.
 *      In user space, IsrLock is the same as binary semaphore.
 *      In kernel space, take IsrLock will monopolize CPU until thread or interrupt give IsrLock.
 */
CLX_ERROR_NO_T
osal_createIsrLock(
    const C8_T          *ptr_isrlock_name,
    CLX_ISRLOCK_ID_T    *ptr_isrlock_id);

/* FUNCTION NAME:  osal_destroyIsrLock
 * PURPOSE:
 *      OS abstration API to destroy IsrLock.
 * INPUT:
 *      ptr_isrlock_id -- Pointer to IsrLock ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully destory the IsrLock.
 *      CLX_E_OTHERS  -- Fail to destory the IsrLock.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_destroyIsrLock(
    CLX_ISRLOCK_ID_T    *ptr_isrlock_id);

/* FUNCTION NAME:  osal_takeIsrLock
 * PURPOSE:
 *      OS abstration API to take srLock.
 * INPUT:
 *      ptr_isrlock_id    -- Pointer to srLock ID
 * OUTPUT:
 *      ptr_irq_flags     -- CPU interrupt enable/disable status.
 * RETURN:
 *      CLX_E_OK      -- Successfully take the srLock.
 * NOTES:
 *      In user space, taking srLock is the same as taking binary semaphore.
 *      In kernel space, taking srLock must disable all CPU interrupt, in some OS, may be need to disable preempt.
 *      In multi-core CPU, it must have a flag to indicate that shared resource is used by this interrupt or thread.
 */
CLX_ERROR_NO_T
osal_takeIsrLock(
    CLX_ISRLOCK_ID_T    *ptr_isrlock_id,
    CLX_IRQ_FLAGS_T     *ptr_irq_flags);

/* FUNCTION NAME:  osal_giveIsrLock
 * PURPOSE:
 *      OS abstration API to give IsrLock.
 * INPUT:
 *      ptr_isrlock_id    -- Pointer to IsrLock ID
 *      ptr_irq_flags     -- CPU interrupt enable/disable status.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully give the IsrLock.
 * NOTES:
 *      In user space, giving IsrLock is the same as giving binary semaphore.
 *      In kernel space, giving IsrLock must enable CPU interrupt that disabled in osal_takeIsrLock,
 *      if osal_takeIsrLock disable preempt, it must enable preempt.
 *      In multi-core CPU, it must reset a flag to indicate that shared resource is released now.
 */
CLX_ERROR_NO_T
osal_giveIsrLock(
    CLX_ISRLOCK_ID_T    *ptr_isrlock_id,
    CLX_IRQ_FLAGS_T     *ptr_irq_flags);

/* FUNCTION NAME:  osal_getTime
 * PURPOSE:
 *      OS abstration API to get current time since Unix Epoch.
 * INPUT:
 *      None
 * OUTPUT:
 *      ptr_time     -- time in micro-seconds
 * RETURN:
 *      CLX_E_OK     -- Successfully get time.
 *      CLX_E_OTHERS -- Fail to get time.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_getTime(
    CLX_TIME_T *ptr_time);

/* FUNCTION NAME:  osal_getCalendarTime
 * PURPOSE:
 *      OS abstration API to get current time since Unix Epoch.
 * INPUT:
 *      None
 * OUTPUT:
 *      ptr_tm       -- time in OSAL_TM_T
 * RETURN:
 *      CLX_E_OK     -- Successfully get time.
 *      CLX_E_OTHERS -- Fail to get time.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
osal_getCalendarTime(
    OSAL_TM_T *ptr_tm);

#endif  /* OSAL_H */
