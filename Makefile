# The old way of compiling and installing (0.x):
# make 
# sudo make install
#
# The new way of compiling and installing (1.x):
# mkdir build && cd build && cmake .. && make
# sudo make install
#

.PHONY: default install clean

default: build/meteor_demod

build/meteor_demod:
	@echo "==========================================================="
	@echo "!!! Please fix your scripts to use the new build system !!!"
	@echo "!!! Check comments in Makefile to see how               !!!"
	@echo "==========================================================="
	mkdir -p build
	cd build && cmake .. && make

install: default
	cd build && make install

uninstall:
	cd build && make uninstall

clean:
	rm -rf build

