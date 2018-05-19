CFLAGS=-g -Wall

lcdtime : lcdtime.c

.PHONY: clean tar

clean :
	-rm -f core *~ \#* lcdtime

tar :
	expr `cat version` + 1 >version.new
	rm version
	mv version.new version
	cd .. ; tar czf lcdtime-`cat lcdtime/version`.tar.gz lcdtime/
	
