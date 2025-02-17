/* nit.c

   Network Interface Tap (NIT) network interface code, by Ted Lemon
   with one crucial tidbit of help from Stu Grossmen. */

/*
 * Copyright (c) 1996, 1998, 1999 The Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.  */

#ifndef EMBED
#ifndef lint
static char copyright[] =
"$Id: nit.c 5329 2007-05-27 05:44:08Z vapier $ Copyright (c) 1996 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */
#endif

#include "dhcpd.h"
#if defined (USE_NIT_SEND) || defined (USE_NIT_RECEIVE)
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <sys/time.h>
#include <net/nit.h>
#include <net/nit_if.h>
#include <net/nit_pf.h>
#include <net/nit_buf.h>
#include <sys/stropts.h>
#include <net/packetfilt.h>

#include <netinet/in_systm.h>
#include "includes/netinet/ip.h"
#include "includes/netinet/udp.h"
#include "includes/netinet/if_ether.h"

/* Reinitializes the specified interface after an address change.   This
   is not required for packet-filter APIs. */

#ifdef USE_NIT_SEND
void if_reinitialize_send (info)
	struct interface_info *info;
{
}
#endif

#ifdef USE_NIT_RECEIVE
void if_reinitialize_receive (info)
	struct interface_info *info;
{
}
#endif

/* Called by get_interface_list for each interface that's discovered.
   Opens a packet filter for each interface and adds it to the select
   mask. */

int if_register_nit (info)
	struct interface_info *info;
{
	int sock;
	char filename[50];
	struct ifreq ifr;
	struct strioctl sio;

	/* Open a NIT device */
	sock = open ("/dev/nit", O_RDWR);
	if (sock < 0)
		error ("Can't open NIT device for %s: %m", info -> name);

	/* Set the NIT device to point at this interface. */
	sio.ic_cmd = NIOCBIND;
	sio.ic_len = sizeof *(info -> ifp);
	sio.ic_dp = (char *)(info -> ifp);
	sio.ic_timout = INFTIM;
	if (ioctl (sock, I_STR, &sio) < 0)
		error ("Can't attach interface %s to nit device: %m",
		       info -> name);

	/* Get the low-level address... */
	sio.ic_cmd = SIOCGIFADDR;
	sio.ic_len = sizeof ifr;
	sio.ic_dp = (char *)&ifr;
	sio.ic_timout = INFTIM;
	if (ioctl (sock, I_STR, &sio) < 0)
		error ("Can't get physical layer address for %s: %m",
		       info -> name);

	/* XXX code below assumes ethernet interface! */
	info -> hw_address.hlen = 6;
	info -> hw_address.htype = ARPHRD_ETHER;
	memcpy (info -> hw_address.haddr, ifr.ifr_ifru.ifru_addr.sa_data, 6);

	if (ioctl (sock, I_PUSH, "pf") < 0)
		error ("Can't push packet filter onto NIT for %s: %m",
		       info -> name);

	return sock;
}
#endif /* USE_NIT_SEND || USE_NIT_RECEIVE */

#ifdef USE_NIT_SEND
void if_register_send (info)
	struct interface_info *info;
{
	/* If we're using the nit API for sending and receiving,
	   we don't need to register this interface twice. */
#ifndef USE_NIT_RECEIVE
	struct packetfilt pf;
	struct strioctl sio;

	info -> wfdesc = if_register_nit (info);

	pf.Pf_Priority = 0;
	pf.Pf_FilterLen = 1;
	pf.Pf_Filter [0] = ENF_PUSHZERO;

	/* Set up an NIT filter that rejects everything... */
	sio.ic_cmd = NIOCSETF;
	sio.ic_len = sizeof pf;
	sio.ic_dp = (char *)&pf;
	sio.ic_timout = INFTIM;
	if (ioctl (info -> wfdesc, I_STR, &sio) < 0)
		error ("Can't set NIT filter: %m");
#else
	info -> wfdesc = info -> rfdesc;
#endif
        if (!quiet_interface_discovery)
		note ("Sending on   NIT/%s%s%s",
		      print_hw_addr (info -> hw_address.htype,
				     info -> hw_address.hlen,
				     info -> hw_address.haddr),
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}
#endif /* USE_NIT_SEND */

#ifdef USE_NIT_RECEIVE
/* Packet filter program...
   XXX Changes to the filter program may require changes to the constant
   offsets used in if_register_send to patch the NIT program! XXX */

void if_register_receive (info)
	struct interface_info *info;
{
	int flag = 1;
	u_int32_t x;
	struct packetfilt pf;
	struct strioctl sio;
	u_int16_t addr [2];
	struct timeval t;

	/* Open a NIT device and hang it on this interface... */
	info -> rfdesc = if_register_nit (info);

	/* Set the snap length to 0, which means always take the whole
	   packet. */
	x = 0;
	if (ioctl (info -> rfdesc, NIOCSSNAP, &x) < 0)
		error ("Can't set NIT snap length on %s: %m", info -> name);

	/* Set the stream to byte stream mode */
	if (ioctl (info -> rfdesc, I_SRDOPT, RMSGN) != 0)
		note ("I_SRDOPT failed on %s: %m", info -> name);

#if 0
	/* Push on the chunker... */
	if (ioctl (info -> rfdesc, I_PUSH, "nbuf") < 0)
		error ("Can't push chunker onto NIT STREAM: %m");

	/* Set the timeout to zero. */
	t.tv_sec = 0;
	t.tv_usec = 0;
	if (ioctl (info -> rfdesc, NIOCSTIME, &t) < 0)
		error ("Can't set chunk timeout: %m");
#endif

	/* Ask for no header... */
	x = 0;
	if (ioctl (info -> rfdesc, NIOCSFLAGS, &x) < 0)
		error ("Can't set NIT flags on %s: %m", info -> name);

	/* Set up the NIT filter program. */
	/* XXX Unlike the BPF filter program, this one won't work if the
	   XXX IP packet is fragmented or if there are options on the IP
	   XXX header. */
	pf.Pf_Priority = 0;
	pf.Pf_FilterLen = 0;

	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 6;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = htons (ETHERTYPE_IP);
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT;
	pf.Pf_Filter [pf.Pf_FilterLen++] = htons (IPPROTO_UDP);
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 11;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_AND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = htons (0xFF);
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 18;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = local_port;

	/* Install the filter... */
	sio.ic_cmd = NIOCSETF;
	sio.ic_len = sizeof pf;
	sio.ic_dp = (char *)&pf;
	sio.ic_timout = INFTIM;
	if (ioctl (info -> rfdesc, I_STR, &sio) < 0)
		error ("Can't set NIT filter on %s: %m", info -> name);

        if (!quiet_interface_discovery)
		note ("Listening on NIT/%s%s%s",
		      print_hw_addr (info -> hw_address.htype,
				     info -> hw_address.hlen,
				     info -> hw_address.haddr),
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}
#endif /* USE_NIT_RECEIVE */

#ifdef USE_NIT_SEND
ssize_t send_packet (interface, packet, raw, len, from, to, hto)
	struct interface_info *interface;
	struct packet *packet;
	struct dhcp_packet *raw;
	size_t len;
	struct in_addr from;
	struct sockaddr_in *to;
	struct hardware *hto;
{
	int bufp;
	unsigned char buf [1536 + sizeof (struct sockaddr)];
	struct sockaddr *junk;
	struct strbuf ctl, data;
	int hw_end;
	struct sockaddr_in foo;
	int result;

	if (!strcmp (interface -> name, "fallback"))
		return send_fallback (interface, packet, raw,
				      len, from, to, hto);

	/* Start with the sockaddr struct... */
	junk = (struct sockaddr *)&buf [0];
	bufp = ((unsigned char *)&junk -> sa_data [0]) - &buf [0];

	/* Assemble the headers... */
	assemble_hw_header (interface, buf, &bufp, hto);
	hw_end = bufp;
	assemble_udp_ip_header (interface, buf, &bufp, from.s_addr,
				to -> sin_addr.s_addr, to -> sin_port,
				raw, len);

	/* Copy the data into the buffer (yuk). */
	memcpy (buf + bufp, raw, len);

	/* Set up the sockaddr structure... */
#if USE_SIN_LEN
	junk -> sa_len = hw_end - 2; /* XXX */
#endif
	junk -> sa_family = AF_UNSPEC;

#if 0 /* Already done. */
	memcpy (junk.sa_data, buf, hw_len);
#endif

	/* Set up the msg_buf structure... */
	ctl.buf = (char *)&buf [0];
	ctl.maxlen = ctl.len = hw_end;
	data.buf = (char *)&buf [hw_end];
	data.maxlen = data.len = bufp + len - hw_end;

	result = putmsg (interface -> wfdesc, &ctl, &data, 0);
	if (result < 0)
		warn ("send_packet_nit: %m");
	return result;
}
#endif /* USE_NIT_SEND */

#ifdef USE_NIT_RECEIVE
ssize_t receive_packet (interface, buf, len, from, hfrom)
	struct interface_info *interface;
	unsigned char *buf;
	size_t len;
	struct sockaddr_in *from;
	struct hardware *hfrom;
{
	int nread;
	int length = 0;
	int offset = 0;
	unsigned char ibuf [1536];
	int bufix = 0;

	length = read (interface -> rfdesc, ibuf, sizeof ibuf);
	if (length <= 0)
		return length;

	/* Decode the physical header... */
	offset = decode_hw_header (interface, ibuf, bufix, hfrom);

	/* If a physical layer checksum failed (dunno of any
	   physical layer that supports this, but WTH), skip this
	   packet. */
	if (offset < 0) {
		return 0;
	}

	bufix += offset;
	length -= offset;

	/* Decode the IP and UDP headers... */
	offset = decode_udp_ip_header (interface, ibuf, bufix,
				       from, (unsigned char *)0, length);

	/* If the IP or UDP checksum was bad, skip the packet... */
	if (offset < 0)
		return 0;

	bufix += offset;
	length -= offset;

	/* Copy out the data in the packet... */
	memcpy (buf, &ibuf [bufix], length);
	return length;
}

int can_unicast_without_arp ()
{
	return 1;
}

int can_receive_unicast_unconfigured (ip)
	struct interface_info *ip;
{
	return 1;
}

void maybe_setup_fallback ()
{
	struct interface_info *fbi;
	fbi = setup_fallback ();
	if (fbi) {
		if_register_fallback (fbi);
		add_protocol ("fallback", fallback_interface -> fbdesc,
			      fallback_discard, fallback_interface);
	}
}
#endif
