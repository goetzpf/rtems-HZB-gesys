#include <rtems/seterr.h>			/* rtems_set_errno_and_return_minus_one() */
#include <rtems.h>						/* RTEMS_SUCCESSFUL */
#include <errno.h>						/* errno     */
#include <stdio.h>						/* xprintf() */
#include <string.h>						/* strlen()  */
#include <limits.h>						/* NAME_MAX  */
#include <sys/select.h>				/* select()  */

#include "rsh_basic.h"


#define rsh_path_maxlen 256
char *rsh_mnt_pnt;
char rsh_server[rsh_user_maxlen]; /* = "193.149.12.29"; */
char rsh_user[rsh_user_maxlen]   = "ioc";
char rsh_dir[NAME_MAX + 1] = "/";
char rsh_buffer[512]; /* general purpose buffer... */
bool rsh_enabled = false;

int rsh_read_fd(int fd, unsigned char *buf, unsigned long size, unsigned long *rsize)
{
	fd_set					r, w, e;       /* read set */
	struct timeval	timeout;
	int 						got = 0;       /* avoids compiler warning */
	unsigned long		pos = 0;

	*rsize = 0;
	if (fd < 0) rtems_set_errno_and_return_minus_one(EBADF);
	if (size == 0) {
		close(fd);
		return RTEMS_SUCCESSFUL;
	}

	while (1) {
#ifdef DEBUGMSG
		printf("rsh_read_fd: select\n");
#endif
		FD_ZERO(&r); FD_SET(fd, &r);
		FD_ZERO(&w); FD_ZERO(&e);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		switch (select(fd + 1, &r, &w, &e, &timeout)) {
		case 0: /* timeout */
			close(fd);
			fprintf(stderr,"rsh_read_fd: host connection timeout\n");
			rtems_set_errno_and_return_minus_one(EBUSY);
			break;
		case -1: /* error */
			close(fd);
			fprintf(stderr,"rsh_read_fd: host connection error\n");
			rtems_set_errno_and_return_minus_one(ECONNABORTED);
			break;
		case 1: /* host delivers us a error message */
		default: /* only to satisfy the compiler... */
			if (FD_ISSET(fd, &r))
				got = read(fd, buf + pos, size - pos);
			else got = 0;
	
			pos += (unsigned long) got;
			*rsize += (unsigned long) got;
			if ((got == 0) || (pos == size)) { /* exit */
				close(fd);
#ifdef DEBUGMSG
				printf("rsh_read_fd: finished, bytes read: %lu\n", *rsize);
#endif
				return RTEMS_SUCCESSFUL;
			}
		}
	}
	return RTEMS_SUCCESSFUL; /* never reached */
}


int rsh_write_fd(int fd, unsigned char *buf, unsigned long size, unsigned long *wsize)
{
	fd_set					r, w, e;
	struct timeval	timeout;
	int 						put = 0;       /* avoids compiler warning */
	unsigned long		pos = 0;

	*wsize = 0;
	if (fd < 0) rtems_set_errno_and_return_minus_one(EBADF);
	if (size == 0) {
		close(fd);
		return RTEMS_SUCCESSFUL;
	}

	while (1) {
#ifdef DEBUGMSG
		printf("rsh_write_fd: select\n");
#endif
		FD_ZERO(&r); FD_ZERO(&e);
		FD_ZERO(&w); FD_SET(fd, &w);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		switch (select(fd + 1, &r, &w, &e, &timeout)) {
		case 0: /* timeout */
			close(fd);
			fprintf(stderr,"rsh_write_fd: host connection timeout\n");
			rtems_set_errno_and_return_minus_one(EBUSY);
			break;
		case -1: /* error */
			close(fd);
			fprintf(stderr,"rsh_write_fd: host connection error\n");
			rtems_set_errno_and_return_minus_one(ECONNABORTED);
			break;
		case 1: /* socket is ready for write operation */
		default: /* only to satisfy the compiler... */ 
			if (FD_ISSET(fd, &w))
				put = write(fd, buf + pos, size - pos);
			else put = 0;
			pos += (unsigned long) put;
			*wsize += (unsigned long) put;
			if ((put == 0) || (pos == size)) { /* exit */
				close(fd);
#ifdef DEBUGMSG
				printf("rsh_write_fd: finished, bytes written: %lu\n", *wsize);
#endif
				return RTEMS_SUCCESSFUL;
			}
		}
	}
	return RTEMS_SUCCESSFUL; /* never reached */
}


/* main glue function: encapsulates all remote calls and delivers a 
   file descriptor to fetch the content of the answer */
int rsh_cmd(char *cmd)
{
  #define RSH_PORT 514
	int 	fd, errfd;
	char 	*pwillchanged = rsh_server;

  extern int rcmd();
#ifdef DEBUGMSG
	printf("rsh_cmd: \"%s\"\n", cmd);
#endif
	fd=rcmd(&pwillchanged, RSH_PORT, rsh_user, rsh_user, cmd, &errfd);
	if (fd<0) {
		fprintf(stderr,"rcmd: got no remote stdin/stdout descriptor\n");
    fd = -1;
		goto cleanup;
	}
	if (errfd<0) {
		fprintf(stderr,"rcmd: got no remote stderr descriptor\n");
		goto cleanup;
	}

cleanup:
	if (errfd >= 0)
  {
		fd_set					r, w, e;
		struct timeval	timeout;
    char 						buffer;

		/* if the error "stream" isn't empty it seems the 
       command execution on the host is failed */
		FD_ZERO(&r); FD_ZERO(&w); 
		FD_ZERO(&e); FD_SET(errfd, &e);
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		switch (select(errfd + 1, &r, &w, &e, &timeout)) {
		case 0: /* if timeout then is the error buffer probably empty */
			break;
		case 1:
		default: /* if (FD_ISSET(errfd, &e)) */
			if (!read(errfd, &buffer, sizeof(buffer))) break;
		case -1: /* error case */
			if (fd >= 0) close(fd);
			fd = -2;
			break;
		}
		close(errfd);
  }
	return fd;
}


void rsh_setserver(char *server)
{
  strncpy(rsh_server, server, sizeof(rsh_server));
  rsh_server[sizeof(rsh_server) - 1] = 0;
}


/* return 1: check on the remote site if an directory exist */
/* at the moment it seems the function wont work!!! */
int rsh_checkdir(const char *path)
{
	int fd;

	sprintf(rsh_buffer, "cd %s", path);
  fd = rsh_cmd(rsh_buffer);
  /* on success no error message will retrieved */
  if (fd >= 0) close(fd);
	return (fd >= 0);
}


/* return 1 if file exist */
int rsh_checkfile(const char *file)
{
	int 					fd;
	unsigned long	rsize;

	sprintf(rsh_buffer, "ls %s", file);
  fd = rsh_cmd(rsh_buffer);

	/* readout host's message */
	if (fd >= 0) {
		if (rsh_read_fd(fd, (void *) rsh_buffer, sizeof(rsh_buffer), &rsize) == RTEMS_SUCCESSFUL) {
			/* on succsess only the filename will returned from the host */
			if (rsize == 0) return 0; /* error */
			if (rsize > strlen(file)) rsize = strlen(file);
			return (!strncmp(file, rsh_buffer, rsize));
		}
	}
	return 0; /* unknown error */
}


int rsh_chdir(char *uname)
{
	int 						fd;
	unsigned long 	rsize;

	if (strncmp(uname, rsh_mnt_pnt, strlen(rsh_mnt_pnt)) == 0) /* absolute path */
		sprintf(rsh_buffer, "cd %s; pwd", uname + strlen(rsh_mnt_pnt));
	else /* relative path */
		sprintf(rsh_buffer, "cd %s/%s; pwd", rsh_dir, uname);

#ifdef DEBUGMSG
	printf("rsh_chdir trying: \"%s\"\n", rsh_buffer);
#endif

  fd = rsh_cmd(rsh_buffer);
	if (fd >= 0) {
		if (rsh_read_fd(fd, (void *)rsh_buffer, sizeof(rsh_buffer), &rsize) == RTEMS_SUCCESSFUL) {
			if (rsize) rsh_buffer[rsize - 1] = 0; /* cut string */
			strncpy(rsh_dir, rsh_buffer, NAME_MAX); /* read stripped remote path */
		}
	} else {
		fprintf(stderr,"rsh_chdir: %s: No such file or directory\n", uname);
		rtems_set_errno_and_return_minus_one(ENOENT);
	}
	return RTEMS_SUCCESSFUL;
}


void rsh_pwd(void)
{
	sprintf(rsh_buffer, "%s%s", rsh_mnt_pnt, rsh_dir);
	puts(rsh_buffer);
}


void buildpath(char *buffer, char *path, bool isAbsPath)
{
  if (isAbsPath)
		sprintf(buffer, "/%s", path);
	else
		sprintf(buffer, "%s/%s", rsh_dir, path);

#ifdef DEBUGMSG
	printf("buildpath: %s\n", buffer);
#endif
}

