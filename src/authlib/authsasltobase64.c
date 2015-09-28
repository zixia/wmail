#include	<stdlib.h>
#include	"rfc2045/rfc2045.h"

static int write_challenge(const char *p, size_t l, void *vp)
{
	char **cp=(char **)vp;

	while (l)
	{
		if (*p == '\r' || *p == '\n')
		{
			++p;
			--l;
			continue;
		}
		**cp = *p++;
		++*cp;

		--l;
	}

	return 0;
}

char *authsasl_tobase64(const char *p, int l)
{
	char *write_challenge_buf;
	char *write_challenge_ptr;

	struct rfc2045_encode_info encodeInfo;

	if (l < 0)	l=strlen(p);

	write_challenge_buf=malloc((l+3)/3*4+1);
	if (!write_challenge_buf)
		return (0);

	write_challenge_ptr=write_challenge_buf;

	rfc2045_encode_start(&encodeInfo, "base64", &write_challenge,
			     &write_challenge_ptr);

	rfc2045_encode(&encodeInfo, p, l);
	rfc2045_encode_end(&encodeInfo);
	*write_challenge_ptr=0;
	return (write_challenge_buf);
}
