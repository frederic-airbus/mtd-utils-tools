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



git clone https://github.com/frederic-airbus/mtd-utils-tools.git
Clonage dans 'mtd-utils-tools'...
remote: Enumerating objects: 45, done.
remote: Counting objects: 100% (45/45), done.
remote: Compressing objects: 100% (41/41), done.
remote: Total 45 (delta 16), reused 20 (delta 4), pack-reused 0 (from 0)
Réception d'objets: 100% (45/45), 31.95 Kio | 1.77 Mio/s, fait.
Résolution des deltas: 100% (16/16), fait.

git clone https://github.com/airbus-forks/mtd-utils-fork.git
Clonage dans 'mtd-utils-fork'...
Username for 'https://github.com': frederic-airbus
Password for 'https://frederic-airbus@github.com': 
remote: Invalid username or token. Password authentication is not supported for Git operations.
fatal: Échec d'authentification pour 'https://github.com/airbus-forks/mtd-utils-fork.git/'
