/******************************************************************************
**                                                                          **
** Copyright (c) 2007 Browan Communications Inc.                            **
** All rights reserved.                                                     **
**                                                                          **
******************************************************************************

Version History:
----------------

Version     : 1.0
Date        : MAY 14, 2010
Revised By  : jone.li
Description : Original Version
******************************************************************************
***/
#include <linux/time.h>
struct timeval cfm_tv_now(void)
{
	struct timeval t;
	do_gettimeofday(&t);
	return t;
}
/*
 * -1, 0, 1 if the first arg is smaller, equal or greater to the second.
 */
int cfm_tv_cmp(struct timeval a, struct timeval b)
{
	if (a.tv_sec < b.tv_sec)
		return -1;
	if (a.tv_sec > b.tv_sec)
		return 1;
	/* now seconds are equal */
	if (a.tv_usec < b.tv_usec)
		return -1;
	if (a.tv_usec > b.tv_usec)
		return 1;
	return 0;
}
int cfm_tv_eq(struct timeval a, struct timeval b)
{
	return (a.tv_sec == b.tv_sec && a.tv_usec == b.tv_usec);
}
int cfm_tvdiff_ms(struct timeval end, struct timeval start)
{
	return  ((end.tv_sec - start.tv_sec) * 1000) +
		(((1000000 + end.tv_usec - start.tv_usec) / 1000) - 1000);
}
