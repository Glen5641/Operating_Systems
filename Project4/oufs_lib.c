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
 *         -x = Error deallocating inode
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
 *         -x = Error deallocating block
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
 *         -x = an error has occurred
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
 *         -x = an error has occurred
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
 *  Given an Inode and directory name, this function finds
 *  the inode reference of the directory name
 *
 *  @param inode INODE the directory being searched
 *  @param directory_name the string that should match a directory entry name
 *  @return INODE_REFERENCE = Found directory entry
 *         UNALLOCATED_INODE = Directory entry not found
 *
 */
int oufs_find_directory_entry(INODE *inode, char *directory_name) {

  // Read the directory block associated with the inode given
  BLOCK block;
  if (vdisk_read_block(inode->data[0], &block) < 0) {
    fprintf(stderr, "Could not read current inode %d's data block",
            inode->data[0]);
  }

  // Loop through all directory entries and find matching Name
  for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
    if (!strcmp(block.directory.entry[i].name, directory_name)) {

      // Return the matching name
      return block.directory.entry[i].inode_reference;
    }
  }

  // If name is not found in current directory, return unallocated.
  return UNALLOCATED_INODE;
}

/**
 *  Given a current working directory and either an absolute or relative path,
 * find both the inode of the file or directory and the inode of the parent
 * directory.  If one or both are not found, then they are set to
 * UNALLOCATED_INODE.
 *
 *  This implementation handles a variety of strange cases, such as consecutive
 * /'s and /'s at the end of of the path (we have to maintain some extra state
 * to make this work properly).
 *
 * @param cwd Absolute path for the current working directory
 * @param path Absolute or relative path of the file/directory to be found
 * @param parent Inode reference for the parent directory
 * @param child  Inode reference for the file or directory specified by path
 * @param local_name String name of the file or directory without any path
 * information (i.e., name relative to the parent)
 * @return 0 if no errors
 *         -1 if child not found
 *         -2 if child is file
 *         -x if an error
 *
 */
int oufs_find_file(char *cwd, char *path, INODE_REFERENCE *parent,
                   INODE_REFERENCE *child, char *local_name) {
  INODE_REFERENCE grandparent;
  char full_path[MAX_PATH_LENGTH];

  // Construct an absolute path the file/directory in question
  if (path[0] == '/') {
    strncpy(full_path, path, MAX_PATH_LENGTH - 1);
  } else {
    if (strlen(cwd) > 1) {
      strncpy(full_path, cwd, MAX_PATH_LENGTH - 1);
      strncat(full_path, "/", 2);
      strncat(full_path, path,
              MAX_PATH_LENGTH - 1 - strnlen(full_path, MAX_PATH_LENGTH));
    } else {
      strncpy(full_path, "/", 2);
      strncat(full_path, path, MAX_PATH_LENGTH - 2);
    }
  }

  if (debug) {
    fprintf(stderr, "Full path: %s\n", full_path);
  };

  // Start scanning from the root directory
  // Root directory inode
  grandparent = *parent = *child = 0;
  if (debug)
    fprintf(stderr, "Start search: %d\n", *parent);

  // Parse the full path
  char *directory_name;
  directory_name = strtok(full_path, "/");
  while (directory_name != NULL) {
    if (strlen(directory_name) >= FILE_NAME_SIZE - 1)
      // Truncate the name
      directory_name[FILE_NAME_SIZE - 1] = 0;
    if (debug) {
      fprintf(stderr, "Directory: %s\n", directory_name);
    }
    if (strlen(directory_name) != 0) {
      // We have a non-empty name
      // Remember this name
      if (local_name != NULL) {
        // Copy local name of file
        strncpy(local_name, directory_name, MAX_PATH_LENGTH - 1);
        // Make sure we have a termination
        local_name[MAX_PATH_LENGTH - 1] = 0;
      }

      // Real next element
      INODE inode;
      // Fetch the inode that corresponds to the child
      if (oufs_read_inode_by_reference(*child, &inode) != 0) {
        return (-3);
      }

      // Check the type of the inode
      if (inode.type != 'D') {
        // Parent is not a directory
        *parent = *child = UNALLOCATED_INODE;
        return (-2); // Not a valid directory
      }
      // Get the new inode that corresponds to the name by searching the current
      // directory
      INODE_REFERENCE new_inode =
          oufs_find_directory_entry(&inode, directory_name);
      grandparent = *parent;
      *parent = *child;
      *child = new_inode;
      if (new_inode == UNALLOCATED_INODE) {
        // name not found
        //  Is there another (nontrivial) step in the path?
        //  Loop until end or we have found a nontrivial name
        do {
          directory_name = strtok(NULL, "/");
          if (directory_name != NULL &&
              strlen(directory_name) >= FILE_NAME_SIZE - 1)
            // Truncate the name
            directory_name[FILE_NAME_SIZE - 1] = 0;
        } while (directory_name != NULL && (strcmp(directory_name, "") == 0));

        if (directory_name != NULL) {
          // There are more sub-items - so the parent does not exist
          *parent = UNALLOCATED_INODE;
        };
        // Directory/file does not exist
        return (-1);
      };
    }
    // Go on to the next directory
    directory_name = strtok(NULL, "/");
    if (directory_name != NULL && strlen(directory_name) >= FILE_NAME_SIZE - 1)
      // Truncate the name
      directory_name[FILE_NAME_SIZE - 1] = 0;
  };

  // Item found.
  if (*child == UNALLOCATED_INODE) {
    // We went too far - roll back one step ***
    *child = *parent;
    *parent = grandparent;
  }
  if (debug) {
    fprintf(stderr, "Found: %d, %d\n", *parent, *child);
  }
  // Success!
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
 *  Allocate new directory's inode and dblock and update the parent inode.
 *
 *  @param parent INODE_REFERENCE used for the parent inode
 *  @return 0 = directory made and new inode reference is found
 *          UNALLOCATED_INODE = Error exists
 *
 */
int oufs_allocate_new_directory(INODE *parent,
                                INODE_REFERENCE parent_reference) {

  INODE_REFERENCE new_inode_reference;

  // Allocate new block on master and return unallocated if master block is full
  BLOCK_REFERENCE new_block_reference = oufs_allocate_new_block();
  if (new_block_reference == UNALLOCATED_BLOCK) {
    fprintf(stderr, "Out of memory\n");
    return UNALLOCATED_INODE;
  }

  // Find first unallocated block on parent and insert new block reference
  for (int i = 0; i < BLOCKS_PER_INODE; i++) {
    if (parent->data[i] == UNALLOCATED_BLOCK) {
      parent->data[i] = new_block_reference;

      // Allocate new inode or revert and return unallocated if master block is
      // full
      new_inode_reference = oufs_allocate_new_inode();
      if (new_inode_reference == UNALLOCATED_INODE) {
        oufs_deallocate_block(new_block_reference);
        parent->data[i] = UNALLOCATED_BLOCK;
        fprintf(stderr, "Out of memory\n");
        return UNALLOCATED_INODE;
      }

      // Make clean directory block for new reference
      BLOCK block;
      oufs_clean_directory_block(new_inode_reference, parent_reference, &block);

      // Write to disk or revert back and return unallocated if there is an
      // issue
      if (vdisk_write_block(new_block_reference, &block) != 0) {
        oufs_deallocate_inode(new_inode_reference);
        oufs_deallocate_block(new_block_reference);
        parent->data[i] = UNALLOCATED_BLOCK;
        fprintf(stderr, "Failed to write new block %d\n", new_block_reference);
        return UNALLOCATED_INODE;
      }

      // Create new references Inode
      INODE new_inode;
      new_inode.type = IT_DIRECTORY;
      new_inode.n_references = 1;
      new_inode.data[0] = new_block_reference;
      for (int j = 1; j < BLOCKS_PER_INODE; j++) {
        new_inode.data[j] = UNALLOCATED_BLOCK;
      }
      new_inode.size = 2;

      // Write to new inode to disk or revert back and return unallocated if
      // issue
      if (oufs_write_inode_by_reference(new_inode_reference, &new_inode) < 0) {
        oufs_deallocate_inode(new_inode_reference);
        oufs_deallocate_block(new_block_reference);
        parent->data[i] = UNALLOCATED_BLOCK;
        BLOCK empty;
        if (vdisk_write_block(new_block_reference, &empty) < 0) {
          // If get here, disk is corrupt
          fprintf(stderr, "VDisk Corruption: Needs Reformatted\n");
          return UNALLOCATED_INODE;
        }
      }
      return new_inode_reference;
    }
  }

  // If made this far, return UNALLOCATED
  return UNALLOCATED_INODE;
}

/**
 *  Make a new directory
 *
 * @param cwd Absolute path representing the current working directory
 * @param path Absolute or relative path to the file/directory
 * @return 0 if success
 *         -x if error
 *
 */
int oufs_mkdir(char *cwd, char *path) {
  INODE_REFERENCE parent;
  INODE_REFERENCE child;
  char local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_mkdir(): ret = %d\n", ret);
    return (-1);
  };

  if (child != UNALLOCATED_INODE) {
    INODE child_inode;
    if (oufs_read_inode_by_reference(child, &child_inode) != 0) {
      return (-5);
    }
    if (child_inode.type == IT_FILE) {
      child = UNALLOCATED_INODE;
    }
  }

  if (parent != UNALLOCATED_INODE && child == UNALLOCATED_INODE) {
    // Parent exists and child does not

    // Get the parent inode
    INODE inode;
    if (oufs_read_inode_by_reference(parent, &inode) != 0) {
      return (-5);
    }

    if (inode.type == IT_DIRECTORY) {
      // Parent is a directory
      BLOCK block;
      // Read the directory
      if (vdisk_read_block(inode.data[0], &block) != 0) {
        return (-6);
      }
      // Find a hole in the directory entry list
      for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if (block.directory.entry[i].inode_reference == UNALLOCATED_INODE) {
          // Found the hole: use this one
          if (debug)
            fprintf(stderr, "Making in parent inode: %d\n", parent);

          INODE_REFERENCE inode_reference =
              oufs_allocate_new_directory(&inode, parent);
          if (inode_reference == UNALLOCATED_INODE) {
            fprintf(stderr, "Disk is full\n");
            return (-4);
          }
          // Add the item to the current directory
          block.directory.entry[i].inode_reference = inode_reference;
          if (debug)
            fprintf(stderr, "new file: %s\n", local_name);
          for (int j = 0; j < FILE_NAME_SIZE; j++){
            block.directory.entry[i].name[j] = '\0';
          }
          for (int j = 0; (j < FILE_NAME_SIZE) && (local_name[j] != 0 && local_name[j] != '\0'); j++) {
            block.directory.entry[i].name[j] = local_name[j];
          }

          // Write the block back out
          if (vdisk_write_block(inode.data[0], &block) != 0) {
            return (-7);
          }

          // Update the parent inode size
          inode.size++;

          // Write out the  parent inode
          if (oufs_write_inode_by_reference(parent, &inode) != 0) {
            return (-8);
          }

          // All done
          return (0);
        }
      }
      // No holes
      fprintf(stderr, "Parent is full\n");
      return (-4);
    } else {
      // Parent is not a directory
      fprintf(stderr, "Parent is a file\n");
      return (-3);
    }
  } else if (child != UNALLOCATED_INODE) {
    // Child exists
    fprintf(stderr, "%s already exists\n", path);
    return (-1);
  } else {
    // Parent does not exist
    fprintf(stderr, "Parent does not exist\n");
    return (-2);
  }
  // Should not get to this point
  return (-100);
}

/**
 *  Given the CWD and PATH, traverse the file system and
 *  delete a directory only if it is valid and has size 2.
 *
 *  @param CWD char* (Current Working Directory of the file system)
 *  @param PATH char* (Path specified by the user)
 *  @param virtual_disk_name char* (disk that is being wrote)
 *  @return 0 = successfully removed directory
 *         -x = Error
 *
 */
int oufs_rmdir(char *cwd, char *path) {
  INODE_REFERENCE parent;
  INODE_REFERENCE child;
  char local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_mkdir(): ret = %d\n", ret);
    return (-1);
  };

  if (parent != UNALLOCATED_INODE && child != UNALLOCATED_INODE) {
    // Parent exists and child Exists
    INODE parent_inode, child_inode, empty_inode;
    // Get the parent Inode
    if (oufs_read_inode_by_reference(parent, &parent_inode) != 0) {
      return (-3);
    }
    // Get the child inode to write new child block
    if (oufs_read_inode_by_reference(child, &child_inode) != 0) {
      return (-3);
    }
    if (parent_inode.type != 'D' || child_inode.type != 'D') {
      return (-2);
    }
    if (child_inode.size > 2) {
      fprintf(stderr, "Directory is not empty\n");
      return (-5);
    }

    // Get the parent Block
    BLOCK parent_block;
    if (vdisk_read_block(parent_inode.data[0], &parent_block) != 0) {
      return (-6);
    }

    // Update Parent Block
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
      if (parent_block.directory.entry[i].inode_reference !=
          UNALLOCATED_INODE) {
        if (!strcmp(parent_block.directory.entry[i].name, local_name)) {
          INODE entry_inode, empty_inode;
          if (oufs_read_inode_by_reference(
                  parent_block.directory.entry[i].inode_reference,
                  &entry_inode) != 0) {
            return (-3);
          }
          if (entry_inode.type == IT_DIRECTORY) {
            DIRECTORY_ENTRY entry = {{0}, UNALLOCATED_INODE};
            entry.inode_reference = UNALLOCATED_INODE;
            for (int j = 0; j < MAX_PATH_LENGTH; j++)
              entry.name[j] = '\0';
            parent_block.directory.entry[i] = entry;
            parent_block.directory.entry[i].inode_reference = UNALLOCATED_INODE;
          }
          entry_inode = empty_inode;
        }
      }
    }
    if (vdisk_write_block(parent_inode.data[0], &parent_block) != 0) {
      return (-6);
    }

    // Update Parent Inode
    for (int i = 0; i < BLOCKS_PER_INODE; i++) {
      if (parent_inode.data[i] == child_inode.data[0]) {
        parent_inode.data[i] = UNALLOCATED_BLOCK;
        parent_inode.size = parent_inode.size - 1;
        parent_inode.n_references--;
      }
    }
    oufs_write_inode_by_reference(parent, &parent_inode);

    // Overwrite the child data block
    if (child_inode.n_references == 1) {
      // Deallocate From Master
      oufs_deallocate_block(child_inode.data[0]);
      oufs_deallocate_inode(child);

      BLOCK empty_block = {0, 0, 0, 0};
      if (vdisk_write_block(child_inode.data[0], &empty_block) != 0) {
        return (-6);
      }

      // Overwrite the child inode
      empty_inode.type = IT_NONE;
      empty_inode.n_references = 0;
      empty_inode.size = 0;
      for (int i = 0; i < BLOCKS_PER_INODE; i++) {
        empty_inode.data[i] = UNALLOCATED_BLOCK;
      }
      if (oufs_write_inode_by_reference(child, &empty_inode) != 0) {
        return (-6);
      }
    } else {
      child_inode.n_references--;
      if (oufs_write_inode_by_reference(child, &child_inode) != 0) {
        return (-6);
      }
    }

  } else {
    fprintf(stderr, "%s does not exist\n", path);
    return (-1);
  }
}

/**
 *  Given the CWD and PATH, traverse the file system and
 *  list the contents of a directory in lexigraphic order
 *
 *  @param CWD char* (Current Working Directory of the file system)
 *  @param PATH char* (Path specified by the user)
 *  @param virtual_disk_name char* (disk that is being wrote)
 *  @return 0 = successfully listed all elements
 *         -x = error
 *
 */
int oufs_list(char *cwd, char *path) {
  INODE_REFERENCE parent;
  INODE_REFERENCE child;
  char local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_mkdir(): ret = %d\n", ret);
    return (-1);
  };

  if (parent != UNALLOCATED_INODE && child != UNALLOCATED_INODE) {
    INODE child_inode;
    // Get the parent Inode
    if (oufs_read_inode_by_reference(child, &child_inode) != 0) {
      return (-3);
    }

    // Only print file name if file is found
    if (child_inode.type != 'D') {
      if (child_inode.type == 'F') {
        fprintf(stdout, "%s\n", local_name);
        return (0);
      } else {
        return (-3);
      }
    }

    // Read child block
    BLOCK block;
    if (vdisk_read_block(child_inode.data[0], &block) != 0) {
      return (-6);
    }

    // Gather all entries of directory
    INODE entry_inode, empty_inode;
    char **entries =
        (char **)malloc((DIRECTORY_ENTRIES_PER_BLOCK) * sizeof(char *));
    int j = 0;
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
      if ((block.directory.entry[i].name[0] != '\0') &&
          (strcmp(block.directory.entry[i].name, ".")) &&
          (strcmp(block.directory.entry[i].name, ".."))) {
        entries[j] = malloc(sizeof(char *));
        strcpy(entries[j], block.directory.entry[i].name);
        entry_inode = empty_inode;
        if (block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
          oufs_read_inode_by_reference(block.directory.entry[i].inode_reference,
                                       &entry_inode);
          if (entry_inode.type == IT_DIRECTORY) {
            strcat(entries[j], "/");
          }
        }
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

  } else {
    fprintf(stderr, "%s does not exist\n", path);
    return (-1);
  }
}

/**
 *  Allocate a new file for writing
 *
 *  @param fp the filepointer that is being read from
 *  @param buf the characters being read
 *  @param len the length of buffer
 *  @return 0 = successfully allocated file
 *         -1 = an error has occurred
 *
 */
int oufs_allocate_new_file(char *cwd, char *path) {

  if (debug)
    fprintf(stderr, "passed trailing test\n");

  INODE_REFERENCE parent;
  INODE_REFERENCE child;
  char local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_mkdir(): ret = %d\n", ret);
    return (-1);
  };

  if (debug)
    fprintf(stderr, "found file\n");

  if (parent != UNALLOCATED_INODE && child == UNALLOCATED_INODE) {

    // Allocate new child inode
    child = oufs_allocate_new_inode();

    if (debug)
      fprintf(stderr, "child = %d\n", child);

    // Update New Inode and write
    INODE new_inode = {0};
    new_inode.type = IT_FILE;
    new_inode.n_references = 1;
    for (int i = 0; i <= BLOCKS_PER_INODE; i++) {
      new_inode.data[i] = UNALLOCATED_BLOCK;
    }
    new_inode.size = 0;
    if (debug) {
      fprintf(stderr, "type = (%c), ref = (%d), size = (%d)", new_inode.type,
              new_inode.n_references, new_inode.size);
    }
    oufs_write_inode_by_reference(child, &new_inode);

    if (debug)
      fprintf(stderr, "wrote inode to disk\n");

    // Read parent inode for updating
    INODE parent_inode;
    if (oufs_read_inode_by_reference(parent, &parent_inode) != 0) {
      return (-3);
    }

    if (debug)
      fprintf(stderr, "read parent inode from disk\n");

    // Read parent block for updating
    BLOCK block;
    if (vdisk_read_block(parent_inode.data[0], &block) != 0) {
      return (-3);
    }

    if (debug)
      fprintf(stderr, "read parent block\n");

    // Create new directory entry
    DIRECTORY_ENTRY new_entry = {0};
    strncpy(new_entry.name, local_name, FILE_NAME_SIZE - 1);
    new_entry.inode_reference = child;
    if (debug) {
      fprintf(stderr, "name = (%s), ref = (%d)", new_entry.name,
              new_entry.inode_reference);
    }

    // Check for same name entries
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
      if (!strncmp(block.directory.entry[i].name, local_name,
                   FILE_NAME_SIZE - 1)) {
        if (debug)
          fprintf(stderr, "found matching name\n");
        if (block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
          if (debug)
            fprintf(stderr, "entry is allocated\n");

          INODE inode;
          oufs_read_inode_by_reference(block.directory.entry[i].inode_reference,
                                       &inode);
          if (inode.type == IT_FILE || inode.type == IT_NONE) {
            if (debug)
              fprintf(stderr, "Entry is file, exiting\n");
            return (0);
          }
        } else {
          if (debug)
            fprintf(stderr, "Entry is unallocated?\n");
          return (0);
        }
      }
    }

    // Add entry to directory
    for (int i = 0; (i < DIRECTORY_ENTRIES_PER_BLOCK); i++) {
      if (block.directory.entry[i].inode_reference == UNALLOCATED_INODE) {
        if (debug)
          fprintf(stderr, "Added Entry\n");
        block.directory.entry[i] = new_entry;
        break;
      }
    }

    // Write back parent block
    if (vdisk_write_block(parent_inode.data[0], &block) != 0) {
      return (-3);
    }

    if (debug)
      fprintf(stderr, "wrote parent block to disk\n");

    // Increment parent inode size and write back
    parent_inode.size = parent_inode.size + 1;
    if (oufs_write_inode_by_reference(parent, &parent_inode) != 0) {
      return (-3);
    }

    if (debug)
      fprintf(stderr, "incremented parent size and wrote inode to disk\n");

    // Return success
    return (0);

  } else {
    if (child != UNALLOCATED_INODE) {

      if (debug)
        fprintf(stderr, "Child exists\n");

      // Read child inode
      INODE child_inode;
      if (oufs_read_inode_by_reference(child, &child_inode) != 0) {
        return (-3);
      }

      if (debug)
        fprintf(stderr, "Child inode read from disk\n");

      // Check inode type
      if (child_inode.type == 'F') {
        if (debug)
          fprintf(stderr, "Child file already exists\n");

        // Truncate file
        for (int i = 0; i < BLOCKS_PER_INODE; i++) {
          if (child_inode.data[i] != UNALLOCATED_BLOCK) {
            BLOCK empty_block;
            if (vdisk_write_block(child_inode.data[i], &empty_block) != 0) {
              return (-3);
            }
            oufs_deallocate_block(child_inode.data[i]);
            child_inode.data[i] = UNALLOCATED_BLOCK;
          }
        }
        child_inode.size = 0;

        // Write back to disk
        if (oufs_write_inode_by_reference(child, &child_inode) != 0) {
          return (-3);
        }

        // Return success
        return (0);
      }
      fprintf(stderr, "%s is a directory\n", path);
      return (-1);
    }
  }
}

/**
 *  Open file for reading or writing depending on the mode
 *
 *  @param cwd the current working directory
 *  @param path The path of the file
 *  @param mode the access mode of the file
 *  @return 0 = successfully opened file
 *         -1 = an error has occurred
 *
 */
OUFILE oufs_fopen(char *cwd, char *path, char mode) {
  OUFILE empty;

  if (debug)
    fprintf(stderr, "passed trailing / test\n");

  INODE_REFERENCE parent;
  INODE_REFERENCE child;
  char local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_mkdir(): ret = %d\n", ret);
    return empty;
  };

  if (debug)
    fprintf(stderr, "found file on disk\n");

  if (parent != UNALLOCATED_INODE && child != UNALLOCATED_INODE) {

    // Read file inode and create file pointer
    OUFILE f;
    f.inode_reference = child;
    f.mode = mode;
    f.offset = 0;
    if (mode == 'a') {
      INODE inode;
      if (oufs_read_inode_by_reference(child, &inode) != 0) {
        return empty;
      }
      f.offset = inode.size;
    }

    if (debug)
      fprintf(stderr, "Child file pointer created and returned\n");

    // Return file pointer
    return f;

  } else {
    if (debug)
      fprintf(stderr, "Child doesnt exist\n");
    // Allocated new file
    if (oufs_allocate_new_file(cwd, path) != 0) {
      return empty;
    }
    if (debug)
      fprintf(stderr, "New file allocated and recursed\n");

    // Recurse back to top and create pointer for allocated file
    return (oufs_fopen(cwd, path, mode));
  }
}

/**
 *  Close file pointer
 *
 *  @param fp the file pointer to be closed
 *  @return 0 = successfully closed file
 *         -1 = an error has occurred
 *
 */
void oufs_fclose(OUFILE *fp) {

  // Read inode by fp
  INODE inode;
  if (oufs_read_inode_by_reference(fp->inode_reference, &inode) != 0) {
    return;
  }

  if (debug)
    fprintf(stderr, "Inode read from block by file pointer\n");

  // Find empty blocks in the inode and deallocate them
  for (int i = 0; i < BLOCKS_PER_INODE; i++) {
    if (inode.data[i] != UNALLOCATED_BLOCK) {
      BLOCK block;
      if (debug)
        fprintf(stderr, "block: (%d)\n", inode.data[i]);
      if (vdisk_read_block(inode.data[i], &block) != 0) {
        return;
      }
      if (debug)
        fprintf(stderr, "block read from inode\n");
      if (block.data.data[0] == 0xff) {
        inode.data[i] = UNALLOCATED_BLOCK;
        if (debug)
          fprintf(stderr, "Unallocating found empty block\n");
        oufs_deallocate_block(inode.data[i]);
        if (debug)
          fprintf(stderr, "Block unallocated\n");
      }
    }
  }

  // Write inode back to disk and return
  if (oufs_write_inode_by_reference(fp->inode_reference, &inode) != 0) {
    return;
  }

  if (debug)
    fprintf(stderr, "file pointer inode wrote to block\n");
  return;
}

/**
 *  write to file using a buffer
 *
 *  @param fp the filepointer that is being wrote to
 *  @param buf the characters being wrote
 *  @param len the length of buffer
 *  @return 0 = successfully write to file
 *         -x = an error has occurred
 *
 */
int oufs_fwrite(OUFILE *fp, unsigned char *buf, int len) {

  if (debug) {
    fprintf(stderr, "file ref: (%d)\n", fp->inode_reference);
    fprintf(stderr, "file mode: (%c)\n", fp->mode);
  }

  // Check file pointer permissions
  if (fp->mode == 'r') {
    fprintf(stderr, "Invalid permission to write\n");
    return (-1);
  }

  if (debug)
    fprintf(stderr, "file pointer passed permissions\n");

  // Read file pointer inode
  INODE inode;
  INODE empty_inode;
  if (oufs_read_inode_by_reference(fp->inode_reference, &inode) != 0) {
    return (-3);
  }

  // Memory check
  if ((inode.size + len) > ((BLOCK_SIZE - 1) * BLOCKS_PER_INODE)) {
    fprintf(stderr, "Not enough memory\n");
    return (-2);
  }

  // Get blocks allocated and last block size
  int blocks_allocated = inode.size / (BLOCK_SIZE); // TOTAL SIZE / 254
  int last_block_size = inode.size % (BLOCK_SIZE);  // TOTAL SIZE % 254

  int touched_blocks =
      (last_block_size + len) / (BLOCK_SIZE); // Touched Total Size / 254
  if (((last_block_size + len) % (BLOCK_SIZE)) > 0) {
    touched_blocks = touched_blocks + 1;
  }

  // Declare N blocks for reading
  BLOCK_REFERENCE allocated_block_references[touched_blocks];
  BLOCK allocated_block[touched_blocks];
  BLOCK empty_block;

  // Allocate all blocks
  for (int i = 0; i < touched_blocks; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      allocated_block[i].data.data[j] = 0xff;
    }
  }

  // Grab last block if it is there
  if (last_block_size > 0 && fp->mode == 'a') {

    // Grab last block from disk
    int i = 0;
    while (inode.data[i] != UNALLOCATED_BLOCK && i < BLOCKS_PER_INODE) {
      i = i + 1;
    }
    i = i - 1;
    allocated_block[0] = empty_block;
    if (vdisk_read_block(inode.data[i], &allocated_block[0]) != 0) {
      return (-3);
    }
    allocated_block_references[0] = inode.data[i];

    // Allocate rest of blocks
    for (i = 1; i < touched_blocks; i++) {
      allocated_block_references[i] = oufs_allocate_new_block();
      for (int j = 0; j < BLOCK_SIZE; j++) {
        allocated_block[i].data.data[j] = 0xff;
      }
    }

    // Write data to blocks
    int block_count = 0;
    int data_count = last_block_size + 1;
    for (i = 0; i < len; i++) {
      allocated_block[block_count].data.data[data_count] = buf[i];
      data_count = data_count + 1;
      if (data_count >= (BLOCK_SIZE)) {
        data_count = 0;
        block_count = block_count + 1;
      }
    }

    // Go to last allocated block
    i = 0;
    while (inode.data[i] != UNALLOCATED_BLOCK && i < BLOCKS_PER_INODE) {
      i = i + 1;
    }
    i = i - 1;

    // Write all new blocks plus last previous block if used
    for (int j = 0; j < touched_blocks; j++) {
      inode.data[i] = allocated_block_references[j];
      if (vdisk_write_block(inode.data[i], &allocated_block[j]) != 0) {
        return (-3);
      }
      i++;
    }

    // Add to inode size and write inode back
    inode.size = inode.size + len;
    if (oufs_write_inode_by_reference(fp->inode_reference, &inode) != 0) {
      return (-3);
    }

    // Return success
    return (0);

  } else if (last_block_size == 0 && fp->mode != 'r') {

    // Allocate all blocks
    for (int i = 0; i < touched_blocks; i++) {
      allocated_block_references[i] = oufs_allocate_new_block();
      for (int j = 0; j < BLOCK_SIZE; j++) {
        allocated_block[i].data.data[j] = 0xff;
      }
    }

    // Write data to blocks
    int block_count = 0;
    int data_count = 0;
    for (int i = 0; i < len; i++) {
      allocated_block[block_count].data.data[data_count] = buf[i];
      data_count = data_count + 1;
      if (data_count >= (BLOCK_SIZE)) {
        data_count = 0;
        block_count = block_count + 1;
      }
    }

    // Write all new blocks to disk
    for (int j = 0; j < touched_blocks; j++) {
      if (vdisk_write_block(allocated_block_references[j],
                            &allocated_block[j]) != 0) {
        return (-3);
      }
    }

    // Read fp inode
    INODE write_inode;
    if (oufs_read_inode_by_reference(fp->inode_reference, &write_inode) != 0) {
      return (-3);
    }

    // Add refs to inode block
    for (int j = 0; j < touched_blocks; j++) {
      write_inode.data[j] = allocated_block_references[j];
    }

    // Add to size and set to IT_FILE and write back
    write_inode.size = write_inode.size + len;
    write_inode.type = IT_FILE;
    if (oufs_write_inode_by_reference(fp->inode_reference, &write_inode) != 0) {
      return (-3);
    }

    // Return successful
    return (0);
  } else {
    fprintf(stderr, "Do not have permission to write\n");
    return (-1);
  }
}

/**
 *  Read from file inot buffer
 *
 *  @param fp the filepointer that is being read from
 *  @param buf the characters being read
 *  @param len the length of buffer
 *  @return 0 = successfully read from file
 *         -x = an error has occurred
 *
 */
int oufs_fread(OUFILE *fp, unsigned char *buf, int len) {

  if (debug)
    fprintf(stderr, "Length: (%d)\n", len);

  // Check permissions
  if (fp->mode != 'r') {
    fprintf(stderr, "Invalid permission to write\n");
    return (-1);
  }

  // Read inode by file pointer
  INODE inode;
  if (oufs_read_inode_by_reference(fp->inode_reference, &inode) != 0) {
    fprintf(stderr, "Inode Ref: (%d) not found\n", fp->inode_reference);
    return (-3);
  }

  int length = inode.size;
  int blocks_allocated = length / (BLOCK_SIZE);
  if (blocks_allocated > BLOCKS_PER_INODE) {
    fprintf(stderr, "File corrupt\n");
  }

  // Read all blocks and populate buffer with data
  BLOCK block, empty_block;
  int block_count = 0;
  int j = 0;
  for (int i = 0; i < length; i++) {
    if (i % (BLOCK_SIZE) == 0) {
      if (vdisk_read_block(inode.data[block_count], &block) != 0) {
        return (-3);
      }
      block_count++;
      j = 0;
    }
    buf[i] = block.data.data[j];
    j++;
  }

  // Return successful
  return (0);
}

/**
 *  Remove a file from directory
 *
 *  @param cwd the current working directory
 *  @param path the path to the file being removed
 *  @return 0 = successfully removed file
 *         -x = an error has occurred
 *
 */
int oufs_remove(char *cwd, char *path) {

  // Check for trailing /
  if (path[strlen(path) - 1] == '/') {
    fprintf(stderr, "Path has a trailing /\n");
    return (-3);
  }

  INODE_REFERENCE parent;
  INODE_REFERENCE child;
  char local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_remove(): ret = %d\n", ret);
    return (-1);
  };

  if (parent != UNALLOCATED_INODE && child != UNALLOCATED_INODE) {

    // Read child inode
    INODE child_inode;
    if (oufs_read_inode_by_reference(child, &child_inode) != 0) {
      return (-3);
    }

    // Check inode type
    if (child_inode.type != 'F') {
      return (-3);
    }

    // Read parent inode
    INODE parent_inode;
    if (oufs_read_inode_by_reference(parent, &parent_inode) != 0) {
      return (-3);
    }

    // Increment size and write parent back
    parent_inode.size--;
    if (oufs_write_inode_by_reference(parent, &parent_inode) != 0) {
      return (-3);
    }

    // Read parent block
    BLOCK block;
    if (vdisk_read_block(parent_inode.data[0], &block) != 0) {
      return (-3);
    }

    // Remove directory entry from parent block
    DIRECTORY_ENTRY entry;
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
      entry = block.directory.entry[i];
      if (!strcmp(entry.name, local_name)) {
        INODE entry_inode;
        if (oufs_read_inode_by_reference(entry.inode_reference, &entry_inode) !=
            0) {
          return (-3);
        }
        if (entry_inode.type == IT_FILE) {
          entry.inode_reference = UNALLOCATED_INODE;
          if (debug)
            fprintf(stderr, "name: (%s), local: (%s)", entry.name, local_name);
          for (int j = 0; j < FILE_NAME_SIZE; j++) {
            entry.name[j] = '\0';
          }
        }
        block.directory.entry[i] = entry;
      }
    }

    // Write the parent block back
    if (vdisk_write_block(parent_inode.data[0], &block) != 0) {
      return (-3);
    }

    // If references is 1, delete block and deallocated all
    if (child_inode.n_references == 1) {
      child_inode.type = IT_NONE;
      child_inode.n_references = 0;
      child_inode.size = 0;
      for (int i = 0; i < BLOCKS_PER_INODE; i++) {
        if (child_inode.data[i] != UNALLOCATED_BLOCK) {
          if (oufs_deallocate_block(child_inode.data[i]) != 0) {
            return (-3);
          }

          // Write child block as empty
          BLOCK empty_block = {0, 0, 0, 0};
          if (vdisk_write_block(child_inode.data[i], &empty_block) != 0) {
            return (-3);
          }
          child_inode.data[i] = UNALLOCATED_BLOCK;
        }
      }

      // Write child inode back
      if (oufs_write_inode_by_reference(child, &child_inode) != 0) {
        return (-3);
      }
      if (oufs_deallocate_inode(child) != 0) {
        return (-3);
      }
    } else {
      // Decrement references and write child back
      child_inode.n_references--;
      if (oufs_write_inode_by_reference(child, &child_inode) != 0) {
        return (-3);
      }
    }

    // Return success
    return (0);
  } else {
    fprintf(stderr, "Path does not exist\n");
  }
}

/**
 *  Link a file or directory to another
 *
 *  @param cwd the current working directory
 *  @param path the path to the file being linked
 *  @param path_dst the path to the file being linked to
 *  @return 0 = successfully linked blocks
 *         -x = an error has occurred
 *
 */
int oufs_link(char *cwd, char *path_src, char *path_dst) {

  INODE_REFERENCE src_parent, dst_parent;
  INODE_REFERENCE src_child, dst_child;
  char src_local_name[MAX_PATH_LENGTH], dst_local_name[MAX_PATH_LENGTH];
  int ret;

  // Attempt to find the specified directory
  if ((ret = oufs_find_file(cwd, path_src, &src_parent, &src_child,
                            src_local_name)) < -1) {
    if (debug)
      fprintf(stderr, "oufs_remove(): ret = %d\n", ret);
    return (-1);
  };

  if (src_parent != UNALLOCATED_INODE && src_child != UNALLOCATED_INODE) {

    // Attempt to find the specified directory
    if ((ret = oufs_find_file(cwd, path_dst, &dst_parent, &dst_child,
                              dst_local_name)) < -1) {
      if (debug)
        fprintf(stderr, "oufs_remove(): ret = %d\n", ret);
      return (-1);
    };

    if (dst_parent != UNALLOCATED_INODE && dst_child == UNALLOCATED_INODE) {

      // Read parent inode of dst
      INODE dst_parent_inode;
      if (oufs_read_inode_by_reference(dst_parent, &dst_parent_inode) != 0) {
        return (-3);
      }

      // If parent is not directory, error
      if (dst_parent_inode.type != 'D') {
        return (-3);
      }

      // Read parent block
      BLOCK block;
      if (vdisk_read_block(dst_parent_inode.data[0], &block) != 0) {
        return (-3);
      }

      // Create new entry
      DIRECTORY_ENTRY new_entry = {0};
      strncpy(new_entry.name, dst_local_name, FILE_NAME_SIZE - 1);
      new_entry.inode_reference = src_child;
      if (debug) {
        fprintf(stderr, "name = (%s), ref = (%d)", new_entry.name,
                new_entry.inode_reference);
      }

      // Find open entry placement
      for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++) {
        if (!strncmp(block.directory.entry[i].name, dst_local_name,
                     FILE_NAME_SIZE - 1)) {
          if (block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
            INODE inode;
            oufs_read_inode_by_reference(
                block.directory.entry[i].inode_reference, &inode);
            if (inode.type == IT_FILE || inode.type == IT_NONE) {
              return (0);
            }
          } else {
            return (0);
          }
        }
      }

      // add entry to parent block
      for (int i = 0; (i < DIRECTORY_ENTRIES_PER_BLOCK); i++) {
        if (block.directory.entry[i].inode_reference == UNALLOCATED_INODE) {
          if (debug)
            fprintf(stderr, "Added Entry\n");
          block.directory.entry[i] = new_entry;
          break;
        }
      }

      // Write parent dst block
      if (vdisk_write_block(dst_parent_inode.data[0], &block) != 0) {
        return (-3);
      }

      // add to size and write parent dst inode
      dst_parent_inode.size = dst_parent_inode.size + 1;
      if (oufs_write_inode_by_reference(dst_parent, &dst_parent_inode) != 0) {
        return (-3);
      }

      // Add a reference to src inode
      INODE src_child_inode;
      if (oufs_read_inode_by_reference(src_child, &src_child_inode) != 0) {
        return (-3);
      }
      src_child_inode.n_references = src_child_inode.n_references + 1;
      if (oufs_write_inode_by_reference(src_child, &src_child_inode) != 0) {
        return (-3);
      }

      // Return success
      return (0);

    } else {
      // Error
      return (-3);
    }
  } else {
    // Error
    return (-3);
  }
}

/**
 *  Given a virtual disk name, create and format virtual disk
 *
 *  @param virtual_disk_name Name of disk to be created
 *  @return 0 = successfully formatted disk
 *         -x = Error
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
