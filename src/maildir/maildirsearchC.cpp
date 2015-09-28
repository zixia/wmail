/*
** Copyright 2002-2003 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include "config.h"
#include "maildirsearch.h"

static const char rcsid[]="$Id: maildirsearchC.cpp,v 1.1.1.1 2003/05/07 02:14:20 lfan Exp $";

mail::Search::Search()
{
	maildir_search_init(&sei);
}

mail::Search::~Search()
{
	maildir_search_destroy(&sei);
}

