/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/********************************************************************************
 * include/sched.h
 *
 *   Copyright (C) 2007-2009, 2011, 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ********************************************************************************/
/**
 * @defgroup SCHED_KERNEL SCHED
 * @ingroup KERNEL
 */

/// @file sched.h
/// @brief Sched APIs

#ifndef __INCLUDE_SCHED_H
#define __INCLUDE_SCHED_H

/********************************************************************************
 * Included Files
 ********************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <tinyara/sched.h>

/********************************************************************************
 * Pre-processor Definitions
 ********************************************************************************/

/* Task Management Definitions **************************************************/

/**
 * @ingroup SCHED_KERNEL
 * @{
 */
/* POSIX-like scheduling policies */

#define SCHED_FIFO     1		/* FIFO per priority scheduling policy */
#define SCHED_RR       2		/* Round robin scheduling policy */
#define SCHED_SPORADIC 3		/* Not supported */
#define SCHED_OTHER    4		/* Not supported */

/* Pthread definitions **********************************************************/

#define PTHREAD_KEYS_MAX CONFIG_NPTHREAD_KEYS

/**
 * @} */
/********************************************************************************
 * Public Type Definitions
 ********************************************************************************/

/**
 * @ingroup SCHED_KERNEL
 * @brief  POSIX-like scheduling parameter structure
 */
struct sched_param {
	int sched_priority;
};

/********************************************************************************
 * Public Data
 ********************************************************************************/

#ifndef __ASSEMBLY__
#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/********************************************************************************
 * Public Function Prototypes
 ********************************************************************************/

/* Task Control Interfaces (non-standard) */
/**
 * @cond
 * @internal
 */
int task_init(FAR struct tcb_s *tcb, const char *name, int priority, FAR uint32_t *stack, uint32_t stack_size, main_t entry, FAR char *const argv[]);
/**
 * @internal
 */
int task_activate(FAR struct tcb_s *tcb);

/**
 * @endcond
 */

#ifndef CONFIG_BUILD_KERNEL
/**
 * @ingroup SCHED_KERNEL
 * @brief  creates and activates a new task with a specified
 *   priority and returns its system-assigned ID.
 * @details [SYSTEM CALL API]
 *       The entry address entry is the address of the "main" function of the
 *   task.  This function will be called once the C environment has been
 *   set up.  The specified function will be called with four arguments.
 *   Should the specified routine return, a call to exit() will
 *   automatically be made.
 *
 * @param[in] Name of the new task
 * @param[in] Priority of the new task
 * @param[in] size (in bytes) of the stack needed
 * @param[in] Entry point of a new task
 * @param[in]  A pointer to an array of input parameters. Up to
 *                CONFIG_MAX_TASK_ARG parameters may be provided.  If fewer
 *                than CONFIG_MAX_TASK_ARG parameters are passed, the list
 *                should be terminated with a NULL argv[] value. If no
 *                parameters are required, argv may be NULL.
 * @return non-zero process ID of the new task or ERROR if memory is
 *   insufficient or the task cannot be created.  The errno will be set to
 *   indicate the nature of the error (always ENOMEM).
 * @since Tizen RT v1.0
 */
int task_create(FAR const char *name, int priority, int stack_size, main_t entry, FAR char *const argv[]);
#endif
/**
 * @ingroup SCHED_KERNEL
 * @brief  causes a specified task to cease to exist.
 * @details [SYSTEM CALL API]
 *      Its  stack and TCB will be deallocated.  This function is
 *   the companion to task_create().
 *   This is the version of the function exposed to the user; it is simply
 *   a wrapper around the internal, task_terminate function.
 *
 *   The logic in this function only deletes non-running tasks.  If the 'pid'
 *   parameter refers to to the currently runing task, then processing is
 *   redirected to exit().  This can only happen if a task calls task_delete()
 *   in order to delete itself.
 *
 *   In fact, this function (and task_terminate) are the final functions
 *   called all task termination sequences.  task_delete may be called
 *   from:
 *
 *   - task_restart(),
 *   - pthread_cancel(),
 *   - and directly from user code.
 *
 *   Other exit paths (exit(), _eixt(), and pthread_exit()) will go through
 *   task_terminate()
 *
 * @param[in] The task ID of the task to delete.  A pid of zero
 *         signifies the calling task.
 * @return OK on success; or ERROR on failure
 * @since Tizen RT v1.0
 */
int task_delete(pid_t pid);

/**
 * @ingroup SCHED_KERNEL
 * @brief  restart a task.
 * @details [SYSTEM CALL API]
 *     The task is first terminated and then
 *   reinitialized with same ID, priority, original entry point, stack size,
 *   and parameters it had when it was first started.
 *
 *   This function can fail if:
 *   (1) A pid of zero or the pid of the calling task is provided
 *      (functionality not implemented)
 *   (2) The pid is not associated with any task known to the system.
 *
 * @param[in] The task ID of the task to delete.  An ID of zero signifies the
 *         calling task.
 * @return OK on success; or ERROR on failure
 * @since Tizen RT v1.0
 */
int task_restart(pid_t pid);

/* Task Scheduling Interfaces (based on POSIX APIs) */
/**
 * @ingroup SCHED_KERNEL
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @details [SYSTEM CALL API]
 * @since Tizen RT v1.0
 */
int sched_setparam(pid_t pid, const struct sched_param *param);
/**
 * @ingroup SCHED_KERNEL
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @details [SYSTEM CALL API]
 * @since Tizen RT v1.0
 */
int sched_getparam(pid_t pid, struct sched_param *param);
/**
 * @ingroup SCHED_KERNEL
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @details [SYSTEM CALL API]
 * @since Tizen RT v1.0
 */
int sched_setscheduler(pid_t pid, int policy, FAR const struct sched_param *param);
/**
 * @ingroup SCHED_KERNEL
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @details [SYSTEM CALL API]
 * @since Tizen RT v1.0
 */
int sched_getscheduler(pid_t pid);
/**
 * @ingroup SCHED_KERNEL
 * @details [SYSTEM CALL API]
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since Tizen RT v1.0
 */
int sched_yield(void);
/**
		  * @} *///end for SCHED_KERNEL

/**
 * @addtogroup SCHED_KERNEL
 * @{
 */
/**
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since Tizen RT v1.0
 */
int sched_get_priority_max(int policy);
/**
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since Tizen RT v1.0
 */
int sched_get_priority_min(int policy);
/**
* @} */
/**
 * @ingroup SCHED_KERNEL
 * @brief  POSIX APIs (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @details [SYSTEM CALL API]
 * @since Tizen RT v1.0
 */
int sched_rr_get_interval(pid_t pid, FAR struct timespec *interval);

/* Task Switching Interfaces (non-standard) */
/**
 * @ingroup SCHED_KERNEL
 * @brief disable context switching
 * @details [SYSTEM CALL API]
 * @return On success, OK is returned. On failure, ERROR is returned.
 * @since Tizen RT v1.0
 */
int sched_lock(void);
/**
 * @ingroup SCHED_KERNEL
 * @brief re-enable the context switching which blocked from sched_lock()
 * @details [SYSTEM CALL API]
 *     This function decrements the preemption lock count.  Typically this
 *   is paired with sched_lock() and concludes a critical section of
 *   code.  Preemption will not be unlocked until sched_unlock() has
 *   been called as many times as sched_lock().  When the lockcount is
 *   decremented to zero, any tasks that were eligible to preempt the
 *   current task will execute.
 * @return On success, OK is returned. On failure, ERROR is returned.
 * @since Tizen RT v1.0
 */
int sched_unlock(void);
/**
 * @ingroup SCHED_KERNEL
 * @brief returns the current value of the lockcount
 * @details [SYSTEM CALL API]
 *      This function returns the current value of the lockcount. If zero,
 *   pre-emption is enabled; if non-zero, this value indicates the number
 *   of times that sched_lock() has been called on this thread of
 *   execution.  sched_unlock() will have to called that many times from
 *   this thread in order to re-enable pre-emption.
 *
 * @return On success, lockcount is returned.
 * @since Tizen RT v1.0
 */
int sched_lockcount(void);

/* If instrumentation of the scheduler is enabled, then some outboard logic
 * must provide the following interfaces.
 */

#ifdef CONFIG_SCHED_INSTRUMENTATION
/**
 *@cond
 *@ internal
 */
void sched_note_start(FAR struct tcb_s *tcb);
/**
 *@ internal
 */
void sched_note_stop(FAR struct tcb_s *tcb);
/**
 *@ internal
 */
void sched_note_switch(FAR struct tcb_s *pFromTcb, FAR struct tcb_s *pToTcb);

/**
 *@endcond
 */

#else
#define sched_note_start(t)
#define sched_note_stop(t)
#define sched_note_switch(t1, t2)
#endif							/* CONFIG_SCHED_INSTRUMENTATION */

#undef EXTERN
#if defined(__cplusplus)
}
#endif
#endif							/* __ASSEMBLY__ */

#endif							/* __INCLUDE_SCHED_H */
