#include <rtems/libio.h>
#include <rtems/libio_.h>
#include <rtems/seterr.h>			/* rtems_set_errno_and_return_minus_one() */
#include <stdio.h>						/* printf() */
#include <stdlib.h>						/* malloc() */

#include "rsh_basic.h"				/* rsh_cmd(), rsh_buffer */
#include "rsh_node.h"



/* begin inode handling */
ino_t *inode_list;

extern uint32_t rtems_libio_number_iops;

void rsh_initialize_inode_list(void)
{
	inode_list = malloc(rtems_libio_number_iops * sizeof(*inode_list));
	memset(inode_list, 0, rtems_libio_number_iops * sizeof(*inode_list));
}

void rsh_free_inode_list(void)
{
	free(inode_list);
}

/* if the number is already in list then return -1 */
int rsh_check_inode(ino_t inode)
{
	uint32_t i;

	for (i = 0; i < rtems_libio_number_iops; ++i)
		if (inode_list[i] == inode) rtems_set_errno_and_return_minus_one(EACCES);
	return RTEMS_SUCCESSFUL;
}

int rsh_tag_inode(ino_t inode)
{
	uint32_t i;

	for (i = 0; i < rtems_libio_number_iops; ++i)
		if (inode_list[i] == 0) {
			inode_list[i] = inode;
			return RTEMS_SUCCESSFUL;
		}
	rtems_set_errno_and_return_minus_one(EMFILE);
}

int rsh_free_inode(ino_t inode)
{
	uint32_t i;

	for (i = 0; i < rtems_libio_number_iops; ++i)
		if (inode_list[i] == inode) {
			inode_list[i] = 0;
			return RTEMS_SUCCESSFUL;
		}
	rtems_set_errno_and_return_minus_one(EBADF);
}
/* end of inode handling */



/* at the moment this will only by remote files used */
int rsh_fill_inode(const char *fname, struct rsh_node_t *buf)
{
	rtems_filesystem_node_types_t	flag = 0;
	int														fd, i;
	unsigned long									rsize, major;

	if (rsh_checkfile(fname)) flag = RTEMS_FILESYSTEM_MEMORY_FILE;
/*		else if (rsh_checkdir(fname)) flag = RTEMS_FILESYSTEM_DIRECTORY; */

	if (!flag) rtems_set_errno_and_return_minus_one( ENOENT );

	memset(buf, 0, sizeof(*buf));
	buf->node_type = flag;
	strcpy(buf->fname, fname);
	buf->exist = flag;
	buf->data = NULL;

	/* retrieve: devicenumber, inode, access(raw), number of hard links, userid, 
			groupid, filesize, blocksize, number of blocks, access-time, mod-time, 
			change-time */
	sprintf(rsh_buffer, "stat -L -c'%%d %%i %%f %%h %%u %%g %%s %%B %%b %%X %%Y %%Z' %s", fname);
  fd = rsh_cmd(rsh_buffer);

	if (fd >= 0) {
		if (rsh_read_fd(fd, (void *)rsh_buffer, sizeof(rsh_buffer), &rsize) != RTEMS_SUCCESSFUL)
			return -1;
	} else return -1;

	i = sscanf(rsh_buffer, "%lu %lu %x %hu %hu %hu %lu %lu %lu %lu %lu %lu",
			(unsigned long *) &major,
			(unsigned long *) &(buf->st_stat.st_ino),
			&(buf->st_stat.st_mode),
			(unsigned short *) &(buf->st_stat.st_nlink),
			(unsigned short *) &(buf->st_stat.st_uid),
			(unsigned short *) &(buf->st_stat.st_gid),
			&(buf->st_stat.st_size),
			&(buf->st_stat.st_blksize),
			&(buf->st_stat.st_blocks),
			&(buf->st_stat.st_atime),
			&(buf->st_stat.st_mtime),
			&(buf->st_stat.st_ctime));
	buf->st_stat.st_dev = rtems_filesystem_make_dev_t(major, 0);
	buf->st_stat.st_rdev = buf->st_stat.st_dev;

#ifdef DEBUGMSG
	printf("device: %lu, inode: %lu, mode: %x, links: %hu, UID: %hu, GIG: %hu, size: %lu, " \
					"blksize: %lu\nblocks: %lu, atime: %lu, mtime: %lu, ctime: %lu\n", 
			(unsigned long) major,
			(unsigned long) buf->st_stat.st_ino,
			buf->st_stat.st_mode,
			buf->st_stat.st_nlink,
			buf->st_stat.st_uid,
			buf->st_stat.st_gid,
			buf->st_stat.st_size,
			buf->st_stat.st_blksize,
			buf->st_stat.st_blocks,
			buf->st_stat.st_atime,
			buf->st_stat.st_mtime,
			buf->st_stat.st_ctime);
#endif
	return (i == 12) ? RTEMS_SUCCESSFUL : -1;
}

/*	sprintf(rsh_buffer, "ls --all --classify %s | grep '/' | tr -d '/'", node->host_name); */
