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

/* FILE NAME:  osal.c
 * AUTHOR: ChiaHung Lee, Xianfeng Pan
 * PURPOSE:
 *
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARATIONS
 */

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <clx_error.h>
#include <clx_types.h>
#include <osal/osal.h>
#include <osal/osali.h>

/* NAMING CONSTANT DECLARATIONS
 */

#define OSAL_US_PER_SECOND      (1000000)   /* macro second per second      */
#define OSAL_NS_PER_USECOND     (1000)      /* nano second per macro second */
#define OSAL_TIME_YEAR_OFFSET   (1900)

/* MACRO FUNCTION DECLARATIONS
 */
#define OSAL_LOG_ERR(msg, ...) \
            osal_printf("\033[31m<osal:%d>\033[0m"msg, __LINE__, ##__VA_ARGS__)

#define OSAL_LOG_WARNING(msg, ...) \
        if (_osal_log_flag & OSAL_WARNING_LOG_ENABLE) \
            osal_printf(msg, ##__VA_ARGS__)

#define OSAL_LOG_DBG(msg, ...) \
        if (_osal_log_flag & OSAL_DEBUG_LOG_ENABLE) \
            osal_printf(msg, ##__VA_ARGS__)

#define OSAL_PRINT_ERR(msg, ...)                    printf(msg, ##__VA_ARGS__)


#define OSAL_CHECK_MEM_SIZE(__size__)               _osal_chkMemSizeRange(__FUNCTION__, __size__)
#define OSAL_CHECK_MEM_PTR(__ptr_mem__, __len__)    _osal_chkMemPtrRange(__FUNCTION__, __ptr_mem__, __len__)

/* DATA TYPE DECLARATIONS
 */

typedef sem_t               LINUX_SEMA_T;
typedef pthread_t           LINUX_THREAD_T;
typedef pthread_mutex_t     LINUX_MUTEX_T;
typedef pthread_attr_t      LINUX_THREAD_ATTR_T;
typedef struct sched_param  LINUX_SCHED_PARAM_T;
typedef pthread_cond_t      LINUX_COND_T;
typedef pthread_mutexattr_t LINUX_MUTEXATTR_T;
typedef time_t              LINUX_TIME_T;
typedef struct timeval      LINUX_TIMEVAL_T;
typedef struct tm           LINUX_TM_T;

typedef struct
{
    UI32_T                      signature;        /* signature, use to check mem */
    UI32_T                      cookie_type;      /* cookie type                 */
    UI32_T                      real_len;         /* real lenth of mem           */
    UI8_T                       padding[4];       /* to make below user data align 64-bit address */
    UI8_T                       data[0];          /* data buffer                 */
} OSAL_MEM_CB_T;

typedef struct OSAL_SEMA_CB_S
{
    LINUX_SEMA_T                sema;             /* semaphore            */
    C8_T                        sema_name[OSAL_SEMA_NAME_LEN + 1];/* semaphore name */
} OSAL_SEMA_CB_T;

typedef struct OSAL_THREAD_CB_S
{
    struct OSAL_THREAD_CB_S     *ptr_next_thread; /* next thread          */
    CLX_THREAD_ID_T             thread_id;        /* thread id            */
    C8_T                        thread_name[OSAL_THREAD_NAME_LEN + 1];/* thread name */
    UI32_T                      priority;         /* priority, 0~99       */
    UI32_T                      stack_size;       /* stack size           */
    BOOL_T                      is_stop;          /* thread status        */
} OSAL_THREAD_CB_T;

/* GLOBAL VARIABLE DECLARATIONS
 */
//DIAG_SET_MODULE_INFO(CLX_MODULE_OSAL, "osal.c");

/* STATIC VARIABLE DECLARATIONS
 */

/* Thread */
static OSAL_THREAD_CB_T         *_ptr_osal_thread_list_head = NULL;
static LINUX_MUTEX_T            _osal_thread_cb_lock = PTHREAD_MUTEX_INITIALIZER;

/* Log */
static BOOL_T                   _osal_is_inited = FALSE;
static UI32_T                   _osal_log_flag = 0;

/* LOCAL SUBPROGRAM DECLARATIONS
 */

static CLX_ERROR_NO_T
_osal_chainThread(
    OSAL_THREAD_CB_T *ptr_thread_cb)
{
    OSAL_THREAD_CB_T *ptr_cb = NULL;

    pthread_mutex_lock(&_osal_thread_cb_lock);
    if (NULL == _ptr_osal_thread_list_head)
    {
        _ptr_osal_thread_list_head = ptr_thread_cb;
        _ptr_osal_thread_list_head->ptr_next_thread = NULL;
    }
    else
    {
        /* Traverse the whole thread list. */
        for (ptr_cb = _ptr_osal_thread_list_head;
             ptr_cb != NULL;
             ptr_cb = ptr_cb->ptr_next_thread)
        {
            if (NULL == ptr_cb->ptr_next_thread)
            {
                ptr_cb->ptr_next_thread = ptr_thread_cb;
                ptr_thread_cb->ptr_next_thread = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&_osal_thread_cb_lock);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_unchainThread(
    CLX_THREAD_ID_T     *ptr_thread_id)
{
    CLX_ERROR_NO_T      rc = CLX_E_OTHERS;
    OSAL_THREAD_CB_T    *ptr_cb = NULL;
    OSAL_THREAD_CB_T    *ptr_cb_prev = NULL;

    pthread_mutex_lock(&_osal_thread_cb_lock);
    /* Traverse the whole thread list. */
    for (ptr_cb = _ptr_osal_thread_list_head;
         ptr_cb != NULL;
         ptr_cb = ptr_cb->ptr_next_thread)
    {
        if (*ptr_thread_id != (ptr_cb->thread_id))
        {
            ptr_cb_prev = ptr_cb;
        }
        else
        {
            /* remove the current thread cb out of the link list */
            if (ptr_cb == _ptr_osal_thread_list_head)
            {
                _ptr_osal_thread_list_head = ptr_cb->ptr_next_thread;
            }
            else
            {
                ptr_cb_prev->ptr_next_thread = ptr_cb->ptr_next_thread;
            }
            rc = CLX_E_OK;
            break;
        }
    }
    pthread_mutex_unlock(&_osal_thread_cb_lock);

    return (rc);
}

static OSAL_THREAD_CB_T *
_osal_getCBByID(
    const CLX_THREAD_ID_T thread_id)
{
    OSAL_THREAD_CB_T *ptr_tmp = NULL;

    pthread_mutex_lock(&_osal_thread_cb_lock);
    ptr_tmp =_ptr_osal_thread_list_head;
    while (NULL != ptr_tmp)
    {
        if (ptr_tmp->thread_id == thread_id)
        {
            break;
        }
        ptr_tmp = ptr_tmp->ptr_next_thread;
    }
    pthread_mutex_unlock(&_osal_thread_cb_lock);
    return ptr_tmp;
}

static CLX_ERROR_NO_T
_osal_chkMemSizeRange(
    const C8_T                  *ptr_fname,
    const UI32_T                size)
{
#if defined OSAL_EN_MEM_CHK
    if (0 == size)
    {
        OSAL_LOG_ERR("%s: size is 0.\n", ptr_fname);
        return (CLX_E_OTHERS);
    }
#endif

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_chkMemPtrRange(
    const C8_T                  *ptr_fname,
    const void                  *ptr_mem,
    const UI32_T                len)
{
#if defined OSAL_EN_MEM_CHK
    if (NULL == ptr_mem)
    {
        OSAL_LOG_ERR("%s: NULL pointer.\n", ptr_fname);
        return (CLX_E_OTHERS);
    }
    else if ((CLX_HUGE_T)ptr_mem > (CLX_HUGE_T)(-1) - len)
    {
        OSAL_LOG_ERR("%s: ptr_mem is %p, len is %d out of virtual memory range.\n", ptr_fname, ptr_mem, len);
        return (CLX_E_OTHERS);
    }
#endif

    return (CLX_E_OK);
}

/* EXPORTED SUBPROGRAM BODIES
 */

/* FUNCTION NAME: osal_setLogFlag
 * PURPOSE:
 *      set log flag
 * INPUT:
 *      enable  -- if enable log flag
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
void
osal_setLogFlag(
    const UI32_T    log_flag,
    const BOOL_T    enable)
{
    if (TRUE == enable)
    {
        _osal_log_flag |= log_flag;
    }
    else
    {
        _osal_log_flag &= ~log_flag;
    }
}

/* FUNCTION NAME: osal_getLogFlag
 * PURPOSE:
 *      get log flag
 * INPUT:
 *      None.
 * OUTPUT:
 *      None.
 * RETURN:
 *      osal log flag
 *
 * NOTES:
 *
 */
UI32_T
osal_getLogFlag(
    void)
{
    return _osal_log_flag;
}

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
    void)
{
    CLX_TIME_T          time = 0;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    if (TRUE == OSAL_IS_INITED())
    {
        return (CLX_E_OK);
    }

    _ptr_osal_thread_list_head = NULL;

    /* Set random seed. */
    rc = osal_getTime(&time);
    if (CLX_E_OK != rc)
    {
        OSAL_LOG_ERR("osal_init random seed fail.\n");
    }
    else
    {
        osal_srand(time);
        OSAL_IS_INITED() = TRUE;
    }

    return (rc);
}

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
osal_deinit(void)
{
    OSAL_THREAD_CB_T    *ptr_thread = _ptr_osal_thread_list_head;

    if (FALSE == OSAL_IS_INITED())
    {
        return (CLX_E_OK);
    }

    while (NULL != ptr_thread)
    {
        OSAL_PRINT_ERR("Find a thread not destroyed: %s\n",
                       ptr_thread->thread_name);

        ptr_thread = ptr_thread->ptr_next_thread;
    }

    OSAL_IS_INITED() = FALSE;

    return (CLX_E_OK);
}

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
    const UI32_T    size)
{
    OSAL_MEM_CB_T   *ptr_mem_cb = NULL;
    UI8_T           *ptr_mem    = NULL;
    UI32_T          align_size  = 0;
    UI32_T          alloc_len   = 0;

    if (CLX_E_OK == OSAL_CHECK_MEM_SIZE(size))
    {
        /* to let the signature locate at 4-byte-aligned addr */
        align_size = ((size + 3) / 4) * 4;

        alloc_len = sizeof(OSAL_MEM_CB_T) + align_size + sizeof(OSAL_MEM_END_SIGNATURE);

        ptr_mem_cb = (OSAL_MEM_CB_T *)malloc(alloc_len);

        /* Update the memory control block  */
        if (NULL != ptr_mem_cb)
        {
            ptr_mem_cb->cookie_type = OSAL_MEM_COOKIE_MALLOC;
            ptr_mem_cb->signature = OSAL_MEM_BEGIN_SIGNATURE;
            ptr_mem_cb->real_len = align_size;

            /* update END_SIGNATURE at (ptr_mem_cb + alloc_len) */
            ptr_mem  = (UI8_T *)ptr_mem_cb->data;
            ptr_mem += align_size;
            (*((UI32_T *)(ptr_mem)))= (UI32_T)OSAL_MEM_END_SIGNATURE;
        }
    }

    if (NULL == ptr_mem_cb)
    {
        return (NULL);
    }
    else
    {
        return (ptr_mem_cb->data);
    }
}

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
    const void      *ptr_mem)
{
    OSAL_MEM_CB_T   *ptr_mem_cb  = NULL;
    UI8_T           *ptr_mem_end = NULL;
    UI32_T          size = 0;
    CLX_ERROR_NO_T  rc = CLX_E_OTHERS;

    rc = OSAL_CHECK_MEM_PTR((void *)ptr_mem, 0);

    if (CLX_E_OK == rc)
    {
        /* Calculate address of the begin signature. */
        ptr_mem_cb = (OSAL_MEM_CB_T *)(ptr_mem - sizeof(OSAL_MEM_CB_T));

        if (OSAL_MEM_BEGIN_SIGNATURE != ptr_mem_cb->signature)
        {
            OSAL_LOG_ERR("osal_free: begin signature error, there may be out of range writing\n");
            rc = CLX_E_OTHERS;
        }
        else
        {
            size = ptr_mem_cb->real_len;
            ptr_mem_end = (UI8_T *)ptr_mem;
            ptr_mem_end += size;
            if ((UI32_T)OSAL_MEM_END_SIGNATURE != (*((UI32_T *)(ptr_mem_end))))
            {
                OSAL_LOG_ERR("osal_free: end signature error, there may be out of range writing\n");
                rc = CLX_E_OTHERS;
            }
        }
    }

    if (CLX_E_OK == rc)
    {
        switch (ptr_mem_cb->cookie_type)
        {
            case  OSAL_MEM_COOKIE_MALLOC:
                free(ptr_mem_cb);
                break;

            default:
                OSAL_LOG_ERR("osal_free: Illegal Memory cookie");
                break;
        }
    }
}

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
    void)
{
    CLX_THREAD_ID_T thread_id = pthread_self();
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); /* DO NOT use PTHREAD_CANCEL_ASYNCHRONOUS */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (NULL == _osal_getCBByID(thread_id))
    {
        usleep(1);
    }
}

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
osal_createThread(
    const C8_T          *ptr_thread_name,
    const UI32_T        stack_size,
    const UI32_T        priority,
    void                (function)(void*),
    void                *ptr_arg,
    CLX_THREAD_ID_T     *ptr_thread_id)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    C8_T                tmp_thread_name[OSAL_THREAD_NAME_LEN + 1] = OSAL_THREAD_DFT_NAME;
    OSAL_THREAD_CB_T    *ptr_thread_cb = NULL;
    LINUX_THREAD_T      thread_id;
    LINUX_THREAD_ATTR_T thread_attr;
    LINUX_SCHED_PARAM_T thread_param;
    UI32_T              thread_stack_size = 0;
    UI32_T              thread_priority = 0;

    HAL_CHECK_PTR(function);
    HAL_CHECK_PTR(ptr_thread_id);

    /* Process the thread name. */
    if ((NULL != ptr_thread_name) && (0 != osal_strlen(ptr_thread_name)))
    {
        osal_strncpy(tmp_thread_name, ptr_thread_name, OSAL_THREAD_NAME_LEN);
        tmp_thread_name[OSAL_THREAD_NAME_LEN] = '\0';
    }

    /* Initialize the attribute structure. */
    if (pthread_attr_init(&thread_attr))
    {
        rc = (CLX_E_OTHERS);
    }
    else
    {
        /* Set stack size. */
        thread_stack_size = stack_size;
        pthread_attr_setstacksize(&thread_attr, thread_stack_size);

        /* Set the schedule policy and real-time priority. */
        pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        thread_priority = (priority <= OSAL_THREAD_HIGHEST_PRIORITY)?
                           priority : OSAL_THREAD_HIGHEST_PRIORITY;
        thread_param.sched_priority = thread_priority;
        pthread_attr_setschedparam(&thread_attr, &thread_param);

        /* If you want to use the current scheduling policy, try this:
         * (notice: set PTHREAD_EXPLICIT_SCHED need root permission)
         * pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
         */

        /* If you want to set the thread detach state (i.e. without pthread_join()), try this:
         * pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
         */

        /* Memory allocate for the thread control block. */
        ptr_thread_cb = (OSAL_THREAD_CB_T *)osal_alloc(sizeof(OSAL_THREAD_CB_T));
        if (NULL == ptr_thread_cb)
        {
            OSAL_LOG_ERR("osal_createThread alloc fail\n");
            return (CLX_E_NO_MEMORY);
        }

        /* Create pthread through Linux API and check its result. */
        if (pthread_create(&thread_id, &thread_attr, (void *(*)(void *))function, (void *)ptr_arg))
        {
            osal_free(ptr_thread_cb);
            OSAL_LOG_ERR("osal_createThread create fail\n");
            return (CLX_E_OTHERS);
        }

        /* Fill up the control block. */
        *ptr_thread_id = (CLX_THREAD_ID_T)thread_id;
        ptr_thread_cb->thread_id = (CLX_THREAD_ID_T)thread_id;
        osal_strncpy(ptr_thread_cb->thread_name, tmp_thread_name, OSAL_THREAD_NAME_LEN);
        ptr_thread_cb->thread_name[OSAL_THREAD_NAME_LEN] = '\0';
        ptr_thread_cb->priority = priority;
        ptr_thread_cb->stack_size = stack_size;
        ptr_thread_cb->is_stop = FALSE;

        /* Chain the control block. */
        _osal_chainThread(ptr_thread_cb);
    }
    return (rc);
}

/* FUNCTION NAME:  osal_stopThread
 * PURPOSE:
 *      OS abstration API to stop thread.
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
    CLX_THREAD_ID_T  *ptr_thread_id)
{
    OSAL_THREAD_CB_T    *ptr_thread_cb = NULL;

    HAL_CHECK_PTR(ptr_thread_id);

    ptr_thread_cb = _osal_getCBByID(*ptr_thread_id);
    if (NULL == ptr_thread_cb)
    {
        return (CLX_E_ENTRY_NOT_FOUND);
    }
    ptr_thread_cb->is_stop = TRUE;

    return (CLX_E_OK);
}

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
    CLX_THREAD_ID_T     *ptr_thread_id)
{
    OSAL_THREAD_CB_T    *ptr_thread_cb = NULL;

    HAL_CHECK_PTR(ptr_thread_id);

    ptr_thread_cb = _osal_getCBByID(*ptr_thread_id);
    if (NULL == ptr_thread_cb)
    {
        return (CLX_E_ENTRY_NOT_FOUND);
    }

    /* wait the thread exit */
    if (pthread_join((pthread_t)(*ptr_thread_id), NULL))
    {
        OSAL_LOG_ERR("osal_destroyThread %s fail\n", ptr_thread_cb->thread_name);
        return (CLX_E_OTHERS);
    }
    _osal_unchainThread(ptr_thread_id);
    osal_free(ptr_thread_cb);
    *ptr_thread_id = 0;

    return (CLX_E_OK);
}

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
    void)
{
    pthread_exit(NULL);
}

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
    void)
{
    OSAL_THREAD_CB_T    *ptr_thread_cb = NULL;

    ptr_thread_cb = _osal_getCBByID((CLX_THREAD_ID_T)pthread_self());
    if (NULL == ptr_thread_cb)
    {
        return (CLX_E_ENTRY_NOT_FOUND);
    }

    if (TRUE == ptr_thread_cb->is_stop)
    {
        return (CLX_E_OTHERS);
    }
    else
    {
        return (CLX_E_OK);
    }
}

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
    const UI32_T        usecond)
{
    LINUX_TIMEVAL_T     time_out;
    UI32_T              time_out_usec;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    if (0 != usecond)
    {
        time_out_usec = usecond;
        time_out.tv_sec = (time_t) (time_out_usec / OSAL_US_PER_SECOND);
        time_out.tv_usec = (suseconds_t) (time_out_usec % OSAL_US_PER_SECOND);
        if (0 != select(0, NULL, NULL, NULL, &time_out))
        {
            rc = CLX_E_OTHERS;
        }
    }

    return (rc);
}

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
    const UI32_T        usecond)
{
    CLX_TIME_T          cur_time;
    CLX_TIME_T          init_time;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    if (0 != usecond)
    {
        osal_getTime(&init_time);
        while (1)
        {
            osal_getTime(&cur_time);

            if (cur_time == init_time)
                continue;

            if (cur_time > init_time)
            {
                /* normal case */
                if ((cur_time - init_time) >= usecond)
                {
                    /* wait timeout */
                    break;
                }
            }
            else
            {
                /* wrap case */
                if (((0xFFFFFFFF - init_time) + cur_time) >= usecond)
                {
                    /* wait timeout */
                    break;
                }
            }
        }
    }

    return (rc);
}

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
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id)
{
    OSAL_SEMA_CB_T      *ptr_sema_cb = NULL;
    C8_T                tmp_sema_name[OSAL_SEMA_NAME_LEN + 1] = OSAL_SEMA_DFT_NAME;

    HAL_CHECK_MIN_MAX_RANGE(sema_count, 0, OSAL_SEMA_MAX_CNT);
    HAL_CHECK_PTR(ptr_semaphore_id);

    /* Process the semaphore name. */
    if ((NULL != ptr_sema_name) && (0 != osal_strlen(ptr_sema_name)))
    {
        osal_strncpy(tmp_sema_name, ptr_sema_name, OSAL_SEMA_NAME_LEN);
        tmp_sema_name[OSAL_SEMA_NAME_LEN] = '\0';
    }

    /* Memory allocate for the semaphore control block. */
    ptr_sema_cb = (OSAL_SEMA_CB_T *)osal_alloc(sizeof(OSAL_SEMA_CB_T));
    if (NULL == ptr_sema_cb)
    {
        OSAL_LOG_ERR("osal_createSemaphore: alloc fail\n");
        return (CLX_E_NO_MEMORY);
    }

    /* Init the semaphore. */
    memset(ptr_sema_cb, 0, sizeof(*ptr_sema_cb));
    sem_init(&ptr_sema_cb->sema, 0, sema_count);
    osal_strncpy(ptr_sema_cb->sema_name, tmp_sema_name, OSAL_SEMA_NAME_LEN);
    ptr_sema_cb->sema_name[OSAL_SEMA_NAME_LEN] = '\0';
    *ptr_semaphore_id = (CLX_SEMAPHORE_ID_T)ptr_sema_cb;

    return (CLX_E_OK);
}

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
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id)
{
    OSAL_SEMA_CB_T      *ptr_sema_cb = NULL;

    HAL_CHECK_PTR(ptr_semaphore_id);
    ptr_sema_cb = (OSAL_SEMA_CB_T *)*ptr_semaphore_id;
    HAL_CHECK_PTR(ptr_sema_cb);

    sem_destroy(&ptr_sema_cb->sema);
    osal_free(ptr_sema_cb);
    *ptr_semaphore_id = 0;

    return (CLX_E_OK);
}

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
    UI32_T              time_out)
{
    OSAL_SEMA_CB_T      *ptr_sema_cb = NULL;
    LINUX_TIMEVAL_T     time;
    struct timespec     max_wait;

    HAL_CHECK_PTR(ptr_semaphore_id);
    ptr_sema_cb = (OSAL_SEMA_CB_T *)(* ptr_semaphore_id);
    HAL_CHECK_PTR(ptr_sema_cb);

    /* Wait Semaphore. */
    if (CLX_SEMAPHORE_WAIT_FOREVER == time_out)
    {
        if (0 != sem_wait(&ptr_sema_cb->sema))
        {
            return (CLX_E_OTHERS);
        }
    }
    else
    {
        /* Get timeout. */
        gettimeofday(&time, NULL);
        time.tv_sec  += (time_out / OSAL_US_PER_SECOND);
        time.tv_usec += (time_out % OSAL_US_PER_SECOND);
        if (time.tv_usec >= OSAL_US_PER_SECOND)
        {
            time.tv_usec -= OSAL_US_PER_SECOND;
            time.tv_sec++;
        }
        max_wait.tv_sec  = time.tv_sec;
        max_wait.tv_nsec = time.tv_usec * OSAL_NS_PER_USECOND;

        if (0 != sem_timedwait(&ptr_sema_cb->sema, &max_wait))
        {
            return (CLX_E_OTHERS);
        }
    }

    return (CLX_E_OK);
}

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
    CLX_SEMAPHORE_ID_T  *ptr_semaphore_id)
{
    OSAL_SEMA_CB_T      *ptr_sema_cb = NULL;

    HAL_CHECK_PTR(ptr_semaphore_id);
    ptr_sema_cb = (OSAL_SEMA_CB_T *)(*ptr_semaphore_id);
    HAL_CHECK_PTR(ptr_sema_cb);

    sem_post(&ptr_sema_cb->sema);

    return (CLX_E_OK);
}

/* FUNCTION NAME:  osal_createIsrLock
 * PURPOSE:
 *      OS abstration API to create IsrLock.
 * INPUT:
 *      ptr_isrlock_name -- pointer to the string of IsrLock name
 * OUTPUT:
 *      ptr_isrlock_id -- Pointer to IsrLock ID
 * RETURN:
 *      CLX_E_OK      -- Successfully create IsrLock.
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
    CLX_ISRLOCK_ID_T    *ptr_isrlock_id)
{
    return osal_createSemaphore(ptr_isrlock_name, 1, (CLX_SEMAPHORE_ID_T *)ptr_isrlock_id);
}

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
    CLX_ISRLOCK_ID_T    *ptr_isrlock_id)
{
    return osal_destroySemaphore((CLX_SEMAPHORE_ID_T *) ptr_isrlock_id);
}

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
    CLX_IRQ_FLAGS_T     *ptr_irq_flags)
{
    return osal_takeSemaphore((CLX_SEMAPHORE_ID_T *) ptr_isrlock_id, CLX_SEMAPHORE_WAIT_FOREVER);
}

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
    CLX_IRQ_FLAGS_T     *ptr_irq_flags)
{
    return osal_giveSemaphore((CLX_SEMAPHORE_ID_T *) ptr_isrlock_id);
}

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
    CLX_TIME_T          *ptr_time)
{
    LINUX_TIMEVAL_T     usec_time;
    UI64_T              sec_to_usec;

    HAL_CHECK_PTR(ptr_time);

    gettimeofday(&usec_time, NULL);
    UI64_ASSIGN(sec_to_usec, 0, usec_time.tv_sec);
    UI64_MULT_UI32(sec_to_usec, OSAL_US_PER_SECOND);
    UI64_ADD_UI32(sec_to_usec, usec_time.tv_usec);

    /* give the lower 32-bit usec to user */
    *(CLX_TIME_T *)ptr_time = UI64_LOW(sec_to_usec);

    return (CLX_E_OK);
}

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
    OSAL_TM_T           *ptr_tm)
{
    LINUX_TIME_T        cur_time;
    LINUX_TM_T          *cur_tm_time;

    HAL_CHECK_PTR(ptr_tm);
    time(&cur_time);
    cur_tm_time   = localtime(&cur_time);
    ptr_tm->year  = cur_tm_time->tm_year + OSAL_TIME_YEAR_OFFSET;
    ptr_tm->month = cur_tm_time->tm_mon + 1;
    ptr_tm->day   = cur_tm_time->tm_mday;
    ptr_tm->hour  = cur_tm_time->tm_hour;
    ptr_tm->min   = cur_tm_time->tm_min;
    ptr_tm->sec   = cur_tm_time->tm_sec;

    return (CLX_E_OK);
}
