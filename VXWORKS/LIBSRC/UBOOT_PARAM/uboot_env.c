/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
*        \file  uboot_env.c
*
*      \author  ts
*        $Date: 2014/02/20 11:45:13 $
*    $Revision: 1.2 $
*
*        \brief  helper functions to access U-Boot variables on EM10A
*
*     Switches: INCLUDE_RTP.
*/
/*-------------------------------[ History ]---------------------------------
*
* $Log: uboot_env.c,v $
* Revision 1.2  2014/02/20 11:45:13  ts
* R: several warnings about signed/unsigned pointer comparison in gnu compiler
* M: use char * throughout the U-Boot functions
*
* Revision 1.1  2014/01/28 13:04:23  ts
* Initial Revision
*
*
*---------------------------------------------------------------------------
* (c) Copyright 2014 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
****************************************************************************/

static UINT32 G_crctab[256];

LOCAL unsigned int crc32(UINT32 crc, char *buf, UINT32 len)
{
	static int bTableDone = 0; /* calculate table just once! */
	UINT32 remainder;
	UINT8 octet;
	int i, j;
	char *p, *q;

	/* This check is not thread safe; there is no mutex. */
	if (bTableDone == 0) {
	  /* Calculate CRC table. */
	  for (i = 0; i < 256; i++) {
	    remainder = i;  /* remainder from polynomial division */
	    for (j = 0; j < 8; j++) {
	      if (remainder & 1) {
		remainder >>= 1;
					remainder ^= IEEE_CRC32_POLY;
	      } else
		remainder >>= 1;
	    }
	    G_crctab[i] = remainder;
	  }
	  bTableDone = 1;
	}
 
	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ G_crctab[(crc & 0xff) ^ octet];
	}	
	return ~crc;
}

LOCAL char *match_env_pair (char * s1, char * s2)
{

  while (*s1 == *s2++)
    if (*s1++ == '=')
      return (s2);
  if (*s1 == '\0' && *(s2 - 1) == '=')
    return (s2);
  return (NULL);
}


/*******************************************************************************
* uboot_env_get - search for a U-Boot variable and return pointer to it
*
* RETURNS: if name given: pointer to matching env. pair ("name=value" or NULL 
*                         if environment variable <name> doesnt exist
*
*          if name is '\0' ("") pointer to start of env.variables. If verbosity
*          is >0 then all variables are dumped on console.
*
* ERRNO: N/A
*/
LOCAL char *uboot_env_get ( char *envname, char *envdatastart, int verbosity)
{
  char *env, *nxt;
  char *name = envname;
  char *val = NULL;
  char *envstrings=envdatastart + UBOOT_ENV_DATA_OFFS;

	if ( *envname == '\0' ) { /* return start of env. or dump all */
		for (env = envstrings; *env; env = nxt + 1) {
		  for (nxt = env; *nxt; ++nxt) {
		    if ( nxt >= (envstrings + UBOOT_ENV_DATA_SIZE )) {
		      fprintf (stderr, "*** %s: missing \0\0 within ENV_SIZE \n");
		      return(NULL);
		    }
		  }
		  if (verbosity)
		    printf ("%s\n", env);
		}
		return(envstrings);
	}	
	for (env = envstrings; *env; env = nxt + 1) {
		for (nxt = env; *nxt; ++nxt) {
			if ( nxt >= (envstrings + UBOOT_ENV_DATA_SIZE )) {
			  fprintf (stderr, "*** environment section not terminated (2x'\0')\n");
			  return(NULL);
			}	
		}
		val = match_env_pair (name, env);
		if (val) {
		  if (verbosity)
		    printf("%s=%s\n", name, val);
		  break;
		};
	};
	if (!val) {
	  fprintf (stderr, "*** variable \"%s\" not defined\n", name);
	  return(NULL);
	}

	return(val);
}


/*******************************************************************************
* uboot_env_set - set a U-Boot variable or alter its current value.
*                 
*                 If envvalue="" then the variable is deleted
* ERRNO: N/A
*/

LOCAL STATUS uboot_env_set(char *envname, char *envvalue, char *envdatastart, int verbosity )
{
  int len=0, crc=0;
  char *env=NULL, *nxt=NULL, *val=NULL;
  char *oldval = NULL;
  char *name=NULL;
  unsigned int envCount=0;
  char *envstrings=(char*)envdatastart + UBOOT_ENV_DATA_OFFS;

  if ( *envname == '\0' ) {
    fprintf (stderr, "*** %s: invalid env.variable (NULL)\n", __FUNCTION__);
    return (ERROR);
  }

  name = envname;

  /* search if variable already exists */
  for (nxt = env = envstrings; *env; env = nxt + 1) {
    for (nxt = env; *nxt; ++nxt) {
      if ( nxt >= (envstrings + UBOOT_ENV_DATA_SIZE )) 
	{
	  fprintf (stderr, "*** %s: missing \0\0 within ENV_SIZE \n");
	  return (ERROR);
	}
    }
    if ((oldval = match_env_pair (name, env)) != NULL)
      break;
  }

  if (oldval) {
    /* protect eth Addr and serial# - set by U-Boot */
    if ((strcmp (name, "ethaddr") == 0) || (strcmp (name, "serial#") == 0)) {
      fprintf (stderr, "*** \"%s\" is protected\n ", name);
      return (ERROR);
    }

    if (*++nxt == '\0') {
      *env = '\0';
    } else {
      for (;;) {
	*env = *nxt++;
	if ((*env == '\0') && (*nxt == '\0'))
	  break;
	++env;
      }
    }
    *++env = '\0';
  }

  /* Delete only ? */
  if ( *envvalue=='\0') {
      if (verbosity)
         printf("deleting '%s'\n", name );
    goto done;
  }

  /*
   * Append new definition at the end
   */
  for (env = envstrings; *env || *(env + 1); ++env)
    envCount++;

  if (env > envstrings) {
    envCount++;
    ++env;
  }

  /* prevent overflows  */
  len = strlen (name) + 2 + strlen (envvalue) + 1;
  if ( (envCount+len) >= UBOOT_ENV_DATA_SIZE  ) {
    fprintf (stderr, "*** environment overflow, \"%s\" deleted\n",name);
    return (ERROR);
  }

  while ((*env = *name++) != '\0')
    env++;

  val = envvalue;
  *env = '=';
  while ((*++env = *val++) != '\0');
	
  /* end is marked with double '\0' */
  *++env = '\0';

 done:
  /* Update CRC */
  if (verbosity)
   printf("update CRC...\n");
  crc = crc32 (0, envstrings, UBOOT_ENV_DATA_SIZE);

  /* store new CRC at beginning of env.section */
  *(unsigned int*)envdatastart = crc;

  if (verbosity)
    printf("CRC = 0x%08x\n", crc);

  /* write environment back to flash */

  return (OK);

}
