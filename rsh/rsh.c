/* Idea: path dependend functions rsh_cd & rsh_pwd working only on the host side
   on the local machine only file functions offered */

#include <rtems/libio.h>
#include <rtems/libio_.h>
#include <stdio.h>					/* printf() */
#include <rtems/seterr.h>	   	    /* rtems_set_errno_and_return_minus_one() */
#include <stdlib.h>					/* malloc() */
#include <dirent.h>					/* struct dirent */

#include "rsh_basic.h"
#include "rsh_driver.h"
#include "rsh_node.h"
#include "rsh_files.h"


/* forward declarations */
extern struct _rtems_filesystem_operations_table rsh_fs_ops;
extern rtems_filesystem_limits_and_options_t rtems_rsh_limits_and_options;


/* the root dir-node of our "filesystem" */
struct rsh_node_t *rsh_root_node, *rsh_relative_path;


/* evaluate path function */
static int rsh_evalpath(
	const char	*path,													/* IN */
	int					flags,													/* IN */
	rtems_filesystem_location_info_t *pathloc)	/* IN/OUT */
{
	rtems_filesystem_location_info_t    newloc;
	int																	updir = 0, abspath = 0, offset = 0;
	char																buffer[512];
	struct rsh_node_t 									*newnode, tmpnode;

#ifdef DEBUGMSG
	printf("rsh_evalpath(%s, %i, %p)\n", path, flags, pathloc);
#endif

	if (!rsh_enabled) { /* disabled try to access root */
		if ((path[0] == '.') && ((path[0] != 0) && (path[1] == '.'))) { /* relative access of parent dir */
			updir = 1; offset = 2;
		} else if ((path[0] == '/') || (path[0] == '\\')) { /* absolute pathnames */
			updir = 1; abspath = 1;
		}
		if (updir) {
			/* if (abspath) */
				newloc = rtems_filesystem_root;
			/* else */
			*pathloc = newloc;
			return (*pathloc->ops->evalpath_h)(&(path[offset]), flags, pathloc);
		} else return -1; /* error */
	} else { /* rsh mode enabled */
	  pathloc->ops         = &rsh_fs_ops;
		pathloc->handlers    = &rtems_rsh_file_handlers; /* only files supported */

		/* pathloc argument: if the user enters an absolute filename or 
			 cross the mount point from a different directory then the filesystem
			 will deliver the root node of the mounted fs - here rsh_root_node.
			 If the user enters a relative filename (and the rsh mode is enabled) the
			 the system will transfer the current location (only if the current dir is 
			 benath the mount-point!). In our case is this rsh_relative_path. So we can 
			 easily check what the users intension is. */
		if (strcmp(path, "enable") == 0) {
			pathloc->node_access = rsh_relative_path; /* return pathloc with different addr */
			return RTEMS_SUCCESSFUL; /* user want ONLY to enable the rsh-mode */
		}
		buildpath(buffer, (char *) path, (pathloc->node_access == rsh_root_node));

		if (rsh_fill_inode(buffer, &tmpnode) != RTEMS_SUCCESSFUL)
			rtems_set_errno_and_return_minus_one( ENOENT );

		if (RTEMS_SUCCESSFUL != rsh_check_inode(tmpnode.st_stat.st_ino)) { /* not open yet */
#ifdef DEBUGMSG
			printf("rsh_evalpath: ERROR: file is already open\n");
#endif
			rtems_set_errno_and_return_minus_one(EEXIST);
		}
		if (RTEMS_SUCCESSFUL != rsh_tag_inode(tmpnode.st_stat.st_ino)) { /* too many open files */
#ifdef DEBUGMSG
			printf("rsh_evalpath: ERROR: too many open files\n");
#endif
			rtems_set_errno_and_return_minus_one(ENFILE);
		}

#ifdef DEBUGMSG
		printf("rsh_evalpath: creating new file node\n");
#endif

		/* create the new node */
		newnode = malloc(sizeof(struct rsh_node_t));
		if (newnode == NULL) {
			rsh_free_inode(tmpnode.st_stat.st_ino);
#ifdef DEBUGMSG
			printf("rsh_evalpath: creating new file node - ERROR no mem\n");
#endif
			rtems_set_errno_and_return_minus_one(ENOMEM);
		}
		memcpy(newnode, &tmpnode, sizeof(struct rsh_node_t));

		/* return new created file node */
		pathloc->node_access = newnode;
	}
	return RTEMS_SUCCESSFUL;
}


/* rsh doesn't support directory creation */
static int rsh_evalformake(
	const char                       *path,       /* IN */
	rtems_filesystem_location_info_t *pathloc,    /* IN/OUT */
	const char                      **pname)      /* OUT    */
{
#ifdef DEBUGMSG
	printf("rsh_evalformake(%s, %p, out)\n", path, pathloc);
#endif
	*pname = path; /* deliver pathname - needed by mknod */
	return RTEMS_SUCCESSFUL;
}


/* return the type of assigned node 
 * return: RTEMS_FILESYSTEM_DIRECTORY or
 *         RTEMS_FILESYSTEM_MEMORY_FILE
 */
static rtems_filesystem_node_types_t rsh_node_type(
     rtems_filesystem_location_info_t        *pathloc) /* IN */
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rsh_node_type(%p)\n", pathloc);
#endif

	node = (struct rsh_node_t*) pathloc->node_access;
#ifdef DEBUGMSG
	printf("rsh_node_type: 0x%X\n", node->node_type);
#endif
	return node->node_type;
}


/* creates a new file node */
static int rsh_mknod(
	const char                        *path,       /* IN */
	mode_t                             mode,       /* IN */
	dev_t                              dev,        /* IN */
	rtems_filesystem_location_info_t  *pathloc     /* IN/OUT */
)
{
	int																	fd;
	char																buffer[512];
	struct rsh_node_t 									*newnode, tmpnode;

#ifdef DEBUGMSG
	printf("rsh_mknod(%s, %i, %p)\n", path, mode, pathloc);
#endif

	buildpath(buffer, (char *) path, (pathloc->node_access == rsh_root_node));

	/* create file for getting a valid and unique inode number */
	sprintf(rsh_buffer, "touch %s", buffer);
	fd = rsh_cmd(rsh_buffer); 
  if (fd >= 0) close (fd);

	rsh_fill_inode(buffer, &tmpnode); /* file MUST exist */
	if (RTEMS_SUCCESSFUL != rsh_check_inode(tmpnode.st_stat.st_ino))
		rtems_set_errno_and_return_minus_one(EEXIST); /* not open yet */
	if (RTEMS_SUCCESSFUL != rsh_tag_inode(tmpnode.st_stat.st_ino))
		rtems_set_errno_and_return_minus_one(ENFILE); /* too many open files */

	/* create the new node */
	newnode = malloc(sizeof(struct rsh_node_t));
	if (newnode == NULL) {
		rsh_free_inode(tmpnode.st_stat.st_ino);
		rtems_set_errno_and_return_minus_one(ENOMEM);
	}
	memcpy(newnode, &tmpnode, sizeof(struct rsh_node_t));

	/* return new created file node, most structure elements filled in by evalpath() */
	pathloc->ops         = &rsh_fs_ops;
	pathloc->handlers    = &rtems_rsh_file_handlers;
	pathloc->node_access = newnode;

	return RTEMS_SUCCESSFUL;
}


/* we have only two directory entries, its the root of our mounted 
 * filesystem so we can't free it, on the other hand all of the 
 * file node were on the fly generated. They can without much 
 * effort erased */
static int rsh_freenode(
 rtems_filesystem_location_info_t      *pathloc) /* IN */
{
	struct rsh_node_t *node;

#ifdef DEBUGMSG
	printf("rsh_freenode(%p)\n", pathloc);
#endif
	node = (struct rsh_node_t*) pathloc->node_access;

	if (node != NULL) {
		if (node->node_type == RTEMS_FILESYSTEM_MEMORY_FILE) {
#ifdef DEBUGMSG
			printf("rsh_freenode: free file node\n");
#endif
			rsh_free_inode(node->st_stat.st_ino);
			free(pathloc->node_access);
			pathloc->node_access = NULL;
		} else { /* directories not supported yet */
#ifdef DEBUGMSG
			printf("rsh_freenode: dir node -> handling not implemented yet\n");
#endif
		}
	}
	return RTEMS_SUCCESSFUL;
}


/* This op is called as the last step of mounting this FS */
static int rsh_fsmount_me(
	rtems_filesystem_mount_table_entry_t *mt_entry
)
{
#ifdef DEBUGMSG
	printf("rsh_fsmount_me(%p)\n", mt_entry);
#endif

	/* alloc mem for inode list */
	rsh_initialize_inode_list();

	/* create root node for rsh-fs */
	rsh_root_node = malloc(sizeof(*rsh_root_node));
	rsh_root_node->node_type = RTEMS_FILESYSTEM_DIRECTORY;

	rsh_root_node->fname[0] = 0; /* empty: points to root on the host */
	rsh_root_node->exist = 1; /* '/' always exist */
	/* fill stat structure */
	rsh_root_node->st_stat.st_dev = rtems_filesystem_dev_major_t(drvRshId);
	rsh_root_node->st_stat.st_rdev = rsh_root_node->st_stat.st_dev;
	rsh_root_node->st_stat.st_ino = (ino_t) &rsh_fsmount_me; /* is this unique? */
	rsh_root_node->st_stat.st_nlink = 1;
	rsh_root_node->st_stat.st_uid = 0; /* root */
	rsh_root_node->st_stat.st_gid = 0; /* root */
	rsh_root_node->data = (void *) NULL;
	rsh_root_node->st_stat.st_size = 0;
	rsh_root_node->st_stat.st_blksize = 0;
	rsh_root_node->st_stat.st_blocks = 0;

	mt_entry->mt_fs_root.handlers         = &rtems_rsh_file_handlers;
	mt_entry->mt_fs_root.ops              = &rsh_fs_ops;
  mt_entry->mt_fs_root.node_access      = rsh_root_node;
  mt_entry->fs_info                     = NULL; /* pointer to host ip? */
  mt_entry->pathconf_limits_and_options = rtems_rsh_limits_and_options; 
	mt_entry->dev = (char *) rtems_filesystem_dev_major_t(drvRshId);	/* store the drivers id, if anyone needs this */

	/* create a second node to distinguish between absolute and relative path  specifications */
	/* we are not interested in the structures elements, we only need a different address */
	rsh_relative_path = malloc(sizeof(*rsh_relative_path));
	memcpy(rsh_relative_path, rsh_root_node, sizeof(*rsh_relative_path));

	return RTEMS_SUCCESSFUL;
}




struct _rtems_filesystem_operations_table rsh_fs_ops = {
		rsh_evalpath,			/* MANDATORY */
		rsh_evalformake,	/* MANDATORY; may set errno=ENOSYS and return -1 */
		NULL, 						/* nfs_link OPTIONAL; may be NULL */
		NULL, 						/* nfs_unlink OPTIONAL; may be NULL */
		rsh_node_type,		/* nfs_node_type OPTIONAL; may be NULL; BUG in mount - no test!! */
		rsh_mknod,				/* nfs_mknod OPTIONAL; may be NULL */ /* needed for create file / dir */
		NULL,							/* nfs_chown OPTIONAL; may be NULL */
		rsh_freenode,			/* nfs_freenode OPTIONAL; may be NULL; (release node_access) */
		NULL, 						/* nfs_mount OPTIONAL; may be NULL */
		rsh_fsmount_me,		/* nfs_fsmount_me OPTIONAL; may be NULL -- but this makes NO SENSE */
		NULL,							/* nfs_unmount OPTIONAL; may be NULL */
		NULL,							/* nfs_fsunmount_me OPTIONAL; may be NULL */
		NULL,							/* nfs_utime OPTIONAL; may be NULL */
		NULL,							/* nfs_eval_link OPTIONAL; may be NULL */
		NULL,							/* nfs_symlink OPTIONAL; may be NULL */
		NULL							/* nfs_readlink OPTIONAL; may be NULL */
};

rtems_filesystem_limits_and_options_t rtems_rsh_limits_and_options = {
   5,   /* link_max */
   6,   /* max_canon */
   7,   /* max_input */
   255, /* name_max */
   255, /* path_max */
   2,   /* pipe_buf */
   1,   /* posix_async_io */
   2,   /* posix_chown_restrictions */
   3,   /* posix_no_trunc */
   4,   /* posix_prio_io */
   5,   /* posix_sync_io */
   6    /* posix_vdisable */
};

/* shell functions */
extern int rtems_shell_main_pwd(int argc, char *argv[]);

void rsh_mount(char *mntpoint)
{
	char 																	*rshDriver = "/dev/rsh";
	int 																	status;
	rtems_filesystem_mount_table_entry_t	*mtab;
	rtems_device_major_number							major;

    rsh_mnt_pnt = strdup(mntpoint);
    status = mkdir( rsh_mnt_pnt, S_IRWXU | S_IRWXG | S_IRWXO );
    if ( status == -1 )
	{
		printf("error: mkdir\n");
		return;
	}

/* driver registering */
	if (RTEMS_SUCCESSFUL != rtems_io_register_driver(0, &drvRsh, &major)) {
		fprintf(stderr,"Registering rsh driver failed (driver) - %s\n", strerror(errno));
		return;
	}
    if (RTEMS_SUCCESSFUL != rtems_io_register_name(rshDriver, major, 0)) {
		fprintf(stderr, "Registering rsh driver failed (name)\n");
		return;
    }
	
	/* build a device Id; needed to an get entry in the device driver table */
	drvRshId = rtems_filesystem_make_dev_t(major, 0);

#ifdef DEBUGMSG
	printf("rsh-driver Id: %i\n", (int) major);
#endif

	status = mount(&mtab,
			  &rsh_fs_ops,
			  RTEMS_FILESYSTEM_READ_WRITE,
			  rshDriver, /* dummy driver */
			  rsh_mnt_pnt);
	if (status)
	{
		fprintf(stderr, "rsh driver: could'nt mount '/RSH'\n");
		return;
	}
	rsh_initialize_inode_list();

    extern char rsh_server[];
	printf("mounting the rsh filesystem on %s (%s@%s)\n", rsh_mnt_pnt, rsh_user, rsh_server);
}


void rsh_unmount(void)
{
	rsh_free_inode_list();

	free(rsh_root_node);
	free(rsh_relative_path);
	if (RTEMS_SUCCESSFUL != unmount(rsh_mnt_pnt)) {
		fprintf(stderr,"Couldn't unmount rsh filesystem - %s\n", strerror(errno));
		return;
	}
	rmdir(rsh_mnt_pnt);
}


void rsh_enable(void)
{
	rsh_enabled = true;
	sprintf(rsh_buffer, "%s/enable", rsh_mnt_pnt);
	chdir(rsh_buffer);
}

void rsh_disable(void)
{
	rsh_enabled = false;
	chdir("/");
}

void rsh_iam(char *uname)
{
	strncpy(rsh_user, uname, rsh_user_maxlen);
    rsh_user[rsh_user_maxlen - 1] = 0;
}

void rsh_whoami(void)
{
	printf("rsh user: %s\n", rsh_user);
}

void pwd(void)
{
	if (rsh_enabled) rsh_pwd();
		else rtems_shell_main_pwd(0, NULL);
}

int cd(char *path)
{
	return (rsh_enabled) ? rsh_chdir(path) : chdir(path);
}


/* test section */
#if 0
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void copyFile(char *from, char *to)
{
	int fd_from, fd_to;
	char buffer[512];
	unsigned long read_pos = 0, writepos = 0;
	int put, get;

	fd_from = open(from, O_RDONLY);
	fd_to = open(to, O_RDWR | O_CREAT | O_APPEND);

	printf("copy fds %i, %i\n", fd_from, fd_to);

	while ((get = read(fd_from, buffer, sizeof(buffer))) > 0)
	{
		put = write(fd_to, buffer, get);
		read_pos += get;
	}

	close(fd_from);
	close(fd_to);
}


void TEST(void)
{
	int fd;
	char buf[512] = "boss\n", buf1[512];

	printf("cd(\"/home/eichel/ctl\")\n");
	cd("/home/eichel/ctl");


	creatershfile("hallo");

/* first test: read */
	printf("fd = open(\"ioc/test.txt\", O_RDONLY)\n");
	fd = open("ioc/test.txt", O_RDONLY);

	printf("read(fd, buf1, sizeof(buf1))\n");
	read(fd, buf1, sizeof(buf1));

	puts(buf1);

	printf("close(fd)\n");
	close(fd);

/* second test: write */
	printf("fd = open(\"ioc/test.txt\", O_RDWR, RTEMS_LIBIO_PERMS_RDWR)\n");
	fd = open("ioc/test.txt", O_RDWR);

	printf("write(fd, buf, sizeof(buf))\n");
	write(fd, buf, sizeof(buf));

	printf("close(fd)\n");
	close(fd);

/* third test: create new file */
	printf("fd = open(\"ioc/new.txt\", O_WRONLY | O_CREAT, RTEMS_LIBIO_PERMS_RDWR)\n");
	fd = open("ioc/new.txt", O_WRONLY | O_CREAT);

	printf("write(fd, buf, sizeof(buf))\n");
	write(fd, buf, sizeof(buf));

	printf("close(fd)\n");
	close(fd);

/* test number 4 */
	copyFile("rsh-rtems/rsh.obj", "ioc/rsh.cpy");
}
#endif
