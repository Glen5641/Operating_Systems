/**
Make a directory in the OU File System.

CS3113

*/

#include <stdio.h>
#include <string.h>

#include "oufs_lib.h"

#define MAX_BUFFER ((BLOCK_SIZE - 2)*BLOCKS_PER_INODE)

int main(int argc, char **argv) {
  // Fetch the key environment vars
  char cwd[MAX_PATH_LENGTH];
  char disk_name[MAX_PATH_LENGTH];
  oufs_get_environment(cwd, disk_name);

  // Check arguments
  if (argc == 2) {
    // Open the virtual disk
    vdisk_disk_open(disk_name);

    OUFILE f = oufs_fopen(cwd, argv[1], 'r');
    OUFILE *fp = &f;
    if(!fp){
      fprintf(stderr, "Could not open file\n");
      return (-1);
    }

    INODE inode;
    if(oufs_read_inode_by_reference(fp->inode_reference, &inode) != 0){
      fprintf(stderr, "Could not read inode\n");
      return -1;
    }

    if(inode.type != IT_FILE){
      fprintf(stderr, "Not a file\n");
      return (0);
    }
    fp->offset = 0;

    unsigned char buf[MAX_BUFFER];
    for(int i = 0; i < MAX_BUFFER; i++){
      buf[i] = 0xff;
    }
    if(oufs_fread(fp, buf, MAX_BUFFER) != 0){
      fprintf(stderr, "Could not read file\n");
      return (-1);
    }

    for(int i = 0; i < MAX_BUFFER; i++){
      if(buf[i] != 0xff){
        fprintf(stdout, "%c", buf[i]);
      }
    }

    oufs_fclose(fp);

    // Clean up
    vdisk_disk_close();

  } else {
    // Wrong number of parameters
    fprintf(stderr, "Usage: zmore <filename>\n");
  }
}
