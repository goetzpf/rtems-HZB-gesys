#ifndef _RSH_H_
#define _RSH_H_

#include "rsh_basic.h"

/* "shell" interface */
extern void rsh_mount(char *mntpoint);
extern void rsh_unmount(void);

extern void rsh_enable(void);
extern void rsh_disable(void);
extern void rsh_iam(char *uname);
extern void rsh_whoami(void);
extern void pwd(void);
extern int cd(char *path);


#endif /* #ifdef _RSH_H_ */
