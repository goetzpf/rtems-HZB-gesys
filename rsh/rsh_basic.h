#ifndef _RSH_BASIC_
#define _RSH_BASIC_

#define rsh_user_maxlen  20

extern char *rsh_mnt_pnt; /* local mount point in filesystem */
extern char rsh_user[rsh_user_maxlen];
extern char rsh_buffer[512]; /* general purpose buffer... */
extern bool rsh_enabled; /* internal flag - used by evalpath to determine the user input */

extern void rsh_enable(void);
extern void rsh_disable(void);

extern int rsh_read_fd(int fd, unsigned char *buf, unsigned long size, unsigned long *rsize);
extern int rsh_write_fd(int fd, unsigned char *buf, unsigned long size, unsigned long *wsize);

/* main glue function: encapsulates all remote calls and delivers a 
   file descriptor to fetch the content of the answer */
extern int rsh_cmd(char *cmd);
/* set server for rsh access */
extern void rsh_setserver(char *server);
/* set user name for remote host access */
extern void rsh_iam(char *uname);
/* print actual user name for remote access */
extern void rsh_whoaim(void);
/* return 1 if an directory on the remote site exist */
extern int rsh_checkdir(const char *path);
/* return 1 if the remote file exist */
extern int rsh_checkfile(const char *file);
extern int rsh_chdir(char *uname);
extern void rsh_pwd(void);
/* creates absolute pathname */
extern void buildpath(char *buffer, char *path, bool isAbsPath);

#endif /* #ifdef _RSH_BASIC_ */
