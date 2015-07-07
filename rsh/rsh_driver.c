#include <rtems.h>
#include "rsh_driver.h"


static rtems_device_driver rsh_initialize(rtems_device_major_number major,
                                          rtems_device_minor_number minor,
                                          void *arg)
{
	/* Something must be present, however, to 
	 * reserve a slot in the driver table.
	 */
#ifdef DEBUGMSG
	printf("RSH-Driver initialize\n");
#endif
	return RTEMS_SUCCESSFUL;
}

static rtems_device_driver rsh_open(rtems_device_major_number major,
                                    rtems_device_minor_number minor,
                                    void *arg)
{
#ifdef DEBUGMSG
	printf("RSH-Driver open(%i, %i, %p)\n", (int) major, (int) minor, arg);
#endif
	return RTEMS_SUCCESSFUL;
}

static rtems_device_driver rsh_close(rtems_device_major_number major,
                                     rtems_device_minor_number minor,
                                     void *arg)
{
#ifdef DEBUGMSG
	printf("RSH-Driver close(%i, %i, %p)\n", (int) major, (int) minor, arg);
#endif
	return RTEMS_SUCCESSFUL;
}

rtems_driver_address_table drvRsh = {
		rsh_initialize,		/* initialize */
		rsh_open,					/* open       */
		rsh_close,				/* close      */
		NULL,							/* read       */
		NULL,							/* write      */
		NULL							/* control    */
};

dev_t	drvRshId;
