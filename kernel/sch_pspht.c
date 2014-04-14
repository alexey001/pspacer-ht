/*
 * net/sched/sch_pspht.c	PSPacer/HT
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
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <net/pkt_sched.h>

#include "sch_pspht.h"

/*
 * phy_rate is the maximum qdisc rate. If the kernel supports ethtool ioctl,
 * it is corrected. Otherwise it statically sets to the Gigabit rate.
 */
static u64 phy_rate;

struct pspht_sched_data
{
	u64 rate;		/* target rate (byte/sec) */

	u64 base;		/* base clock (nano sec) */
	u64 next;		/* a.k.a. class clock (byte) */
	u64 maxburst;		/* maximum allowable burst size (nanosec) */
	unsigned int maxlen;	/* mtu + hard_header_len */

	struct qdisc_watchdog watchdog;
};

static int pspht_enqueue(struct sk_buff *skb, struct Qdisc* sch)
{
	int limit = qdisc_dev(sch)->tx_queue_len;

	/* XXX tx_queue_len of bonding devices is zero. */
	if (!limit)
		limit = 1000;

	if (likely(skb_queue_len(&sch->q) < limit))
		return qdisc_enqueue_tail(skb, sch);

	return qdisc_reshape_fail(skb, sch);
}

static inline void update_clock(struct sk_buff *skb, struct Qdisc *sch,
				struct pspht_sched_data *q)
{
	u64 pkt_len, wait_len;
	int segs;
	int th_len = 0;

	/* NOTE: we need to consider GSO packets here. */
	segs = skb_is_gso(skb) ? skb_shinfo(skb)->gso_segs : 1;
	if (segs > 1) {
		int proto = -1;

		if (skb->protocol == htons(ETH_P_IP)) {
			struct iphdr *iph = ip_hdr(skb);
			proto = iph->protocol;
		} else if (skb->protocol == htons(ETH_P_IPV6)) {
			struct ipv6hdr *iph = ipv6_hdr(skb);
			proto = iph->nexthdr;
		}

		if (proto == IPPROTO_TCP)
			th_len = tcp_hdrlen(skb);
		else if (proto == IPPROTO_UDP)
			th_len = sizeof(struct udphdr);
	}
	pkt_len = (u64)(skb->len +
			(qdisc_dev(sch)->hard_header_len +
			 skb_network_header_len(skb) + th_len) * (segs - 1));

	/* wait_len = (max_rate / target_rate) * pkt_len */
	wait_len = pkt_len * phy_rate;
	do_div(wait_len, q->rate);
	q->next += wait_len;
}

static inline u64 clock2ns(u64 x)
{
	u64 ns = 0;

	if (phy_rate == 1250000000) {		/* 10 Gbps (clock = 0.8 ns) */
		ns = x << 3;
		do_div(ns, 10);
	} else if (phy_rate == 125000000) {	/* 1 Gbps (clock = 8 ns) */
		ns = x << 3;
	} else if (phy_rate == 12500000) {	/* 100 Mbps (clock = 80 ns) */
		ns = x * 80;
	} else if (phy_rate == 1250000) {	/* 10 Mbps */
		ns = x * 800;
	}
	return ns;
}

static struct sk_buff *pspht_dequeue(struct Qdisc *sch)
{
	struct sk_buff *skb = NULL;
	struct pspht_sched_data *q = qdisc_priv(sch);
	psched_time_t now ,next;

	if (skb_queue_len(&sch->q) == 0)
		return NULL;

	now = psched_get_time();
	if (q->base == 0)
		q->base = PSCHED_TICKS2NS(now);
	next = PSCHED_NS2TICKS(q->base + clock2ns(q->next));
	if (now < next) {
		qdisc_watchdog_schedule(&q->watchdog, next);
		return NULL;
	}

	skb = qdisc_dequeue_head(sch);
	update_clock(skb, sch, q);

	/* reset clock */
	if (now > next) {
		if (PSCHED_TICKS2NS(now - next) > q->maxburst) {
			q->base = 0;
			q->next = 0;
		}
	}

	return skb;
}

static int pspht_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct pspht_sched_data *q = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);
	struct ethtool_cmd cmd = { ETHTOOL_GSET };

	if (dev->ethtool_ops && dev->ethtool_ops->get_settings) {
		if (dev->ethtool_ops->get_settings(dev, &cmd) == 0) {
			phy_rate = (u64)cmd.speed * 1000000;
			do_div(phy_rate, BITS_PER_BYTE);
		}
	}

	if (opt == NULL) {
		return -EINVAL;
	} else {
		struct tc_pspht_qopt *ctl = nla_data(opt);

		if (nla_len(opt) < sizeof(*ctl))
			return -EINVAL;

		q->rate = ctl->rate;
		q->maxburst = (u64)ctl->maxburst * 1000UL;
	}

	qdisc_watchdog_init(&q->watchdog, sch);
	q->maxlen = dev->mtu + dev->hard_header_len;

	return 0;
}

static int pspht_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct pspht_sched_data *q = qdisc_priv(sch);

	if (opt == NULL) {
		return -EINVAL;
	} else {
		struct tc_pspht_qopt *ctl = nla_data(opt);

		if (nla_len(opt) < sizeof(*ctl))
			return -EINVAL;

		q->rate = ctl->rate;
		q->maxburst = (u64)ctl->maxburst * 1000UL;
	}

	qdisc_watchdog_cancel(&q->watchdog);
	q->base = 0;
	q->next = 0;

	return 0;
}

static void pspht_reset(struct Qdisc *sch)
{
	struct pspht_sched_data *q = qdisc_priv(sch);

	qdisc_watchdog_cancel(&q->watchdog);
	qdisc_reset_queue(sch);

	q->base = 0;
	q->next = 0;
}

static void pspht_destroy(struct Qdisc *sch)
{
	struct pspht_sched_data *q = qdisc_priv(sch);

	qdisc_watchdog_cancel(&q->watchdog);
	qdisc_destroy(sch);
}

static int pspht_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct pspht_sched_data *q = qdisc_priv(sch);
	unsigned char *b = skb_tail_pointer(skb);
	struct tc_pspht_qopt opt;

	opt.rate = q->rate;
	opt.maxburst = (u32)q->maxburst / 1000;
	if(!nla_put(skb, TCA_OPTIONS, sizeof(opt), &opt))
	{
		return skb->len;
	}
	else
	{
		nlmsg_trim(skb, b);
		return -1;
	}
}

struct Qdisc_ops pspht_qdisc_ops __read_mostly = {
	.id		=	"pspht",
	.priv_size	=	sizeof(struct pspht_sched_data),
	.enqueue	=	pspht_enqueue,
	.dequeue	=	pspht_dequeue,
	.peek		=	qdisc_peek_dequeued,
	.drop		=	qdisc_queue_drop,
	.init		=	pspht_init,
	.reset		=	pspht_reset,
	.change		=	pspht_change,
	.destroy	=	pspht_destroy,
	.dump		=	pspht_dump,
	.owner		=	THIS_MODULE,
};
EXPORT_SYMBOL(pspht_qdisc_ops);

static int __init pspht_module_init(void)
{
	return register_qdisc(&pspht_qdisc_ops);
}

static void __exit pspht_module_exit(void)
{
	unregister_qdisc(&pspht_qdisc_ops);
}

module_init(pspht_module_init)
module_exit(pspht_module_exit)
MODULE_LICENSE("GPL");
