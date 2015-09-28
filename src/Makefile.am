# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:12:57 lfan Exp $
#
# Copyright 1998 - 2003 Double Precision, Inc.  See COPYING for
# distribution information.

@SET_MAKE@

AUTOMAKE_OPTIONS=dist-bzip2

# make distcheck fix

DISTCHECK_CONFIGURE_FLAGS=--enable-cgibindir=$$dc_install_base/cgi-bin --enable-imagedir=$$dc_install_base/images

HTML2TXT=`which lynx 2>/dev/null && echo "lynx -dump -nolist" && exit; echo "links -dump"`

SUBDIRS=numlib @DBSUBDIR@ md5 sha1 random128 libhmac userdb unicode makedat \
	rfc822 rfc2045 maildir liblock soxwrap authlib cgi http11 \
	ldapaddressbook gpglib pcp sqwebmail
DIST_SUBDIRS=numlib gdbmobj bdbobj md5 sha1 random128 libhmac userdb unicode \
	makedat rfc822 rfc2045 maildir liblock soxwrap authlib cgi \
	http11 ldapaddressbook gpglib pcp sqwebmail

noinst_SCRIPTS= sqwebmail.spec

EXTRA_DIST=INSTALL.html INSTALL.vchkpw NEWS.html README NEWS README.html README.pam sysconftool automake.fix COPYING.GPL README.logindomainlist.html

sysconf_DATA=authmodulelist

authmodulelist: authlib/modulelist
	cp authlib/modulelist authmodulelist

# Import userdb documentation+scripts

if HAS_USERDB
userdb_scripts=vchkpw2userdb makeuserdb userdbpw pw2userdb
userdb_data=makeuserdb.8 userdb.8 userdbpw.8
else
userdb_scripts=
userdb_data=
endif

scriptbindir=$(sbindir)
scriptbin_SCRIPTS=$(userdb_scripts)

pkglibPROGRAMS=maildirmake deliverquota reformime makemime mimegpg pcpd

pkglibexecdir=$(libexecdir)/sqwebmail
pkglibexec_PROGRAMS= @pkglibPROGRAMS_exec@

EXTRA_PROGRAMS=

CLEANFILES=$(userdb_scripts) $(pkgman1_DATA) $(pkgman8_DATA) $(sysconf_DATA)

pkgman8dir=@mandir@/man8
pkgman8_DATA=deliverquota.8 $(userdb_data)

pkgman1dir=@mandir@/man1
pkgman1_DATA=maildirmake.1

makeuserdb.8: userdb/makeuserdb.8
	cp userdb/makeuserdb.8 .

userdb.8: userdb/userdb.8
	cp userdb/userdb.8 .

userdbpw.8: userdb/userdbpw.8
	cp userdb/userdbpw.8 .

makeuserdb: userdb/makeuserdb
	cp userdb/makeuserdb .

userdbpw: userdb/userdbpw
	cp userdb/userdbpw .

pw2userdb: userdb/pw2userdb
	cp userdb/pw2userdb .

vchkpw2userdb: userdb/vchkpw2userdb
	cp userdb/vchkpw2userdb vchkpw2userdb

pcpd: pcp/pcpd
	cp pcp/pcpd .

maildirmake: maildir/maildirmake
	cp maildir/maildirmake .

maildirmake.1: maildir/maildirmake.1
	cp maildir/maildirmake.1 .

deliverquota: maildir/deliverquota
	cp maildir/deliverquota .

deliverquota.8: maildir/deliverquota.8
	cp maildir/deliverquota.8 .

reformime: rfc2045/reformime
	cp rfc2045/reformime .

makemime: rfc2045/makemime
	cp rfc2045/makemime .

mimegpg: gpglib/mimegpg
	cp gpglib/mimegpg .

if HAS_USERDB

install-userdb-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgman8dir)
	rm -f $(DESTDIR)$(pkgman8dir)/pw2userdb.8
	rm -f $(DESTDIR)$(pkgman8dir)/vchkpw2userdb.8
	$(LN_S) makeuserdb.8 $(DESTDIR)$(pkgman8dir)/pw2userdb.8
	$(LN_S) makeuserdb.8 $(DESTDIR)$(pkgman8dir)/vchkpw2userdb.8
	$(mkinstalldirs) $(DESTDIR)$(pkglibexecdir)
	$(INSTALL_PROGRAM) makedat/makedatprog $(DESTDIR)$(pkglibexecdir)/makedatprog

install-userdb-script:
	$(mkinstalldirs) $(DESTDIR)$(scriptbindir)
	$(INSTALL_SCRIPT) userdb/userdb.pl $(DESTDIR)$(scriptbindir)/userdb

uninstall-userdb-script:
	rm -f $(DESTDIR)$(scriptbindir)/userdb


uninstall-userdb-local:
	rm -f $(DESTDIR)$(pkgman8dir)/pw2userdb.8
	rm -f $(DESTDIR)$(pkgman8dir)/vchkpw2userdb.8
	rm -f $(DESTDIR)$(pkglibexecdir)/makedatprog
else

install-userdb-local:
	@:

uninstall-userdb-local:
	@:

install-userdb-script:
	@:

uninstall-userdb-script:
	@:


endif

cgibindir=@cgibindir@

#
# Install SqWebMail setuid to root, if we're running as root
#

install-exec-local: install-userdb-script
	$(mkinstalldirs) $(DESTDIR)$(cgibindir)
	test -w $(DESTDIR)$(cgibindir) || exit 0; \
		$(INSTALL_PROGRAM) sqwebmail/wmail \
			$(DESTDIR)$(cgibindir)/wmail ; \
		test -w /etc || continue ; \
			chown root $(DESTDIR)$(cgibindir)/wmail ; \
			chmod u+s $(DESTDIR)$(cgibindir)/wmail
	$(mkinstalldirs) $(DESTDIR)$(libexecdir)/authlib
	for f in `cat authlib/installlist` dummy ; do \
		test "$$f" = "dummy" && continue ; \
		$(INSTALL_PROGRAM) $(INSTALL_STRIP_FLAG) authlib/$$f \
			$(DESTDIR)$(libexecdir)/authlib/$$f ; done
	for f in `cat authlib/installlist.sh` dummy ; do \
		test "$$f" = "dummy" && continue ; \
		$(INSTALL_SCRIPT) authlib/$$f \
			$(DESTDIR)$(libexecdir)/authlib/$$f ; done
	. authlib/authpwdprogs ; for f in `echo $$PWPROGS dummy` ; do \
		test "$$f" = "dummy" && continue ; \
		$(INSTALL_PROGRAM) authlib/$$f \
			$(DESTDIR)$(libexecdir)/authlib/$$f \
			&& chmod u+s $(DESTDIR)$(libexecdir)/authlib/$$f; done
	. authlib/authpwdprogs ; for f in `echo $$PWSCRIPTS dummy` ; do \
		test "$$f" = "dummy" && continue ; \
		$(INSTALL_SCRIPT) authlib/$$f \
			$(DESTDIR)$(libexecdir)/authlib/$$f ; done

install-data-local: install-userdb-local
	. authlib/authdaemonrc ; test "$$authdaemonvar" != "" || exit 0 ; \
		$(mkinstalldirs) $(DESTDIR)@authdaemonvar@ || exit 1 ; \
		chmod 700 $(DESTDIR)@authdaemonvar@
	$(mkinstalldirs) $(DESTDIR)@imagedir@
	for f in `cat sqwebmail/images/filelist` ; do \
		$(INSTALL_DATA) $(srcdir)/sqwebmail/images/$$f $(DESTDIR)@imagedir@/$$f ;\
		done
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	cp /dev/null $(DESTDIR)$(sysconfdir)/nodsn
	@cat sqwebmail/sqwebmail.msg
	@cat authlib/authdaemon.msg

uninstall-local: uninstall-userdb-local uninstall-userdb-script
	test -w $(DESTDIR)$(cgibindir) || exit 0; \
		rm -f $(DESTDIR)$(cgibindir)/sqwebmail
	. authlib/authpwdprogs ; \
	for f in `cat authlib/installlist authlib/installlist.sh ; echo $$PWPROGS $$PWSCRIPTS` ; do \
			rm -f $(DESTDIR)$(libexecdir)/authlib/$$f ; done
	for f in `cat sqwebmail/images/filelist` ; do \
		rm -f $(DESTDIR)@imagedir@/$$f ;\
		done

configure-check:
	@echo "SqWebMail CGI will be installed in @cgibindir@"
	@echo "Images will be installed in @imagedir@"
	@cd sqwebmail && $(MAKE) configure-check

clean-local:
	rm -f config.rpath

all-recursive: all-make-check

all-make-check:
	$(MAKE) all-gmake-check FOO=BAR

all-gmake-check:
	@test "$(FOO)" = "BAR" && exit 0 ; echo "" ; \
		echo "ERROR: Something's wrong.  If you're using xBSD, you must use gmake" ; \
		echo "instead of make, as you were told by the INSTALL file (you did read the" ; \
		echo "INSTALL file, did you?)" ; echo "" ; exit 1

# Maintainer stuff

README: README.html
	$(HTML2TXT) $(srcdir)/README.html >README

NEWS: NEWS.html
	$(HTML2TXT) $(srcdir)/NEWS.html >NEWS

distrelease:
	socksify cvs update
	$(MAKE) distcheck
	$(MAKE) rpm
	socksify cvs commit
	socksify cvs -Q tag -F sqwebmail-latest
	$(MAKE)
	$(MAKE)
	$(MAKE) release

rpm: rpm-build

rpm-build:
	$(MAKE) bump.rpm.release
	$(MAKE) dist
	rm -rf rpm/BUILD/*
	rm -f rpm/RPMS/*/*
	rm -f rpm/SOURCES/*
	rm -f rpm/SPECS/*
	rm -f rpm/SRPMS/*
	rpmbuild -ta @PACKAGE@-@VERSION@.tar.bz2

release:
	rm -f sqwebmail.spec
	$(MAKE) sqwebmail.spec
	$(MAKE) dist

INSTALL.txt: INSTALL.html
	$(HTML2TXT) $(srcdir)/INSTALL.html >INSTALL.txt
	rm -f INSTALL
	cp INSTALL.txt INSTALL

dist-hook:
	rm -f $(distdir)/INSTALL $(distdir)/NEWS $(distdir)/README
	$(HTML2TXT) $(srcdir)/NEWS.html >$(distdir)/NEWS
	$(HTML2TXT) $(srcdir)/README.html >$(distdir)/README
	$(HTML2TXT) $(srcdir)/INSTALL.html >$(distdir)/INSTALL
	cp sqwebmail.spec $(distdir)
	test -f $(srcdir)/rpm.release && cp $(srcdir)/rpm.release $(distdir) \
		; exit 0

bump.rpm.release: sqwebmail.spec
	VERSION="x"; \
	test -f $(srcdir)/rpm.release && . $(srcdir)/rpm.release; \
	NEXT_RELEASE=0; \
	test "$$VERSION" = "@VERSION@" && NEXT_RELEASE="$$RELEASE"; \
	RELEASE=`expr $$NEXT_RELEASE + 1`; \
	echo "VERSION=@VERSION@" >$(srcdir)/rpm.release; \
	echo "RELEASE=$$RELEASE" >>$(srcdir)/rpm.release; \
	sed 's/^Release: .*/Release: '$$RELEASE'%{courier_release}/' \
		<sqwebmail.spec >sqwebmail.spec.new; \
	mv sqwebmail.spec.new sqwebmail.spec

install-configure:
	list="ldapaddressbook.dist `cat authlib/authconfiglist`"; \
		for file in $$list; do \
		file="$(DESTDIR)$(sysconfdir)/$$file" ; \
		case "$$file" in *.dist) \
			@SYSCONFTOOL@ $$file;; esac ; \
        echo "MAILDIRFILTER=../.mailfilter" > $(DESTDIR)$(sysconfdir)/maildirfilterconfig; \
        echo "MAILDIR=./Maildir" >> $(DESTDIR)$(sysconfdir)/maildirfilterconfig; \
	cp -f $(srcdir)/index.html $(DESTDIR)$(cgibindir)/../index.html; done


upload-beta:
	rm -f @PACKAGE@-@VERSION@.tar.bz2.sig; gpg --detach-sign --default-key mrsam@courier-mta.com @PACKAGE@-@VERSION@.tar.bz2; test -f @PACKAGE@-@VERSION@.tar.bz2.sig || exit 1
	socksify scp @PACKAGE@-@VERSION@.tar.bz2 @PACKAGE@-@VERSION@.tar.bz2.sig courier.sourceforge.net:/home/groups/c/co/courier/htdocs/beta/sqwebmail/.
	socksify ssh courier.sourceforge.net echo @PACKAGE@-@VERSION@.tar.bz2 \>/home/groups/c/co/courier/sqwebmail.beta

upload-dist:
	rm -f @PACKAGE@-@VERSION@.tar.bz2.sig; gpg --detach-sign --default-key mrsam@courier-mta.com @PACKAGE@-@VERSION@.tar.bz2; test -f @PACKAGE@-@VERSION@.tar.bz2.sig || exit 1
	socksify cvs rtag -r sqwebmail-latest \
               `echo sqwebmail-@VERSION@ | tr '.' '-'` sqwebmail
	runsocks perl -e 'use Net::FTP; $$ftp=Net::FTP->new("upload.sourceforge.net.", Debug => 1, Passive => 1 ) || die; $$ftp->login("anonymous", "anonymous") || die; $$ftp->cwd("/incoming") && $$ftp->binary() && $$ftp->hash(1) && $$ftp->put("@PACKAGE@-@VERSION@.tar.bz2") && $$ftp->put("@PACKAGE@-@VERSION@.tar.bz2.sig") && exit 0; die;'
