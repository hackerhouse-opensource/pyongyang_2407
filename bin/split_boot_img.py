#!/usr/bin/python

# From mkbootimg source code in Android sources.
# system/core/mkbootimg/mkbootimg.c
# system/core/mkbooting/bootimg.h

# #define BOOT_MAGIC "ANDROID!"
# #define BOOT_MAGIC_SIZE 8
# #define BOOT_NAME_SIZE 16
# #define BOOT_ARGS_SIZE 512

# struct boot_img_hdr
# {
#     unsigned char magic[BOOT_MAGIC_SIZE];

#     unsigned kernel_size;  /* size in bytes */
#     unsigned kernel_addr;  /* physical load addr */

#     unsigned ramdisk_size; /* size in bytes */
#     unsigned ramdisk_addr; /* physical load addr */

#     unsigned second_size;  /* size in bytes */
#     unsigned second_addr;  /* physical load addr */

#     unsigned tags_addr;    /* physical addr for kernel tags */
#     unsigned page_size;    /* flash page size we assume */
#     unsigned unused[2];    /* future expansion: should be 0 */

#     unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

#     unsigned char cmdline[BOOT_ARGS_SIZE];

#     unsigned id[8]; /* timestamp / checksum / sha1 / etc */
# };

import sys
import argparse
import os

INPUT_FILE = "boot.img"
KERNEL_FILE_NAME = "zImage"
RAMDISK_FILE_NAME = "ramdisk.gz"
SECOND_FILE_NAME = "second"


class BootImgHdr:
    MAGIC_SIZE = 8
    MAGIC = "ANDROID!"
    NAME_SIZE = 16
    ARGS_SIZE = 512
    BASE_KERNEL_ADDR = 0x00008000
    BASE_RAMDISK_ADDR = 0x01000000
    BASE_SECOND_ADDR = 0x00F00000
    BASE_TAGS_ADDR = 0x00000100


class IOFile:

    UNSIGNED_INT_SIZE = 4

    def __init__(self, file_name):
        try:
            self.file = open(file_name, "rb")
        except IOError as e:
            print "Error opening: {0}".format(file_name)
            print "I/O error({0}): {1}".format(e.errno, e.strerror)
            sys.exit(1)

    def read(self, number_bytes, string):
        try:
            stream = self.file.read(number_bytes)
        except IOError as e:
            print "Error reading from: {0}".format(string)
            print "I/O error({0}): {1}".format(e.errno, e.strerror)
            self.close()
            sys.exit(1)

        if len(stream) < number_bytes:
            print "Error reading ", string
            self.close()
            sys.exit(1)
        else:
            return stream

    def seek(self, nBytes, pos):
        try:
            self.file.seek(nBytes, pos)
        except IOError as e:
            print "Error seeking"
            print "I/O error({0}): {1}".format(e.errno, e.strerror)
            self.close()
            sys.exit(1)

    def close(self):
        if self.file:
            self.file.close()

    def write(self, file_name, start, file_size):
        self.file.seek(start, 0)
        with open(file_name, "wb") as out_file:
            s = self.read(file_size, file_name)
            try:
                out_file.write(s)
            except IOError as e:
                print "Error writting file: {0}".format(file_name)
                print "I/O error({0}): {1}".format(e.errno, e.strerror)
                self.close()
                sys.exit(1)

            print "File {0} written (length={1}).".format(file_name, file_size)

    def get_unsigned_int(self, number_s):
        "Convert some bytes to an unsigned int"
        number_s = number_s[::-1]
        number_s = number_s.encode('hex')
        return int(number_s, 16)

    def read_uint(self, string=None):
        s = self.read(self.UNSIGNED_INT_SIZE, string)
        result = self.get_unsigned_int(s)
        if string:
            print "{0}: {1}".format(string, result)
        return result


def main():

    parser = argparse.ArgumentParser()

    parser.add_argument("-v", "--verbosity", action="store_true",
                        help="increase output verbosity")
    parser.add_argument("-i", "--input_file", default=INPUT_FILE,
                        help="boot image input file")
    help_o = "write kernel, ramdisk, etc in this output directory"
    parser.add_argument("-o", "--output_dir", default=".",
                        help=help_o)
    args = parser.parse_args()

    in_file_name = args.input_file
    output_dir = args.output_dir
    kernel_file_name = os.path.join(output_dir, KERNEL_FILE_NAME)
    ramdisk_file_name = os.path.join(output_dir, RAMDISK_FILE_NAME)
    second_file_name = os.path.join(output_dir, SECOND_FILE_NAME)
    verb = args.verbosity

    if verb:
        print
        print "input file: {0}".format(in_file_name)
        print "output directory: {0}".format(output_dir)
        print

    in_file = IOFile(in_file_name)

    # Create output directory if it does not exist
    if not os.path.exists(output_dir):
        print "Creating directory: {}".format(output_dir)
        os.makedirs(output_dir)
    elif not os.path.isdir(output_dir):
        print "ERROR: {0} exists but it is not a directory.".format(output_dir)
        sys.exit(1)

    hdr = BootImgHdr()

    hdr.boot_magic = in_file.read(hdr.MAGIC_SIZE, "boot magic")
    if hdr.boot_magic == hdr.MAGIC:
        if verb:
            print "boot_magic correct: ", hdr.boot_magic
    else:
        print "boot_magic not correct: ", hdr.boot_magic
        in_file.close()
        sys.exit(1)

    # Read kernel size
    hdr.kernel_size = in_file.read_uint("kernel size")

    # Read kernel address
    if verb:
        hdr.kernel_addr = in_file.read_uint("kernel address")
    else:
        hdr.kernel_addr = in_file.read_uint()
    hdr.base = hdr.kernel_addr - hdr.BASE_KERNEL_ADDR
    if verb:
        print "Base: {0}  (hex): {1}".format(hdr.base, hex(hdr.base))
    else:
        print "Base (hex): {0}".format(hex(hdr.base))

    # Read ramdisk size
    hdr.ramdisk_size = in_file.read_uint("ramdisk size")

    # Read ramdisk address
    if verb:
        hdr.ramdisk_addr = in_file.read_uint("ramdisk address")
    else:
        hdr.ramdisk_addr = in_file.read_uint()
    base = hdr.ramdisk_addr - hdr.BASE_RAMDISK_ADDR
    if base != hdr.base:
        print ("ERROR: Base from ramdisk address does not match"
               " the one from kernel address. {0}".format(base))

    # Read second size
    hdr.second_size = in_file.read_uint("second size")

    # Read second address
    if verb:
        hdr.second_addr = in_file.read_uint("second address")
    else:
        hdr.second_addr = in_file.read_uint()
    base = hdr.second_addr - hdr.BASE_SECOND_ADDR
    if base != hdr.base:
        print ("ERROR: Base from second address does not match"
               " the one from kernel address. {0}".format(base))

    # Read tags address
    if verb:
        hdr.tags_addr = in_file.read_uint("tags address")
    else:
        hdr.tags_addr = in_file.read_uint()
    base = hdr.tags_addr - hdr.BASE_TAGS_ADDR
    if base != hdr.base:
        print ("ERROR: Base from tags address does not match"
               " the one from kernel address. {0}".format(base))

    # Read page size
    hdr.page_size = in_file.read_uint("page size")

    # Discard unused parts
    in_file.seek(in_file.UNSIGNED_INT_SIZE * 2, 1)

    # Read asciiz product name.
    #unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */
    hdr.product_name = in_file.read(hdr.NAME_SIZE, "product name")
    print "product name: ", hdr.product_name.rstrip("\0")

    # Read cmdline.
    hdr.cmdline = in_file.read(hdr.ARGS_SIZE, "cmdline")
    print "cmdline: ", hdr.cmdline.rstrip("\0")

    # /*
    # ** +-----------------+
    # ** | boot header     | 1 page
    # ** +-----------------+
    # ** | kernel          | n pages
    # ** +-----------------+
    # ** | ramdisk         | m pages
    # ** +-----------------+
    # ** | second stage    | o pages
    # ** +-----------------+
    # **
    # ** n = (kernel_size + page_size - 1) / page_size
    # ** m = (ramdisk_size + page_size - 1) / page_size
    # ** o = (second_size + page_size - 1) / page_size
    # **
    # ** 0. all entities are page_size aligned in flash
    # ** 1. kernel and ramdisk are required (size != 0)
    # ** 2. second is optional (second_size == 0 -> no second)
    # ** 3. load each element (kernel, ramdisk, second) at
    # **    the specified physical address (kernel_addr, etc)
    # ** 4. prepare tags at tag_addr.  kernel_args[] is
    # **    appended to the kernel commandline in the tags.
    # ** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
    # ** 6. if second_size != 0: jump to second_addr
    # **    else: jump to kernel_addr
    # */

    # Download kernel
    kernel_page = 1
    in_file.write(kernel_file_name, kernel_page * hdr.page_size,
                  hdr.kernel_size)

    # Download ramdisk
    ramdisk_page = ((hdr.kernel_size + hdr.page_size - 1) / hdr.page_size +
                    kernel_page)
    in_file.write(ramdisk_file_name, ramdisk_page * hdr.page_size,
                  hdr.ramdisk_size)

    # Download second file if available
    if hdr.second_size != 0:
        second_page = ((hdr.second_size + hdr.page_size - 1) / hdr.page_size +
                       ramdisk_page)
        in_file.write(second_file_name, second_page * hdr.page_size,
                      hdr.second_size)

if __name__ == "__main__":
    main()
