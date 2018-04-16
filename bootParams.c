/*+**************************************************************************
 *
 * Project:        RTEMS-GeSys
 * Module:         boot parameters access
 * File:           bootParams.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libtecla.h>
#include <sys/types.h>

#include <bsp/bootLib.h>
#include <bsp/NVRAMaccess.h>
#include "bootParams.h"

static const size_t param_lengths[] = {
    BOOT_DEV_LEN,
    BOOT_HOST_LEN,
    BOOT_HOST_LEN,
    BOOT_TARGET_ADDR_LEN,
    BOOT_TARGET_ADDR_LEN,
    BOOT_ADDR_LEN,
    BOOT_ADDR_LEN,
    BOOT_FILE_LEN,
    BOOT_FILE_LEN,
    BOOT_USR_LEN,
    BOOT_PASSWORD_LEN,
    BOOT_OTHER_LEN,
    4,
    4,
    4
};

static const size_t param_offsets[] = {
    offsetof(BOOT_PARAMS, bootDev),
    offsetof(BOOT_PARAMS, hostName),
    offsetof(BOOT_PARAMS, targetName),
    offsetof(BOOT_PARAMS, ead),
    offsetof(BOOT_PARAMS, bad),
    offsetof(BOOT_PARAMS, had),
    offsetof(BOOT_PARAMS, gad),
    offsetof(BOOT_PARAMS, bootFile),
    offsetof(BOOT_PARAMS, startupScript),
    offsetof(BOOT_PARAMS, usr),
    offsetof(BOOT_PARAMS, passwd),
    offsetof(BOOT_PARAMS, other),
    offsetof(BOOT_PARAMS, procNum),
    offsetof(BOOT_PARAMS, flags),
    offsetof(BOOT_PARAMS, unitNum)
};

/*+**************************************************************************
 *  lookup table: dispatch prompt to BOOT_PARAMS field
 **************************************************************************-*/
static const size_t lookup[] = {
    0,
    14,
    12,
    1,
    7,
    3,
    4,
    5,
    6,
    9,
    10,
    13,
    2,
    8,
    11
};

/*+**************************************************************************
 *  menu strings used by bootShow and bootChange
 **************************************************************************-*/
static const char *prompts[] =
{
    "boot device          : ",
    "unit number          : ",
    "processor number     : ",
/*    "host name            : ", */
    "host RSH mount point : ",
    "file name            : ",
    "inet on ethernet (e) : ",
    "inet on backplane (b): ",
    "host inet (h)        : ",
    "gateway inet (g)     : ",
    "user (u)             : ",
    "ftp password (pw) (blank = use rsh):",
    "flags (f)            : ",
    "target name (tn)     : ",
    "startup script (s)   : ",
/*    "other (o)            : " */
    "NFS [server:]mnt (o) : "
};
#define MAX_LINES (sizeof(prompts)/sizeof(*prompts))

/*+**************************************************************************
 *  boot flags meaning
 **************************************************************************-*/
static const char *usage[] =
{
    "boot flags meaning:                   ",
    "    0x01 - enable RSH                 ",
    "    0x02 - enable tftp                ",
    "    0x04 - enable NFS                 ",
    "    0x08 - get script by tftp (otherwise by RSH)",
    "    0x10 - unused                     ",
    "    0x20 - unused                     ",
    "    0x40 - unused                     ",
    "    0x80 - unused                     "
};


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
void bootShow(void)
{
    BOOT_PARAMS params;
    int i;
    char *str;
    
    /* fill params from NVRAM */
    readNVram(&params);
    
    for (i = 0; i < MAX_LINES; ++i)
    {
        str = (char *) ((int) &params + (int) param_offsets[lookup[i]]);
#ifndef DEBUG
        if (((lookup[i] < 12) && (*str != 0)) || (lookup[i] > 11)) /* display all numerical values */
#endif        
        {
            fputs(prompts[i], stdout);
            if (lookup[i] > 11)
                printf((lookup[i] == 13) ? "0x%x" : "%i", *(int *) str);
            else fputs(str, stdout);
            fputs("\n", stdout);
        }
    }
}


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
void bootChange(void)
{
    BOOT_PARAMS params;
    GetLine	*gl = NULL;
    int actline = 0, len, abort = 0, field;
    char *line, *paramptr, intbuf[10];

    fputs("'.' = clear field;  '-' = go to previous field;  ^C = quit\n\n", stdout);
#ifndef EVERY_LINE
    gl = new_GetLine(BOOT_FIELD_LEN - 1, 0); /* maxlinelen, no history */
    gl_configure_getline(gl,0,0,0);
    /* override 80x24 char default terminal size,
       prevents ugly cursor jumps on backspace commands */
    gl_terminal_size(gl, 200, 1);
#endif

    /* read the nvram */
    readNVram(&params);
    do
    {
        field = lookup[actline]; /* get vxWorks field number */
        paramptr = (char *) (((char *) &params) + param_offsets[field]);
	    
#ifdef EVERY_LINE
        gl = new_GetLine(param_lengths[field] - 1, 0); /* maxlinelen, no history */
	    gl_configure_getline(gl,0,0,0);
        gl_terminal_size(gl, 200, 1); /* see comment above */
#endif
        
        if (field == 13) /* print boot flags */
        {
            int i, lines = sizeof(usage) / sizeof(usage[0]);
            for (i = 0; i < lines; ++i) printf("%s\n", usage[i]);
        }

        if (field > 11) /* integer inputs */
        {
            sprintf(intbuf, (field == 13) ? "0x%x" : "%i", *((int *) paramptr));
            line = gl_get_line(gl, (char *) prompts[actline], intbuf, -1);
        } else line = gl_get_line(gl, (char *) prompts[actline], paramptr, -1);
        
        if (line == NULL)
        {
#ifdef EVERY_LINE
            del_GetLine(gl);
#endif
            abort = 1;
            break;
        } else if (strcmp("-\n", line) == 0)
        {
            if (actline) --actline;
        } else if (strcmp(".\n", line) == 0)
        {
            *paramptr = 0;
            ++actline;
        } else /* save user input */
        {
            if (field > 11) /* integer inputs */
            {
                *((int *) paramptr) = (int) strtol(line, NULL, 0);
            
            } else /* string inputs */
            {
                int showmsg = 0;
                
                /* adjust length according to field size */
                len = strlen(line);
                if ((len > 0) && (line[len - 1] == '\n')) --len; /* ignore \n */
                if (len >= param_lengths[field])
                {
                    len = param_lengths[field] - 1;
                    showmsg = 1;
                }

                strncpy(paramptr, line, len); /* save inputline */
                paramptr[len] = 0; /* terminate string */
                if (showmsg)
                {
                    printf("Max string length exceeded, truncated to '%s'\n", paramptr);
                    fflush(stdout);
                }
            }
            ++actline;
        }
#ifdef EVERY_LINE
        gl = del_GetLine(gl);
#endif
    } while (actline < MAX_LINES);
#ifndef EVERY_LINE
    gl = del_GetLine(gl);
#endif
    if (!abort) /* copy boot params back! */
        writeNVram(&params);
}
