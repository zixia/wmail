#ifndef mimegpgheader_h
#define mimegpgheader_h
/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char mimegpgheader_h_rcsid[]="$Id: mimegpgheader.h,v 1.1.1.1 2003/05/07 02:14:58 lfan Exp $";

#include "config.h"
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct header {
	struct header *next;
	char *header;
} ;

struct read_header_context {
	int continue_header;
	int header_len;
	struct header *first, *last;
} ;

void init_read_header_context(struct read_header_context *);
void read_header(struct read_header_context *, const char *);
struct header *finish_header(struct read_header_context *);
#define READ_START_OF_LINE(cts) ((cts).continue_header == 0)

void free_header(struct header *p);

struct header *find_header(struct header *p, const char *n);
struct header *find_header(struct header *p, const char *n);
const char *find_header_txt(struct header *p, const char *n);

struct mime_header {
	char *header_name;
	struct mime_attribute *attr_list;
} ;

struct mime_attribute {
	struct mime_attribute *next;
	char *name, *value;
} ;

void free_mime_header(struct mime_header *);
struct mime_header *parse_mime_header(const char *);
void set_mime_attr(struct mime_header *, const char *, const char *);
const char *get_mime_attr(struct mime_header *, const char *);
void print_mime_header(FILE *, struct mime_header *);

#ifdef  __cplusplus
} ;
#endif

#endif
