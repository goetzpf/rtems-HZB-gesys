/*+**************************************************************************
 *
 * Project:        RTEMS-GeSys
 * Module:         boot parameters access
 * File:           bootParams.h
 *
 * Description:    access to RTEMS boot parameters
 *
 * Author(s):      Dan Eichel
 *
 * Copyright (c) 2013     Helmholtz-Zentrum Berlin 
 *                      fuer Materialien und Energie
 *                            Berlin, Germany
 *
 **************************************************************************-*/

#ifndef __BOOTPARAMS__
#define __BOOTPARAMS__

#include <rtems/rtems_bsdnet.h>


/*+**************************************************************************
 *
 * Function:    bootShow
 *
 * Description: Shows the actual boot parameters
 *              (like the vxWorks bootShow command)
 *
 * Arg In:      none
 *
 * Return(s):   none
 *
 **************************************************************************-*/
extern void bootShow(void);


/*+**************************************************************************
 *
 * Function:    bootChange
 *
 * Description: Editor for RTEMS boot parameters
 *              (similar to the vxWorks c / bootChange command)
 *
 * Arg In:      none
 *
 * Return(s):   none
 *
 **************************************************************************-*/
extern void bootChange(void);


#endif /* __BOOTPARAMS__ */
