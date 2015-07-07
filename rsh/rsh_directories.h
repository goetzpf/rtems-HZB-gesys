#ifndef _RSH_DIRS_
#define _RSH_DIRS_

#include <rtems/libio.h>
#include <rtems/libio_.h>
/*
static int rtems_rsh_dir_open(rtems_libio_t *iop, const char *new_name, uint32_t flags, uint32_t mode);
static int rtems_rsh_dir_close(rtems_libio_t *iop);
static int rtems_rsh_dir_read(rtems_libio_t *iop, void *buffer, size_t count);
static off_t rtems_rsh_dir_lseek(rtems_libio_t *iop, off_t length, int whence);
static int rtems_rsh_dir_fstat(rtems_filesystem_location_info_t *iop, struct stat *buf);
*/
extern rtems_filesystem_file_handlers_r rtems_rsh_dir_handlers;

#endif /* #ifndef _RSH_DIRS_ */