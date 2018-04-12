all:
	$(MAKE) -C src

install: all
	mkdir -p $(HOME)/local/bin $(HOME)/local/share
	cp src/kstem $(HOME)/local/bin/
	rm -Rf $(HOME)/local/share/kstem
	cp -R data $(HOME)/local/share/kstem
	
clean:
	$(MAKE) -C src clean