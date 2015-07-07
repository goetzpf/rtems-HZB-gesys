#include <errno.h>						/* errno       */
#include <stdio.h>						/* printf(...) */
#include <stdlib.h>						/* realloc(...) */
#include <rtems/seterr.h>
#include <dirent.h>						/* struct dirent */

#include "rsh_node.h"
#include "rsh_directories.h"

/* reentrant mode */

#define SEM_TAKE rtems_semaphore_obtain((rtems_id) iop->sem, RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define SEM_GIVE rtems_semaphore_release((rtems_id) iop->sem)

/*
 * The rsh open handler
 */
/*
 * we have nothing to do on request
 * because the eval path function will do all the work...
*/
static int rtems_rsh_dir_open(
    rtems_libio_t *iop,
    const char    *new_name,
    uint32_t       flags,
    uint32_t       mode)
{
#ifdef DEBUGMSG
	printf("rtems_rsh_dir_open(%p, \"%s\", %x, %x)\n", 
			iop, 
			new_name, 
			(unsigned int) flags, 
			(unsigned int) mode);
#endif /* DEBUGMSG */
	return 0;
}


/* also a dummy function */
static int rtems_rsh_dir_close(rtems_libio_t *iop)
{
#ifdef DEBUGMSG
	printf("rtems_rsh_dir_close(%p)\n", iop);
#endif /* DEBUGMSG */
	return 0;
}


static int rtems_rsh_dir_read(
  rtems_libio_t *iop,
  void          *buffer,
  size_t        count
)
{
	struct dirent								tmp_dirent;
	int    											i, entries, max_entries, bytes_moved = 0;
	struct rsh_node_t						*rsh_node;
	struct rsh_dir_node_data_t	*dataptr;
#ifdef DEBUGMSG
	printf("rtems_rsh_dir_read(%p, %p, %li)\n", iop, buffer, (unsigned long) count);
#endif /* DEBUGMSG */

	SEM_TAKE;
	rsh_node = (struct rsh_node_t *) iop->file_info;
	dataptr = (struct rsh_dir_node_data_t*) rsh_node->data;

	max_entries = dataptr->used_entries;

	if (iop->offset < max_entries * sizeof(struct dirent)) {
		entries = count / sizeof(struct dirent); /* how many entries should read out */
		if (entries > max_entries) entries = max_entries; /* our hack contains only three members */
		for (i = iop->offset / sizeof(struct dirent); i + entries < max_entries; ++i) {
			tmp_dirent.d_ino = i + 100; /* individual inode number, needed by readdir and getcwd */
			tmp_dirent.d_off = i * sizeof(struct dirent);
			tmp_dirent.d_reclen = sizeof(struct dirent); /* always */

			strcpy(tmp_dirent.d_name, dataptr->dir_entries[i]);

			tmp_dirent.d_namlen = strlen(tmp_dirent.d_name);
			iop->offset += sizeof(struct dirent); /* point to the next entry */
			memcpy(buffer, (void *) &tmp_dirent, sizeof(struct dirent));
			bytes_moved += sizeof(struct dirent);
		}
	}
	SEM_GIVE;
	return bytes_moved;
}


static off_t rtems_rsh_dir_lseek(
  rtems_libio_t   *iop,
  off_t           offset,
  int             whence
)
{
	off_t abspos;

#ifdef DEBUGMSG
	printf("rtems_rsh_dir_lssek(%p, %li, %i)\n", iop, (long) offset, whence);
#endif /* DEBUGMSG */
	SEM_TAKE;
	switch (whence)
	{
		case SEEK_SET:
			abspos = offset;
		break;
		case SEEK_CUR:
			abspos = iop->offset + offset;
		break;
		case SEEK_END:
		default:
			SEM_GIVE;
			rtems_set_errno_and_return_minus_one( EINVAL );
		break;
	}
	if (abspos < 0)
	{
		SEM_GIVE;
		rtems_set_errno_and_return_minus_one( EINVAL );
	}	else if (abspos >= iop->size)
		if (iop->flags & LIBIO_FLAGS_APPEND) iop->offset = abspos;
	
	SEM_GIVE;
	return abspos;
}


static int rtems_rsh_dir_fstat(
  rtems_filesystem_location_info_t *iop,
  struct stat *buf)
{
	struct rsh_node_t *rsh_node;

#ifdef DEBUGMSG
	printf("rtems_rsh_dir_fstat(%p, %p)\n", iop, buf);
#endif /* DEBUGMSG */
	/* return the stat of the directory */
	rsh_node = (struct rsh_node_t *) iop->node_access;
	memcpy((void *) buf, (void *) &(rsh_node->st_stat), sizeof(struct stat));

#ifdef DEBUGMSG
	printf("dev: %lu,%lu, ino: %lu\n", 
		rtems_filesystem_dev_major_t(buf->st_dev),
		rtems_filesystem_dev_minor_t(buf->st_dev),
		buf->st_ino);
#endif
	return RTEMS_SUCCESSFUL;
}


rtems_filesystem_file_handlers_r rtems_rsh_dir_handlers = {
    rtems_rsh_dir_open,   /* open  */
    rtems_rsh_dir_close,  /* close */
    rtems_rsh_dir_read,   /* read  */
    NULL,                 /* write */
    NULL,                 /* ioctl */
    rtems_rsh_dir_lseek,  /* lseek */    
    rtems_rsh_dir_fstat,  /* fstat */    
    NULL,                 /* fchmod */   
    NULL,                 /* ftruncate */
    NULL,                 /* fpathconf */
    NULL,                 /* fsync */    
    NULL,                 /* fdatasync */
    NULL,                 /* fcntl */
    NULL                  /* rmnod */
};

