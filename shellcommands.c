/*+**************************************************************************
 *
 * Project:        RTEMS-GeSys
 * Module:         useful shell functions
 * File:           shellcommands.c
 *
 * Description:    useful functions for shell usage
 *
 * Author(s):      Dan Eichel
 *
 * Copyright (c) 2013     Helmholtz-Zentrum Berlin 
 *                      fuer Materialien und Energie
 *                            Berlin, Germany
 *
 **************************************************************************-*/

#include <stdlib.h>               /* NULL */
#include <sys/types.h>
#include <sys/stat.h>

const char *epicsUsrOsTargetName(void)
{
    static char* T_A =
#if defined(RTEMS_TA)
    RTEMS_TA;
#else
    "RTEMS-generic";
#endif
    
    return T_A;
}

void cat(char *source)
{
    struct stat info;
    int fd, fsize, got;

    fd = open(source);
    if (fd <= 0) return; /* somthing bad has happend */
    
    fstat(fd, &info);
    fsize = info.st_size;
    
	while (fsize)
    {
    	char buf[32];
        int i;
	    
        if ((got = read( fd, buf, sizeof(buf))) < 0) break;
        for (i = 0; i < got; ++i) putchar((int) buf[i]);
        fsize -= got;
    }
    close(fd);
}

#if 1
extern rtems_shell_main_cp(int argc, char *argv[]);
void cp(char *source, char *dest, char *args)
{
    char *argv[3];
    
    argv[0] = (args != NULL) ? args : "";
    argv[1] = source;
    argv[2] = dest;

    rtems_shell_main_cp(3, argv);
}
#endif

