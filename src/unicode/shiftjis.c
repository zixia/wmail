/*
 * Shift_JIS <=> Unicode translate functions.
 *   by Yu Kobayashi <mail@yukoba.jp>
 *
 */

#include <stdio.h>
#include <string.h>
#include "unicode.h"

#define SJIS_DEBUG 0

extern const unicode_char* jis2uni_tbls[];
extern const unsigned* uni2jis_tbls[];

static unicode_char *c2u(const struct unicode_info *u,
			 const char *sjis_str, int *err)
{
	unicode_char *uc=0;
	unsigned char hi=0, lo=0;
	int len=0;
	int i=0;
	int pos=0;

#if SJIS_DEBUG>0
	FILE* debugFile;
	
	debugFile = fopen("/tmp/debug.log", "a");
	fprintf(debugFile, "shiftjis c2u start\n");
	fflush(debugFile);
#endif
	
	if(err) *err = -1;
	
	len = strlen(sjis_str);
#if SJIS_DEBUG>0
	fprintf(debugFile, "shiftjis c2u len = %d\n",len);
	fflush(debugFile);
#endif
	
	uc = (unicode_char*)malloc((len+1) * sizeof(unicode_char) *2);
	
	for(i=0; i<len;) {
#if SJIS_DEBUG>0
		fprintf(debugFile, "shiftjis c2u i = %d\n",i);
		fflush(debugFile);
#endif
	
		if((unsigned)sjis_str[i] < 0x80) {
#if SJIS_DEBUG>0
			fprintf(debugFile, "shiftjis c2u sjis_str[i] = %c\n", sjis_str[i]);
			fflush(debugFile);
#endif
	
			uc[pos++] = (unicode_char)sjis_str[i];
			i++;
		} else {
			hi = (unsigned char)sjis_str[i];
			lo = (unsigned char)sjis_str[i+1];
			
#if SJIS_DEBUG>0
			fprintf(debugFile, "shiftjis c2u hi = %ud lo = %ud\n", hi, lo);
			fflush(debugFile);
#endif
	
			/* SJIS -> JIS */
			if( lo < 0x9f ) {
				if( hi < 0xa0 ) {
					hi -= 0x81;
					hi *= 2;
					hi += 0x21;
				} else {
					hi -= 0xe0;
					hi *= 2;
					hi += 0x5f;
				}
				if( lo > 0x7f )
					--lo;
				lo -= 0x1f;
			} else {
				if( hi < 0xa0 ) {
					hi -= 0x81;
					hi *= 2;
					hi += 0x22;
				} else {
					hi -= 0xe0;
					hi *= 2;
					hi += 0x60;
				}
				lo -= 0x7e;
			}
			
#if SJIS_DEBUG>0
			fprintf(debugFile, "shiftjis c2u hi = %ud lo = %ud\n", hi, lo);
			fflush(debugFile);
#endif
	
			/* JIS -> Unicode */
			if (jis2uni_tbls[hi-0x21] != NULL)
				uc[pos++] = jis2uni_tbls[hi-0x21][lo-0x21];
			else
				uc[pos++] = lo;
			
			i+=2;
		}
	}
	uc[pos++] = 0;

#if SJIS_DEBUG>0
	fprintf(debugFile, "shiftjis c2u done\n");
	fclose(debugFile);
#endif
 	return uc;
}

static char *u2c(const struct unicode_info *u,
		 const unicode_char *str, int *err)
{
	int i=0;
	int pos=0;
	int len=0;
	char* s;
	
	if(err) *err = -1;
	
	while(str[len])
		len++;
	s = malloc((len+1)*2);
	
	for(i=0; str[i]; i++) {
		int jis_char = 0;
		unsigned char hi=0, lo=0;

		unsigned char str_i_high=str[i] >> 8;

		if (uni2jis_tbls[str_i_high] == NULL)
			hi=lo=0xFF;
		/* Unicode -> JIS */
		else
		{
			jis_char = uni2jis_tbls[str_i_high][str[i] & 0xff];
			hi = jis_char >> 8;
			lo = jis_char & 0xff;
		
		/* JIS -> SJIS */
			if( ( hi % 2 ) == 0 )
				lo += 0x7d;
			else
				lo += 0x1f;
   
			if( lo > 0x7e )
				++ lo;
   
			if( hi < 0x5f ) {
				++hi;
				hi /= 2;
				hi += 0x70;
			} else {
				++hi;
				hi /= 2;
				hi += 0xb0;
			}
		}
		s[pos++] = hi;
		s[pos++] = lo;
	}
	s[pos] = 0;
    
	return s;
}

static char *toupper_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
  unicode_char *uc = c2u(u, cp, ip);
  char *s;
  size_t i;
  int dummy;
  
  if (!uc)
    return (NULL);

  for (i=0; uc[i] && i<10000; i++) {
    if ((unicode_char)'a' <= uc[i] && uc[i] <= (unicode_char)'z')
      uc[i] = uc[i] - ((unicode_char)'a' - (unicode_char)'A');
  }
  
  s = u2c(u, uc, &dummy);
  free(uc);
  return (s);
}

static char *tolower_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
  unicode_char *uc = c2u(u, cp, ip);
  char *s;
  size_t i;
  int dummy;
  
  if (!uc)
    return (NULL);

  for (i=0; uc[i]; i++) {
    if ((unicode_char)'A' <= uc[i] && uc[i] <= (unicode_char)'Z')
      uc[i] = uc[i] + ((unicode_char)'a' - (unicode_char)'A');
  }

  s = u2c(u, uc, &dummy);
  free(uc);
  
  return (s);
}


static char *totitle_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
  unicode_char *uc = c2u(u, cp, ip);
  char *s;
  int dummy;
  
  if (!uc)
    return (NULL);

  /* Uh, sorry, what's "title" char? */
  /*
   * for (i=0; uc[i]; i++)
   * uc[i] = unicode_tc(uc[i]);
   */

  s = u2c(u, uc, &dummy);
  free(uc);
  return (s);
}

extern const struct unicode_info unicode_UTF8;

const struct unicode_info unicode_SHIFT_JIS = {
  "SHIFT_JIS",
  UNICODE_MB | UNICODE_SISO,
  c2u,
  u2c,
  toupper_func,
  tolower_func,
  totitle_func,
  &unicode_UTF8
};

