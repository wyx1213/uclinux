/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002,2003,2004,2005,2006,2007  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include "eXosip2.h"
#include <eXosip2/eXosip.h>

#include <osip2/osip_mt.h>
#include <osip2/osip_condv.h>

/* #include <osip2/global.h> */
#include <osipparser2/osip_md5.h>
#include "milenage.h"

/* TAKEN from rcf2617.txt */

#define HASHLEN 16
typedef char HASH[HASHLEN];

#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN + 1];

#define IN
#define OUT

/* AKA */
#define MAX_HEADER_LEN  2049
#define KLEN 16
typedef u_char K[KLEN];
#define RANDLEN 16
typedef u_char RAND[RANDLEN];
#define AUTNLEN 16
typedef u_char AUTN[AUTNLEN];

#define AKLEN 6
typedef u_char AK[AKLEN];
#define AMFLEN 2
typedef u_char AMF[AMFLEN];
#define MACLEN 8
typedef u_char MAC[MACLEN];
#define CKLEN 16
typedef u_char CK[CKLEN];
#define IKLEN 16
typedef u_char IK[IKLEN];
#define SQNLEN 6
typedef u_char SQN[SQNLEN];
#define AUTSLEN 14
typedef char AUTS[AUTSLEN];
#define AUTS64LEN 21
typedef char AUTS64[AUTS64LEN];
#define RESLEN 8
typedef unsigned char RES[RESLEN];
#define RESHEXLEN 17
typedef char RESHEX[RESHEXLEN];
typedef char RESHEXAKA2[RESHEXLEN+IKLEN*2+CKLEN*2];

AMF amf="\0\0";
AMF amfstar="\0\0";

/* end AKA */

extern eXosip_t eXosip;

/* Private functions */
void CvtHex (IN HASH Bin, OUT HASHHEX Hex);
static void DigestCalcHA1 (IN const char *pszAlg, IN const char *pszUserName,
                           IN const char *pszRealm,
                           IN const char *pszPassword,
                           IN const char *pszNonce, IN const char *pszCNonce,
                           OUT HASHHEX SessionKey);
static void DigestCalcResponse (IN HASHHEX HA1, IN const char *pszNonce,
                                IN const char *pszNonceCount,
                                IN const char *pszCNonce,
                                IN const char *pszQop,
                                IN int Aka,
                                IN const char *pszMethod,
                                IN const char *pszDigestUri,
                                IN HASHHEX HEntity, OUT HASHHEX Response);
static void DigestCalcResponseAka(IN const char *pszPassword,
				  IN const char *pszNonce,
				  IN const char *pszCNonce,
				  IN const char *pszQop,
				  IN const char *pszMethod,
				  IN const char *pszDigestUri,
				  IN int version,
				  OUT HASHHEX Response);

void
CvtHex (IN HASH Bin, OUT HASHHEX Hex)
{
  unsigned short i;
  unsigned char j;

  for (i = 0; i < HASHLEN; i++)
    {
      j = (Bin[i] >> 4) & 0xf;
      if (j <= 9)
        Hex[i * 2] = (j + '0');
      else
        Hex[i * 2] = (j + 'a' - 10);
      j = Bin[i] & 0xf;
      if (j <= 9)
        Hex[i * 2 + 1] = (j + '0');
      else
        Hex[i * 2 + 1] = (j + 'a' - 10);
    };
  Hex[HASHHEXLEN] = '\0';
}

/* calculate H(A1) as per spec */
static void
DigestCalcHA1 (IN const char *pszAlg,
               IN const char *pszUserName,
               IN const char *pszRealm,
               IN const char *pszPassword,
               IN const char *pszNonce,
               IN const char *pszCNonce, OUT HASHHEX SessionKey)
{
  osip_MD5_CTX Md5Ctx;
  HASH HA1;

  osip_MD5Init (&Md5Ctx);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszUserName, strlen (pszUserName));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszRealm, strlen (pszRealm));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszPassword, strlen (pszPassword));
  osip_MD5Final ((unsigned char *) HA1, &Md5Ctx);
  if ((pszAlg != NULL) && osip_strcasecmp (pszAlg, "md5-sess") == 0)
    {
      osip_MD5Init (&Md5Ctx);
      osip_MD5Update (&Md5Ctx, (unsigned char *) HA1, HASHLEN);
      osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
      osip_MD5Update (&Md5Ctx, (unsigned char *) pszNonce, strlen (pszNonce));
      osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
      osip_MD5Update (&Md5Ctx, (unsigned char *) pszCNonce, strlen (pszCNonce));
      osip_MD5Final ((unsigned char *) HA1, &Md5Ctx);
    }
  CvtHex (HA1, SessionKey);
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
static void
DigestCalcResponse (IN HASHHEX HA1,     /* H(A1) */
                    IN const char *pszNonce,    /* nonce from server */
                    IN const char *pszNonceCount,       /* 8 hex digits */
                    IN const char *pszCNonce,   /* client nonce */
                    IN const char *pszQop,      /* qop-value: "", "auth", "auth-int" */
                    IN int Aka,		/* Calculating AKAv1-MD5 response */
                    IN const char *pszMethod,   /* method from the request */
                    IN const char *pszDigestUri,        /* requested URL */
                    IN HASHHEX HEntity, /* H(entity body) if qop="auth-int" */
                    OUT HASHHEX Response
                    /* request-digest or response-digest */ )
{
  osip_MD5_CTX Md5Ctx;
  HASH HA2;
  HASH RespHash;
  HASHHEX HA2Hex;

  /* calculate H(A2) */
  osip_MD5Init (&Md5Ctx);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszMethod, strlen (pszMethod));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszDigestUri, strlen (pszDigestUri));

  if (pszQop == NULL)
    {
      goto auth_withoutqop;
    }
  else if (0 == strcmp (pszQop, "auth-int"))
    {
      goto auth_withauth_int;
    }
  else if (0 == strcmp (pszQop, "auth"))
    {
      goto auth_withauth;
    }

auth_withoutqop:
  osip_MD5Final ((unsigned char *) HA2, &Md5Ctx);
  CvtHex (HA2, HA2Hex);

  /* calculate response */
  osip_MD5Init (&Md5Ctx);
  osip_MD5Update (&Md5Ctx, (unsigned char *) HA1, HASHHEXLEN);
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszNonce, strlen (pszNonce));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);

  goto end;

auth_withauth_int:

  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) HEntity, HASHHEXLEN);

auth_withauth:
  osip_MD5Final ((unsigned char *) HA2, &Md5Ctx);
  CvtHex (HA2, HA2Hex);

  /* calculate response */
  osip_MD5Init (&Md5Ctx);
  osip_MD5Update (&Md5Ctx, (unsigned char *) HA1, HASHHEXLEN);
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszNonce, strlen (pszNonce));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  if(Aka == 0){
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszNonceCount, strlen (pszNonceCount));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszCNonce, strlen (pszCNonce));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  osip_MD5Update (&Md5Ctx, (unsigned char *) pszQop, strlen (pszQop));
  osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
  }
end:
  osip_MD5Update (&Md5Ctx, (unsigned char *) HA2Hex, HASHHEXLEN);
  osip_MD5Final ((unsigned char *) RespHash, &Md5Ctx);
  CvtHex (RespHash, Response);
}

/*"
ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";*/
static int base64_val(char x)\
{
	switch(x){
		case '=': return -1;
		case 'A': return 0;
		case 'B': return 1;
		case 'C': return 2;
		case 'D': return 3;
		case 'E': return 4;
		case 'F': return 5;
		case 'G': return 6;
		case 'H': return 7;
		case 'I': return 8;
		case 'J': return 9;
		case 'K': return 10;
		case 'L': return 11;
		case 'M': return 12;
		case 'N': return 13;
		case 'O': return 14;
		case 'P': return 15;
		case 'Q': return 16;
		case 'R': return 17;
		case 'S': return 18;
		case 'T': return 19;
		case 'U': return 20;
		case 'V': return 21;
		case 'W': return 22;
		case 'X': return 23;
		case 'Y': return 24;
		case 'Z': return 25;
		case 'a': return 26;
		case 'b': return 27;
		case 'c': return 28;
		case 'd': return 29;
		case 'e': return 30;
		case 'f': return 31;
		case 'g': return 32;
		case 'h': return 33;
		case 'i': return 34;
		case 'j': return 35;
		case 'k': return 36;
		case 'l': return 37;
		case 'm': return 38;
		case 'n': return 39;
		case 'o': return 40;
		case 'p': return 41;
		case 'q': return 42;
		case 'r': return 43;
		case 's': return 44;
		case 't': return 45;
		case 'u': return 46;
		case 'v': return 47;
		case 'w': return 48;
		case 'x': return 49;
		case 'y': return 50;
		case 'z': return 51;
		case '0': return 52;
		case '1': return 53;
		case '2': return 54;
		case '3': return 55;
		case '4': return 56;
		case '5': return 57;
		case '6': return 58;
		case '7': return 59;
		case '8': return 60;
		case '9': return 61;
		case '+': return 62;
		case '/': return 63;
	}
	return 0;
}


static char *base64_decode_string(const char *buf, unsigned int len, int *newlen)
{
	int i,j,x1,x2,x3,x4;
	char *out;
	out = (char *)malloc( ( len * 3/4 ) + 8 );
	for(i=0,j=0;i+3<len;i+=4){
		x1=base64_val(buf[i]);
		x2=base64_val(buf[i+1]);
		x3=base64_val(buf[i+2]);
		x4=base64_val(buf[i+3]);
		out[j++]=(x1<<2) | ((x2 & 0x30)>>4);
		out[j++]=((x2 & 0x0F)<<4) | ((x3 & 0x3C)>>2);
		out[j++]=((x3 & 0x03)<<6) | (x4 & 0x3F);
	}
	if (i<len) {
		x1 = base64_val(buf[i]);
		if (i+1<len)
			x2=base64_val(buf[i+1]);
		else 
			x2=-1;
		if (i+2<len)		
			x3=base64_val(buf[i+2]);
		else
			x3=-1;
		if(i+3<len)	
			x4=base64_val(buf[i+3]);
		else x4=-1;
		if (x2!=-1) {
			out[j++]=(x1<<2) | ((x2 & 0x30)>>4);
			if (x3==-1) {
				out[j++]=((x2 & 0x0F)<<4) | ((x3 & 0x3C)>>2);
				if (x4==-1) {
					out[j++]=((x3 & 0x03)<<6) | (x4 & 0x3F);
				}
			}
		}
			
	}
			
	out[j++] = 0;
	*newlen=j;
	return out;
}

char base64[64]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char *base64_encode_string(const char *buf, unsigned int len, int *newlen)
{
	int i,k;
	int triplets,rest;
	char *out,*ptr;	

	triplets = len/3;
	rest = len%3;
	out = (char *)malloc( ( triplets * 4 ) + 8 );
	
	ptr = out;
	for(i=0;i<triplets*3;i+=3){
		k = (((unsigned char) buf[i])&0xFC)>>2;
		*ptr=base64[k];ptr++;

		k = (((unsigned char) buf[i])&0x03)<<4;
		k |=(((unsigned char) buf[i+1])&0xF0)>>4;
		*ptr=base64[k];ptr++;

		k = (((unsigned char) buf[i+1])&0x0F)<<2;
		k |=(((unsigned char) buf[i+2])&0xC0)>>6;
		*ptr=base64[k];ptr++;

		k = (((unsigned char) buf[i+2])&0x3F);
		*ptr=base64[k];ptr++;
	}
	i=triplets*3;
	switch(rest){
		case 0:
			break;
		case 1:
			k = (((unsigned char) buf[i])&0xFC)>>2;
			*ptr=base64[k];ptr++;

			k = (((unsigned char) buf[i])&0x03)<<4;
			*ptr=base64[k];ptr++;

			*ptr='=';ptr++;

			*ptr='=';ptr++;
			break;
		case 2:
			k = (((unsigned char) buf[i])&0xFC)>>2;
			*ptr=base64[k];ptr++;

			k = (((unsigned char) buf[i])&0x03)<<4;
			k |=(((unsigned char) buf[i+1])&0xF0)>>4;
			*ptr=base64[k];ptr++;

			k = (((unsigned char) buf[i+1])&0x0F)<<2;
			*ptr=base64[k];ptr++;

			*ptr='=';ptr++;
			break;
	}

	*newlen = ptr-out;
	return out;
}



char hexa[16]="0123456789abcdef";
/* calculate AKA request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponseAka(IN const char *pszPassword,
			   IN const char *pszNonce,    /* nonce from server */
			   IN const char *pszCNonce,   /* client nonce */
			   IN const char *pszQop,      /* qop-value: "", "auth", "auth-int" */
			   IN const char *pszMethod,   /* method from the request */
			   IN const char *pszDigestUri,        /* requested URL */
			   IN int version,		/* AKA version */
			   OUT RESHEXAKA2 resp_hex         /* request-digest or response-digest */ ) {
                     	
    char tmp[MAX_HEADER_LEN];
    /* static unsigned int mync = 1; */
    /* int has_opaque = 0; */
	char *nonce64, *nonce;
	int noncelen;
	RAND rnd;
	MAC mac,xmac;
	SQN sqn_he;
	K k;
	RES res;
	CK ck;
	IK ik;
	AK ak;
	int i,j;


	/* Compute the AKA response */
	resp_hex[0]=0;
	sprintf(tmp,"%s",pszNonce);
	nonce64 = tmp;
	nonce = base64_decode_string(nonce64,strlen(tmp),&noncelen);
	if (noncelen<RANDLEN+AUTNLEN) {
	  /* Nonce is too short */
	  goto done;
	}
	memcpy(rnd,nonce,RANDLEN);
	/* memcpy(autn,nonce+RANDLEN,AUTNLEN); */
	memcpy(sqn_he,nonce+RANDLEN,SQNLEN);
	memcpy(mac,nonce+RANDLEN+SQNLEN+AMFLEN,MACLEN);

	j = strlen(pszPassword);
	memcpy(k,pszPassword,j);
	memset(k+j,0,KLEN-j);

	/* compute XMAC */
	f1(k,rnd,sqn_he,amf,xmac);
	if (memcmp(mac,xmac,MACLEN)!=0) {
	  /*
	    createAuthHeaderAKAv1MD5 : MAC != eXpectedMAC
	    -> Server might not know the secret (man-in-the-middle attack?)
	    return 0;
	  */
	}

	/* compute the response and keys */	
	f2345(k,rnd,res,ck,ik,ak);
	/* no check for sqn is performed, so no AUTS synchronization performed */
	
	/* Format data for output in the SIP message */
	for(i=0;i<RESLEN;i++){
		resp_hex[2*i]=hexa[(res[i]&0xF0)>>4];
		resp_hex[2*i+1]=hexa[res[i]&0x0F];
	}
	resp_hex[RESHEXLEN-1]=0;

done:
	switch (version){
		case 1:
			/* AKA v1 */
			/* Nothing to do */
			break;
		case 2:
			/* AKA v2 */
			resp_hex[RESHEXLEN+IKLEN*2+CKLEN*2-1] = 0;
			for(i=0;i<IKLEN;i++){
				resp_hex[RESLEN*2+2*i]=hexa[(ik[i]&0xF0)>>4];
				resp_hex[RESLEN*2+2*i+1]=hexa[ik[i]&0x0F];
			}
			for(i=0;i<CKLEN;i++){
				resp_hex[RESLEN*2+IKLEN*2+2*i]=hexa[(ck[i]&0xF0)>>4];
				resp_hex[RESLEN*2+IKLEN*2+2*i+1]=hexa[ck[i]&0x0F];
			}
			break;
	}
}

int
__eXosip_create_authorization_header (osip_www_authenticate_t *wa,
                                      const char *rquri, const char *username,
                                      const char *passwd, const char *ha1,
                                      osip_authorization_t ** auth,
                                      const char *method,
				      const char *pCNonce,
				      int iNonceCount)
{
  osip_authorization_t *aut;

  char *qop=NULL;
  char *Alg="MD5";
  int version = 0;

  /* make some test */
  if (passwd == NULL)
    return -1;
  if (wa == NULL || wa->auth_type == NULL
      || (wa->realm == NULL) || (wa->nonce == NULL))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "www_authenticate header is not acceptable.\n"));
      return -1;
    }
  if (0 != osip_strcasecmp ("Digest", wa->auth_type))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "Authentication method not supported. (Digest only).\n"));
      return -1;
    }
  /* "MD5" is invalid, but some servers use it. */
  if (wa->algorithm != NULL)
    {
      if (0 == osip_strcasecmp ("MD5", wa->algorithm)
	  || 0 == osip_strcasecmp ("\"MD5\"", wa->algorithm))
	{
	}
      else if (0 == osip_strcasecmp ("AKAv1-MD5", wa->algorithm)
	       || 0 == osip_strcasecmp ("\"AKAv1-MD5\"", wa->algorithm))
	{
	  Alg = "AKAv1-MD5";
	}
      else if (0 == osip_strcasecmp ("AKAv2-MD5", wa->algorithm)
	       || 0 == osip_strcasecmp ("\"AKAv2-MD5\"", wa->algorithm))
	{
	    Alg = "AKAv2-MD5";
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "Authentication method not supported. (MD5, AKAv1-MD5, AKAv2-MD5)\n"));
	  return -1;
	}
    }

  if (0 != osip_authorization_init (&aut))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "allocation with authorization_init failed.\n"));
      return -1;
    }

  /* just copy some feilds from response to new request */
  osip_authorization_set_auth_type (aut, osip_strdup ("Digest"));
  osip_authorization_set_realm (aut,
                                osip_strdup (osip_www_authenticate_get_realm
                                             (wa)));
  osip_authorization_set_nonce (aut,
                                osip_strdup (osip_www_authenticate_get_nonce
                                             (wa)));
  if (osip_www_authenticate_get_opaque (wa) != NULL)
    osip_authorization_set_opaque (aut,
                                   osip_strdup
                                   (osip_www_authenticate_get_opaque (wa)));
  /* copy the username field in new request */
  aut->username = osip_malloc (strlen (username) + 3);
  sprintf (aut->username, "\"%s\"", username);

  {
    char *tmp = osip_malloc (strlen (rquri) + 3);

    sprintf (tmp, "\"%s\"", rquri);
    osip_authorization_set_uri (aut, tmp);
  }

  osip_authorization_set_algorithm (aut, osip_strdup (Alg));

  qop = osip_www_authenticate_get_qop_options (wa);
  if (qop==NULL || qop[0]=='\0' || strlen(qop)<4)
    qop=NULL;


  {
    char *pszNonce =
      osip_strdup_without_quote (osip_www_authenticate_get_nonce (wa));
    char *pszCNonce = NULL;
    const char *pszUser = username;
    char *pszRealm =
      osip_strdup_without_quote (osip_authorization_get_realm (aut));
    const char *pszPass = NULL;
    char *pszAlg = osip_strdup (Alg);
    char *szNonceCount = NULL;
    const char *pszMethod = method;     /* previous_answer->cseq->method; */
    char *pszQop = NULL;
    const char *pszURI = rquri;

    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;
    RESHEXAKA2 Response2;
    const char *pha1 = NULL;

	if (qop!=NULL)
      {
	    /* only accept qop="auth" */
	    pszQop = osip_strdup("auth");

		szNonceCount = osip_malloc(10);
		snprintf(szNonceCount, 9, "%.8i", iNonceCount);

		pszCNonce = osip_strdup (pCNonce);
	
		osip_authorization_set_message_qop (aut, osip_strdup ("auth"));
		osip_authorization_set_nonce_count (aut, osip_strdup (szNonceCount));
	
		{
		  char *tmp = osip_malloc (strlen (pszCNonce) + 3);
		  sprintf (tmp, "\"%s\"", pszCNonce);
		  osip_authorization_set_cnonce (aut, tmp);
		}
      }

    pszPass = passwd;

    /* Depending on which algorithm the response will be calculated, MD5 or AKAv1-MD5 */
    if(0 == strcmp(Alg,"MD5"))
      {
      if (ha1 && ha1[0])
      {
        /* Depending on algorithm=md5 */
        pha1 = ha1;
      }
    else
      {
        DigestCalcHA1 ("MD5", pszUser, pszRealm, pszPass, pszNonce,
                       pszCNonce, HA1);
        pha1 = HA1;
      }

      version = 0;
      DigestCalcResponse ((char *) pha1, pszNonce, szNonceCount, pszCNonce,
			  pszQop, version, pszMethod, pszURI, HA2, Response);
      }
    else
      {
	if(0==strcmp(Alg,"AKAv1-MD5"))
	  version = 1;
	else
	  version = 2;
	
	DigestCalcResponseAka(pszPass, pszNonce,pszCNonce,
			      pszQop,pszMethod,pszURI,version,Response2);
	if (ha1 && ha1[0])
	  {
	    /* Depending on algorithm=md5 */
	    pha1 = ha1;
	  }
	else
	  {
	    DigestCalcHA1 ("MD5", pszUser, pszRealm, Response2, pszNonce,
			   pszCNonce, HA1);
	    pha1 = HA1;
	  }
	
	DigestCalcResponse ((char *) pha1, pszNonce, szNonceCount, pszCNonce,
			    pszQop, version, pszMethod, pszURI, HA2, Response);
	
      }

    OSIP_TRACE (osip_trace
                (__FILE__, __LINE__, OSIP_INFO4, NULL,
                 "Response in authorization |%s|\n", Response));
    {
      char *resp = osip_malloc (35);

      sprintf (resp, "\"%s\"", Response);
      osip_authorization_set_response (aut, resp);
    }
    osip_free (pszAlg);         /* xkd, 2004-5-13 */
    osip_free (pszNonce);
    osip_free (pszCNonce);
    osip_free (pszRealm);
    osip_free (pszQop);
    osip_free (szNonceCount);
  }

  *auth = aut;
  return 0;
}

int
__eXosip_create_proxy_authorization_header (osip_proxy_authenticate_t *wa,
					    const char *rquri,
                                            const char *username,
                                            const char *passwd,
                                            const char *ha1,
                                            osip_proxy_authorization_t **
                                            auth, const char *method,
					    const char *pCNonce,
					    int iNonceCount)
{
  osip_proxy_authorization_t *aut;

  char *qop=NULL;
  char *Alg="MD5";
  int version = 0;

  /* make some test */
  if (passwd == NULL)
    return -1;
  if (wa == NULL || wa->auth_type == NULL
      || (wa->realm == NULL) || (wa->nonce == NULL))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "www_authenticate header is not acceptable.\n"));
      return -1;
    }
  if (0 != osip_strcasecmp ("Digest", wa->auth_type))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "Authentication method not supported. (Digest only).\n"));
      return -1;
    }

  /* "MD5" is invalid, but some servers use it. */
  if (wa->algorithm != NULL)
    {
      if (0 == osip_strcasecmp ("MD5", wa->algorithm)
	  || 0 == osip_strcasecmp ("\"MD5\"", wa->algorithm))
	{
	}
      else if (0 == osip_strcasecmp ("AKAv1-MD5", wa->algorithm)
	       || 0 == osip_strcasecmp ("\"AKAv1-MD5\"", wa->algorithm))
	{
	  Alg = "AKAv1-MD5";
	}
      else if (0 == osip_strcasecmp ("AKAv2-MD5", wa->algorithm)
	       || 0 == osip_strcasecmp ("\"AKAv2-MD5\"", wa->algorithm))
	{
	    Alg = "AKAv2-MD5";
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "Authentication method not supported. (MD5, AKAv1-MD5, AKAv2-MD5)\n"));
	  return -1;
	}
    }

  if (0 != osip_proxy_authorization_init (&aut))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "allocation with authorization_init failed.\n"));
      return -1;
    }

  /* just copy some feilds from response to new request */
  osip_proxy_authorization_set_auth_type (aut, osip_strdup ("Digest"));
  osip_proxy_authorization_set_realm (aut,
                                      osip_strdup
                                      (osip_proxy_authenticate_get_realm (wa)));
  osip_proxy_authorization_set_nonce (aut,
                                      osip_strdup
                                      (osip_proxy_authenticate_get_nonce (wa)));
  if (osip_proxy_authenticate_get_opaque (wa) != NULL)
    osip_proxy_authorization_set_opaque (aut,
                                         osip_strdup
                                         (osip_proxy_authenticate_get_opaque
                                          (wa)));
  /* copy the username field in new request */
  aut->username = osip_malloc (strlen (username) + 3);
  sprintf (aut->username, "\"%s\"", username);

  {
    char *tmp = osip_malloc (strlen (rquri) + 3);

    sprintf (tmp, "\"%s\"", rquri);
    osip_proxy_authorization_set_uri (aut, tmp);
  }
  osip_proxy_authorization_set_algorithm (aut, osip_strdup (Alg));

  qop = osip_www_authenticate_get_qop_options (wa);
  if (qop==NULL || qop[0]=='\0' || strlen(qop)<4)
    qop=NULL;

  {
    char *pszNonce = NULL;
    char *pszCNonce = NULL;
    const char *pszUser = username;
    char *pszRealm =
      osip_strdup_without_quote (osip_proxy_authorization_get_realm (aut));
    const char *pszPass = NULL;
    char *pszAlg = osip_strdup (Alg);
    char *szNonceCount = NULL;
    char *pszMethod = (char *) method;  /* previous_answer->cseq->method; */
    char *pszQop = NULL;
    const char *pszURI = rquri;

    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;
    RESHEXAKA2 Response2;
    const char *pha1 = NULL;

    pszPass = passwd;

    if (osip_www_authenticate_get_nonce (wa) == NULL)
      return -1;
    pszNonce = osip_strdup_without_quote (osip_www_authenticate_get_nonce (wa));

    if (qop!=NULL)
      {
	    /* only accept qop="auth" */
	    pszQop = osip_strdup("auth");
		
		szNonceCount = osip_malloc(10);
		snprintf(szNonceCount, 9, "%.8i", iNonceCount);

		pszCNonce = osip_strdup (pCNonce);

		osip_proxy_authorization_set_message_qop (aut, osip_strdup ("auth"));
		osip_proxy_authorization_set_nonce_count (aut, osip_strdup (szNonceCount));

		{
		  char *tmp = osip_malloc (strlen (pszCNonce) + 3);
		  sprintf (tmp, "\"%s\"", pszCNonce);
		  osip_proxy_authorization_set_cnonce (aut, tmp);
		}
      }

    if(0 == strcmp(Alg,"MD5"))
      {
	if (ha1 && ha1[0])
	  {
	    /* Depending on algorithm=md5 */
	    pha1 = ha1;
	  } else
	    {
	      DigestCalcHA1 ("MD5", pszUser, pszRealm, pszPass, pszNonce,
			     pszCNonce, HA1);
	      pha1 = HA1;
	    }
	version = 0;
	DigestCalcResponse ((char *) pha1, pszNonce, szNonceCount, pszCNonce,
			    pszQop, version, pszMethod, pszURI, HA2, Response);
      }
    else
      {
	if(0==strcmp(Alg,"AKAv1-MD5"))
	  version = 1;
	else
	  version = 2;

	DigestCalcResponseAka(pszPass, pszNonce,pszCNonce,pszQop,pszMethod,pszURI,version,Response2);
	if (ha1 && ha1[0])
	  {
	    /* Depending on algorithm=md5 */
	    pha1 = ha1;
	  }
	else
	  {
	    DigestCalcHA1 ("MD5", pszUser, pszRealm, Response2, pszNonce,
			   pszCNonce, HA1);
	    pha1 = HA1;
	  }
	
	DigestCalcResponse ((char *) pha1, pszNonce, szNonceCount, pszCNonce,
			    pszQop, version, pszMethod, pszURI, HA2, Response);
      }

    OSIP_TRACE (osip_trace
                (__FILE__, __LINE__, OSIP_INFO4, NULL,
                 "Response in proxy_authorization |%s|\n", Response));
    {
      char *resp = osip_malloc (35);

      sprintf (resp, "\"%s\"", Response);
      osip_proxy_authorization_set_response (aut, resp);
    }
    osip_free (pszAlg);         /* xkd, 2004-5-13 */
    osip_free (pszNonce);
    osip_free (pszCNonce);
    osip_free (pszRealm);
    osip_free (pszQop);
    osip_free (szNonceCount);
  }

  *auth = aut;
  return 0;
}

int _eXosip_store_nonce(const char *call_id, osip_proxy_authenticate_t *wa, int answer_code)
{
	struct eXosip_http_auth *http_auth;
	int pos;

	/* update entries with same call_id */
	for (pos=0;pos<MAX_EXOSIP_HTTP_AUTH;pos++)
	{
		http_auth = &eXosip.http_auths[pos];
		if (http_auth->pszCallId[0]=='\0')
			continue;
		if (osip_strcasecmp(http_auth->pszCallId, call_id)==0
			&& osip_strcasecmp(http_auth->wa->realm, wa->realm)==0)
		{
			osip_proxy_authenticate_free(http_auth->wa);
			http_auth->wa=NULL;
			osip_proxy_authenticate_clone(wa, &(http_auth->wa));
			http_auth->iNonceCount = 1;
			if (http_auth->wa==NULL)
				memset(http_auth, 0, sizeof(struct eXosip_http_auth));
			return 0;
		}
	}

	/* not found */
	for (pos=0;pos<MAX_EXOSIP_HTTP_AUTH;pos++)
	{
		http_auth = &eXosip.http_auths[pos];
		if (http_auth->pszCallId[0]=='\0')
		{
			snprintf(http_auth->pszCallId, sizeof(http_auth->pszCallId),
				call_id);
			snprintf(http_auth->pszCNonce, sizeof(http_auth->pszCNonce),
				"0a4f113b");
			http_auth->iNonceCount = 1;
			osip_proxy_authenticate_clone(wa, &(http_auth->wa));
			http_auth->answer_code = answer_code;
			if (http_auth->wa==NULL)
				memset(http_auth, 0, sizeof(struct eXosip_http_auth));
			return 0;
		}
	}

	OSIP_TRACE (osip_trace
                (__FILE__, __LINE__, OSIP_ERROR, NULL,
                 "Compile with higher MAX_EXOSIP_HTTP_AUTH value (current=%i)\n", MAX_EXOSIP_HTTP_AUTH));
	return -1;
}

int _eXosip_delete_nonce(const char *call_id)
{
	struct eXosip_http_auth *http_auth;
	int pos;

	/* update entries with same call_id */
	for (pos=0;pos<MAX_EXOSIP_HTTP_AUTH;pos++)
	{
		http_auth = &eXosip.http_auths[pos];
		if (http_auth->pszCallId[0]=='\0')
			continue;
		if (osip_strcasecmp(http_auth->pszCallId, call_id)==0)
		{
			osip_proxy_authenticate_free(http_auth->wa);
			memset(http_auth, 0, sizeof(struct eXosip_http_auth));
			return 0;
		}
	}
	return -1;
}
