/*
 * net/sched/sch_pspht.h PSPacer/HT
 *
 *		Copyright (C) 2009-2010 National Institute of Advanced
 *		Industrial Science and Technology (AIST), Japan.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	TAKANO Ryousei, <takano-ryousei@aist.go.jp>
 */

#include <linux/version.h>

/* TODO: move to linux/pkt_sched.h */
/* TPQ section */

struct tc_pspht_qopt
{
	__u32	rate;		/* bytes/sec */
	__u32	gap;		/* byte */
	__u32	maxburst;	/* byte */
};
