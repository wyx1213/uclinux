/*
 * Copyright (C) 2001,2002,2003,2004 Philippe Gerum <rpm@xenomai.org>.
 *
 * 64-bit PowerPC adoption
 *   copyright (C) 2005 Taneli Vähäkangas and Heikki Lindholm
 *   
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _XENO_ASM_POWERPC_BITS_TIMER_H
#define _XENO_ASM_POWERPC_BITS_TIMER_H

#ifndef __KERNEL__
#error "Pure kernel header included from user-space!"
#endif

static inline void xnarch_program_timer_shot(unsigned long delay)
{
	/*
	 * Even though some architectures may use a 64 bits delay
	 * here, we voluntarily limit to 32 bits, 4 billions ticks
	 * should be enough for now. Would a timer needs more, an
	 * extra call to the tick handler would simply occur after 4
	 * billions ticks.  Since the timebase value is used to
	 * express CPU ticks on the PowerPC port, there is no need to
	 * rescale the delay value.
	 */
	rthal_timer_program_shot(delay);
}

static inline int xnarch_send_timer_ipi(xnarch_cpumask_t mask)
{
#ifdef CONFIG_SMP
	return rthal_send_ipi(RTHAL_TIMER_IPI, mask);
#else /* ! CONFIG_SMP */
	return 0;
#endif /* CONFIG_SMP */
}

#endif /* !_XENO_ASM_POWERPC_BITS_TIMER_H */
