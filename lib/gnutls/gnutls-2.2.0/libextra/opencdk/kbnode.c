/* kbnode.c -  keyblock node utility functions
 *        Copyright (C) 1998-2001 Free Software Foundation, Inc.
 *        Copyright (C) 2002, 2003, 2007 Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * OpenCDK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * OpenCDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "opencdk.h"
#include "main.h"
#include "packet.h"


/**
 * cdk_kbnode_new:
 * @pkt: the packet to add
 *
 * Allocate a new key node and add the packet.
 **/
cdk_kbnode_t
cdk_kbnode_new (cdk_packet_t pkt)
{
  cdk_kbnode_t n;
  
  n = cdk_calloc (1, sizeof *n);
  if (!n)
    return NULL;
  n->pkt = pkt;
  return n;
}


void
_cdk_kbnode_clone (cdk_kbnode_t node)
{
  /* Mark the node as clone which means that the packet
     will not be freed, just the node itself. */
  if (node)
    node->is_cloned = 1;
}


/**
 * cdk_kbnode_release:
 * @n: the key node
 *
 * Release the memory of the node.
 **/
void
cdk_kbnode_release (cdk_kbnode_t node)
{
  cdk_kbnode_t n2;
  
  while (node) 
    {
      n2 = node->next;
      if (!node->is_cloned)
	cdk_pkt_release (node->pkt);
      cdk_free (node);
      node = n2;
    }
}


/**
 * cdk_kbnode_delete:
 * @node: the key node
 *
 * Mark the given node as deleted.
 **/
void
cdk_kbnode_delete (cdk_kbnode_t node)
{
  if (node)
    node->is_deleted = 1;
}


/* Append NODE to ROOT.  ROOT must exist! */
void
_cdk_kbnode_add (cdk_kbnode_t root, cdk_kbnode_t node)
{
  cdk_kbnode_t n1;
  
  for (n1 = root; n1->next; n1 = n1->next)
    ;
  n1->next = node;
}


/**
 * cdk_kbnode_insert:
 * @root: the root key node
 * @node: the node to add
 * @pkttype: packet type
 *
 * Insert @node into the list after @root but before a packet which is not of
 * type @pkttype (only if @pkttype != 0).
 **/
void
cdk_kbnode_insert (cdk_kbnode_t root, cdk_kbnode_t node, int pkttype)
{
  if (!pkttype)
    {
      node->next = root->next;
      root->next = node;
    }
  else 
    {
      cdk_kbnode_t n1;
      
      for (n1 = root; n1->next; n1 = n1->next)
	if (pkttype != n1->next->pkt->pkttype) 
	  {
	    node->next = n1->next;
	    n1->next = node;
	    return;
	  }
      /* No such packet, append */
      node->next = NULL;
      n1->next = node;
    }
}


/**
 * cdk_kbnode_find_prev:
 * @root: the root key node
 * @node: the key node
 * @pkttype: packet type
 *
 * Find the previous node (if @pkttype = 0) or the previous node
 * with pkttype @pkttype in the list starting with @root of @node.
 **/
cdk_kbnode_t
cdk_kbnode_find_prev (cdk_kbnode_t root, cdk_kbnode_t node, int pkttype)
{
  cdk_kbnode_t n1;
  
  for (n1 = NULL; root && root != node; root = root->next)
    {
      if (!pkttype || root->pkt->pkttype == pkttype)
	n1 = root;
    }
  return n1;
}


/**
 * cdk_kbnode_find_next:
 * @node: the key node
 * @pkttype: packet type
 *
 * Ditto, but find the next packet.  The behaviour is trivial if
 * @pkttype is 0 but if it is specified, the next node with a packet
 * of this type is returned.  The function has some knowledge about
 * the valid ordering of packets: e.g. if the next signature packet
 * is requested, the function will not return one if it encounters
 * a user-id.
 **/
cdk_kbnode_t
cdk_kbnode_find_next (cdk_kbnode_t node, int pkttype)
{
  for (node = node->next; node; node = node->next)
    {
      if (!pkttype)
	return node;
      else if (pkttype == CDK_PKT_USER_ID &&
	       (node->pkt->pkttype == CDK_PKT_PUBLIC_KEY ||
		node->pkt->pkttype == CDK_PKT_SECRET_KEY))
	return NULL;
      else if (pkttype == CDK_PKT_SIGNATURE &&
	       (node->pkt->pkttype == CDK_PKT_USER_ID ||
		node->pkt->pkttype == CDK_PKT_PUBLIC_KEY ||
		node->pkt->pkttype == CDK_PKT_SECRET_KEY))
	return NULL;
      else if (node->pkt->pkttype == pkttype)
	return node;
    }
  return NULL;
}


/**
 * cdk_kbnode_find:
 * @node: the key node
 * @pkttype: packet type
 *
 * Try to find the next node with the packettype @pkttype.
 **/
cdk_kbnode_t
cdk_kbnode_find (cdk_kbnode_t node, int pkttype)
{
  for (; node; node = node->next)
    {
      if (node->pkt->pkttype == pkttype)
	return node;
    }
  return NULL;
}


/**
 * cdk_kbnode_find_packet:
 * @node: the key node
 * @pkttype: packet type
 *
 * Same as cdk_kbnode_find but it returns the packet instead of the node.
 **/
cdk_packet_t
cdk_kbnode_find_packet (cdk_kbnode_t node, int pkttype)
{
  cdk_kbnode_t res;
  
  res = cdk_kbnode_find (node, pkttype);
  return res? res->pkt : NULL;
}


/****************
 * Walk through a list of kbnodes. This function returns
 * the next kbnode for each call; before using the function the first
 * time, the caller must set CONTEXT to NULL (This has simply the effect
 * to start with ROOT).
 */
cdk_kbnode_t
cdk_kbnode_walk (cdk_kbnode_t root, cdk_kbnode_t * context, int all)
{
  cdk_kbnode_t n;
  
  do 
    {
      if(! *context ) 
	{
	  *context = root;
	  n = root;
	}
      else 
	{
	  n = (*context)->next;
	  *context = n;
	}
    }
  while (!all && n && n->is_deleted);
  return n;
}


/**
 * cdk_kbnode_commit:
 * @root: the nodes
 * 
 * Commit changes made to the kblist at ROOT. Note that ROOT my change,
 * and it is therefore passed by reference.
 * The function has the effect of removing all nodes marked as deleted.
 * returns true if any node has been changed 
 */
int
cdk_kbnode_commit (cdk_kbnode_t * root)
{
  cdk_kbnode_t n, nl;
  int changed = 0;
  
  for (n = *root, nl = NULL; n; n = nl->next)
    {
      if (n->is_deleted)
	{
	  if (n == *root)
	    *root = nl = n->next;
	  else
	    nl->next = n->next;
	  if (!n->is_cloned)
	    cdk_pkt_release (n->pkt);
	  cdk_free (n);
	  changed = 1;
	}
      else
	nl = n;
    }
  return changed;
}


/**
 * cdk_kbnode_remove:
 * @root: the root node
 * @node: the node to delete
 * 
 * Remove a node from the root node.
 */
void
cdk_kbnode_remove (cdk_kbnode_t *root, cdk_kbnode_t node)
{
  cdk_kbnode_t n, nl;
  
  for (n = *root, nl = NULL; n; n = nl->next)
    {
      if (n == node)
	{
	  if (n == *root)
	    *root = nl = n->next;
	  else
	    nl->next = n->next;
	  if (!n->is_cloned)
	    cdk_pkt_release (n->pkt);
	  cdk_free (n);
	}
      else
	nl = n;
    }
}



/**
 * cdk_cdknode_move:
 * @root: root node
 * @node: the node to move
 * @where: destination place where to move the node.
 * 
 * Move NODE behind right after WHERE or to the beginning if WHERE is NULL.
 */
void
cdk_kbnode_move (cdk_kbnode_t * root, cdk_kbnode_t node, cdk_kbnode_t where)
{
  cdk_kbnode_t tmp, prev;
  
  if (!root || !*root || !node)
    return;
  for (prev = *root; prev && prev->next != node; prev = prev->next)
    ;
  if (!prev)
    return; /* Node is not in the list */
  
  if (!where) 
    { /* Move node before root */
      if (node == *root)
	return;
      prev->next = node->next;
      node->next = *root;
      *root = node;
      return;
    }
  if (node == where) /* Move it after where. */
    return;
  tmp = node->next;
  node->next = where->next;
  where->next = node;
  prev->next = tmp;
}


/**
 * cdk_kbnode_get_packet:
 * @node: the key node
 *
 * Return the packet which is stored inside the node in @node.
 **/
cdk_packet_t
cdk_kbnode_get_packet (cdk_kbnode_t node)
{
  if (node)
    return node->pkt;
  return NULL;
}


/**
 * cdk_kbnode_read_from_mem:
 * @ret_node: the new key node
 * @buf: the buffer which stores the key sequence
 * @buflen: the length of the buffer
 *
 * Try to read a key node from the memory buffer @buf.
 **/
cdk_error_t
cdk_kbnode_read_from_mem (cdk_kbnode_t *ret_node,
                          const byte *buf, size_t buflen)
{
  cdk_stream_t inp;
  cdk_error_t rc;
  
  if (!buflen || !ret_node || !buf)
    return CDK_Inv_Value;
  
  *ret_node = NULL;
  rc = cdk_stream_tmp_from_mem (buf, buflen, &inp);
  if (rc)
    return rc;
  rc = cdk_keydb_get_keyblock (inp, ret_node);
  cdk_stream_close (inp);
  return rc;
}

/**
 * cdk_kbnode_write_to_mem_alloc:
 * @node: the key node
 * @r_buf: buffer to hold the raw data
 * @r_buflen: buffer length of the allocated raw data.
 * 
 * The function acts similar to cdk_kbnode_write_to_mem but
 * it allocates the buffer to avoid the lengthy second run.
 */
cdk_error_t
cdk_kbnode_write_to_mem_alloc (cdk_kbnode_t node, 
			       byte **r_buf, size_t *r_buflen)
{
  cdk_kbnode_t n;
  cdk_stream_t s;
  cdk_error_t rc;
  size_t len;
  
  if (!node)
    return CDK_Inv_Value;
  
  *r_buf = NULL;
  *r_buflen = 0;
  
  rc = cdk_stream_tmp_new (&s);
  if (rc)
    return rc;
  
  for (n = node; n; n = n->next)
    {
      /* Skip all packets which cannot occur in a key composition. */
      if (n->pkt->pkttype != CDK_PKT_PUBLIC_KEY &&
	  n->pkt->pkttype != CDK_PKT_PUBLIC_SUBKEY &&
	  n->pkt->pkttype != CDK_PKT_SECRET_KEY &&
	  n->pkt->pkttype != CDK_PKT_SECRET_SUBKEY &&
	  n->pkt->pkttype != CDK_PKT_SIGNATURE &&
	  n->pkt->pkttype != CDK_PKT_USER_ID &&
	  n->pkt->pkttype != CDK_PKT_ATTRIBUTE)
	continue;
      rc = cdk_pkt_write (s, n->pkt);
      if (rc)
	{
	  cdk_stream_close (s);
	  return rc;
	}
    }
  
  cdk_stream_seek (s, 0);
  len  = cdk_stream_get_length (s);
  *r_buf = cdk_calloc (1, len);
  *r_buflen = cdk_stream_read (s, *r_buf, len);
  cdk_stream_close (s);
  return 0;
}
  
  
/**
 * cdk_kbnode_write_to_mem:
 * @node: the key node
 * @buf: the buffer to store the node data
 * @r_nbytes: the new length of the buffer.
 *
 * Try to write the contents of the key node to the buffer @buf and
 * return the length of it in @r_nbytes. If buf is zero, only the
 * length of the node is calculated and returned in @r_nbytes.
 * Whenever it is possible, the cdk_kbnode_write_to_mem_alloc should be used.
 **/
cdk_error_t
cdk_kbnode_write_to_mem (cdk_kbnode_t node, byte *buf, size_t *r_nbytes)
{
  cdk_kbnode_t n;
  cdk_stream_t s;
  size_t len;
  cdk_error_t rc;
  
  if (!node)
    return CDK_Inv_Value;
  
  rc = cdk_stream_tmp_new (&s);
  if (rc)
    return rc;
  
  for (n = node; n; n = n->next)
    {
      /* Skip all packets which cannot occur in a key composition. */
      if (n->pkt->pkttype != CDK_PKT_PUBLIC_KEY &&
	  n->pkt->pkttype != CDK_PKT_PUBLIC_SUBKEY &&
	  n->pkt->pkttype != CDK_PKT_SECRET_KEY &&
	  n->pkt->pkttype != CDK_PKT_SECRET_SUBKEY &&
	  n->pkt->pkttype != CDK_PKT_SIGNATURE &&
	  n->pkt->pkttype != CDK_PKT_USER_ID &&
	  n->pkt->pkttype != CDK_PKT_ATTRIBUTE)
	continue;
      rc = cdk_pkt_write (s, n->pkt);
      if (rc)
	{
	  cdk_stream_close (s);
	  return rc;
	}
    }
  
  cdk_stream_seek (s, 0);
  len = cdk_stream_get_length (s);
  if (!buf) 
    {
      *r_nbytes = len; /* Only return the length of the buffer */
      cdk_stream_close (s);
      return 0;
    }
  if (*r_nbytes < len)
    rc = CDK_Too_Short;
  if (!rc)
    *r_nbytes = cdk_stream_read (s, buf, len);
  cdk_stream_close (s);
  return rc;
}


/**
 * cdk_kbnode_hash:
 * @node: the key node
 * @hashctx: opaque pointer to the hash context
 * @is_v4: OpenPGP signature (yes=1, no=0)
 * @pkttype: packet type to hash (if zero use the packet type from the node)
 * @flags: flags which depend on the operation
 *
 * Hash the key node contents. Two modes are supported. If the packet
 * type is used (!= 0) then the function searches the first node with
 * this type. Otherwise the node is seen as a single node and the type
 * is extracted from it.
 **/
cdk_error_t
cdk_kbnode_hash (cdk_kbnode_t node, gcry_md_hd_t md, int is_v4,
                 int pkttype, int flags)
{
  cdk_packet_t pkt;
  
  if (!node || !md)
    return CDK_Inv_Value;
  if (!pkttype)
    {      
      pkt = cdk_kbnode_get_packet (node);
      pkttype = pkt->pkttype;
    }  
  else
    {      
      pkt = cdk_kbnode_find_packet (node, pkttype);
      if (!pkt)
	return CDK_Inv_Packet;
    }
  
  switch (pkttype) 
    {
    case CDK_PKT_PUBLIC_KEY:
    case CDK_PKT_PUBLIC_SUBKEY:
      _cdk_hash_pubkey (pkt->pkt.public_key, md, flags & 1);
      break;
      
    case CDK_PKT_USER_ID:
      _cdk_hash_userid (pkt->pkt.user_id, is_v4, md); 
      break;
      
    case CDK_PKT_SIGNATURE:
      _cdk_hash_sig_data (pkt->pkt.signature, md);
      break;
      
    default:
      return CDK_Inv_Mode;
    }
  return 0;
}
