sds: sds.o zmalloc.o
adlist: adlist.o zmalloc.o
dict: dict.o zmalloc.o
anet: anet.o
ae: ae.o zmalloc.o


zmalloc.o: zmalloc.c
sds.o: sds.c sds.h
adlist.o: adlist.c adlist.h
dict.o: dict.c dict.h
anet.o: anet.h anet.c
ae.o: ae.h ae.c

clean:
	rm *.o sds adlist dict anet ae
