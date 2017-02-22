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
 * drivers/pipes/pipe.c
 *
 *   Copyright (C) 2008-2009, 2015 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Compilation Switches
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <tinyara/fs/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

#include "pipe_common.h"

#if CONFIG_DEV_PIPE_SIZE > 0

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define MAX_PIPES 32

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int pipe_close(FAR struct file *filep);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations pipe_fops = {
	pipecommon_open,			/* open */
	pipe_close,					/* close */
	pipecommon_read,			/* read */
	pipecommon_write,			/* write */
	0,							/* seek */
	pipecommon_ioctl,			/* ioctl */
#ifndef CONFIG_DISABLE_POLL
	pipecommon_poll,			/* poll */
#endif
	pipecommon_unlink			/* unlink */
};

static sem_t g_pipesem = SEM_INITIALIZER(1);
static uint32_t g_pipeset = 0;
static uint32_t g_pipecreated = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pipe_allocate
 ****************************************************************************/

static inline int pipe_allocate(void)
{
	int pipeno;
	int ret = -ENFILE;

	for (pipeno = 0; pipeno < MAX_PIPES; pipeno++) {
		if ((g_pipeset & (1 << pipeno)) == 0) {
			g_pipeset |= (1 << pipeno);
			ret = pipeno;
			break;
		}
	}

	return ret;
}

/****************************************************************************
 * Name: pipe_free
 ****************************************************************************/

static inline void pipe_free(int pipeno)
{
	int ret;

	ret = sem_wait(&g_pipesem);
	if (ret == 0) {
		g_pipeset &= ~(1 << pipeno);
		(void)sem_post(&g_pipesem);
	}
}

/****************************************************************************
 * Name: pipe_close
 ****************************************************************************/

static int pipe_close(FAR struct file *filep)
{
	FAR struct inode *inode = filep->f_inode;
	FAR struct pipe_dev_s *dev = inode->i_private;
	uint8_t pipeno = dev->d_pipeno;
	int ret;

	DEBUGASSERT(dev);

	/* Perform common close operations */

	ret = pipecommon_close(filep);
	dev = inode->i_private;
	if (ret == 0 && (dev && dev->d_refs == 0)) {
		/* Release the pipe when there are no further open references to it. */

		pipe_free(pipeno);
	}

	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pipe
 *
 * Description:
 *   pipe() creates a pair of file descriptors, pointing to a pipe inode,
 *   and  places them in the array pointed to by 'fd'. fd[0] is for reading,
 *   fd[1] is for writing.
 *
 * Inputs:
 *   fd[2] - The user provided array in which to catch the pipe file
 *   descriptors
 *
 * Return:
 *   0 is returned on success; otherwise, -1 is returned with errno set
 *   appropriately.
 *
 ****************************************************************************/

int pipe(int fd[2])
{
	FAR struct pipe_dev_s *dev = NULL;
	char devname[16];
	int pipeno;
	int err;
	int ret;

	/* Get exclusive access to the pipe allocation data */

	ret = sem_wait(&g_pipesem);
	if (ret < 0) {
		/* sem_wait() will have already set errno */

		return ERROR;
	}

	/* Allocate a minor number for the pipe device */

	pipeno = pipe_allocate();
	if (pipeno < 0) {
		(void)sem_post(&g_pipesem);
		err = -pipeno;
		goto errout;
	}

	/* Create a pathname to the pipe device */

	sprintf(devname, "/dev/pipe%d", pipeno);

	/* Check if the pipe device has already been created */

	if ((g_pipecreated & (1 << pipeno)) == 0) {
		/* No.. Allocate and initialize a new device structure instance */

		dev = pipecommon_allocdev();
		if (!dev) {
			(void)sem_post(&g_pipesem);
			err = ENOMEM;
			goto errout_with_pipe;
		}

		dev->d_pipeno = pipeno;

		/* Register the pipe device */

		ret = register_driver(devname, &pipe_fops, 0666, (void *)dev);
		if (ret != 0) {
			(void)sem_post(&g_pipesem);
			err = -ret;
			goto errout_with_dev;
		}

		/* Remember that we created this device */

		g_pipecreated |= (1 << pipeno);
	}

	(void)sem_post(&g_pipesem);

	/* Get a write file descriptor */

	fd[1] = open(devname, O_WRONLY);
	if (fd[1] < 0) {
		err = -fd[1];
		goto errout_with_driver;
	}

	/* Get a read file descriptor */

	fd[0] = open(devname, O_RDONLY);
	if (fd[0] < 0) {
		err = -fd[0];
		goto errout_with_wrfd;
	}

	return OK;

errout_with_wrfd:
	close(fd[1]);

errout_with_driver:
	unregister_driver(devname);

errout_with_dev:
	if (dev) {
		pipecommon_freedev(dev);
	}

errout_with_pipe:
	pipe_free(pipeno);

errout:
	set_errno(err);
	return ERROR;
}

#endif							/* CONFIG_DEV_PIPE_SIZE > 0 */
