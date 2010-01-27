/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003  Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: tty.c 629 2004-10-11 00:45:00Z mrbrown $
# TTY filesystem for UDPTTY.
*/

#include "tty.h"
#include "inet.h"
#include "udp.h"
#include "smap.h"
#include "linux/if_ether.h"

#define DEVNAME "tty"

static u32 g_ip_addr_dst;
static u32 g_ip_addr_src;
static u16 g_ip_port_log;
static u16 g_ip_port_src;

extern iop_device_t tty_device;
static int tty_sema = -1;

static u8 tty_sndbuf[1514] __attribute__((aligned(64))); /* 1 MTU */

/* Init TTY */
void ttyInit(g_param_t *g_param)
{
	udp_pkt_t *udp_pkt;

	close(0);
	close(1);
	DelDrv(DEVNAME);

	if (AddDrv(&tty_device) < 0)
		return;

	open(DEVNAME "00:", 0x1000|O_RDWR);
	open(DEVNAME "00:", O_WRONLY);

	/* Initialize the static elements of our UDP packet */
	udp_pkt = (udp_pkt_t *)tty_sndbuf;

	memcpy(udp_pkt->eth.h_dest, g_param->eth_addr_dst, ETH_ALEN*2);
	udp_pkt->eth.h_proto = 0x0008;	/* Network byte order: 0x800 */

	udp_pkt->ip.hlen = 0x45;
	udp_pkt->ip.tos = 0;
	udp_pkt->ip.id = 0;
	udp_pkt->ip.flags = 0;
	udp_pkt->ip.frag_offset = 0;
	udp_pkt->ip.ttl = 64;
	udp_pkt->ip.proto = 0x11;
	memcpy(&udp_pkt->ip.addr_src.addr, &g_param->ip_addr_src, 4);
	memcpy(&udp_pkt->ip.addr_dst.addr, &g_param->ip_addr_dst, 4);

	udp_pkt->udp_port_src = g_param->ip_port_src;
	udp_pkt->udp_port_dst = g_param->ip_port_log;

	/* these are stored in network byte order, careful later */
	g_ip_addr_dst = g_param->ip_addr_dst;
	g_ip_addr_src = g_param->ip_addr_src;
	g_ip_port_log = g_param->ip_port_log;
	g_ip_port_src = g_param->ip_port_src;
}

/*
 * udptty_output: send an UDP ethernet frame
 */
static int udptty_output(void *buf, int size)
{
	udp_pkt_t *udp_pkt;
	int pktsize, udpsize;
	int oldstate;

	if ((size + sizeof(udp_pkt_t)) > sizeof(tty_sndbuf))
		size = sizeof(tty_sndbuf) - sizeof(udp_pkt_t);

	udp_pkt = (udp_pkt_t *)tty_sndbuf;
	pktsize = size + sizeof(udp_pkt_t);

	udp_pkt->ip.len = htons(pktsize - 14);				/* Subtract the ethernet header size */

	udp_pkt->ip.csum = 0;
	udp_pkt->ip.csum = inet_chksum(&udp_pkt->ip, 20);	/* Checksum the IP header (20 bytes) */

	udpsize = htons(size + 8);							/* Size of the UDP header + data */
	udp_pkt->udp_len = udpsize;
	memcpy(tty_sndbuf + sizeof(udp_pkt_t), buf, size);

	udp_pkt->udp_csum = 0;								/* Don't care... */

	/* send the eth frame */
	CpuSuspendIntr(&oldstate);
	while (smap_xmit(udp_pkt, pktsize) != 0);
	CpuResumeIntr(oldstate);

	return 0;
}

/* TTY driver.  */

static int tty_init(iop_device_t *device)
{
	if ((tty_sema = CreateMutex(IOP_MUTEX_UNLOCKED)) < 0)
		return -1;

	return 0;
}

static int tty_deinit(iop_device_t *device)
{
	DeleteSema(tty_sema);
	return 0;
}

static int tty_stdout_fd(void) { return 1; }

static int tty_write(iop_file_t *file, void *buf, size_t size)
{
	int res = 0;

	WaitSema(tty_sema);
	res = udptty_output(buf, size);

	SignalSema(tty_sema);
	return res;
}

static int tty_error(void) { return -EIO; }

static iop_device_ops_t tty_ops = { tty_init, tty_deinit, (void *)tty_error,
	(void *)tty_stdout_fd, (void *)tty_stdout_fd, (void *)tty_error,
	(void *)tty_write, (void *)tty_error, (void *)tty_error,
	(void *)tty_error, (void *)tty_error, (void *)tty_error,
	(void *)tty_error, (void *)tty_error, (void *)tty_error,
	(void *)tty_error,  (void *)tty_error };

iop_device_t tty_device =
{ DEVNAME, IOP_DT_CHAR|IOP_DT_CONS, 1, "TTY via SMAP UDP", &tty_ops };
