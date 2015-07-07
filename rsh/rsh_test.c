#include <stdio.h>						/* xprintf() */
#include <string.h>

extern int rcmd();

void creatershfile(const char *content)
{
    int	fd = -1;
    int perrfd;
    char *willbechanged = "193.149.12.29";
		char *luser = "ioc";
		char *ruser = "ioc";
		char *cmd = "cat > /home/eichel/ctl/ioc/test.txt";
		int count = 0;
    
    #define RSH_PORT 514
	  fd = rcmd(&willbechanged, RSH_PORT, luser, ruser, cmd, &perrfd);
    if (fd >= 0) 
		{
			count = write(fd, content, strlen(content));

			printf("remote file created with size: %u\n", count);
			close( fd );
		}
		if (perrfd >= 0) close( perrfd );
}

void rshtest(char *server, char *luser, char *ruser, char *cmd)
{
    int	fd = -1;
    int perrfd;
    char *willbechanged = server;
    char buf[512];
    
    int got, put, n;
    char *b;
    long fsize = 0;
    
    #define RSH_PORT 514
	  fd = rcmd(&willbechanged, RSH_PORT, luser, ruser, cmd, &perrfd);
    
	if ( perrfd ) 
    {
		if (fd<0) {
			printf("rcmd"/* (%s)*/": got no remote stdout descriptor\n"
							/* ,strerror(errno)*/);

			if ( perrfd >= 0 )
				close( perrfd );
			perrfd = -1;
		} else if ( perrfd<0 ) {
			printf("rcmd"/* (%s)*/": got no remote stderr descriptor\n"
							/* ,strerror(errno)*/);
			if ( fd>=0 )
				close( fd );
			fd = -1;
		}
	} 
    
	while ( (got = read( fd, buf, sizeof(buf) )) ) 
    {
	    for ( b = buf, n = got; n > 0; n-=put, b+=put, fsize += put ) 
        {
		    if ( (put = write(1, b, n)) <= 0 ) 
            {
			    fprintf(stderr,"rshCopy() unable to write(%li)", fsize);
			    return;
            }
        }
    }
    printf("bytes copied: %li\n", fsize);

fsize = 0;	
while ( (got = read( perrfd, buf, sizeof(buf) )) ) 
    {
	    for ( b = buf, n = got; n > 0; n-=put, b+=put, fsize += put ) 
        {
		    if ( (put = write(1, b, n)) <= 0 ) 
            {
			    fprintf(stderr,"rshCopy() unable to write(%li)", fsize);
			    return;
            }
        }
    }
    
    
/*    
    memset( buf, 0, sizeof(buf) );
    while (got = read( fd, buf, sizeof(buf)))
        for (i = 0; i < got; ++i) putc(buf[i], stdout);
*/
    
	if (perrfd >= 0) close( perrfd );
    if (fd >= 0) close( fd );
}
