cd libselinux/src
gcc -c callbacks.c check_context.c freecon.c init.c label.c label_file.c label_android_property.c -I../include -I../../core/include -DDARWIN -DHOST
ar rcs libselinux.a *.o
cd ../..

cd zlib/src
gcc -c adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inflate.c inftrees.c inffast.c trees.c uncompr.c zutil.c -O3 -USE_MMAP -I..
ar rcs libz.a *.o
cd ../..

cd core/libsparse
gcc -c backed_block.c output_file.c sparse.c sparse_crc32.c sparse_err.c sparse_read.c -Iinclude
ar rcs libsparse.a *.o
gcc -o simg2img simg2img.c sparse_crc32.c -Iinclude libsparse.a -I../../zlib ../../zlib/src/libz.a
cp simg2img ../../../bin
cd ../..

cd extras/ext4_utils
gcc -o make_ext4fs make_ext4fs_main.c make_ext4fs.c ext4fixup.c ext4_utils.c allocate.c contents.c extent.c indirect.c uuid.c sha1.c wipe.c crc16.c -I../../libselinux/include -I../../core/libsparse/include -I../../core/include/ ../../libselinux/src/libselinux.a ../../core/libsparse/libsparse.a ../../zlib/src/libz.a -DHOST -DANDROID
cp make_ext4fs ../../../bin
cd ../..
