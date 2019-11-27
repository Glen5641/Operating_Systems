***********************
*	Title: Project3     *
*	Name: Clayton Glenn *
* Date 11/11/2018     *
***********************
------------------------------------------------------------------------------
README
------------------------------------------------------------------------------
-----------------------------------ZFORMAT------------------------------------
The first task was to format the disk. Ways to open, close, read, and
write the vdisk is given in the project skeleton. All that was needed was to
first allocate the master and all inode blocks and write them to the vdisk.
In doing so, the master will have inode 0 and the root directory wrote to it.
Inode 0 will have the root directory linked and the root directory resides in
data block 9 with the inode 0 as a reference and the entries "." and "..".
These are both pointing to Inode 0. The rest of the blocks other than the root
will be set wrote as unallocated blocks to fill the rest of the vdisk.
------------------------------------ZMKDIR------------------------------------
The second task was to make a directory. First, the CWD and path are checked
for errors. A find directory function was made to find the directory from the
cwd and the relative path or just an absolute path if one exists. The find
directory function uses given vdisk functions, and the given oufs function to
read an inode by reference. The find directory function returns all the needed
references to update all blocks associated with making a directory including;
the parent inode and the parent directory block. A new inode and new block are
allocated for the new directory which is initialized empty and wrote to the
disk. The parent directory block has the new entry updated and the parent inode
is updated with the new directory block. The master is also updated in
allocating the new directory block and inode.
------------------------------------ZFILEZ------------------------------------
The third task is to make a filez executable to see our new directory. The CWD
and Path are error checked and the make directory function is reverse
engineered to find the specified directory by path. This directory has entries
that are placed in an array of strings. This array excludes the "." and ".."
directories. The array of strings is sorted by qsort function and the
associated comparing function. The "." and ".." directories should always be
first so they are printed first with a trailing '/'. Then the array of sorted
strings are now printed in ascii order with a trailing '/'.
------------------------------------ZRMDIR------------------------------------
The fourth task to kill is to remove a directory. The make directory function
is also reverse engineered in this case also. The CWD and PATH is checked for
errors and existence. If the path exists, then the directory is now searched
for and found. The parent inode has the directory block deallocated and the
parent block has the directory entry removed. An unallocated block is now wrote
to the child block and the master has the child inode and block deallocated.
-----------------------------------ZINSPECT-----------------------------------
This function is given in the skeleton given to us and shows the contents of
the master, any inode, or dblock.
------------------------------------------------------------------------------
COMMANDS
------------------------------------------------------------------------------
                 format = ./zformat
         make directory = ./zmkdir [path]
       remove directory = ./zrmdir [path]
list files in directory = ./zfilez [path]
 inspect block in vdisk = ./zinspect [-master|-inode|-inodee|-dblock] [block#]
------------------------------------------------------------------------------
BUGS
------------------------------------------------------------------------------
As of right now, no potential bugs have been found.
A bug has been patched where leaving the vdisk open while writing is eventually
writing null blocks to the vdisk. Closing and reopening the vdisk is time and
cpu consuming.
------------------------------------------------------------------------------
SOURCES
------------------------------------------------------------------------------
https://stackoverflow.com/questions/5901181/c-string-append
https://bewuethr.github.io/sorting-strings-in-c-with-qsort/
Given project3_parts.tar.gz
------------------------------------------------------------------------------
