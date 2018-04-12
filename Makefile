all:
	$(MAKE) -C src

install: all
	cp src/kstem $(HOME)/local/bin/
	cp -R data $(HOME)/local/share/kstem
	
clean:
	$(MAKE) -C src clean