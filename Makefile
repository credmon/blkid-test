
blkid-test: blkid-test.c
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -o blkid-test blkid-test.c -lblkid

clean:
	rm -f *.o a.out blkid-test
