#ifndef _RSH_FILES_
#define _RSH_FILES_

#include <rtems/libio.h>
#include "rsh_basic.h"
/*
static int rtems_rsh_file_open(rtems_libio_t *iop, const char *new_name, uint32_t flags, uint32_t mode);
static int rtems_rsh_file_close(rtems_libio_t *iop);
static int rtems_rsh_file_read(rtems_libio_t *iop, void *buffer, size_t count);
static int rtems_rsh_file_write(rtems_libio_t *iop, const void *buffer, size_t count);
static int rtems_rsh_file_ioctl(rtems_libio_t *iop, uint32_t command, void *buffer);
static off_t rtems_rsh_file_lseek(rtems_libio_t *iop, off_t length, int whence);
*/
extern rtems_filesystem_file_handlers_r rtems_rsh_file_handlers;

#endif /* #ifndef _RSH_FILES_ */