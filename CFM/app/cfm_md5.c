#include <stdio.h>
#include <string.h>
#include "md5.h"
#include "cfm_md5.h"

static void convert_reply(char *out, unsigned char *in);

static void convert_reply(char *out, unsigned char *in)
{
	int x;
	for (x=0;x<16;x++){
	   out += sprintf(out, "%2.2x", (int)in[x]);
        }
}
int MD5_convert(unsigned char  *buffer, unsigned len, unsigned char *md5Result)
{
	struct MD5Context md5;
	char reply[16];
	char realreply[64];
	int md5Len;
	
	memset(reply,0,sizeof(reply));
	memset(realreply, 0, sizeof(realreply));
	MD5Init(&md5);
	MD5Update(&md5, (const unsigned char *)buffer, len);
	MD5Final((unsigned char *)reply, &md5);	
	convert_reply(realreply, (unsigned char *)reply);
	//printf("realreply len:%d\n", strlen(realreply));
	//printf("md5:%s\n",realreply);
	md5Len = strlen(realreply);
	memcpy(md5Result, realreply, md5Len);

	return md5Len;
}


