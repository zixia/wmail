#$Id: Makefile.am,v 1.3 2004/01/07 12:06:09 roy Exp $

@SET_MAKE@

htmllibdir=@htmldir@/@lang@
conf=LOCALE LANGUAGE LANGUAGE_PREF ISPELLDICT CHARSET TIMEZONELIST
html = abooklist.html  attachments.html autoresponder.html \
             empty.html expired.html folder.html \
             folders.html forward.html index.html invalid.html keyimport.html \
             filter.html main.html left.html myconfig.html passwd.html \
             newmsg.html printnocookie.html \
             printredirect.html quickadd.html readmsg.html \
             redirect.html signature.html\
             gpg.html gpgcreate.html gpgerr.html sendok.html\
             navbar.inc.html spam.html

htmllib_DATA=$(conf) $(html)

EXTRA_DIST=$(htmllib_DATA)

clonedist: distdir
	-chmod -R a+r $(distdir)
	d=`pwd` ; cd $(distdir) ; GZIP=$(GZIP_ENV) $(AMTAR) chozf $$d/clone.tar.gz .
	-rm -rf $(distdir)

install-perms:
	@echo "$(htmllibdir) 555 bin bin" >permissions.dat
	@for f in $(conf) ; do\
		echo $(htmllibdir)/$$f 644 bin bin config >>permissions.dat ;\
		done
	@for f in $(html) ; do\
		echo $(htmllibdir)/$$f 644 bin bin >>permissions.dat ;\
		done

