#include "oufs_lib.h"
#include <stdlib.h>

#define debug 0

/**
 * Read the ZPWD and ZDISK environment variables & copy their values into cwd
 * and disk_name. If these environment variables are not set, then reasonable
 * defaults are given.
 *
 * @param cwd String buffer in which to place the OUFS current working
 * directory.
 * @param disk_name String buffer containing the file name of the virtual disk.
 */
void oufs_get_environment(char *cwd, char *disk_name) {
  // Current working directory for the OUFS
  char *str = getenv("ZPWD");
  if (str == NULL) {
    // Provide default
    strcpy(cwd, "/");
  } else {
    // Exists
    strncpy(cwd, str, MAX_PATH_LENGTH - 1);
  }

  // Virtual disk location
  str = getenv("ZDISK");
  if (str == NULL) {
    // Default
    strcpy(disk_name, "vdisk1");
  } else {
    // Exists: copy
    strncpy(disk_name, str, MAX_PATH_LENGTH - 1);
  }
}

/**
 * Configure a directory entry so that it has no name and no inode
 *
 * @param entry The directory entry to be cleaned
 */
void oufs_clean_directory_entry(DIRECTORY_ENTRY *entry) {
  entry->name[0] = 0; // No name
  entry->inode_reference = UNALLOCATED_INODE;
}

/**
 * Initialize a directory block as an empty directory
 *
 * @param self Inode reference index for this directory
 * @param self Inode reference index for the parent directory
 * @param block The block containing the directory contents
 *
 */
void oufs_clean_directory_block(INODE_REFERENCE self, INODE_REFERENCE parent,
                                BLOCK *block) {
  // Debugging output
  if (debug)
    fprintf(stderr, "New clean directory: self=%d, parent=%d\n", self, parent);

  // Create an empty directory entry
  DIRECTORY_ENTRY entry;
  oufs_clean_directory_entry(&entry);

  // Copy empty directory entries across the entire directory list
  for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
    block->directory.entry[i] = entry;
  }

  // Now we will set up the two fixed directory entries

  // Self
  strncpy(entry.name, ".", 2);
  entry.inode_reference = self;
  block->directory.entry[0] = entry;

  // Parent (same as self
  strncpy(entry.name, "..", 3);
  entry.inode_reference = parent;
  block->directory.entry[1] = entry;
}

/**
 *  Find the first open bit from 00:ff
 *
 *  @param value of Master bit
 *  @return 0 = successfully returned correct bit
 *          8 = an error has occurred
 *
 */
int oufs_find_open_bit(unsigned char value) {
  if (debug)
    fprintf(stderr, "value=%d\n", value);

  // Check all bits 00:ff
  switch (value) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
  case 0x08:
  case 0x0a:
  case 0x0c:
  case 0x0e:
    return 0;
  case 0x01:
  case 0x05:
  case 0x09:
  case 0x0d:
    return 1;
  case 0x03:
  case 0x0b:
    return 2;
  case 0x07:
    return 3;
  case 0x0f:
  case 0x2f:
  case 0x4f:
  case 0x6f:
  case 0x8f:
  case 0xaf:
  case 0xcf:
  case 0xef:
    return 4;
  case 0x1f:
  case 0x5f:
  case 0x9f:
  case 0xdf:
    return 5;
  case 0x3f:
  case 0xbf:
    return 6;
  case 0x7f:
    return 7;
  default:
    return 8;
  }
}

/**
 * Allocate a new data block
 *
 * If one is found, then the corresponding bit in the block allocation table is
 * set
 *
 * @return The index of the allocated data block.  If no blocks are available,
 * then UNALLOCATED_BLOCK is returned
 *
 */
BLOCK_REFERENCE oufs_allocate_new_block() {
  BLOCK block;
  // Read the master block
  vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);

  // Scan for an available block
  int block_byte;
  int flag;

  // Loop over each byte in the allocation table.
  for (block_byte = 0, flag = 1; flag && block_byte < N_BLOCKS_IN_DISK / 8;
       ++block_byte) {
    if (block.master.block_allocated_flag[block_byte] != 0xff) {
      // Found a byte that has an opening: stop scanning
      flag = 0;
      break;
    };
  };
  // Did we find a candidate byte in the table?
  if (flag == 1) {
    // No
    if (debug)
      fprintf(stderr, "No blocks\n");
    return (UNALLOCATED_BLOCK);
  }

  // Found an available data block

  // Set the block allocated bit
  // Find the FIRST bit in the byte that is 0 (we scan in bit order: 0 ... 7)
  int block_bit =
      oufs_find_open_bit(block.master.block_allocated_flag[block_byte]);

  // Now set the bit in the allocation table
  block.master.block_allocated_flag[block_byte] |= (1 << block_bit);

  // Write out the updated master block
  vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);

  if (debug)
    fprintf(stderr, "Allocating block=%d (%d)\n", block_byte, block_bit);

  // Compute the block index
  BLOCK_REFERENCE block_reference = (block_byte << 3) + block_bit;

  if (debug)
    fprintf(stderr, "Allocating block=%d\n", block_reference);

  // Done
  return (block_reference);
}

/**
 * Allocate a new inode
 *
 * If one is found, then the corresponding bit in the inode allocation table is
 * set
 *
 * @return The index of the allocated inode block.  If no blocks are available,
 * then UNALLOCATED_INODE is returned
 *
 */
INODE_REFERENCE oufs_allocate_new_inode() {
  BLOCK block;
  // Read the master block
  vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);

  // Scan for an available block
  int inode_byte;
  int flag;

  // Loop over each byte in the allocation table.
  for (inode_byte = 0, flag = 1; flag && inode_byte < N_INODES / 8;
       ++inode_byte) {
    if (block.master.inode_allocated_flag[inode_byte] != 0xff) {
      // Found a byte that has an opening: stop scanning
      flag = 0;
      break;
    };
  };
  // Did we find a candidate byte in the table?
  if (flag == 1) {
    // No
    if (debug)
      fprintf(stderr, "No blocks\n");
    return (UNALLOCATED_INODE);
  }

  // Found an available data block

  // Set the block allocated bit
  // Find the FIRST bit in the byte that is 0 (we scan in bit order: 0 ... 7)
  int inode_bit =
      oufs_find_open_bit(block.master.inode_allocated_flag[inode_byte]);

  // Now set the bit in the allocation table
  block.master.inode_allocated_flag[inode_byte] |= (1 << inode_bit);

  // Write out the updated master block
  vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);

  if (debug)
    fprintf(stderr, "Allocating inode=%d (%d)\n", inode_byte, inode_bit);

  // Compute the block index
  INODE_REFERENCE inode_reference = (inode_byte << 3) + inode_bit;

  if (debug)
    fprintf(stderr, "Allocating inode=%d\n", inode_reference);

  // Done
  return (inode_reference);
}

/**
 * Deallocate a specified inode
 * The inode is found from reference, then the corresponding bit in the
 * inode allocation table is cleared.
 *
 * @inode_ref INODE_REFERENCE The inode to be deallocated
 *
 * @return 0 = Inode deallocated
 *         1 = Error deallocating inode
 *
 */
int oufs_deallocate_inode(INODE_REFERENCE inode_ref) {

  BLOCK block;

  // Read the master block
  vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);

  // Scan for an specified block
  int inode_byte = inode_ref / (8);
  int inode_bit = inode_ref % (8);

  if (inode_byte > (N_INODES >> 3)) {
    fprintf(stderr, "Out of disk range\n");
    return (-1);
  }

  // Deallocate the specified bit from the specified byte
  block.master.inode_allocated_flag[inode_byte] &= ~(1 << (inode_bit));

  // Write out the updated master block
  vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);

  return (0);
}

/**
 * Deallocate a specified block
 * The block is found from reference, then the corresponding bit in the
 * inode allocation table is cleared.
 *
 * @block_ref BLOCK_REFERENCE The block to be deallocated
 *
 * @return 0 = Block deallocated
 *         1 = Error deallocating block
 *
 */
int oufs_deallocate_block(BLOCK_REFERENCE block_ref) {

  BLOCK block;

  // Read the master block
  vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);

  // Scan for an specified block
  int block_byte = block_ref / (8);
  int block_bit = block_ref % (8);

  if (block_byte > (N_BLOCKS_IN_DISK >> 3)) {
    fprintf(stderr, "Out of disk range\n");
    return (-1);
  }

  // Deallocate the specified bit from the specified byte
  block.master.block_allocated_flag[block_byte] &= ~(1 << block_bit);

  // Write out the updated master block
  vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);

  return (0);
}

/**
 *  Given an inode reference, read the inode from the virtual disk.
 *
 *  @param i Inode reference (index into the inode list)
 *  @param inode Pointer to an inode memory structure.  This structure will be
 *                filled in before return)
 *  @return 0 = successfully loaded the inode
 *         -1 = an error has occurred
 *
 */
int oufs_read_inode_by_reference(INODE_REFERENCE i, INODE *inode) {

  if (debug)
    fprintf(stderr, "Fetching inode %d\n", i);

  // Find the address of the inode block and the inode within the block
  BLOCK_REFERENCE block = i / INODES_PER_BLOCK + 1;
  int element = (i % INODES_PER_BLOCK);

  BLOCK b;
  if (vdisk_read_block(block, &b) == 0) {
    // Successfully loaded the block: copy just this inode
    *inode = b.inodes.inode[element];
    return (0);
  }
  // Error case
  return (-1);
}

/**
 *  Given an inode reference, write the inode to the virtual disk.
 *
 *  @param i Inode reference (index into the inode list)
 *  @param inode Pointer to an inode memory structure.
 *  @return 0 = successfully loaded the inode
 *         -1 = an error has occurred
 *
 */
int oufs_write_inode_by_reference(INODE_REFERENCE i, INODE *inode) {

  if (debug)
    fprintf(stderr, "Writing inode %d\n", i);

  // Find the address of the inode block and the inode within the block
  BLOCK_REFERENCE block = i / INODES_PER_BLOCK + 1;
  int element = (i % INODES_PER_BLOCK);

  BLOCK b;
  if (vdisk_read_block(block, &b) != 0) {
    fprintf(stderr, "Failed to read inode for writing\n");
  }
  b.inodes.inode[element] = *inode;

  if (vdisk_write_block(block, &b) == 0) {
    // Successfully wrote inode
    return (0);
  }
  // Error case
  return (-1);
}

/**
 *  Find file from directory and return the file parent and child references
 *
 *  @param cwd char*
 *  @param path char*
 *  @param parent INODE_REFERENCE
 *  @param child INODE_REFERENCE
 *  @param local_name char*
 *  @return 0 = successfully loaded the inode
 *         -1 = an error has occurred
 *
 */
int oufs_find_file(char *cwd, char *path, INODE_REFERENCE *parent,
                   INODE_REFERENCE *child, char *local_name) {}

/**
 *  Find directory from a specified path off
 *  of a vdisk and return the associated references.
 *
 *  @param path char* The path of the directory to be found
 *  @param inode_ref INODE_REFERENCE A reference to the parent inode
 *  @param new_inode_ref INODE_REFERENCE A reference to the child inode
 *  @param block_ref BLOCK_REFERENCE A reference to the parent directory
 *  @param new_block_ref BLOCK_REFERENCE A reference to the child directory
 *  @param entry char** The relative path to the directory
 *  @param virtual_disk_name char* The vdisk being read
 *  @return 2 = Existent directory is found
 *          1 = Non-Existent directory is found
 *         -1 = Disk has inode corruption
 *         -2 = Cannot make directory in file without this program :)
 *         -3 = Disk has data block corruption
 *         -4 = Path does not exist
 *
 */
int oufs_find_directory(char *path, INODE_REFERENCE *inode_ref,
                        INODE_REFERENCE *new_inode_ref,
                        BLOCK_REFERENCE *block_ref,
                        BLOCK_REFERENCE *new_block_ref, char **entry,
                        char *virtual_disk_name) {

  BLOCK block, empty_block = {0, 0, 0, 0};
  INODE inode, empty_inode = {0};

  // Read Root directory
  vdisk_read_block(*block_ref, &block);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Tokenize first directory entry for search
  char *path_entry = strtok(path, "/");

  // read initial inode to search through
  oufs_read_inode_by_reference(*inode_ref, &inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  do {
    // Loop through all entries in root to find correct entry
    int i = 0;
    int flag = 0;
    for (; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
      if (block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
        if (!strcmp(block.directory.entry[i].name, path_entry)) {
          flag = 1;
          break;
        }
      }
    }

    // Init parent references
    *inode_ref = *new_inode_ref;
    *block_ref = *new_block_ref;

    // If entry was found, Move to that Directory
    if (flag) {

      // Grab inode reference from the entry
      *new_inode_ref = block.directory.entry[i].inode_reference;

      // Read inode reference and print error if disk is corrupt
      inode = empty_inode;
      if (oufs_read_inode_by_reference(*new_inode_ref, &inode)) {
        fprintf(stderr, "Disk is corrupt by inode\n");
        return (-1);
      }

      // Check if inode is a directory
      if (inode.type != IT_DIRECTORY) {
        fprintf(stderr,
                "Cannot make directory in file without this program :)\n");
        return (-2);
      }

      // Grab the block reference to the inodes root directory
      *new_block_ref = inode.data[0];

      // Read the block by block reference
      block = empty_block;
      if (vdisk_read_block(*new_block_ref, &block)) {
        fprintf(stderr, "Disk is corrupt by block\n");
        return (-3);
      }

    } else {

      // If the path is still tokenizing when entry is not found, exit
      if ((strtok(NULL, "/"))) {
        fprintf(stderr, "%s does not exist\n", path);
        return (-4);
      } else {

        // Copy the path_entry to entry
        if ((*entry = malloc(FILE_NAME_SIZE)))
          strncpy(*entry, path_entry, FILE_NAME_SIZE);

        // Return non_existent path with 1
        return 1;
      }
    }

    // Copy the path_entry to entry
    if ((*entry = malloc(FILE_NAME_SIZE + 1)))
      strncpy(*entry, path_entry, FILE_NAME_SIZE);
  } while ((path_entry = strtok(NULL, "/")));

  // Return existent path with 2
  return 2;
}

/**
 *  Given the CWD and PATH, traverse the file system and
 *  make a directory only if it is a valid path.
 *
 *  @param CWD char* (Current Working Directory of the file system)
 *  @param PATH char* (Path specified by the user)
 *  @param virtual_disk_name char* (disk that is being wrote)
 *  @return 0 = successfully made directory
 *         -1 = Path is Root Directory or Empty
 *         -2 = CWD does not exist
 *         -3 = Path does not exist
 *         -4 = No more space on disk
 *         -5 = Directory name is too long
 *
 */
int oufs_mkdir(char *cwd, char *path, char *virtual_disk_name) {

  // Check if path is there and not root directory
  if ((!path) || (!strcmp(path, "/"))) {
    fprintf(stderr, "Path is Root Directory or Empty\n");
    return (-1);
  }

  BLOCK empty_block = {0, 0, 0, 0};
  char *entry;
  INODE_REFERENCE i_parent_ref = 0, i_child_ref = 0;
  BLOCK_REFERENCE b_parent_ref = ROOT_DIRECTORY_BLOCK,
                  b_child_ref = ROOT_DIRECTORY_BLOCK;

  // If CWD is not root, traverse to correct inode and block
  if (strcmp(cwd, "/")) {
    if (oufs_find_directory(cwd, &i_parent_ref, &i_child_ref, &b_parent_ref,
                            &b_child_ref, &entry, virtual_disk_name) != 2) {
      fprintf(stderr, "CWD does not exist\n");
      return (-2);
    }
  }

  if ((!strcmp(cwd, ".")) || (!strcmp(cwd, ".."))) {
    fprintf(stderr, "Path already exists\n");
    return (-3);
  }

  // Use specified path to traverse to dst inode and block
  if (oufs_find_directory(path, &i_parent_ref, &i_child_ref, &b_parent_ref,
                          &b_child_ref, &entry, virtual_disk_name) != 1) {
    fprintf(stderr, "Path already exists\n");
    return (-3);
  }

  if (strlen(entry) >= (FILE_NAME_SIZE - 1)) {
    fprintf(stderr, "Directory name is too long\n");
    return (-5);
  }

  INODE max_check;
  oufs_read_inode_by_reference(i_parent_ref, &max_check);
  int i = 0;
  while ((max_check.data[i] != UNALLOCATED_BLOCK) && (i < BLOCKS_PER_INODE))
    i++;
  if (i >= BLOCKS_PER_INODE) {
    fprintf(stderr, "Parent directory does not have enough space\n");
    return (-4);
  }

  // Update Master Block and create new references
  b_child_ref = oufs_allocate_new_block();
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);
  // If blocks are full
  if (b_child_ref == UNALLOCATED_BLOCK) {
    fprintf(stderr, "No more space on Disk\n");
    return (-4);
  }

  // If inodes are full
  i_child_ref = oufs_allocate_new_inode();
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);
  if (i_child_ref == UNALLOCATED_INODE) {
    fprintf(stderr, "No more space on Disk\n");
    return (-4);
  }

  // Create clean directory and write
  BLOCK new_block = empty_block;
  oufs_clean_directory_block(i_child_ref, i_parent_ref, &new_block);
  vdisk_write_block(b_child_ref, &new_block);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update New Inode and write
  INODE new_inode = {0};
  new_inode.type = IT_DIRECTORY;
  new_inode.n_references = 1;
  new_inode.data[0] = b_child_ref;
  for (int i = 1; i <= BLOCKS_PER_INODE; i++) {
    new_inode.data[i] = UNALLOCATED_BLOCK;
  }
  new_inode.size = 2;
  oufs_write_inode_by_reference(i_child_ref, &new_inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update parent Inode and write
  INODE inode;
  oufs_read_inode_by_reference(i_parent_ref, &inode);
  i = 0;
  while (inode.data[i] != UNALLOCATED_BLOCK)
    i++;
  inode.data[i] = b_child_ref;
  inode.size = inode.size + 1;
  oufs_write_inode_by_reference(i_parent_ref, &inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update Parent BLOCK
  BLOCK block = empty_block;
  vdisk_read_block(b_parent_ref, &block);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);
  DIRECTORY_ENTRY new_entry = {0};
  strcpy(new_entry.name, entry);
  new_entry.inode_reference = i_child_ref;
  i = 0;
  while ((block.directory.entry[i].name[0] != 0))
    i++;
  block.directory.entry[i] = new_entry;
  vdisk_write_block(b_parent_ref, &block);

  return (0);
}

/**
 *  Compare two strings lexigraphically
 *
 *  @param a char* comparable string
 *  @param b char* comparable string
 *  @return 0 = a and b are equal
 *         -# = a is less than b
 *         +# = a is more than b
 *
 */
int comparing_func(const void *a, const void *b) {
  char *astring = *(char **)a;
  char *bstring = *(char **)b;

  return strcmp(astring, bstring);
}

/**
 *  Given the CWD and PATH, traverse the file system and
 *  list the contents of a directory in lexigraphic order
 *
 *  @param CWD char* (Current Working Directory of the file system)
 *  @param PATH char* (Path specified by the user)
 *  @param virtual_disk_name char* (disk that is being wrote)
 *  @return 0 = successfully listed all elements
 *         -1 = CWD does not exist
 *         -2 = Path does not exist
 *
 */
int oufs_list(char *cwd, char *path, char *virtual_disk_name) {

  INODE_REFERENCE parent_inode_ref = 0, child_inode_ref = 0;
  BLOCK_REFERENCE parent_block_ref = ROOT_DIRECTORY_BLOCK,
                  child_block_ref = ROOT_DIRECTORY_BLOCK;
  char *entry;

  // If CWD is not root, traverse to cwd
  if (strcmp(cwd, "/")) {
    if (oufs_find_directory(cwd, &parent_inode_ref, &child_inode_ref,
                            &parent_block_ref, &child_block_ref, &entry,
                            virtual_disk_name) != 2) {
      fprintf(stderr, "CWD does not exist\n");
      return (-1);
    }
  }

  // If path is populated, Search for directory block
  if (path && strcmp(path, "/") && strcmp(path, "")) {
    if (oufs_find_directory(path, &parent_inode_ref, &child_inode_ref,
                            &parent_block_ref, &child_block_ref, &entry,
                            virtual_disk_name) != 2) {
      fprintf(stderr, "Path does not exist\n");
      return (-2);
    }
  }

  // If path is root set ref
  if (!strcmp(path, "/")) {
    child_block_ref = ROOT_DIRECTORY_BLOCK;
  }

  // Populate a string array of actual entries and append '/'
  BLOCK block;
  char **entries =
      (char **)malloc((DIRECTORY_ENTRIES_PER_BLOCK) * sizeof(char *));
  vdisk_read_block(child_block_ref, &block);
  int j = 0;
  for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
    if ((block.directory.entry[i].name[0] != '\0') &&
        (strcmp(block.directory.entry[i].name, ".")) &&
        (strcmp(block.directory.entry[i].name, ".."))) {
      entries[j] = malloc(sizeof(char *));
      strcpy(entries[j], block.directory.entry[i].name);
      strcat(entries[j], "/");
      j++;
    }
  }

  // Print '.' and '..' directories and then sort all others and print them
  fprintf(stdout, "%s/\n", block.directory.entry[0].name);
  fprintf(stdout, "%s/\n", block.directory.entry[1].name);
  qsort(entries, j, (sizeof(char *)), comparing_func);
  for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
    if (entries[i])
      fprintf(stdout, "%s\n", entries[i]);
    free(entries[i]);
  }
  free(entries);

  return (0);
}

/**
 *  Given the CWD and PATH, traverse the file system and
 *  delete a directory only if it is valid and has size 2.
 *
 *  @param CWD char* (Current Working Directory of the file system)
 *  @param PATH char* (Path specified by the user)
 *  @param virtual_disk_name char* (disk that is being wrote)
 *  @return 0 = successfully removed directory
 *         -1 = Path is Root Directory or Empty
 *         -2 = Cannot remove Parent of CWD
 *         -3 = CWD does not exist
 *         -4 = Path does not exist
 *         -5 = Directory is not empty
 *
 */
int oufs_rmdir(char *cwd, char *path, char *virtual_disk_name) {

  // Initialize all needed variables
  char *entry;
  BLOCK empty_block = {0, 0, 0, 0};
  BLOCK block = empty_block;
  INODE inode, empty_inode;
  DIRECTORY_ENTRY new_entry;
  INODE_REFERENCE i_parent_ref = 0, i_child_ref = 0;
  BLOCK_REFERENCE b_parent_ref = ROOT_DIRECTORY_BLOCK,
                  b_child_ref = ROOT_DIRECTORY_BLOCK;

  // Check if path is there and not root directory
  if ((!path) || (!strcmp(path, "/"))) {
    fprintf(stderr, "Path is Root Directory or Empty\n");
    return (-1);
  }

  // Check if path is not the parent of CWD
  if ((!strcmp(path, ".."))) {
    fprintf(stderr, "Cannot remove cwd parent.\n");
    return (-2);
  }

  // Set INODE and BLOCK search to CWD
  if (strcmp(cwd, "/")) {
    if (oufs_find_directory(cwd, &i_parent_ref, &i_child_ref, &b_parent_ref,
                            &b_child_ref, &entry, virtual_disk_name) != 2) {
      fprintf(stderr, "CWD does not exist\n");
      return (-3);
    }
  }

  // If path is the current, remove CWD.
  // If not, set INODE and BLOCK search to PATH.
  if ((strcmp(path, "."))) {
    if (oufs_find_directory(path, &i_parent_ref, &i_child_ref, &b_parent_ref,
                            &b_child_ref, &entry, virtual_disk_name) != 2) {
      fprintf(stderr, "Path Does not exist\n");
      return (-4);
    }
  }

  // Check the size of the directory to remove
  inode = empty_inode;
  oufs_read_inode_by_reference(i_child_ref, &inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);
  if (inode.size > 2) {
    fprintf(stderr, "Directory is not empty\n");
    return (-5);
  }

  // Write blank BLOCK to Child Block reference
  vdisk_write_block(b_child_ref, &block);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update Master Block with deallocated BLOCK
  oufs_deallocate_block(b_child_ref);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update Master Block with deallocated INODE
  oufs_deallocate_inode(i_child_ref);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Initial wiped inode reference and write to child inode ref
  inode = empty_inode;
  inode.type = IT_NONE;
  inode.n_references = 0;
  inode.size = 0;
  for (int j = 0; j < BLOCKS_PER_INODE; j++) {
    inode.data[j] = UNALLOCATED_BLOCK;
  }
  oufs_write_inode_by_reference(i_child_ref, &inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update parent Inode and write
  inode = empty_inode;
  oufs_read_inode_by_reference(i_parent_ref, &inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);
  int i = 0;
  while ((inode.data[i] != b_child_ref) && i < BLOCKS_PER_INODE)
    i++;
  inode.data[i] = UNALLOCATED_BLOCK;
  inode.size = inode.size - 1;
  oufs_write_inode_by_reference(i_parent_ref, &inode);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);

  // Update Parent BLOCK
  block = empty_block;
  vdisk_read_block(b_parent_ref, &block);
  vdisk_disk_close();
  vdisk_disk_open(virtual_disk_name);
  oufs_clean_directory_entry(&new_entry);
  i = 0;
  while (strcmp(block.directory.entry[i].name, entry))
    i++;
  block.directory.entry[i] = new_entry;
  vdisk_write_block(b_parent_ref, &block);
  return (0);
}

/**
 *  DESCRIPTION
 *
 *  @param cwd the current working directory
 *  @param path The path of the file
 *  @param mode the access mode of the file
 *  @return 0 = successfully DESCRIPTION
 *         -1 = an error has occurred
 *
 */
OUFILE *oufs_fopen(char *cwd, char *path, char *mode) {}

/**
 *  DESCRIPTION
 *
 *  @param fp the file pointer to be closed
 *  @return 0 = successfully DESCRIPTION
 *         -1 = an error has occurred
 *
 */
void oufs_fclose(OUFILE *fp) {}

/**
 *  DESCRIPTION
 *
 *  @param fp the filepointer that is being wrote to
 *  @param buf the characters being wrote
 *  @param len the length of buffer
 *  @return 0 = successfully DESCRIPTION
 *         -1 = an error has occurred
 *
 */
int oufs_fwrite(OUFILE *fp, unsigned char *buf, int len) {}

/**
 *  DESCRIPTION
 *
 *  @param fp the filepointer that is being read from
 *  @param buf the characters being read
 *  @param len the length of buffer
 *  @return 0 = successfully DESCRIPTION
 *         -1 = an error has occurred
 *
 */
int oufs_fread(OUFILE *fp, unsigned char *buf, int len) {}

/**
 *  DESCRIPTION
 *
 *  @param cwd the current working directory
 *  @param path the path to the file being removed
 *  @return 0 = successfully DESCRIPTION
 *         -1 = an error has occurred
 *
 */
int oufs_remove(char *cwd, char *path) {}

/**
 *  DESCRIPTION
 *
 *  @param cwd the current working directory
 *  @param path the path to the file being linked
 *  @param path_dst the path to the file being linked to
 *  @return 0 = successfully DESCRIPTION
 *         -1 = an error has occurred
 *
 */
int oufs_link(char *cwd, char *path_src, char *path_dst) {}

/**
 *  Given a virtual disk name, create and format virtual disk
 *
 *  @param virtual_disk_name Name of disk to be created
 *  @return 0 = successfully formatted disk
 *         -1 = Disk name is too long
 *         -2 = Unable to format disk
 *
 */
int oufs_format_disk(char *virtual_disk_name) {

  // Check disk name length
  if (strlen(virtual_disk_name) > (MAX_PATH_LENGTH - 1)) {
    fprintf(stderr, "%s is too long\n", virtual_disk_name);
    return (-1);
  }

  // If vdisk creation fails
  if (vdisk_disk_open(virtual_disk_name)) {
    fprintf(stderr, "Unable to format Disk %s\n", virtual_disk_name);
    return (-2);
  }

  // Init a varying block and an empty block for reinitiallization
  BLOCK empty_block = {0, 0, 0, 0};
  BLOCK block = empty_block;

  ////////////////////* INITIALIZE MASTER BLOCK *///////////////////
  // Initialize the master block with Zero Inode and Root Directory
  block = empty_block;
  block.master.inode_allocated_flag[0] |= (1 << 0);
  block.master.block_allocated_flag[0] = 0xff;
  block.master.block_allocated_flag[1] |= (1 << 0);
  block.master.block_allocated_flag[1] |= (1 << 1);
  vdisk_write_block(0, &block);
  //////////////////////////////////////////////////////////////////

  ///////////////* INITIALIZE FIRST INODE BLOCK *///////////////////
  // Initialize the Zero Inode with Root Directory
  block = empty_block;
  block.inodes.inode[0].type = IT_DIRECTORY;
  block.inodes.inode[0].n_references = 1;
  block.inodes.inode[0].data[0] = ROOT_DIRECTORY_BLOCK;
  for (int i = 1; i <= BLOCKS_PER_INODE; i++) {
    block.inodes.inode[0].data[i] = UNALLOCATED_BLOCK;
  }
  block.inodes.inode[0].size = 2;

  // Initialize the Rest of the First inode Block
  for (int j = 1; j < INODES_PER_BLOCK; j++) {
    block.inodes.inode[j].type = IT_NONE;
    block.inodes.inode[j].n_references = 0;
    block.inodes.inode[j].size = 0;
    for (int i = 0; i < BLOCKS_PER_INODE; i++) {
      block.inodes.inode[j].data[i] = UNALLOCATED_BLOCK;
    }
  }
  vdisk_write_block(1, &block);
  /////////////////////////////////////////////////////////////////

  ///////////////* ALLOCATE ALL INODE BLOCKS */////////////////////
  block = empty_block;
  for (int i = 0; i < INODES_PER_BLOCK; i++) {
    block.inodes.inode[i].type = IT_NONE;
    block.inodes.inode[i].n_references = 0;
    block.inodes.inode[i].size = 0;
    for (int j = 0; j < BLOCKS_PER_INODE; j++) {
      block.inodes.inode[i].data[j] = UNALLOCATED_BLOCK;
    }
  }
  for (int i = 2; i <= N_INODE_BLOCKS; i++) {
    vdisk_write_block(i, &block);
  }
  /////////////////////////////////////////////////////////////////

  //////////////* INITIALIZE ROOT DIRECTORY BLOCK *////////////////
  block = empty_block;
  oufs_clean_directory_block(0, 0, &block);
  vdisk_write_block(ROOT_DIRECTORY_BLOCK, &block);
  /////////////////////////////////////////////////////////////////

  /////////* FILL REST OF DISK WITH UNALLOCATED BLOCKS *///////////
  block = empty_block;
  int i = ROOT_DIRECTORY_BLOCK + 1;
  for (; i < N_BLOCKS_IN_DISK; i++) {
    vdisk_write_block(i, &block);
  }
  //////////////////////////////////////////////////////////////////

  vdisk_disk_close();
}
