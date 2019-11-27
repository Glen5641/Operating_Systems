all:
	gcc oufs_lib.c vdisk.c zinspect.c -o zinspect
	gcc oufs_lib.c vdisk.c zformat.c -o zformat
	gcc oufs_lib.c vdisk.c zmkdir.c -o zmkdir
	gcc oufs_lib.c vdisk.c zfilez.c -o zfilez
	gcc oufs_lib.c vdisk.c zrmdir.c -o zrmdir
	gcc oufs_lib.c vdisk.c ztouch.c -o ztouch
	gcc oufs_lib.c vdisk.c zcreate.c -o zcreate
	gcc oufs_lib.c vdisk.c zremove.c -o zremove
	gcc oufs_lib.c vdisk.c zappend.c -o zappend
	gcc oufs_lib.c vdisk.c zlink.c -o zlink
	gcc oufs_lib.c vdisk.c zmore.c -o zmore
clean:
	rm zinspect
	rm zformat
	rm zmkdir
	rm zfilez
	rm zrmdir
	rm ztouch
	rm zcreate
	rm zremove
	rm zappend
	rm zmore
	rm zlink
	rm vdisk1
	-rm *.o$(objects)
	find . -empty -type d -delete
