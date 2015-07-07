#ifndef _RSH_DRIVER_
#define _RSH_DRIVER_

#include <rtems/libio.h>
#include <rtems/libio_.h>

/* we need a dummy driver entry table to get a
 * major number from the system
 */
extern rtems_driver_address_table drvRsh;
/* holds the driver Id returned from OS by registering */
extern dev_t                      drvRshId;

#endif /* #ifdef _RSH_DRIVER_ */
