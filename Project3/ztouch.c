/**
Make a directory in the OU File System.

CS3113

*/

#include <stdio.h>
#include <string.h>

#include "oufs_lib.h"

int main(int argc, char **argv) {
  // Fetch the key environment vars
  char cwd[MAX_PATH_LENGTH];
  char disk_name[MAX_PATH_LENGTH];
  oufs_get_environment(cwd, disk_name);

  // Check arguments
  if (argc == 2) {
    // Open the virtual disk
    vdisk_disk_open(disk_name);

    INODE_REFERENCE parent;
    INODE_REFERENCE child;
    char local_name[MAX_PATH_LENGTH];
    int ret;

    // Attempt to find the specified directory
    if ((ret = oufs_find_file(cwd, argv[1], &parent, &child, local_name)) < -1) {
      return (-1);
    };

    if (parent != UNALLOCATED_INODE && child != UNALLOCATED_INODE) {
      INODE child_inode;
      if (oufs_read_inode_by_reference(child, &child_inode) != 0) {
        return (-3);
      }

      if (child_inode.type == 'F') {
          return (0);
      }
    }

    // Make the specified directory
    oufs_allocate_new_file(cwd, argv[1]);

    // Clean up
    vdisk_disk_close();

  } else {
    // Wrong number of parameters
    fprintf(stderr, "Usage: ztouch <filename>\n");
  }
}
