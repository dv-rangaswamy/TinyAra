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

/// @file task.c
/// @brief Test Case Example for Task API

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <tinyara/sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "tc_internal.h"

#define TEST_STRING     "test"
#define ONEXIT_VAL      123
#define SEC_1           1
#define SEC_2           2
#define USEC_10         10

static int g_callback;
#ifndef CONFIG_BUILD_PROTECTED
static volatile int vfork_val;
static volatile int task_cnt;
pid_t existed_pid[CONFIG_MAX_TASKS];
static volatile int vfork_cnt;
static volatile pid_t ppid;
static volatile int existed_pid_idx;
#endif

/**
* @fn                   :create_task
* @brief                :utility function for tc_task_create
* @return               :int
*/
static int create_task(int argc, char *argv[])
{
	if (strcmp(argv[1], TEST_STRING) == 0) {
		g_callback = OK;
	}

	return OK;
}

/**
* @fn                   :delete_task
* @brief                :utility function for tc_task_task_delete
* @return               :int
*/
static int delete_task(int argc, char *argv[])
{
	g_callback = OK;
	task_delete(getpid());

	/* this sleep is for entering callcellation point */

	sleep(SEC_1);
	g_callback = ERROR;
	return OK;
}

/**
* @fn                   :restart_task
* @brief                :utility function for tc_task_task_restart
* @return               :int
*/
static int restart_task(int argc, char *argv[])
{
	g_callback++;
	sleep(SEC_1);
	return OK;
}

/**
* @fn                   :exit_task function
* @brief                :utility function for tc_task_exit
* @return               :void
*/
static int exit_task(int argc, char *argv[])
{
	g_callback = OK;
	exit(OK);
	g_callback = ERROR;
	return OK;
}

/**
* @fn                   :fn_atexit
* @brief                :utility function for tc_task_atexit
* @return               :void
*/
static void fn_atexit(void)
{
	g_callback = OK;
}

/**
* @fn                   :atexit_task
* @brief                :utility function for tc_task_atexit
* @return               :int
*/
static int atexit_task(int argc, char *argv[])
{
	atexit(fn_atexit);
	return OK;
}

/**
* @fn                   :fn_onExit
* @brief                :utility function for tc_task_on_exit
* @return               :void
*/
static void fn_onexit(int status, void *arg)
{
	if (status != ONEXIT_VAL || strcmp((char *)arg, TEST_STRING) != 0) {
		return;
	}

	g_callback = OK;
}

/**
* @fn                   :onexit_task
* @brief                :utility function for tc_task_on_exit
* @return               :int
*/
static int onexit_task(int argc, char *argv[])
{
	if (on_exit(fn_onexit, TEST_STRING) != OK) {
		return ERROR;
	}

	exit(ONEXIT_VAL);
	return OK;
}

/**
* @fn                   :getpid_task
* @brief                :utility function for tc_task_getpid
* @return               :int
*/
static int getpid_task(int argc, char *argv[])
{
	g_callback = (int)getpid();
	return OK;
}

#ifndef CONFIG_BUILD_PROTECTED
/**
* @fn                   :task_cnt_func
* @brief                :handler of sched_foreach for counting the alive tasks and saving the pids
* @return               :void
*/
static void task_cnt_func(struct tcb_s *tcb, void *arg)
{
	existed_pid[task_cnt] = tcb->pid;
	task_cnt++;
}

/**
* @fn                   :vfork_task function
* @brief                :utility function for tc_task_vfork
* @return               :void
*/
static int vfork_task(int argc, char *argv[])
{
	pid_t pid;
	int repeat_criteria;
	int repeat_iter;

	/* initialize the task_cnt and existed_pid arr */
	task_cnt = vfork_val = 0;
	for (existed_pid_idx = 0; existed_pid_idx < CONFIG_MAX_TASKS; existed_pid_idx++) {
		existed_pid[existed_pid_idx] = -1;
	}

	ppid = getpid();

	sched_foreach(task_cnt_func, NULL);
	existed_pid_idx = task_cnt;

	for (vfork_cnt = 0; vfork_cnt < (CONFIG_MAX_TASKS - task_cnt + 1); vfork_cnt++) {
		pid = vfork();
		if (pid == 0) {
			vfork_val++;
			/* check that created pid is repeated or not */
			if (existed_pid[existed_pid_idx] != -1) {
				g_callback = ERROR;
				exit(OK);
			} else {
				existed_pid[existed_pid_idx++] = getpid();
			}
		} else if (pid < 0) {
			if (vfork_val >= (CONFIG_MAX_TASKS - task_cnt) && errno == EPERM) {
				/* the num of tasks is full, and the errno is set to EPERM */
				break;
			}
			g_callback = ERROR;
		}
	}

	while (vfork_val != CONFIG_MAX_TASKS - task_cnt) {
		usleep(USEC_10);
	}

	if (getpid() != ppid) {
		exit(OK);
	}

	/* check all pids in existed_pid arr whether repeated or not */
	for (repeat_criteria = 0; repeat_criteria < CONFIG_MAX_TASKS; repeat_criteria++) {
		for (repeat_iter = repeat_criteria + 1; repeat_iter < CONFIG_MAX_TASKS; repeat_iter++) {
			if (existed_pid[repeat_criteria] == existed_pid[repeat_iter]) {
				g_callback = ERROR;
				break;
			}
		}
		if (g_callback == ERROR) {
			break;
		}
	}

	return OK;
}
#endif

/**
* @fn                   :tc_task_task_create
* @brief                :Creates a task with the arguments provided like taskname,Priority,stacksize and function name
* @Scenario             :Creates a task with the arguments provided like taskname,Priority,stacksize and function name
* API's covered         :task_create
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_task_task_create(void)
{
	int pid;
	const char *task_param[2] = { TEST_STRING, NULL };
	g_callback = ERROR;
	pid = task_create("tc_task_create", SCHED_PRIORITY_MAX - 1, 1024, create_task, (char * const *)task_param);
	TC_ASSERT_GT("task_create", pid, 0);
	TC_ASSERT_EQ("task_create", g_callback, OK);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_task_delete
* @brief                :Delete a task
* @Scenario             :Delete a task
* API's covered         :task_create, task_delete
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_task_task_delete(void)
{
	int pid;
	int ret_chk;
	g_callback = ERROR;
	pid = task_create("tc_task_del", SCHED_PRIORITY_MAX - 1, 1024, delete_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);

	ret_chk = task_delete(pid);
	TC_ASSERT_LT("task_delete", ret_chk, 0);
	TC_ASSERT_EQ("task_delete", g_callback, OK);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_task_restart
* @brief                :Restart a task
* @Scenario             :Restart a task
* API's covered         :task_restart
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_task_task_restart(void)
{
	int pid;
	int ret_chk;
	unsigned int remain;
	g_callback = 0;
	pid = task_create("tc_task_re", SCHED_PRIORITY_MAX - 1, 1024, restart_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);

	ret_chk = task_restart(pid);

	/* wait for terminating restart_task */

	remain = sleep(SEC_2);
	while (remain > 0) {
		remain = sleep(remain);
	}

	TC_ASSERT_EQ("task_restart", ret_chk, 0);

	/* g_icounter shall be increment when do start and restart operation */

	TC_ASSERT_EQ("task_restart", g_callback, 2);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_exit
* @brief                :exit function will terminate the task
* @Scenario             :make a task which calls exit(). after calling exit,
*                       that task cannot do anything such as printf
* API's covered         :on_exit
* Preconditions         :task_create
* Postconditions        :none
* @return               :void
*/
static void tc_task_exit(void)
{
	int pid;
	g_callback = ERROR;
	pid = task_create("tc_exit", SCHED_PRIORITY_MAX - 1, 1024, exit_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);
	TC_ASSERT_EQ("task_exit", g_callback, OK);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_atexit
* @brief                :Register a function to be called at normal process termination
* @Scenario             :Register a function to be called at normal process termination
* API's covered         :atexit
* Preconditions         :none
* Postconditions        :none
* @return               :return SUCCESS on success
*/
static void tc_task_atexit(void)
{
	int pid;
	g_callback = ERROR;
	pid = task_create("tc_atexit", SCHED_PRIORITY_MAX - 1, 1024, atexit_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);
	TC_ASSERT_EQ("task_atexit", g_callback, OK);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_on_exit
* @brief                :on_exit() function registers the given function to be called\
*                        at normal process termination
* @Scenario             :The function is passed the status argument\
*                        given to the last call to exit and the arg argument from on_exit().
* API's covered         :on_exit
* Preconditions         :task_create
* Postconditions        :none
* @return               :void
*/
static void tc_task_on_exit(void)
{
	int pid;
	g_callback = ERROR;

	pid = task_create("tc_on_exit", SCHED_PRIORITY_MAX - 1, 1024, onexit_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);
	TC_ASSERT_EQ("on_exit", g_callback, OK);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_prctl
* @brief                :functions will set and get the process name and then compare the fetched\
*                        name with the set name child task, constructed from a regular executable file.
* @Scenario             :functions will set and get the process name and then compare the fetched\
*                        name with the set name child task, constructed from a regular executable file.
* API's covered         :prctl
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_task_prctl(void)
{
	int ret_chk;
	const char *setname = "a Test program";
	char getname[CONFIG_TASK_NAME_SIZE + 1];
	char oldname[CONFIG_TASK_NAME_SIZE + 1];

	/* save taskname */

	ret_chk = prctl(PR_GET_NAME, (unsigned long)oldname, 0, 0, 0);
	TC_ASSERT_EQ("prctl", ret_chk, OK);

	/* set taskname */

	ret_chk = prctl(PR_SET_NAME, (unsigned long)setname, 0, 0, 0);
	TC_ASSERT_EQ("prctl", ret_chk, OK);

	/* get taskname */

	ret_chk = prctl(PR_GET_NAME, (unsigned long)getname, 0, 0, 0);
	TC_ASSERT_EQ_CLEANUP("prctl", ret_chk, OK, get_errno(), prctl(PR_SET_NAME, (unsigned long)oldname, 0, 0, 0));

	/* compare getname and setname */

	TC_ASSERT_EQ_CLEANUP("prctl", strncmp(getname, setname, CONFIG_TASK_NAME_SIZE), 0, get_errno(), prctl(PR_SET_NAME, (unsigned long)oldname, 0, 0, 0));

	prctl(PR_SET_NAME, (unsigned long)oldname, 0, 0, 0);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_task_getpid
* @brief                :Returns the process ID of the calling process
* @Scenario             :Returns the process ID of the calling process
* API's covered         :getpid
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_task_getpid(void)
{
	int pid;
	g_callback = ERROR;
	pid = task_create("tc_getpid", SCHED_PRIORITY_MAX - 1, 1024, getpid_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);
	TC_ASSERT_EQ("getpid", pid, g_callback);
	TC_SUCCESS_RESULT();
}

#ifndef CONFIG_BUILD_PROTECTED
/**
* @fn                   :tc_task_vfork
* @brief                :make children through vfork
* @Scenario             :make children above CONFIG_MAX_TASKS through vfork, and check the return value
*                       and check all pids are different of not
* API's covered         :vfork
* Preconditions         :N/A
* Postconditions        :none
* @return               :void
*/
static void tc_task_vfork(void)
{
	int pid;
	g_callback = OK;
	pid = task_create("tc_vfork", SCHED_PRIORITY_MAX - 1, 1024, vfork_task, (char * const *)NULL);
	TC_ASSERT_GT("task_create", pid, 0);
	TC_ASSERT_EQ("task_restart", g_callback, OK);

	TC_SUCCESS_RESULT();
}
#endif
/****************************************************************************
 * Name: task
 ****************************************************************************/
int task_main(void)
{
	tc_task_task_create();
	tc_task_task_delete();
	tc_task_task_restart();
	tc_task_exit();
	tc_task_atexit();
	tc_task_on_exit();
	tc_task_prctl();
	tc_task_getpid();
#if defined(CONFIG_ARCH_HAVE_VFORK) && defined(CONFIG_SCHED_WAITPID) && !defined(CONFIG_BUILD_PROTECTED)
	tc_task_vfork();
#endif
	return 0;
}
