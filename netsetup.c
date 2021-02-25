/*+**************************************************************************
 *
 * Project:        RTEMS-GeSys
 * Module:         network startup
 * File:           netsetup.c
 *
 * Description:    RTEMS network startup initialisation with parameters from NVRAM
 *
 * Author(s):      Dan Eichel
 *
 * Copyright (c) 2015     Helmholtz-Zentrum Berlin 
 *                      fuer Materialien und Energie
 *                            Berlin, Germany
 *
 **************************************************************************-*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libtecla.h>
#include <sys/types.h>
#include <rtems/userenv.h>

#include <bsp/bootLib.h>
#include <bsp/NVRAMaccess.h>
#include "bootParams.h"
#ifdef RSH_SUPPORT
#include "rsh.h"
#endif


/*+**************************************************************************
 *
 * Function:    gen_network_setup
 *
 * Description: initializes the RTEMS kernel with boot parameters from NVRAM
 *
 * Arg In:      cfg - unused pointer
 *              ifcfg - unused pointer
 *
 * Return(s):   0 - success
 *
 **************************************************************************-*/
int gen_network_setup(struct rtems_bsdnet_config *cfg, struct rtems_bsdnet_ifconfig *ifcfg)
{
    static BOOT_PARAMS params;
    static char ip_netmask[INET_ADDRSTRLEN];
    uint32_t subnetmask;
    char *cptr;
    
    char buffer[512] = "";
    
    /* fill params from NVRAM */
    readNVram(&params);

	if ( !ifcfg )
    {
    	if ((strcmp(RTEMS_TA, "RTEMS-mvme3100") == 0) && (strcmp(params.bootDev, "enet") == 0) && (params.unitNum == 0))
        {
        	strcpy(buffer, "tse1"); /* translate user settings: enet0 <-> tse1 */
        } else
        {         
			sprintf(buffer, "%s%i", params.bootDev,	params.unitNum); /* build full device name */
        }
    	ifcfg = cfg->ifconfig; /* point to the first interface configuration */
        while ((ifcfg) && (strcmp(buffer, ifcfg->name))) { ifcfg = ifcfg->next; }
	
		if (!ifcfg)
        {
			fprintf(stderr,"No interface configuration found\n");
			return -1;
		}
	}
	printf("Configuring '%s' from boot environment parameters...\n", ifcfg->name);


    if ((cptr = strchr(params.ead, ':')) != NULL) /* subnet mask found */
    {
        *cptr++ = 0; /* split inetaddr and subnet mask */
        subnetmask = strtoul(cptr, NULL, 16);
    } else subnetmask = DEFAULT_SUBNETMASK; /* default: 255.255.255.0 */
    

    /*
     * Assume that the boot server is also the name, log and ntp server!
     */
    cfg->bootp = NULL;
    cfg->hostname = params.targetName;
    cfg->gateway = params.gad;
    cfg->name_server[0] = \
    cfg->ntp_server[0]  = \
    rtems_bsdnet_bootp_server_name     = params.had;
    rtems_bsdnet_bootp_server_address.s_addr = bootlib_addrToInt(params.had);
    rtems_bsdnet_bootp_boot_file_name = params.bootFile;
    rtems_bsdnet_bootp_cmdline = params.startupScript;
	
    ifcfg->ip_address = params.ead;
    ifcfg->ip_netmask = bootlib_addrToStr(ip_netmask, subnetmask);

    sprintf(buffer, "/%s", params.hostName);
    setenv("HOSTNAME", buffer, 1);

    if (params.flags & 0x01) setenv("USERSH", "yes", 1);
    if (params.flags & 0x02) setenv("USETFTP", "yes", 1);
    if (params.flags & 0x04) setenv("USENFS", "yes", 1);

    if (params.flags & 0x08) /* tftp script download */
    {
        strcpy(buffer, "/TFTP/BOOTP_HOST");
        setenv("USETFTP", "yes", 1);
    }
    else /* rsh */
    {
        sprintf(buffer, "/%s", params.hostName);
        setenv("USERSH", "yes", 1);
    }
    strcat(buffer, rtems_bsdnet_bootp_cmdline);
    setenv("INIT", buffer, 1);
    
    if ((cptr = strchr(params.other, ':')) != NULL) /* server and mount point defined! */
    {
        *cptr++ = 0; /* split */
        setenv("NFSSERVER", params.other, 1);    
        setenv("NFSMNTPNT", cptr, 1);
    } else setenv("NFSSERVER", params.had, 1);    

    setenv("TARGETNAME", params.targetName, 1);
    setenv("RSHUSER", params.usr, 1);

    return 0;
}
