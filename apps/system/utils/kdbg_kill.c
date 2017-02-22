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
/****************************************************************************
 * apps/system/utils/kdbg_kill.c
 *
 *   Copyright (C) 2007-2009, 2011-2012, 2014 Gregory Nutt. All rights reserved.
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
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sched.h>
#include <pthread.h>

#if defined(CONFIG_ENABLE_KILLALL) && (CONFIG_TASK_NAME_SIZE <= 0)
#error If you want to use killall, CONFIG_TASK_NAME_SIZE should be > 0
#endif

struct kdbg_sig_s {
	const char *signame;
	int signo;
};

static const struct kdbg_sig_s kdbg_sig[] = {
#ifdef SIGUSR1
	{"SIGUSR1",         SIGUSR1},
#endif
#ifdef SIGUSR2
	{"SIGUSR2",         SIGUSR2},
#endif
#ifdef SIGALRM
	{"SIGALRM",         SIGALRM},
#endif
#ifdef SIGCHLD
	{"SIGCHLD",         SIGCHLD},
#endif
#ifdef SIGPOLL
	{"SIGPOLL",         SIGPOLL},
#endif
#ifdef SIGKILL
	{"SIGKILL",         SIGKILL},
#endif
#ifdef SIGCONDTIMEDOUT
	{"SIGCONDTIMEDOUT", SIGCONDTIMEDOUT},
#endif
#ifdef SIGWORK
	{"SIGWORK",         SIGWORK},
#endif
	{NULL,              0}
};

#if defined(CONFIG_ENABLE_KILLALL)
struct kdbg_killall_arg_s {
	int signo;
	char name[CONFIG_TASK_NAME_SIZE + 1];
	int count;
	int *ret;
};
#endif

static int find_signal(char *ptr)
{
	char *endptr;
	int ret = ERROR;
	int signo;
	int sigidx;

	/* Check incoming parameters.  The first parameter should be "-<signal>" */
	if (ptr == NULL) {
		return ERROR;
	} else if ('0' <= ptr[1] && ptr[1] <= '9') {
		/* Extract the signal number */

		signo = strtol(&ptr[1], &endptr, 0);
		for (sigidx = 0; kdbg_sig[sigidx].signo != 0; sigidx++) {
			if (kdbg_sig[sigidx].signo == signo) {
				ret = kdbg_sig[sigidx].signo;
				break;
			}
		}
	} else {
		/* Find the signal number using signal name */

		for (sigidx = 0; kdbg_sig[sigidx].signame != NULL; sigidx++) {
			if (strcasecmp(&ptr[1], kdbg_sig[sigidx].signame) == 0) {
				ret = kdbg_sig[sigidx].signo;
				break;
			}
		}
	}

	if (ret == ERROR) {
		printf("%s: invalid signal specification\n", ptr + 1);
	}

	return ret;
}

static int send_signal(pid_t pid, int signo)
{
	int ret = OK;
	FAR struct tcb_s *tcb;

	/* Send the signal.  Kill return values:
	 *
	 *   EINVAL An invalid signal was specified.
	 *   EPERM  The process does not have permission to send the signal to any
	 *          of the target processes.
	 *   ESRCH  The pid or process group does not exist.
	 *   ENOSYS Do not support sending signals to process groups.
	 */

	if (signo == SIGKILL) {
		tcb = sched_gettcb(pid);
		if (tcb == NULL) {
			set_errno(ESRCH);
			ret = ERROR;
		} else {
			switch ((tcb->flags & TCB_FLAG_TTYPE_MASK) >> TCB_FLAG_TTYPE_SHIFT) {
			case TCB_FLAG_TTYPE_TASK:
				ret = task_delete(pid) == OK ? OK : ERROR;
				break;
			case TCB_FLAG_TTYPE_PTHREAD:
			case TCB_FLAG_TTYPE_KERNEL:
				ret = pthread_cancel(pid) == OK ? OK : ERROR;
				pthread_join(pid, NULL);
				break;
			default:
				set_errno(EINVAL);
				ret = ERROR;
				break;
			}
		}
	} else if (kill(pid, signo) != OK) {
		ret = ERROR;
	}

	if (ret == ERROR) {
		printf("send_signal failed. errno : %d\n", get_errno());
	}

	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if defined(CONFIG_ENABLE_KILL)
int kdbg_kill(int argc, char **args)
{
	int signo;
	int sigidx;
	pid_t pid;
	char *ptr;
	char *endptr;

	ptr = args[1];
	if (*ptr != '-') {
		printf("Invalid argument\n");
		goto usage;
	} else if (ptr[1] == 'l' && ptr[2] == '\0') {
		/* List signal numbers and it's name */

		for (sigidx = 0; kdbg_sig[sigidx].signame != NULL; sigidx++) {
			printf("%2d) %-15s\n", kdbg_sig[sigidx].signo, kdbg_sig[sigidx].signame);
		}
		return OK;
	}

	if (argc != 3) {
		printf("Invalid argument\n");
		goto usage;
	}

	signo = find_signal(args[1]);
	if (signo == ERROR) {
		return ERROR;
	}

	/* The second parameter should be <pid>  */

	ptr = args[2];
	if (*ptr < '0' || *ptr > '9') {
		goto usage;
	}

	/* Extract the pid */

	pid = strtol(ptr, &endptr, 0);

	/* Send the signal */

	if (send_signal(pid, signo) != OK) {
		return ERROR;
	}

	return OK;

usage:
	printf("Usage: kill -[signum, signame] PID\n");
	printf("       kill -l\n");
	printf("Options:\n");
	printf("-l      list all signal names\n");

	return ERROR;
}
#endif

#if defined(CONFIG_ENABLE_KILLALL)
static void kdbg_killall_handler(FAR struct tcb_s *tcb, FAR void *arg)
{
	struct kdbg_killall_arg_s *killall_arg = (struct kdbg_killall_arg_s *)arg;

	if (strncmp(tcb->name, killall_arg->name, CONFIG_TASK_NAME_SIZE) == 0) {
		killall_arg->count++;
		if (send_signal(tcb->pid, killall_arg->signo) != OK) {
			*(killall_arg->ret) = ERROR;
		}
	}
}

int kdbg_killall(int argc, char **args)
{
	int ret = OK;
	int signo;
	int sigidx;
	char *ptr;
	struct kdbg_killall_arg_s arg;

	ptr = args[1];
	if (*ptr != '-') {
		printf("Invalid argument\n");
		goto usage;
	}

	if (ptr[1] == 'l' && ptr[2] == '\0') {
		/* List signal numbers and it's name */

		for (sigidx = 0; kdbg_sig[sigidx].signame != NULL; sigidx++) {
			printf("%2d) %-15s\n", kdbg_sig[sigidx].signo, kdbg_sig[sigidx].signame);
		}
		return OK;
	}

	if (argc != 3) {
		printf("Invalid argument\n");
		goto usage;
	}

	signo = find_signal(args[1]);
	if (signo == ERROR) {
		return ERROR;
	}

	arg.signo = signo;
	strncpy(arg.name, args[2], CONFIG_TASK_NAME_SIZE);
	arg.count = 0;
	arg.ret = &ret;

	sched_foreach(kdbg_killall_handler, (FAR void *)&arg);
	if (arg.count == 0) {
		printf("%s: no thread found\n", arg.name);
		ret = ERROR;
	}

	return ret;

usage:
	printf("Usage: killall -[signum, signame] NAME\n");
	printf("       killall -l\n");
	printf("Options:\n");
	printf("-l      list all signal names\n");

	return ERROR;
}
#endif
