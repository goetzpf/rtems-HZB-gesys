/*
 * The rsh open handler
 */
static int rtems_rsh_open(
    rtems_libio_t *iop,
    const char    *new_name,
    uint32_t       flags,
    uint32_t       mode
)
{
	printf("rtems_rsh_open(%p, \"%s\", %x, %x)\n", iop, new_name, flags, mode);
	return 0;
}

static int rtems_rsh_close(rtems_libio_t *iop)
{
	printf("rtems_rsh_close(%p)\n", iop);
	return 0;
}

static int rtems_rsh_read(
  rtems_libio_t *iop,
  void       *buffer,
  uint32_t    count
)
{
	printf("rtems_rsh_read(%p, %p, %l)\n", iop, buffer, count);
	return count;
}

static int rtems_rsh_write(
  rtems_libio_t *iop,
  void       *buffer,
  uint32_t    count
)
{
	printf("rtems_rsh_write(%p, %p, %l)\n", iop, buffer, count);
	return count;
}

/* at this time, we only support the basic file operations */
rtems_filesystem_file_handlers_r rtems_rsh_handlers = {
    rtems_rsh_open,   /* open */     
    rtems_rsh_close,  /* close */    
    rtems_rsh_read,   /* read */     
    rtems_rsh_write,  /* write */    
    NULL,              /* ioctl */    
    NULL,              /* lseek */    
    NULL,              /* fstat */    
    NULL,              /* fchmod */   
    NULL, 						 /* ftruncate */
    NULL,              /* fpathconf */
    NULL,              /* fsync */    
    NULL,              /* fdatasync */
    NULL,              /* fcntl */
    NULL               /* rmnod */
};
