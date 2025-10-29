Proto Software to access to an ubifs filesystem not mounted by the linux kernel.
Use to locate Physical Erasable Block of data's.

Main Software file: fsck.ubifs.extract.c

Many improvement can be done ! So it's a quick & dirty SW.

First the software scan mtd partition to find the association between Logical/Physical Block.
mtd partition hardcoded to /dev/mtd1
Seem unable to obtain via ioctl these information.

In the second part of the SW, browse specified directory to find ".json" file's.
Matched file's saved to an array.

In the third part, saved files are extracted to "/home/root" directory.
PEB+Offset of data part of file are printed.
Script to extract file's with a standalone script with nanddump command also generated.

In the fourth part, all filesystem's browsed, for each file's the PEB printed.

Few memory leak's, depend on the number of file's...
Thanks for the skrinker.c file, in fourth part, after each file's the TNC indexation freed !
Without shrinker, depend of the complexity of the filesystem (file number and size), RAM size can reach more than 100MB !



