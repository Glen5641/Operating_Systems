#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "oufs_lib.h"

int main(int argc, char **argv) {

	char cwd[MAX_PATH_LENGTH];
	char disk_name[MAX_PATH_LENGTH];
	oufs_get_environment(cwd, disk_name);

  int i = 0;
  i = oufs_format_disk(disk_name);
  if (i == -1)
    fprintf(stderr, "Path too long\n");
  if (i == -2)
    fprintf(stderr, "Unable to open new disk\n");
  return 0;
}
