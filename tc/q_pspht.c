/*
 * q_pspht.c	PSPacer/HT
 *
 *		Copyright (C) 2009-2010 National Institute of Advanced
 *		Industrial Science and Technology (AIST), Japan.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Ryousei Takano, <takano-ryousei@aist.go.jp>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"
#include "sch_pspht.h"

/* for compat */
#ifndef TIME_UNITS_PER_SEC
#define TIME_UNITS_PER_SEC 1000000
#endif
#if 0 /* To enable it if the version of iproute is ss061002. */
#define get_time get_usecs
#define sprint_time sprint_usecs
#endif


static void explain(void)
{
	fprintf(stderr,
"Usage: ... qdisc add ... pspht [rate RATE]\n"
" rate     target rate\n"
" maxburst max allowable burst size (usec)\n");
}

static void explain1(char *arg)
{
	fprintf(stderr, "Illegal \"%s\"\n", arg);
	explain();
}


static int pspht_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			 struct nlmsghdr *n)
{
	int ok = 0;
	struct tc_pspht_qopt opt;

	memset(&opt, 0, sizeof(opt));
	opt.maxburst = TIME_UNITS_PER_SEC / 10; /* 100 msec */

	while (argc > 0) {
		if (matches(*argv, "rate") == 0) {
			NEXT_ARG();
			if (get_rate(&opt.rate, *argv)) {
				explain1("rate");
				return -1;
			}
			ok++;
		} else if (matches(*argv, "maxburst") == 0) {
			NEXT_ARG();
			if (get_time(&opt.maxburst, *argv) < 0) {
				explain1("maxburst");
				return -1;
			}
		} else if (matches(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "What is \"%s\"?\n", *argv);
			explain();
			return -1;
		}
		argc--;
		argv++;
	}

	if (ok)
		addattr_l(n, 1024, TCA_OPTIONS, &opt, sizeof(opt));
	return 0;
}

static int pspht_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct tc_pspht_qopt *qopt;
	SPRINT_BUF(b1);
	SPRINT_BUF(b2);

	if (opt == NULL)
		return 0;

	if (RTA_PAYLOAD(opt) < sizeof(*qopt))
		return -1;
	qopt = RTA_DATA(opt);
	fprintf(f, "rate %s maxburst %s",
		sprint_rate(qopt->rate, b1), sprint_time(qopt->maxburst, b2));
	return 0;
}

struct qdisc_util pspht_qdisc_util = {
	.id		= "pspht",
	.parse_qopt	= pspht_parse_opt,
	.print_qopt	= pspht_print_opt,
};
