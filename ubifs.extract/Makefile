# SPDX-License-Identifier: GPL-2.0-only
#
# This file is part of UBIFS.
#
# Copyright (C) 2025 AIRBUS Defence & Space
#
# Authors: Frederic Fraysse

MTD_UTILS_DIR=../2.3.0

MTD_INCLUDE_DIR=${MTD_UTILS_DIR}/include
MTD_LIB_DIR=${MTD_UTILS_DIR}/lib
UBIFS_UTILS_DIR=${MTD_UTILS_DIR}/ubifs-utils
UBIFS_FSCK_DIR=${UBIFS_UTILS_DIR}/fsck.ubifs
UBIFS_LIB_DIR=${UBIFS_UTILS_DIR}/libubifs
UBIFS_COMMON_DIR=${UBIFS_UTILS_DIR}/common


THE_INC = \
-I${UBIFS_COMMON_DIR} \
-I${UBIFS_LIB_DIR} \
-I${MTD_INCLUDE_DIR} \
-I${UBIFS_FSCK_DIR}


CC += -Wall  ${THE_INC} -DVERSION=\"2.3.0\" -D_LARGEFILE64_SOURCE   


OBJS = \
${MTD_LIB_DIR}/libcrc32.o \
${MTD_LIB_DIR}/libmtd_legacy.o \
${MTD_LIB_DIR}/libmtd.o \
${MTD_LIB_DIR}/libubi.o \
${MTD_LIB_DIR}/list_sort.o \
${MTD_LIB_DIR}/rbtree.o \
\
${UBIFS_COMMON_DIR}/bitops.o \
${UBIFS_COMMON_DIR}/crc16.o \
${UBIFS_COMMON_DIR}/hexdump.o \
${UBIFS_COMMON_DIR}/kmem.o \
${UBIFS_COMMON_DIR}/sort.o \
\
${UBIFS_FSCK_DIR}/check_files.o \
${UBIFS_FSCK_DIR}/check_space.o \
${UBIFS_FSCK_DIR}/extract_files.o \
${UBIFS_FSCK_DIR}//handle_disconnected.o \
${UBIFS_FSCK_DIR}/problem.o \
${UBIFS_FSCK_DIR}/rebuild_fs.o \
\
${UBIFS_LIB_DIR}//budget.o \
${UBIFS_LIB_DIR}//commit.o \
${UBIFS_LIB_DIR}/debug.o \
${UBIFS_LIB_DIR}//dir.o \
${UBIFS_LIB_DIR}/find.o \
${UBIFS_LIB_DIR}/gc.o \
${UBIFS_LIB_DIR}/journal.o \
${UBIFS_LIB_DIR}/log.o \
${UBIFS_LIB_DIR}/lprops.o \
${UBIFS_LIB_DIR}/lpt_commit.o \
${UBIFS_LIB_DIR}/lpt.o \
${UBIFS_LIB_DIR}/master.o \
${UBIFS_LIB_DIR}/orphan.o \
${UBIFS_LIB_DIR}/recovery.o \
${UBIFS_LIB_DIR}/replay.o \
${UBIFS_LIB_DIR}/sb.o \
${UBIFS_LIB_DIR}/scan.o \
${UBIFS_LIB_DIR}/super.o \
${UBIFS_LIB_DIR}/tnc_commit.o \
${UBIFS_LIB_DIR}/tnc_misc.o \
${UBIFS_LIB_DIR}/tnc.o \
\
${UBIFS_UTILS_DIR}/fsck.ubifs/load_fs.o


# Generate fsck.ubifs replacing EIO with EBADMSG
FSCK_UBIFS_EBADMSG_OBJS=\
${OBJS} \
${UBIFS_FSCK_DIR}/fsck.ubifs.o \
io_ebadmsg_rw.o


FSCK_UBIFS_EXTRACT_OBJS=\
${OBJS} \
fsck.ubifs.extract.o \
io_ebadmsg_ro.o \
ads_dump.o \
peb_leb.o \
dump_fs.o \
shrinker.o



%.o : %.c
	echo $@
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: fsck.ubifs.ebadmsg ubifs.extract 
	@echo ""
	@echo "*************************"
	@echo "Generated file's:"
	@echo "fsck.ubifs.ebadmsg ubifs.extract ubifs.extract.strip.md5sum ubifs.extract.strip.gz"


fsck.ubifs.ebadmsg: ${FSCK_UBIFS_EBADMSG_OBJS}
	echo $@
	${CC} ${FSCK_UBIFS_EBADMSG_OBJS} -lpthread -o fsck.ubifs.ebadmsg

ubifs.extract: ${FSCK_UBIFS_EXTRACT_OBJS}
	echo $@
	${CC} ${FSCK_UBIFS_EXTRACT_OBJS} -lpthread -o $@
	cp $@ $@.strip
	${STRIP} $@.strip
	md5sum $@.strip > $@.strip.md5sum
	rm -f $@.strip.gz
	gzip --best $@.strip

clean:
	echo "Cleaning all objects"
	rm -f ${FSCK_UBIFS_EBADMSG_OBJS} ${FSCK_UBIFS_EXTRACT_OBJS}

