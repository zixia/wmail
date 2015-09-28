/*
** $Id: pref.h,v 1.1.1.1 2003/05/07 02:15:31 lfan Exp $
*/
#ifndef	pref_h
#define	pref_h
/*
** Copyright 1998 - 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/



extern void pref_setprefs();
extern void pref_isoldest1st();
extern void pref_isdisplayfullmsg();
extern void pref_displayautopurge();
extern void pref_sortorder();
extern void pref_pagesize();
extern void pref_displayhtml();
extern void pref_displayflowedtext();
extern void pref_displaynoarchive();
extern void pref_displayweekstart();

extern int pref_flagisoldest1st, pref_flagfullheaders;
extern int pref_flagsortorder;
extern int pref_flagpagesize;
extern int pref_autopurge;
extern int pref_showhtml;
extern int pref_noflowedtext;
extern int pref_noarchive;
extern int pref_startofweek;

extern char *pref_from;
extern char *pref_ldap;

extern void pref_init();
extern void pref_signature();
extern void pref_setfrom(const char *p);
extern void pref_setldap(const char *p);


extern char *pref_getdefaultgpgkey();
extern void pref_setdefaultgpgkey(const char *);
#endif
