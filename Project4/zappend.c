/**
Make a directory in the OU File System.

CS3113

*/

#include <stdio.h>
#include <string.h>

#include "oufs_lib.h"

#define MAX_BUFFER ((BLOCK_SIZE - 2)*BLOCKS_PER_INODE)

#define debug 0

int main(int argc, char **argv) {
  // Fetch the key environment vars
  char cwd[MAX_PATH_LENGTH];
  char disk_name[MAX_PATH_LENGTH];
  oufs_get_environment(cwd, disk_name);

  // Check arguments
  if (argc == 2) {
    // Open the virtual disk
    vdisk_disk_open(disk_name);

    if(debug)
      fprintf(stderr, "opened disk\n");



    OUFILE f = oufs_fopen(cwd, argv[1], 'a');
    OUFILE* fp = &f;
    if(!fp){
      fprintf(stderr, "Could not open file\n");
      if(oufs_allocate_new_file(cwd, argv[1]) != 0){
        fprintf(stderr, "Could not allocate file\n");
        return -1;
        f = oufs_fopen(cwd, argv[1], 'a');
        fp = &f;
      }
      if(debug)
        fprintf(stderr, "allocated file\n");
    }

    if(debug)
      fprintf(stderr, "opened file\n");
    if(debug)
      fprintf(stderr, "inode_ref: (%d), mode: (%c), offset: (%d)\n", fp->inode_reference, fp->mode, fp->offset);

    INODE inode;
    if(oufs_read_inode_by_reference(f.inode_reference, &inode) != 0){
      fprintf(stderr, "Could not read inode\n");
      return -1;
    }
    f.offset = inode.size - 1;

    char buf[MAX_BUFFER];
    char fullbuf[MAX_BUFFER];
    int len = 0;
    while(!feof(stdin)){
      if(fgets(buf, MAX_BUFFER, stdin)){
        int i = 0;
        while(buf[i] != '\0'){
          fullbuf[len] = buf[i];
          i++;
          len++;
          if(len == MAX_BUFFER){
            fprintf(stderr, "Out of memory\n");
            return (0);
          }
        }
      }
    }
    fullbuf[len] = '\0';

    if(oufs_fwrite(fp, fullbuf, len) < 0){
      fprintf(stderr, "Could not open file\n");
      oufs_fclose(fp);
      return -1;
    }

    oufs_fclose(fp);

    if(debug)
      fprintf(stderr, "closed file\n");

    // Clean up
    vdisk_disk_close();

    if(debug)
      fprintf(stderr, "closed disk\n");

  } else {
    // Wrong number of parameters
    fprintf(stderr, "Usage: zappend <filename>\n");
  }
}
