#include <errno.h>						/* errno       */
#include <stdlib.h>						/* malloc(), realloc() */
#include <stdio.h>						/* sprintf() */
#include <rtems/seterr.h>
#include "rsh_node.h"
#include "rsh_files.h"


/* reentrant mode */
/*#define REENTRANT*/

#ifdef REENTRANT
#define SEM_TAKE rtems_semaphore_obtain((rtems_id) iop->sem, RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define SEM_GIVE rtems_semaphore_release((rtems_id) iop->sem)
#else
#define SEM_TAKE
#define SEM_GIVE
#endif

#ifdef CRCCHECK
extern unsigned char calcBufferCRC(unsigned char *buffer, unsigned long len);
#endif

/*
 * The rsh file open handler
 */
static int rtems_rsh_file_open(
    rtems_libio_t *iop,
    const char    *new_name,
    uint32_t       flags,
    uint32_t       mode
)
{
	int 							fd;
	unsigned long 		rsize;
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_open(%p, \"%s\", %x, %x)\n", 
			iop, 
			new_name, 
			(unsigned int) flags, 
			(unsigned int) mode);
#endif /* DEBUGMSG */

	SEM_TAKE;
	/* fetch file informations */
	node = (struct rsh_node_t*) iop->file_info;
	
	/* initialize file pointers */
	iop->size = node->st_stat.st_size;
	iop->offset = 0;

	if (iop->size > 0) {
		node->data = malloc(iop->size); /* reserve space for file content */
		if (node->data == NULL)
			rtems_set_errno_and_return_minus_one(ENOMEM);

		sprintf(rsh_buffer, "cat %s", node->fname);
		fd = rsh_cmd(rsh_buffer);
	
		if ((rsh_read_fd(fd, node->data, iop->size, &rsize) != RTEMS_SUCCESSFUL) ||
				(iop->size != rsize)) {
			free(node->data);
			SEM_GIVE;
			rtems_set_errno_and_return_minus_one(EIO);
		}
#ifdef CRCCHECK
        printf("loaded file %s -> CRC: 0x%X\n", node->fname, \
          calcBufferCRC((unsigned char *) node->data, (unsigned long) iop->size));
#endif
        
	} else node->data = NULL;

	SEM_GIVE;
	return RTEMS_SUCCESSFUL;
}


static int rtems_rsh_file_close(rtems_libio_t *iop)
{
	int 							fd, status = 0;
	unsigned long 		wsize;
	struct rsh_node_t *node;

	SEM_TAKE;
	/* fetch file informations */
	node = (struct rsh_node_t*) iop->file_info;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_close(%p)\n", iop);
#endif /* DEBUGMSG */

	if (((iop->flags & (LIBIO_FLAGS_WRITE | LIBIO_FLAGS_APPEND)) != 0) /* && (content_changed) */ )
	{
#if 0
		if (iop->size > 0) /* don't write empty files back */
		/* but on the other hand: truncate can reduce filesize! */
#endif
		{	
			sprintf(rsh_buffer, "cat > %s", node->fname);
			fd = rsh_cmd(rsh_buffer);
	
			/* copy file content back to host */
			status = ((rsh_write_fd(fd, node->data, iop->size, &wsize) != RTEMS_SUCCESSFUL) ||
								(iop->size != wsize));
		}
	}
	iop->size = (off_t) 0;
	iop->offset = (off_t) 0;
	if (node->data != NULL) free(node->data);
	SEM_GIVE;
	if (status) rtems_set_errno_and_return_minus_one(EIO);
	return RTEMS_SUCCESSFUL;
}


static int rtems_rsh_file_read(
  rtems_libio_t *iop,
  void          *buffer,
  size_t        count)
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_read(%p, %p, %li)\n", iop, buffer, (unsigned long) count);
#endif /* DEBUGMSG */

	SEM_TAKE;
	node = (struct rsh_node_t*) iop->file_info;

	if (count + iop->offset > iop->size) /* range check */
	{
#if 0
		if (0) /* read access on a file can never end up in file enlargement! */
		{
			iop->size = iop->offset + count; /* calc new file size */

			node->data = realloc(node->data, iop->size);
			if (node->data == NULL) {
				SEM_GIVE;
				rtems_set_errno_and_return_minus_one(ENOMEM);
			}
		} else 
#endif
        if (iop->offset < iop->size)
		    count = iop->size - iop->offset;
		else count = 0;
	}
	if (count != 0) memcpy(buffer, node->data + iop->offset, count);
#ifdef DEBUGMSG
	printf("rtems_rsh_file_read: %li bytes read\n", (unsigned long) count);
#endif /* DEBUGMSG */

	SEM_GIVE;
	return count;
}


static int rtems_rsh_file_write(
  rtems_libio_t *iop,
  const void    *buffer,
  size_t        count)
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_write(%p, %p, %li)\n", iop, buffer, (unsigned long) count);
#endif /* DEBUGMSG */

	SEM_TAKE;
	node = (struct rsh_node_t*) iop->file_info;

	if ((iop->flags & LIBIO_FLAGS_APPEND) != 0) iop->offset = iop->size;

	if (count + iop->offset > iop->size) /* access outside bufferrange */
	{
		if (1) /* write operations outside bufferrange causes allways buffer enlargement! */
		{
			iop->size = iop->offset + count; /* calc new file size */

			node->data = realloc(node->data, iop->size); /* enlarge buffer */
			if (node->data == NULL) {
				SEM_GIVE;
				rtems_set_errno_and_return_minus_one(ENOMEM);
			}
		} else if (iop->offset <= iop->size) /* recalc count */
			count = iop->size - iop->offset;
		else count = 0;
	}
	if (count != 0) memcpy(node->data + iop->offset, buffer, count);

	SEM_GIVE;
	return count;
}


static int rtems_rsh_file_ioctl(
  rtems_libio_t *iop,
  uint32_t       command,
  void          *buffer)
{
/* this function is at the moment only a dummy */
#ifdef DEBUGMSG
	printf("rtems_rsh_file_ioctl(%p, %li, %p)\n", iop, (unsigned long) command, buffer);
#endif /* DEBUGMSG */
  return RTEMS_SUCCESSFUL;
}


static off_t rtems_rsh_file_lseek(
  rtems_libio_t   *iop,
  off_t           offset,
  int             whence)
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_lssek(%p, %li, %i)\n", iop, (long) offset, whence);
#endif /* DEBUGMSG */

	SEM_TAKE;
	node = (struct rsh_node_t*) iop->file_info;
	
	if (iop->offset < 0) iop->offset = 0;

	/* the real file enlargement does the write function */
	SEM_GIVE;
	return iop->offset;
}


static int rtems_rsh_file_ftruncate(
  rtems_libio_t *iop,
  off_t          length)
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_truncate(%p, %lu)\n", iop, (unsigned long) length);
#endif /* DEBUGMSG */

	SEM_TAKE;
	node = (struct rsh_node_t *) iop->file_info;

	iop->size = length;
	if (iop->offset > iop->size) iop->offset = iop->size;

	if ((node->data != NULL) && (iop->size > 0)) {
		node->data = realloc(node->data, iop->size);
		if (node->data == NULL) {
			SEM_GIVE;
			rtems_set_errno_and_return_minus_one(ENOMEM);
		}
	}
#ifdef DEBUGMSG
	printf("rtems_rsh_file_truncate finished\n");
#endif /* DEBUGMSG */

	SEM_GIVE;
	return RTEMS_SUCCESSFUL;
}

/* CEXP loader needs fstat to determine TFTP file download or not (see cexp.c line 1144)
   because if the file transfer is via TFTP cexp copys the file content in a memory tmp file
   (The RTEMS TFTPfs is strictly sequential access). But the tmp file can't hold large objs
   (> 5MB or so). But our RSH implementation holds the whole file content in memory and allows
   random file access, so it isn't neccessary to copy the file twice */
static int rtems_rsh_file_fstat(
  rtems_filesystem_location_info_t *loc,
  struct stat                      *buf)
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rtems_rsh_file_fstat(%p, %p)\n", loc, buf);
#endif /* DEBUGMSG */

	SEM_TAKE;
	node = (struct rsh_node_t*) loc->node_access;
	memcpy(buf, &node->st_stat, sizeof(node->st_stat));
	SEM_GIVE;
	return RTEMS_SUCCESSFUL;
}

rtems_filesystem_file_handlers_r rtems_rsh_file_handlers = {
    rtems_rsh_file_open,      /* open */
    rtems_rsh_file_close,     /* close */
    rtems_rsh_file_read,      /* read */
    rtems_rsh_file_write,     /* write */
    rtems_rsh_file_ioctl,     /* ioctl */
    rtems_rsh_file_lseek,     /* lseek */
    rtems_rsh_file_fstat,     /* fstat */
    NULL,                     /* fchmod */
    rtems_rsh_file_ftruncate, /* ftruncate */
    NULL,                     /* fpathconf */
    NULL,                     /* fsync */
    NULL,                     /* fdatasync */
    NULL,                     /* fcntl */
    NULL                      /* rmnod */
};
