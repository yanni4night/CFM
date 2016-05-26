#ifndef CFM_MD5_H_
#define CFM_MD5_H_

struct MD5Packet_queue{
	struct MD5Packet_st * MD5Packet_list;
	int temp;
};
struct MD5Packet_st{
	unsigned char md5Result[64];
	int md5Len;
};

int MD5_convert(unsigned char  *buffer, unsigned len, unsigned char *md5Result);
#endif
