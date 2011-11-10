INSTALL = cp
BUILD_PATH = ./build/

MTYPE := $(shell uname -m)

ifeq ($(MTYPE), x86_64)
	ARCH := amd64
else 
	ARCH := i386
endif

VERSION = 1.0
RELEASE = 0
DEBIAN_PACKAGE = hast3_$(VERSION)-$(RELEASE)_$(ARCH).deb

All:	source

.PHONY: source TAGS build_dir_debian pre_pkg_debian default_dir debian \
	clean distclean doc

default_dir:
	mkdir -p bin

source: default_dir
	cd src; make release=1; make install 

TAGS:
	#find . -type f -print > cscope.files
	cscope -Rqb
	ctags -R --c-kinds=+p --fields=+S

build_dir_debian:
	mkdir -p $(BUILD_PATH)/DEBIAN/
	mkdir -p $(BUILD_PATH)/usr/bin/
	mkdir -p $(BUILD_PATH)/etc/hast3/

pre_pkg_debian:	source build_dir_debian
	$(INSTALL) debian/control.$(ARCH) $(BUILD_PATH)/DEBIAN/control
	$(INSTALL) debian/postinst  $(BUILD_PATH)/DEBIAN
	$(INSTALL) debian/postrm  $(BUILD_PATH)/DEBIAN
	$(INSTALL) bin/* $(BUILD_PATH)/usr/bin/ 
	$(INSTALL) etc/*.conf* $(BUILD_PATH)/etc/hast3/

debian:	pre_pkg_debian
	cd $(BUILD_PATH); dpkg -b . ../$(DEBIAN_PACKAGE)

clean:
	rm -rf $(BUILD_PATH)
	cd src; make clean
	rm -f bin/*.run 	
	rm -f *.deb *.rpm

distclean:	clean
	rm -f tags cscope*

doc:
	doxygen Doxyfile
