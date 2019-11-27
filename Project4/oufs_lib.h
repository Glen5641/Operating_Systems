#ifndef OUFS_LIB
#define OUFS_LIB
#include "oufs.h"

#define MAX_PATH_LENGTH 200

void oufs_get_environment(char *cwd, char *disk_name);
void oufs_clean_directory_entry(DIRECTORY_ENTRY *entry);
void oufs_clean_directory_block(INODE_REFERENCE self, INODE_REFERENCE parent,
                                BLOCK *block);
int oufs_find_open_bit(unsigned char value);
BLOCK_REFERENCE oufs_allocate_new_block();
INODE_REFERENCE oufs_allocate_new_inode();
int oufs_deallocate_inode(INODE_REFERENCE inode_ref);
int oufs_deallocate_block(BLOCK_REFERENCE block_ref);
int oufs_read_inode_by_reference(INODE_REFERENCE i, INODE *inode);
int oufs_write_inode_by_reference(INODE_REFERENCE i, INODE *inode);
int oufs_find_directory_entry(INODE *inode, char *directory_name);
int oufs_find_file(char *cwd, char *path, INODE_REFERENCE *parent,
                   INODE_REFERENCE *child, char *local_name);
int comparing_func(const void *a, const void *b);
int oufs_allocate_new_directory(INODE *parent, INODE_REFERENCE parent_reference);
int oufs_allocate_new_file(char *cwd, char *path);
int oufs_mkdir(char *cwd, char *path);
int oufs_list(char *cwd, char *path);
int oufs_rmdir(char *cwd, char *path);
OUFILE oufs_fopen(char *cwd, char *path, char mode);
void oufs_fclose(OUFILE *fp);
int oufs_fwrite(OUFILE *fp, unsigned char *buf, int len);
int oufs_fread(OUFILE *fp, unsigned char *buf, int len);
int oufs_remove(char *cwd, char *path);
int oufs_link(char *cwd, char *path_src, char *path_dst);
int oufs_format_disk(char *virtual_disk_name);

#endif
