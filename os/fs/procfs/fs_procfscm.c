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
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/arch.h>
#include <tinyara/sched.h>
#include <tinyara/kmalloc.h>
#include <tinyara/fs/fs.h>
#include <tinyara/fs/procfs.h>
#include <tinyara/fs/dirent.h>
#include <tinyara/clock.h>

#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#if defined(CONFIG_ARCH_CHIP_BCM4390X)
#include "wwd_wifi.h"
#elif defined(CONFIG_ARCH_BOARD_SIDK_S5JT200)
#include "slsi_wifi_api.h"
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_PROCFS)
#if defined(CONFIG_CM) && !defined(CONFIG_FS_PROCFS_EXCLUDE_CONNECTIVITY)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Determines the size of an intermediate buffer that must be large enough
 * to handle the longest line generated by this logic.
 */
#define CM_IPADDR_LEN           32
#define CM_LINELEN              32
#define CM_NWBEARER_WLAN        21
#define CM_NWBEARER_BLUETOOTH   22
#define CM_NWBEARER_WPAN        23
#define CM_NWBEARER_ETHERNET    41

/****************************************************************************
 * Private Types
 ****************************************************************************/
/* This enumeration identifies all of the task/thread nodes that can be
 * accessed via the procfs file system.
 */

enum cm_node_e {
	CM_LEVEL0 = 0,				/* The top-level directory */
	CM_NWBEARER,				/* Network Bearer */
	CM_AVAILABLE_BEARER,		/* Available bearers */
	CM_RSSI,					/* Received Signal Strength Indicator */
	CM_IPADDR,					/* Device IP */
	CM_BITRATE					/* Device bitrate */
};

struct cm_node_s {
	FAR const char *relpath;	/* Relative path to the node */
	FAR const char *name;		/* Terminal node segment name */
	uint8_t nodetype;			/* Type of node (see enum proc_node_e) */
	uint8_t dtype;				/* dirent type (see include/dirent.h) */
};

struct cm_dir_s {
	struct procfs_dir_priv_s base;	/* Base directory private data */
	FAR const struct cm_node_s *node;	/* Directory node description */
};

/* This structure describes one open "file" */
struct cm_file_s {
	struct procfs_file_s base;	/* Base open file structure */
	FAR const struct cm_node_s *node;	/* Describes the file node */
	struct cm_dir_s dir;		/* Reference to item being accessed */
	char line[CM_LINELEN];		/* Pre-allocated buffer for formatted lines */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* File system methods */
static int cm_open(FAR struct file *filep, FAR const char *relpath, int oflags, mode_t mode);
static int cm_close(FAR struct file *filep);
static ssize_t cm_read(FAR struct file *filep, FAR char *buffer, size_t buflen);

static int cm_dup(FAR const struct file *oldp, FAR struct file *newp);

static int cm_opendir(const char *relpath, FAR struct fs_dirent_s *dir);
static int cm_closedir(FAR struct fs_dirent_s *dir);
static int cm_readdir(FAR struct fs_dirent_s *dir);
static int cm_rewinddir(FAR struct fs_dirent_s *dir);

static int cm_stat(FAR char *relpath, FAR struct stat *buf);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/* See fs_mount.c -- this structure is explicitly externed there.
 * We use the old-fashioned kind of initializers so that this will compile
 * with any compiler.
 */

const struct procfs_operations cm_operations = {
	cm_open,					/* open */
	cm_close,					/* close */
	cm_read,					/* read */
	NULL,						/* write */

	cm_dup,						/* dup */

	cm_opendir,					/* opendir */
	cm_closedir,				/* closedir */
	cm_readdir,					/* readdir */
	cm_rewinddir,				/* rewinddir */

	cm_stat						/* stat */
};

/* These structures provide information about every node */

static const struct cm_node_s g_cm_level0 = {
	"", "connectivity", (uint8_t)CM_LEVEL0, DTYPE_DIRECTORY	/* Top-level directory */
};

static const struct cm_node_s g_cm_nwbearer = {
	"nwbearer", "nwbearer", (uint8_t)CM_NWBEARER, DTYPE_FILE	/* Task command line */
};

static const struct cm_node_s g_cm_available_bearer = {
	"available_bearer", "available_bearer", (uint8_t)CM_AVAILABLE_BEARER, DTYPE_FILE	/* Task command line */
};

static const struct cm_node_s g_cm_rssi = {
	"rssi", "rssi", (uint8_t)CM_RSSI, DTYPE_FILE	/* Task command line */
};

static const struct cm_node_s g_cm_ipaddr = {
	"ipaddr", "ipaddr", (uint8_t)CM_IPADDR, DTYPE_FILE	/* Task command line */
};

static const struct cm_node_s g_cm_bitrate = {
	"bitrate", "bitrate", (uint8_t)CM_BITRATE, DTYPE_FILE	/* Task command line */
};

/* This is the list of all nodes */

static FAR const struct cm_node_s *const g_cm_nodeinfo[] = {
	&g_cm_level0,
	&g_cm_nwbearer,				/* network bearer used by LWM2M */
	&g_cm_available_bearer,		/* List of available bearers */
	&g_cm_rssi,					/* Rssi */
	&g_cm_ipaddr,				/* IP address */
	&g_cm_bitrate,				/* bitrate */
};

#define CM_NNODES (sizeof(g_cm_nodeinfo)/sizeof(FAR const struct cm_node_s * const))

static const struct cm_node_s *const g_cm_level0info[] = {
	&g_cm_nwbearer,				/* Task command line */
	&g_cm_available_bearer,		/* Task command line */
	&g_cm_rssi,					/* Task command line */
	&g_cm_ipaddr,				/* Task command line */
	&g_cm_bitrate,				/* Task command line */
};

#define CM_NLEVEL0NODES (sizeof(g_cm_level0info)/sizeof(FAR const struct cm_node_s * const))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cm_get_interface
 ****************************************************************************/
static void cm_get_interface(char *mac)
{
#if defined(CONFIG_WICED)
	strcpy(mac, "en1");
#elif defined(CONFIG_NET_ETHERNET)
	strcpy(mac, "eth0");
#elif defined(CONFIG_NET_802154)
	strcpy(mac, "wlan0");
#endif
}

/****************************************************************************
 * Name: cm_nwbearer_read
 ****************************************************************************/
static size_t cm_nwbearer_read(FAR struct cm_file_s *connectivityfile, FAR char *buffer, size_t buflen, off_t offset)
{
	size_t linesize;
	size_t copysize;
	int nwbearer;
#if defined(CONFIG_WICED)
	nwbearer = CM_NWBEARER_WLAN;
#elif defined(CONFIG_NET_ETHERNET)
	nwbearer = CM_NWBEARER_ETHERNET;
#elif defined(CONFIG_NET_802154)
	nwbearer = CM_NWBEARER_WPAN;
#endif
	linesize = snprintf(connectivityfile->line, CM_LINELEN, "%d", nwbearer);
	copysize = procfs_memcpy(connectivityfile->line, linesize, buffer, buflen, &offset);
	return copysize;
}

/****************************************************************************
 * Name: cm_available_bearer_read
 ****************************************************************************/
static size_t cm_available_bearer_read(FAR struct cm_file_s *connectivityfile, FAR char *buffer, size_t buflen, off_t offset)
{
	size_t linesize;
	size_t copysize;
	int nwbearer;
#if defined(CONFIG_WICED)
	nwbearer = CM_NWBEARER_WLAN;
#elif defined(CONFIG_NET_ETHERNET)
	nwbearer = CM_NWBEARER_ETHERNET;
#elif defined(CONFIG_NET_802154)
	nwbearer = CM_NWBEARER_WPAN;
#endif
	linesize = snprintf(connectivityfile->line, CM_LINELEN, "%d", nwbearer);
	copysize = procfs_memcpy(connectivityfile->line, linesize, buffer, buflen, &offset);
	return copysize;
}

/****************************************************************************
 * Name: cm_rssi_read
 ****************************************************************************/
static size_t cm_rssi_read(FAR struct cm_file_s *connectivityfile, FAR char *buffer, size_t buflen, off_t offset)
{
	size_t linesize;
	size_t copysize;
	int rssi_value;
#if defined(CONFIG_WICED) && defined(CONFIG_ARCH_CHIP_BCM4390X)
	wwd_wifi_get_rssi(&rssi_value);
#elif defined(CONFIG_ARCH_BOARD_SIDK_S5JT200)
	WiFiGetRssi(&rssi_value);
	rssi_value -= 255;
#else
	rssi_value = 0;
#endif

	linesize = snprintf(connectivityfile->line, CM_LINELEN, "%d", rssi_value);
	copysize = procfs_memcpy(connectivityfile->line, linesize, buffer, buflen, &offset);

	return copysize;
}

/****************************************************************************
 * Name: cm_ipaddr_read
 ****************************************************************************/
static size_t cm_ipaddr_read(FAR struct cm_file_s *connectivityfile, FAR char *buffer, size_t buflen, off_t offset)
{
	char mac[4];
	char ipaddr[CM_IPADDR_LEN];
	size_t linesize;
	size_t copysize;

	cm_get_interface(mac);
#ifdef CONFIG_NET_IPv4
	struct in_addr addr;
	netlib_get_ipv4addr(mac, &addr);
	inet_ntop(AF_INET, &addr, ipaddr, INET_ADDRSTRLEN);
#endif
#ifdef CONFIG_NET_IPv6
	struct in6_addr addr;
	netlib_get_ipv6addr(mac, &addr);
	inet_ntop(AF_INET6, &addr, ipaddr, INET6_ADDRSTRLEN);
#endif

	linesize = snprintf(connectivityfile->line, CM_LINELEN, "%s", ipaddr);
	copysize = procfs_memcpy(connectivityfile->line, linesize, buffer, buflen, &offset);

	return copysize;
}

/****************************************************************************
 * Name: cm_bitrate_read
 ****************************************************************************/
static size_t cm_bitrate_read(FAR struct cm_file_s *connectivityfile, FAR char *buffer, size_t buflen, off_t offset)
{
	size_t linesize;
	size_t copysize;
	int bitrate;
#if defined(CONFIG_WICED) && defined(CONFIG_ARCH_CHIP_BCM4390X)
	wwd_wifi_get_rate(WWD_STA_INTERFACE, &bitrate);
#else
	bitrate = 0;
#endif

	linesize = snprintf(connectivityfile->line, CM_LINELEN, "%d", bitrate);
	copysize = procfs_memcpy(connectivityfile->line, linesize, buffer, buflen, &offset);

	return copysize;
}

/****************************************************************************
 * Name: cm_findnode
 ****************************************************************************/
static FAR const struct cm_node_s *cm_findnode(FAR const char *relpath)
{
	int i;

	/* Two path forms are accepted:
	 *
	 * "connectivity" - It is a top directory.
	 * "connectivity/<node>" - If <node> is a recognized node then, then it
	 *   is a file or directory.
	 */

	if (strncmp(relpath, "connectivity", 12) != 0) {
		fdbg("ERROR: Bad relpath: %s\n", relpath);
		return NULL;
	}
	relpath += 12;

	if (relpath[0] == '/') {
		relpath++;
	}

	/* Search every string in g_cm_nodeinfo or until a match is found */

	for (i = 0; i < CM_NNODES; i++) {
		if (strcmp(g_cm_nodeinfo[i]->relpath, relpath) == 0) {
			return g_cm_nodeinfo[i];
		}
	}

	/* Not found */

	return NULL;
}

/****************************************************************************
 * Name: cm_open
 ****************************************************************************/

static int cm_open(FAR struct file *filep, FAR const char *relpath, int oflags, mode_t mode)
{
	FAR struct cm_file_s *connectivityfile;
	FAR const struct cm_node_s *node;

	fvdbg("Open '%s'\n", relpath);

	/* PROCFS is read-only.  Any attempt to open with any kind of write
	 * access is not permitted.
	 *
	 * REVISIT:  Write-able proc files could be quite useful.
	 */

	if ((oflags & O_WRONLY) != 0 || (oflags & O_RDONLY) == 0) {
		fdbg("ERROR: Only O_RDONLY supported\n");
		return -EACCES;
	}

	/* Find the nodes of connectivity directory */
	node = cm_findnode(relpath);
	if (!node) {
		/* Entry not found */
		fdbg("ERROR: Invalid path \"%s\"\n", relpath);
		return -ENOENT;
	}

	if (!DIRENT_ISFILE(node->dtype)) {
		fdbg("ERROR: Path \"%s\" is not a regular file\n", relpath);
		return -EISDIR;
	}

	/* Allocate a container to hold the domain selection */
	connectivityfile = (FAR struct cm_file_s *)kmm_zalloc(sizeof(struct cm_file_s));
	if (!connectivityfile) {
		fdbg("ERROR: Failed to allocate file container\n");
		return -ENOMEM;
	}

	connectivityfile->node = node;

	/* Save the index as the open-specific state in filep->f_priv */
	filep->f_priv = (FAR void *)connectivityfile;

	return OK;
}

/****************************************************************************
 * Name: cm_close
 ****************************************************************************/

static int cm_close(FAR struct file *filep)
{
	FAR struct cm_file_s *connectivityfile;

	/* Recover our private data from the struct file instance */

	connectivityfile = (FAR struct cm_file_s *)filep->f_priv;
	DEBUGASSERT(connectivityfile);

	/* Release the file container structure */

	kmm_free(connectivityfile);
	filep->f_priv = NULL;
	return OK;
}

/****************************************************************************
 * Name: cm_read
 ****************************************************************************/

static ssize_t cm_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
	FAR struct cm_file_s *connectivityfile;
	ssize_t ret;

	fvdbg("buffer=%p buflen=%d\n", buffer, (int)buflen);

	/* Recover our private data from the struct file instance */

	connectivityfile = (FAR struct cm_file_s *)filep->f_priv;
	DEBUGASSERT(connectivityfile);

	/* Provide the requested data */
	ret = 0;

	switch (connectivityfile->node->nodetype) {
	case CM_NWBEARER:			/* Network bearer */
		ret = cm_nwbearer_read(connectivityfile, buffer, buflen, filep->f_pos);
		break;
	case CM_AVAILABLE_BEARER:	/* List of available bearers */
		ret = cm_available_bearer_read(connectivityfile, buffer, buflen, filep->f_pos);
		break;
	case CM_RSSI:				/* RSSI */
		ret = cm_rssi_read(connectivityfile, buffer, buflen, filep->f_pos);
		break;
	case CM_IPADDR:			/* Device IP address */
		ret = cm_ipaddr_read(connectivityfile, buffer, buflen, filep->f_pos);
		break;
	case CM_BITRATE:			/* Device bitrate */
		ret = cm_bitrate_read(connectivityfile, buffer, buflen, filep->f_pos);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	/* Update the file offset */

	if (ret > 0) {
		filep->f_pos += ret;
	}

	return ret;
}

/****************************************************************************
 * Name: cm_dup
 *
 * Description:
 *   Duplicate open file data in the new file structure.
 *
 ****************************************************************************/
static int cm_dup(FAR const struct file *oldp, FAR struct file *newp)
{
	FAR struct cm_file_s *oldfile;
	FAR struct cm_file_s *newfile;

	fvdbg("Dup %p->%p\n", oldp, newp);

	/* Recover our private data from the old struct file instance */

	oldfile = (FAR struct cm_file_s *)oldp->f_priv;
	DEBUGASSERT(oldfile);

	/* Allocate a new container to hold the task and node selection */

	newfile = (FAR struct cm_file_s *)kmm_malloc(sizeof(struct cm_file_s));
	if (!newfile) {
		fdbg("ERROR: Failed to allocate file container\n");
		return -ENOMEM;
	}

	/* The copy the file information from the old container to the new */

	memcpy(newfile, oldfile, sizeof(struct cm_file_s));

	/* Save the new container in the new file structure */

	newp->f_priv = (FAR void *)newfile;
	return OK;
}

/****************************************************************************
 * Name: cm_opendir
 *
 * Description:
 *   Open a directory for read access
 *
 ****************************************************************************/
static int cm_opendir(FAR const char *relpath, FAR struct fs_dirent_s *dir)
{
	FAR struct cm_dir_s *connectivitydir;
	FAR const struct cm_node_s *node;

	fvdbg("relpath: \"%s\"\n", relpath ? relpath : "NULL");
	DEBUGASSERT(relpath && dir && !dir->u.procfs);

	/* Find the directory entry being opened */
	node = cm_findnode(relpath);
	if (!node) {
		/* Entry not found */
		fdbg("ERROR: Invalid path \"%s\"\n", relpath);
		return -ENOENT;
	}

	if (!DIRENT_ISDIRECTORY(node->dtype)) {
		fdbg("ERROR: Path \"%s\" is not a regular directory\n", relpath);
		return -ENOTDIR;
	}

	connectivitydir = (FAR struct cm_dir_s *)kmm_zalloc(sizeof(struct cm_dir_s));
	if (!connectivitydir) {
		fdbg("ERROR: Failed to allocate the directory structure\n");
		return -ENOMEM;
	}

	if (node->nodetype == CM_LEVEL0) {
		/* This is a top level directory : connectivity */
		connectivitydir->base.level = 1;
		connectivitydir->base.nentries = CM_NLEVEL0NODES;
		connectivitydir->base.index = 0;
		connectivitydir->node = node;
	}

	dir->u.procfs = (FAR void *)connectivitydir;
	return OK;
}

/****************************************************************************
 * Name: cm_closedir
 *
 * Description: Close the directory listing
 *
 ****************************************************************************/

static int cm_closedir(FAR struct fs_dirent_s *dir)
{
	FAR struct cm_dir_s *priv;

	DEBUGASSERT(dir && dir->u.procfs);
	priv = dir->u.procfs;

	if (priv) {
		kmm_free(priv);
	}

	dir->u.procfs = NULL;
	return OK;
}

/****************************************************************************
 * Name: cm_readdir
 *
 * Description: Read the next directory entry
 *
 ****************************************************************************/

static int cm_readdir(struct fs_dirent_s *dir)
{
	FAR struct cm_dir_s *connectivitydir;
	FAR const struct cm_node_s *node = NULL;
	unsigned int index;
	int ret;

	DEBUGASSERT(dir && dir->u.procfs);
	connectivitydir = dir->u.procfs;

	/* Have we reached the end of the directory */

	index = connectivitydir->base.index;
	if (index >= connectivitydir->base.nentries) {
		/* We signal the end of the directory by returning the special
		 * error -ENOENT
		 */

		fdbg("Entry %d: End of directory\n", index);
		ret = -ENOENT;
	}
	/* No, we are not at the end of the directory */
	else {
		/* Handle the directory listing by the node type */

		switch (connectivitydir->node->nodetype) {
		case CM_LEVEL0:		/* Top level directory */
			DEBUGASSERT(connectivitydir->base.level == 1);
			node = g_cm_level0info[index];
			break;
		default:
			return -ENOENT;
		}

		/* Save the filename and file type */

		dir->fd_dir.d_type = node->dtype;
		strncpy(dir->fd_dir.d_name, node->name, NAME_MAX + 1);

		/* Set up the next directory entry offset.      NOTE that we could use the
		 * standard f_pos instead of our own private index.
		 */

		connectivitydir->base.index = index + 1;
		ret = OK;
	}

	return ret;
}

/****************************************************************************
 * Name: cm_rewindir
 *
 * Description: Reset directory read to the first entry
 *
 ****************************************************************************/

static int cm_rewinddir(struct fs_dirent_s *dir)
{
	FAR struct cm_dir_s *priv;

	DEBUGASSERT(dir && dir->u.procfs);
	priv = dir->u.procfs;

	priv->base.index = 0;
	return OK;
}

/****************************************************************************
 * Name: cm_stat
 *
 * Description: Return information about a file or directory
 *
 ****************************************************************************/

static int cm_stat(char *relpath, struct stat *buf)
{
	FAR const struct cm_node_s *node;

	/* Find the directory entry being opened */
	node = cm_findnode(relpath);
	if (!node) {
		fdbg("ERROR: Invalid path \"%s\"\n", relpath);
		return -ENOENT;
	}

	/* If the node exists, it is the name for a read-only file or
	 * directory.
	 */

	if (node->dtype == DTYPE_FILE) {
		buf->st_mode = S_IFREG | S_IROTH | S_IRGRP | S_IRUSR;
	} else {
		buf->st_mode = S_IFDIR | S_IROTH | S_IRGRP | S_IRUSR;
	}

	/* File/directory size, access block size */

	buf->st_size = 0;
	buf->st_blksize = 0;
	buf->st_blocks = 0;
	return OK;

}
#endif
#endif
