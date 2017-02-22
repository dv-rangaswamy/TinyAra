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
 *
 *   Copyright (C) 2007-2009, 2011-2013 Gregory Nutt. All rights reserved.
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

#ifndef __INCLUDE_FS_FS_UTILS_H
#define __INCLUDE_FS_FS_UTILS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/compiler.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define PWD "PWD"
#define OLD_PWD "OLDPWD"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: fs_initiate
 *
 * Description:
 *   This is called from the User application to initiate the file system
 *
 ****************************************************************************/
int fs_initiate(char *dev_name, char *mount_name);

/****************************************************************************
 * Name: fs_clean
 *
 * Description:
 *   This is called from the User application to clean the file system
 *
 ****************************************************************************/
int fs_clean(char *dev_name);

/****************************************************************************
 * Name: fs_erase
 *
 * Description:
 *   This is called from the User application to erase the file system
 *
 ****************************************************************************/
int fs_erase(char *dev_name);

#ifdef CONFIG_SMARTFS_SECTOR_RECOVERY
/****************************************************************************
 * Name: fs_recover
 *
 * Description:
 *   Recover corrupted sector due to sudden power off
 *
 ****************************************************************************/
int fs_recover(void);
#endif

/****************************************************************************
 * Name: getwd
 *
 * Description:
 *   get current working directory
 *
 ****************************************************************************/
FAR const char *getwd(const char *wd);

/****************************************************************************
 * Name: get_fullpath / get_dirpath
 *
 * Description:
 *   get_fullpath : get full path of target path
 *   get_dirpath : get directory path
 *
 ****************************************************************************/
FAR char *get_fullpath(FAR const char *relpath);
char *get_dirpath(const char *dirpath, const char *relpath);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif							/* __INCLUDE_FS_FS_UTILS_H */
