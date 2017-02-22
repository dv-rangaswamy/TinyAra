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

/// @file libc_sched.c

/// @brief Test Case Example for Libc Sched API

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include "tc_internal.h"

#define MAX_FIFO 255
#define MAX_RR 255
#define MIN_FIFO 1
#define MIN_RR 1

/**
* @fn                   :tc_libc_sched_sched_get_priority_max
* @brief                :returns the maximum priority value
* @Scenario             :Function returns the maximum priority value that can be used with the scheduling algorithm identified by policy.
* API's covered         :sched_get_priority_max
* Preconditions         :NA
* Postconditions        :NA
* @return               :void
*/
static void tc_libc_sched_sched_get_priority_max(void)
{
	int ret_chk;

	ret_chk = sched_get_priority_max(SCHED_FIFO);
	TC_ASSERT_EQ("sched_get_priority_max", ret_chk, MAX_FIFO);

	ret_chk = sched_get_priority_max(SCHED_RR);
	TC_ASSERT_EQ("sched_get_priority_max", ret_chk, MAX_RR);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_libc_sched_sched_get_priority_min
* @brief                :returns the minimum priority value
* @Scenario             :Function returns the minimum priority value that can be used with the scheduling algorithm identified by policy.
* API's covered         :sched_get_priority_max
* Preconditions         :NA
* Postconditions        :NA
* @return               :void
*/
static void tc_libc_sched_sched_get_priority_min(void)
{
	int ret_chk;

	ret_chk = sched_get_priority_min(SCHED_FIFO);
	TC_ASSERT_EQ("sched_get_priority_min", ret_chk, MIN_FIFO);

	ret_chk = sched_get_priority_min(SCHED_RR);
	TC_ASSERT_EQ("sched_get_priority_min", ret_chk, MIN_RR);

	TC_SUCCESS_RESULT();
}

/****************************************************************************
 * Name: libc_sched
 ****************************************************************************/

int libc_sched_main(void)
{
	tc_libc_sched_sched_get_priority_max();
	tc_libc_sched_sched_get_priority_min();

	return 0;
}
