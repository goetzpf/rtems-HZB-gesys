#ifndef _RSH_NODE_
#define _RSH_NODE_

#include <rtems/libio.h>
#include <rtems/libio_.h>
#include <limits.h>         /* NAME_MAX */

struct rsh_dir_node_data_t
{
	int used_entries;
	char dir_entries[3][NAME_MAX + 1];
};

struct rsh_node_t
{
	/* valid values are: $
	* RTEMS_FILESYSTEM_DIRECTORY   (1)
	* RTEMS_FILESYSTEM_MEMORY_FILE (5)
	*/
	rtems_filesystem_node_types_t             node_type;
	/* name on the host */	
	char 																			fname[NAME_MAX + 1];
	/* bool: exist file on the host? */
	int																				exist;
	/* stat infomations about the inode on the host */
	struct stat																st_stat;
	/* pointer to further data structures, if needed */
	void																			*data;
};

extern void rsh_initialize_inode_list(void);
extern void rsh_free_inode_list(void);
extern int rsh_check_inode(ino_t inode);
extern int rsh_tag_inode(ino_t inode);
extern int rsh_free_inode(ino_t inode);
extern int rsh_fill_inode(const char *fname, struct rsh_node_t *buf);

#endif /* _RSH_NODE_ */