/*
 * $Id: authrad_mod.h,v 1.3.8.1 2004/07/18 22:56:23 sobomax Exp $
 *
 * Digest Authentication - Radius support
 *
 * Copyright (C) 2001-2003 Fhg Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * -------
 * 2003-03-09: Based on auth_mod.h from radius_authorize (janakj)
 */


#ifndef AUTHRAD_MOD_H
#define AUTHRAD_MOD_H

#include "../auth/api.h"

extern struct attr attrs[];
extern struct val vals[];
extern void *rh;
extern int ciscopec;

extern pre_auth_f pre_auth_func;
extern post_auth_f post_auth_func;

#endif /* AUTHRAD_MOD_H */
