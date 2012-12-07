##
##  Makefile -- Build procedure for mod_go Apache module
##

builddir=.
top_srcdir=/usr/share/httpd
top_builddir=/usr/share/httpd
include /usr/share/httpd/build/special.mk

#   the used tools
APXS=apxs
APACHECTL=apachectl

#   the script to check if go and godag are installed
.check_ok:
	./check_compiler.sh

#   the default target
all: .check_ok local-shared-build

#   install the shared object file into Apache 
install: install-modules-yes

#   cleanup
clean:
	-rm -f mod_go.o mod_go.lo mod_go.slo mod_go.la 

#   simple test
test: reload
	lynx -mime_header http://localhost/mod_go

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop

