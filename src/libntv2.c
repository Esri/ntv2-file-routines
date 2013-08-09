/* ------------------------------------------------------------------------- */
/* Copyright 2013 Esri                                                       */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License");           */
/* you may not use this file except in compliance with the License.          */
/* You may obtain a copy of the License at                                   */
/*                                                                           */
/*     http://www.apache.org/licenses/LICENSE-2.0                            */
/*                                                                           */
/* Unless required by applicable law or agreed to in writing, software       */
/* distributed under the License is distributed on an "AS IS" BASIS,         */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/* See the License for the specific language governing permissions and       */
/* limitations under the License.                                            */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Routines to copy/dump/validate NTv2 files and transform points            */
/* ------------------------------------------------------------------------- */

#ifdef _WIN32
#  pragma warning (disable: 4996) /* same as "-D _CRT_SECURE_NO_WARNINGS" */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <locale.h>

#include "libntv2.h"
#include "libntv2.i"

/* ------------------------------------------------------------------------- */
/* internal defines and macros                                               */
/* ------------------------------------------------------------------------- */

#define NTV2_SIZE_INT        4     /* size of an int    */
#define NTV2_SIZE_FLT        4     /* size of a  float  */
#define NTV2_SIZE_DBL        8     /* size of a  double */

#define NTV2_NO_PARENT_NAME  "NONE    "
#define NTV2_ALL_BLANKS      "        "
#define NTV2_ALL_ZEROS       "\0\0\0\0\0\0\0\0"
#define NTV2_END_NAME        "END     "

#define NTV2_SWAPI(x,n)      if ( hdr->swap_data ) ntv2_swap_int(x, n)
#define NTV2_SWAPF(x,n)      if ( hdr->swap_data ) ntv2_swap_flt(x, n)
#define NTV2_SWAPD(x,n)      if ( hdr->swap_data ) ntv2_swap_dbl(x, n)

#define NTV2_SHOW_PATH()     if ( rc == NTV2_ERR_OK ) \
                                fprintf(fp, "%s:\n", hdr->path)

#define NTV2_OFFSET_OF(t,m)       ((int)( (size_t)&(((t *)0)->m) ))
#define NTV2_UNUSED_PARAMETER(p)  (void)(p)

/* ------------------------------------------------------------------------- */
/* Floating-point comparison macros                                          */
/* ------------------------------------------------------------------------- */

#define NTV2_EPS_48          3.55271367880050092935562e-15 /* 2^(-48) */
#define NTV2_EPS_49          1.77635683940025046467781e-15 /* 2^(-49) */
#define NTV2_EPS_50          8.88178419700125232338905e-16 /* 2^(-50) */
#define NTV2_EPS_51          4.44089209850062616169453e-16 /* 2^(-51) */
#define NTV2_EPS_52          2.22044604925031308084726e-16 /* 2^(-52) */
#define NTV2_EPS_53          1.11022302462515654042363e-16 /* 2^(-53) */

#define NTV2_EPS             NTV2_EPS_50   /* best compromise between */
                                           /* speed and accuracy      */

#define NTV2_ABS(a)          ( ((a) < 0) ? -(a) : (a) )

#define NTV2_EQ_EPS(a,b,e)   ( ((a) == (b)) || NTV2_ABS((a)-(b)) <= \
                               (e)*(1+(NTV2_ABS(a)+NTV2_ABS(b))/2) )
#define NTV2_EQ(a,b)         ( NTV2_EQ_EPS(a, b, NTV2_EPS) )

#define NTV2_NE_EPS(a,b,e)   ( !NTV2_EQ_EPS(a, b, e) )
#define NTV2_NE(a,b)         ( !NTV2_EQ    (a, b   ) )

#define NTV2_LE_EPS(a,b,e)   ( ((a) < (b))  || NTV2_EQ_EPS(a,b,e) )
#define NTV2_LE(a,b)         ( NTV2_LE_EPS(a, b, NTV2_EPS) )

#define NTV2_LT_EPS(a,b,e)   ( !NTV2_GE_EPS(a, b, e) )
#define NTV2_LT(a,b)         ( !NTV2_GE    (a, b   ) )

#define NTV2_GE_EPS(a,b,e)   ( ((a) > (b))  || NTV2_EQ_EPS(a,b,e) )
#define NTV2_GE(a,b)         ( NTV2_GE_EPS(a, b, NTV2_EPS) )

#define NTV2_GT_EPS(a,b,e)   ( !NTV2_LE_EPS(a, b, e) )
#define NTV2_GT(a,b)         ( !NTV2_LE    (a, b   ) )

#define NTV2_ZERO_EPS(a,e)   ( ((a) == 0.0) || NTV2_ABS(a) <= (e) )
#define NTV2_ZERO(a)         ( NTV2_ZERO_EPS(a, NTV2_EPS) )

/* ------------------------------------------------------------------------- */
/* NTv2 error messages                                                       */
/* ------------------------------------------------------------------------- */

typedef struct ntv2_errs NTV2_ERRS;
struct ntv2_errs
{
   int         err_num;
   const char *err_msg;
};

static const NTV2_ERRS ntv2_errlist[] =
{
   { NTV2_ERR_OK,                      "No error"               },

   /* generic errors */

   { NTV2_ERR_NO_MEMORY,               "No memory"              },
   { NTV2_ERR_IOERR,                   "I/O error"              },
   { NTV2_ERR_NULL_HDR,                "NULL header"            },

   /* warnings */

   { NTV2_ERR_FILE_NEEDS_FIXING,       "File needs fixing"      },

   /* read errors that may be ignored */

   { NTV2_ERR_INVALID_LAT_MIN_MAX,     "LAT_MIN >= LAT_MAX"     },
   { NTV2_ERR_INVALID_LON_MIN_MAX,     "LON_MIN >= LON_MAX"     },
   { NTV2_ERR_INVALID_LAT_MIN,         "Invalid LAT_MIN"        },
   { NTV2_ERR_INVALID_LAT_MAX,         "Invalid LAT_MAX"        },
   { NTV2_ERR_INVALID_LAT_INC,         "Invalid LAT_INC"        },
   { NTV2_ERR_INVALID_LON_MIN,         "Invalid LON_MIN"        },
   { NTV2_ERR_INVALID_LON_MAX,         "Invalid LON_MAX"        },
   { NTV2_ERR_INVALID_LON_INC,         "Invalid LON_INC"        },

   /* unrecoverable errors */

   { NTV2_ERR_INVALID_NUM_OREC,        "Invalid NUM_OREC"       },
   { NTV2_ERR_INVALID_NUM_SREC,        "Invalid NUM_SREC"       },
   { NTV2_ERR_INVALID_NUM_FILE,        "Invalid NUM_FILE"       },
   { NTV2_ERR_INVALID_GS_TYPE,         "Invalid GS_TYPE"        },
   { NTV2_ERR_INVALID_GS_COUNT,        "Invalid GS_COUNT"       },
   { NTV2_ERR_INVALID_DELTA,           "Invalid lat/lon delta"  },
   { NTV2_ERR_INVALID_PARENT_NAME,     "Invalid parent name"    },
   { NTV2_ERR_PARENT_NOT_FOUND,        "Parent not found"       },
   { NTV2_ERR_NO_TOP_LEVEL_PARENT,     "No top-level parent"    },
   { NTV2_ERR_PARENT_LOOP,             "Parent loop"            },
   { NTV2_ERR_PARENT_OVERLAP,          "Parent overlap"         },
   { NTV2_ERR_SUBFILE_OVERLAP,         "Subfile overlap"        },
   { NTV2_ERR_INVALID_EXTENT,          "Invalid extent"         },
   { NTV2_ERR_HDRS_NOT_READ,           "Headers not read yet"   },
   { NTV2_ERR_UNKNOWN_FILE_TYPE,       "Unknown file type"      },
   { NTV2_ERR_FILE_NOT_BINARY,         "File not binary"        },
   { NTV2_ERR_FILE_NOT_ASCII,          "File not ascii"         },
   { NTV2_ERR_NULL_PATH,               "NULL pathname"          },
   { NTV2_ERR_ORIG_DATA_NOT_KEPT,      "Original data not kept" },
   { NTV2_ERR_DATA_NOT_READ,           "Data not read yet"      },
   { NTV2_ERR_CANNOT_OPEN_FILE,        "Cannot open file"       },
   { NTV2_ERR_UNEXPECTED_EOF,          "Unexpected EOF"         },
   { NTV2_ERR_INVALID_LINE,            "Invalid line"           },

   { -1, NULL }
};

const char * ntv2_errmsg(int err_num, char msg_buf[])
{
   const NTV2_ERRS *e;

   for (e = ntv2_errlist; e->err_num >= 0; e++)
   {
      if ( e->err_num == err_num )
      {
         if ( msg_buf == NTV2_NULL )
         {
            return e->err_msg;
         }
         else
         {
            strcpy(msg_buf, e->err_msg);
            return msg_buf;
         }
      }
   }

   if ( msg_buf == NTV2_NULL )
   {
      return "?";
   }
   else
   {
      sprintf(msg_buf, "%d", err_num);
      return msg_buf;
   }
}

/* ------------------------------------------------------------------------- */
/* String routines                                                           */
/* ------------------------------------------------------------------------- */

static char * ntv2_strip(char *str)
{
   char * s;
   char * e = NTV2_NULL;

   for (; isspace(*str); str++) ;

   for (s = str; *s; s++)
   {
      if ( !isspace(*s) )
         e = s;
   }

   if ( e != NTV2_NULL )
      e[1] = 0;
   else
      *str = 0;

   return str;
}

static char * ntv2_strip_buf(char *str)
{
   char * s = ntv2_strip(str);

   if ( s > str )
      strcpy(str, s);
   return str;
}

static int ntv2_strcmp_i(const char *s1, const char *s2)
{
   for (;;)
   {
      int c1 = toupper(*(const unsigned char *)s1);
      int c2 = toupper(*(const unsigned char *)s2);
      int rc;

      rc = (c1 - c2);
      if ( rc != 0 || c1 == 0 || c2 == 0 )
         return (rc);

      s1++;
      s2++;
   }
}

/* Like strncpy(), but guarantees a null-terminated string
 * and returns number of chars copied.
 */
static int ntv2_strncpy(char *buf, const char *str, int n)
{
   char * b = buf;
   const char *s;

   for (s = str; --n && *s; s++)
      *b++ = *s;
   *b = 0;

   return (int)(b - buf);
}

/* ------------------------------------------------------------------------- */
/* String tokenizing                                                         */
/* ------------------------------------------------------------------------- */

#define NTV2_TOKENS_MAX     64
#define NTV2_TOKENS_BUFLEN  256

typedef struct ntv2_token NTV2_TOKEN;
struct ntv2_token
{
   char   buf  [NTV2_TOKENS_BUFLEN];
   char * toks [NTV2_TOKENS_MAX];
   int    num;
};

/* tokenize a buffer
 *
 * This routine splits a line into "tokens", based on the delimiter
 * string.  Each token will have all leading/trailing whitespace
 * and embedding quotes removed from it.
 *
 * If the delimiter string is NULL or empty, then tokenizing will just
 * depend on whitespace.  In this case, multiple whitespace chars will
 * count as a single delimiter.
 *
 * Up to "maxtoks" tokens will be processed.  Any left-over chars will
 * be left in the last token. If there are less tokens than requested,
 * then the remaining token entries will point to an empty string.
 *
 * Return value is the number of tokens found.
 *
 * Note that this routine does not yet support "escaped" chars (\x).
 */
static int ntv2_str_tokenize(
   NTV2_TOKEN *ptoks,
   const char *line,
   const char *delims,
   int         maxtoks)
{
   char * p;
   int ntoks = 1;
   int i;

   /* sanity checks */

   if ( ptoks == NTV2_NULL )
      return 0;

   if ( maxtoks <= 0 || ntoks > NTV2_TOKENS_MAX )
      maxtoks = NTV2_TOKENS_MAX;

   if ( line   == NTV2_NULL )  line   = "";
   if ( delims == NTV2_NULL )  delims = "";

   /* copy the line, removing any leading/trailing whitespace */

   ntv2_strncpy(ptoks->buf, line, sizeof(ptoks->buf));
   ntv2_strip_buf(ptoks->buf);
   ptoks->num = 0;

   if ( *ptoks->buf == 0 )
      return 0;

   /* now do the tokenizing */

   p = ptoks->buf;
   ptoks->toks[0] = p;

   while (ntoks < maxtoks)
   {
      char * s;
      NTV2_BOOL quotes = FALSE;

      for (s = p; *s; s++)
      {
         if ( quotes )
         {
            if ( *s == '"' )
               quotes = FALSE;
            continue;
         }
         else if ( *s == '"' )
         {
            quotes = TRUE;
            continue;
         }

         if ( *delims == 0 )
         {
            if ( !quotes && isspace(*s) )
               break;
         }
         else
         {
            if ( !quotes && *s == *delims )
               break;
         }
      }
      if ( *s == 0 )
         break;

      *s++ = 0;
      ntv2_strip(ptoks->toks[ntoks - 1]);

      for (p = s; isspace(*p); p++) ;
      ptoks->toks[ntoks++] = p;
   }

   /* now strip any enbedding quotes from each token */

   for (i = 0; i < ntoks; i++)
   {
      char * str = ptoks->toks[i];
      int len    = (int)strlen(str);
      int c      = *str;

      if ( (c == '\'' || c == '"') && str[len-1] == c )
      {
         str[len-1] = 0;
         ptoks->toks[i] = ++str;
         ntv2_strip_buf(str);
      }
   }

   /* set rest of requested tokens to empty string */
   for (i = ntoks; i < maxtoks; i++)
      ptoks->toks[i] = "";

   ptoks->num = ntoks;
   return ntoks;
}

/* ------------------------------------------------------------------------- */
/* Byte swapping routines                                                    */
/* ------------------------------------------------------------------------- */

static NTV2_BOOL ntv2_is_big_endian(void)
{
   int one = 1;

   return ( *((char *)&one) == 0 );
}

static NTV2_BOOL ntv2_is_ltl_endian(void)
{
   return ! ntv2_is_big_endian();
}

#define SWAP4(a) \
   ( (((a) & 0x000000ff) << 24) | \
     (((a) & 0x0000ff00) <<  8) | \
     (((a) & 0x00ff0000) >>  8) | \
     (((a) & 0xff000000) >> 24) )

static void ntv2_swap_int(int in[], int ntimes)
{
   int i;

   for (i = 0; i < ntimes; i++)
      in[i] = SWAP4((unsigned int)in[i]);
}

static void ntv2_swap_flt(float in[], int ntimes)
{
  ntv2_swap_int((int *)in, ntimes);
}

static void ntv2_swap_dbl(double in[], int ntimes)
{
   int  i;
   int *p_int, tmpint;

   for (i = 0; i < ntimes; i++)
   {
      p_int = (int *)(&in[i]);
      ntv2_swap_int(p_int, 2);

      tmpint   = p_int[0];
      p_int[0] = p_int[1];
      p_int[1] = tmpint;
   }
}

/* ------------------------------------------------------------------------- */
/* generic utility routines                                                  */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * copy a string and pad with blanks
 */
static void ntv2_strcpy_pad(char *tgt, const char *src)
{
   int i = 0;

   if ( src != NULL )
   {
      for (; i < NTV2_NAME_LEN; i++)
      {
         if ( src[i] == 0 )
            break;
         tgt[i] = src[i];
      }
   }

   for (; i < NTV2_NAME_LEN; i++)
   {
      tgt[i] = ' ';
   }
}

/*------------------------------------------------------------------------
 * convert a string to a double
 *
 * Note: Any '.' in the string is first converted to the localized value.
 *       This means the string cannot be const, as it may be modified.
 */
static double ntv2_atod(char *s)
{
   char   dec_pnt = localeconv()->decimal_point[0];
   double d = 0.0;

   if ( s != NULL && *s != 0 )
   {
      char *p = strchr(s, '.');
      if ( p != NULL )
         *p = dec_pnt;
      d = atof(s);
   }

   return d;
}

/*------------------------------------------------------------------------
 * format a floating point number to a string
 *
 * Numbers are displayed left-adjusted with a max of 8 decimal digits,
 * with extra trailing zeros removed.
 *
 * Also, any localized decimal point character is converted to a '.'.
 */
static char * ntv2_dtoa(char *buf, double dbl)
{
   char dec_pnt = localeconv()->decimal_point[0];
   char *s;
   char *d;
   char *p = NULL;

   sprintf(buf, "%.8f", dbl);
   s = strchr(buf, dec_pnt);
   if ( s != NULL )
   {
      *s = '.';

      s += 2;
      for (d = s; *d; d++)
      {
         if ( *d != '0' )
            p = d;
      }
      if ( p == NULL )
         *s = 0;
      else
         p[1] = 0;
   }

   return buf;
}

/*------------------------------------------------------------------------
 * Read in a line from an ascii stream.
 *
 * This will read in a line, strip all leading and trailing whitespace,
 * and discard any blank lines and comments (anything following a #).
 *
 * Returns NULL at EOF.
 */
static char * ntv2_read_line(FILE *fp, char *buf, size_t buflen)
{
   char * bufp;

   for (;;)
   {
      char *p;

      bufp = fgets(buf, (int)buflen, fp);
      if ( bufp == NULL )
         break;

      p = strchr(bufp, '#');
      if ( p != NULL )
         *p = 0;

      bufp = ntv2_strip(bufp);
      if ( *bufp != 0 )
         break;
   }

   return bufp;
}

/*------------------------------------------------------------------------
 * Read in a tokenized line
 *
 * Returns number of tokens or -1 at EOF
 */
static int ntv2_read_toks(FILE *fp, NTV2_TOKEN *ptok, int maxtoks)
{
   char  buf[NTV2_TOKENS_BUFLEN];
   char *bufp;

   bufp = ntv2_read_line(fp, buf, sizeof(buf));
   if ( bufp == NULL )
      return -1;

   return ntv2_str_tokenize(ptok, bufp, NULL, maxtoks);
}

/*------------------------------------------------------------------------
 * Check if an extent is empty.
 */
static NTV2_BOOL ntv2_extent_is_empty(const NTV2_EXTENT *extent)
{
   if ( extent == NTV2_NULL )
      return TRUE;

   if ( NTV2_EQ(extent->wlon, extent->elon) ||
        NTV2_EQ(extent->slat, extent->nlat) )
   {
      return TRUE;
   }

   return FALSE;
}

/* ------------------------------------------------------------------------- */
/* NTv2 validation routines                                                  */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Cleanup a name string.
 *
 * All strings in NTv2 headers are supposed to be
 * 8-byte fields that are blank-padded to 8 bytes,
 * with no null at the end.
 *
 * However, we have encountered many files in which this is
 * not the case.  We have seen trailing nulls, null/garbage,
 * and even new-line/null/garbage.  This occurs in both
 * the field names and in the value strings.
 * This can cause problems, especially in identifying which
 * records are parents and which are children.
 *
 * We have also seen lower-case (and mixed-case) chars in the field
 * names, although the spec clearly shows all field names should
 * be upper-case.
 *
 * So, our solution is to "cleanup" these fields.  This also
 * gives us the added benefit that any file that we write will
 * always be in "standard" format.
 *
 * Note we always cleanup the record-name and the parent-name in the
 * ntv2-rec struct, but we only cleanup the strings in the overview
 * and subfile records when we write out a new file.
 */
static int ntv2_cleanup_str(
   NTV2_HDR *hdr,
   char     *tgt,
   char     *str,
   NTV2_BOOL is_user_data)
{
   NTV2_BOOL at_end = FALSE;
   int fixed = 0;
   int i;

   for (i = 0; i < NTV2_NAME_LEN; i++)
   {
      int c = ((unsigned char *)str)[i];

      if ( at_end )
      {
         tgt[i] = ' ';
         continue;
      }

      if ( c < ' ' || c > 0x7f )
      {
         tgt[i] = ' ';
         at_end      = TRUE;
         fixed |= NTV2_FIX_UNPRINTABLE_CHAR;
         continue;
      }

      if ( c == ' ' ||
           c == '_' ||
           isupper(c) )
      {
         tgt[i] = (char)c;
         continue;
      }

      if ( !is_user_data && islower(c) )
      {
         tgt[i] = (char)toupper(c);
         fixed |= NTV2_FIX_NAME_LOWERCASE;
         continue;
      }

      if ( !is_user_data )
      {
         tgt[i] = ' ';
         at_end      = TRUE;
         fixed |= NTV2_FIX_NAME_NOT_ALPHA;
      }
      else
      {
         tgt[i] = (char)c;
      }
   }

   hdr->fixed |= fixed;
   return fixed;
}

/*------------------------------------------------------------------------
 * Create a printable string from a name for use in error messages.
 */
static char * ntv2_printable(
   char       *buf,
   const char *str)
{
   char *b = buf;
   int i;

   for (i = 0; i < NTV2_NAME_LEN; i++)
   {
      if ( str[i] < ' ' )
      {
         *b++ = '^';
         *b++ = (str[i] + '@');
      }
      else
      {
         *b++ = str[i];
      }
   }
   *b = 0;

   return buf;
}

/*------------------------------------------------------------------------
 * Validate an overview record.
 *
 * Interestingly, even though the NTv2 spec was developed by the
 * Canadian government, all Canadian files use the keywords
 * "DATUM_F" and "DATUM_T" instead of "SYSTEM_F" and "SYSTEM_T".
 * The Brazilian files also do this.
 */
static int ntv2_validate_ov_field(
   NTV2_HDR   *hdr,
   FILE       *fp,
   char       *str,
   const char *name,
   int         rc)
{
   char temp[NTV2_NAME_LEN+1];

   if (name == NTV2_NULL )
   {
      ntv2_cleanup_str(hdr, temp, str, TRUE);
      temp[NTV2_NAME_LEN] = 0;
      name = temp;
   }

   if ( strncmp(str, name, NTV2_NAME_LEN) != 0 )
   {
      if ( fp != NTV2_NULL )
      {
         char buf[24];

         if ( rc == NTV2_ERR_OK )
         {
            NTV2_SHOW_PATH();
            rc = NTV2_ERR_FILE_NEEDS_FIXING;
         }

         fprintf(fp, "  overview record: \"%s\" should be \"%s\"\n",
            ntv2_printable(buf, str), name);
      }

      strncpy(str, name, NTV2_NAME_LEN);
   }

   return rc;
}

static int ntv2_validate_ov(
   NTV2_HDR *hdr,
   FILE     *fp,
   int       rc)
{
   if ( hdr->overview != NTV2_NULL )
   {
      NTV2_FILE_OV *ov = hdr->overview;

      /* field names */
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_num_orec,  "NUM_OREC", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_num_srec,  "NUM_SREC", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_num_file,  "NUM_FILE", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_gs_type,   "GS_TYPE ", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_version,   "VERSION ", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_system_f,  "SYSTEM_F", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_system_t,  "SYSTEM_T", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_major_f,   "MAJOR_F ", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_major_t,   "MAJOR_T ", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_minor_f,   "MINOR_F ", rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->n_minor_t,   "MINOR_T ", rc);

      /* user values */
      rc = ntv2_validate_ov_field(hdr, fp, ov->s_gs_type,   NTV2_NULL,  rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->s_version,   NTV2_NULL,  rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->s_system_f,  NTV2_NULL,  rc);
      rc = ntv2_validate_ov_field(hdr, fp, ov->s_system_t,  NTV2_NULL,  rc);
   }

   return rc;
}

/*------------------------------------------------------------------------
 * Validate a subfile record.
 */
static int ntv2_validate_sf_field(
   NTV2_HDR   *hdr,
   FILE       *fp,
   int         n,
   char       *str,
   const char *name,
   int         rc)
{
   char temp[NTV2_NAME_LEN+1];

   if (name == NTV2_NULL )
   {
      ntv2_cleanup_str(hdr, temp, str, TRUE);
      name = temp;
      temp[NTV2_NAME_LEN] = 0;
   }

   if ( strncmp(str, name, NTV2_NAME_LEN) != 0 )
   {
      if ( fp != NTV2_NULL )
      {
         char buf[24];

         if ( rc == NTV2_ERR_OK )
         {
            NTV2_SHOW_PATH();
            rc = NTV2_ERR_FILE_NEEDS_FIXING;
         }

         fprintf(fp, "  subfile rec %3d: \"%s\" should be \"%s\"\n",
            n, ntv2_printable(buf, str), name);
      }

      strncpy(str, name, NTV2_NAME_LEN);
   }

   return rc;
}

static int ntv2_validate_sf(
   NTV2_HDR *hdr,
   FILE     *fp,
   int       n,
   int       rc)
{
   if ( hdr->subfiles != NTV2_NULL )
   {
      NTV2_FILE_SF *sf= hdr->subfiles + n;

      /* field names */
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_sub_name, "SUB_NAME", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_parent,   "PARENT  ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_created,  "CREATED ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_updated,  "UPDATED ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_s_lat,    "S_LAT   ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_n_lat,    "N_LAT   ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_e_lon,    "E_LONG  ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_w_lon,    "W_LONG  ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_lat_inc,  "LAT_INC ", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_lon_inc,  "LONG_INC", rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->n_gs_count, "GS_COUNT", rc);

      /* user values */
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->s_sub_name, NTV2_NULL,  rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->s_parent,   NTV2_NULL,  rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->s_created,  NTV2_NULL,  rc);
      rc = ntv2_validate_sf_field(hdr, fp, n, sf->s_updated,  NTV2_NULL,  rc);
   }

   return rc;
}

/*------------------------------------------------------------------------
 * Check if two sub-file records overlap.
 */
static NTV2_BOOL ntv2_overlap(
   NTV2_REC *r1,
   NTV2_REC *r2)
{
   if ( NTV2_GE(r1->lat_min, r2->lat_max) ||
        NTV2_LE(r1->lat_max, r2->lat_min) ||
        NTV2_GE(r1->lon_min, r2->lon_max) ||
        NTV2_LE(r1->lon_max, r2->lon_min) )
   {
      return FALSE;
   }

   return TRUE;
}

/*------------------------------------------------------------------------
 * Validate a subfile against its parent.
 */
static int ntv2_validate_subfile(
   NTV2_HDR *hdr,
   NTV2_REC *par,
   NTV2_REC *sub,
   FILE     *fp,
   int       rc)
{
   double t;
   int n;

   /* ------ is parent's lat_inc a multiple of subfile's lat_inc ? */

   n = (int)((par->lat_inc / sub->lat_inc) + 0.5);
   t = sub->lat_inc * n;
   if ( !NTV2_EQ(t, par->lat_inc) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d : parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LAT_INC not a multiple of parent LAT_INC");
         fprintf(fp, "    sub lat_inc = %.17g\n", sub->lat_inc);
         fprintf(fp, "    par lat_inc = %.17g\n", par->lat_inc);
         fprintf(fp, "    num cells   = %d\n",    n);
         fprintf(fp, "    should be   = %.17g\n", t);
      }
      rc = NTV2_ERR_INVALID_LAT_INC;
   }

   /* ------ is subfile's lat_min >= parent's lat_min ? */

   if ( !NTV2_GE(sub->lat_min, par->lat_min) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LAT_MIN < parent LAT_MIN");
         fprintf(fp, "    sub lat_min = %.17g\n", sub->lat_min);
         fprintf(fp, "    par lat_min = %.17g\n", par->lat_min);
      }
      rc = NTV2_ERR_INVALID_LAT_MIN;
   }

   /* ------ does subfile's lat_min line up with parent's grid line ? */

   n = (int)((sub->lat_min - par->lat_min) / par->lat_inc + 0.5);
   t = par->lat_min + (n * par->lat_inc);
   if ( !NTV2_EQ(t, sub->lat_min) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LAT_MIN not on parent grid line");
         fprintf(fp, "    par LAT_MIN = %.17g\n", par->lat_min);
         fprintf(fp, "    par LAT_INC = %.17g\n", par->lat_inc);
         fprintf(fp, "    num cells   = %d\n",    n);
         fprintf(fp, "    sub LAT_MIN = %.17g\n", sub->lat_min);
         fprintf(fp, "    should be   = %.17g\n", t);
      }
      rc = NTV2_ERR_INVALID_LAT_MIN;
   }

   /* ------ is subfile's lat_max <= parent's lat_max ? */

   if ( !NTV2_LE(sub->lat_max, par->lat_max) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LAT_MAX > parent LAT_MAX");
         fprintf(fp, "    par lat_max = %.17g\n", par->lat_max);
         fprintf(fp, "    sub lat_max = %.17g\n", sub->lat_max);
      }
      rc = NTV2_ERR_INVALID_LAT_MAX;
   }

   /* ------ does subfile's lat_max line up with parent's grid line ? */

   n = (int)((par->lat_max - sub->lat_max) / par->lat_inc + 0.5);
   t = par->lat_max - (n * par->lat_inc);
   if ( !NTV2_EQ(t, sub->lat_max) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LAT_MAX not on parent grid line");
         fprintf(fp, "    par LAT_MAX = %.17g\n", par->lat_max);
         fprintf(fp, "    par LAT_INC = %.17g\n", par->lat_inc);
         fprintf(fp, "    num cells   = %d\n",    n);
         fprintf(fp, "    sub LAT_MAX = %.17g\n", sub->lat_max);
         fprintf(fp, "    should be   = %.17g\n", t);
      }
      rc = NTV2_ERR_INVALID_LAT_MAX;
   }

   /* ------ is parent's lon_inc a multiple of subfile's lon_inc ? */

   n = (int)((par->lon_inc / sub->lon_inc) + 0.5);
   t = sub->lon_inc * n;
   if ( !NTV2_EQ(t, par->lon_inc) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LON_INC not a multiple of parent LON_INC");
         fprintf(fp, "    sub lon_inc = %.17g\n", sub->lon_inc);
         fprintf(fp, "    num cells   = %d\n", n);
         fprintf(fp, "    par lon_inc = %.17g\n", par->lon_inc);
         fprintf(fp, "    should be   = %.17g\n", t);
      }
      rc = NTV2_ERR_INVALID_LON_INC;
   }

   /* ------ is subfile's lon_min >= parent's lon_min ? */

   if ( !NTV2_GE(sub->lon_min, par->lon_min) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LON_MIN < parent LON_MIN");
         fprintf(fp, "    par lon_min = %.17g\n", par->lon_min);
         fprintf(fp, "    sub lon_min = %.17g\n", sub->lon_min);
      }
      rc = NTV2_ERR_INVALID_LON_MIN;
   }

   /* ------ does subfile's lon_min line up with parent's grid line ? */

   n = (int)((sub->lon_min - par->lon_min) / par->lon_inc + 0.5);
   t = par->lon_min + (n * par->lon_inc);
   if ( !NTV2_EQ(t, sub->lon_min) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LON_MIN not on parent grid line");
         fprintf(fp, "    par LON_MIN = %.17g\n", par->lon_min);
         fprintf(fp, "    par LON_INC = %.17g\n", par->lon_inc);
         fprintf(fp, "    num cells   = %d\n",    n);
         fprintf(fp, "    sub LON_MIN = %.17g\n", sub->lon_min);
         fprintf(fp, "    should be   = %.17g\n", t);
      }
      rc = NTV2_ERR_INVALID_LON_MIN;
   }

   /* ------ is subfile's lon_max <= parent's lon_max ? */

   if ( !NTV2_LE(sub->lon_max, par->lon_max) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LON_MAX > parent LON_MAX");
         fprintf(fp, "    par lon_max = %.17g\n", par->lon_max);
         fprintf(fp, "    sub lon_max = %.17g\n", sub->lon_max);
      }
      rc = NTV2_ERR_INVALID_LON_MAX;
   }

   /* ------ does subfile's lon_max line up with parent's grid line ? */

   n = (int)((par->lon_max - sub->lon_max) / par->lon_inc + 0.5);
   t = par->lon_max - (n * par->lon_inc);
   if ( !NTV2_EQ(t, sub->lon_max) )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();
         fprintf(fp, "  record %3d: parent %3d: %s\n",
            sub->rec_num, par->rec_num,
            "subfile LON_MAX not on parent grid line");
         fprintf(fp, "    par LON_MAX = %.17g\n", par->lon_max);
         fprintf(fp, "    par LON_INC = %.17g\n", par->lon_inc);
         fprintf(fp, "    num cells   = %d\n",    n);
         fprintf(fp, "    sub LON_MAX = %.17g\n", sub->lon_max);
         fprintf(fp, "    should be   = %.17g\n", t);
      }
      rc = NTV2_ERR_INVALID_LON_MAX;
   }

   return rc;
}

/*------------------------------------------------------------------------
 * Validate all headers in an NTv2 file.
 */
int ntv2_validate(
   NTV2_HDR *hdr,
   FILE     *fp)
{
   NTV2_REC *rec;
   NTV2_REC *sub;
   NTV2_REC *next;
   int i;
   int rc = NTV2_ERR_OK;

   rc = ntv2_validate_ov(hdr, fp, rc);

   /* ------ sanity checks on each record */

   for (i = 0; i < hdr->num_recs; i++)
   {
      double temp;

      rc = ntv2_validate_sf(hdr, fp, i, rc);

      rec = &hdr->recs[i];

      /* ------ is lat_inc > 0 ? */

      if ( !(rec->lat_inc > 0.0) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "LAT_INC <= 0");
            fprintf(fp, "    LAT_INC     = %.17g\n", rec->lat_inc);
         }
         rc = NTV2_ERR_INVALID_LAT_INC;
      }

      /* ------ is lat_min < lat_max ? */

      if ( !(rec->lat_min < rec->lat_max) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "LAT_MIN >= LAT_MAX");
            fprintf(fp, "    LAT_MIN     = %.17g\n", rec->lat_min);
            fprintf(fp, "    LAT_MAX     = %.17g\n", rec->lat_max);
         }
         rc = NTV2_ERR_INVALID_LAT_MIN_MAX;
      }

      /* ------ is lon_inc > 0 ? */

      if ( !(rec->lon_inc > 0.0) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "LON_INC <= 0");
            fprintf(fp, "    LAT_INC     = %.17g\n", rec->lon_inc);
         }
         rc = NTV2_ERR_INVALID_LON_INC;
      }

      /* ------ is lon_min < lon_max ? */

      if ( !(rec->lon_min < rec->lon_max) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "LON_MIN >= LON_MAX");
            fprintf(fp, "    LON_MIN     = %.17g\n", rec->lon_min);
            fprintf(fp, "    LON_MAX     = %.17g\n", rec->lon_max);
         }
         rc = NTV2_ERR_INVALID_LON_MIN_MAX;
      }

      /* ------ is num_recs > 0 ? */

      if ( !(rec->num > 0) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "GS_COUNT <= 0");
            fprintf(fp, "    num         = %d\n", rec->num);
         }
         rc = NTV2_ERR_INVALID_GS_COUNT;
      }

      /* ------ is (lat_max - lat_min) a multiple of lat_inc ? */

      temp = rec->lat_min + ((rec->nrows-1) * rec->lat_inc);
      if ( !NTV2_EQ(temp, rec->lat_max) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "LAT range not a multiple of LAT_INC");
            fprintf(fp, "    LON_MIN     = %.17g\n", rec->lat_min);
            fprintf(fp, "    LON_MAX     = %.17g\n", rec->lat_max);
            fprintf(fp, "    range       = %.17g\n", rec->lat_max-rec->lat_min);
            fprintf(fp, "    LON_INC     = %.17g\n", rec->lat_inc);
            fprintf(fp, "    n           = %d\n",    rec->nrows-1);
            fprintf(fp, "    t           = %.17g\n", temp);
         }
         rc = NTV2_ERR_INVALID_LAT_INC;
      }

      /* ------ is (lon_max - lon_min) a multiple of lon_inc ? */

      temp = rec->lon_min + ((rec->ncols-1) * rec->lon_inc);
      if ( !NTV2_EQ(temp, rec->lon_max) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "LON range not a multiple of LON_INC");
            fprintf(fp, "    LON_MIN     = %.17g\n", rec->lon_min);
            fprintf(fp, "    LON_MAX     = %.17g\n", rec->lon_max);
            fprintf(fp, "    range       = %.17g\n", rec->lon_max-rec->lon_min);
            fprintf(fp, "    LON_INC     = %.17g\n", rec->lon_inc);
            fprintf(fp, "    n           = %d\n",    rec->ncols-1);
            fprintf(fp, "    t           = %.17g\n", temp);
         }
         rc = NTV2_ERR_INVALID_LON_INC;
      }

      /* ------ is (nrows * ncols) == num ? */

      if ( !((rec->nrows * rec->ncols) == rec->num) )
      {
         if ( fp != NTV2_NULL )
         {
            NTV2_SHOW_PATH();
            fprintf(fp, "  record %3d: %s\n", rec->rec_num,
               "NROWS * NCOLS != COUNT");
            fprintf(fp, "    nrows       = %d\n",    rec->nrows);
            fprintf(fp, "    ncols       = %d\n",    rec->ncols);
            fprintf(fp, "    num         = %d\n",    rec->num);
         }
         rc = NTV2_ERR_INVALID_DELTA;
      }
   }

   if ( rc > NTV2_ERR_START )
   {
      /* no point in going any further */
      return rc;
   }

   /* ------ check if any parents overlap */

   for (rec = hdr->first_parent; rec != NTV2_NULL; rec = rec->next)
   {
      for (next = rec->next; next != NTV2_NULL; next = next->next)
      {
         if ( ntv2_overlap(rec, next) )
         {
            if ( fp != NTV2_NULL )
            {
               NTV2_SHOW_PATH();
               fprintf(fp, "  record %d: record %3d: %s\n",
                  rec->rec_num, next->rec_num,
                  "parent overlap");
            }
            rc = NTV2_ERR_PARENT_OVERLAP;
         }
      }
   }

   /* ------ for each parent:
                - validate all subfiles against their parent
                - verify that no subfiles of a parent overlap
   */

   for (rec = hdr->first_parent; rec != NTV2_NULL; rec = rec->next)
   {
      for (sub = rec->sub; sub != NTV2_NULL; sub = sub->next)
      {
         rc = ntv2_validate_subfile(hdr, rec, sub, fp, rc);

         for (next = sub->next; next != NTV2_NULL; next = next->next)
         {
            if ( ntv2_overlap(sub, next) )
            {
               if ( fp != NTV2_NULL )
               {
                  NTV2_SHOW_PATH();
                  fprintf(fp, "  record %d: record %3d: %s\n",
                     sub->rec_num, next->rec_num,
                     "subfile overlap");
               }
               rc = NTV2_ERR_SUBFILE_OVERLAP;
            }
         }
      }
   }

   /* ------ check if the file needs fixing */

   if ( hdr->fixed != 0 )
   {
      if ( fp != NTV2_NULL )
      {
         NTV2_SHOW_PATH();

         if ( (hdr->fixed & NTV2_FIX_UNPRINTABLE_CHAR      ) != 0 )
            fprintf(fp, "  fix: name contains unprintable character\n");

         if ( (hdr->fixed & NTV2_FIX_NAME_LOWERCASE        ) != 0 )
            fprintf(fp, "  fix: name contains lowercase letter\n");

         if ( (hdr->fixed & NTV2_FIX_NAME_NOT_ALPHA        ) != 0 )
            fprintf(fp, "  fix: name contains non-alphameric character\n");

         if ( (hdr->fixed & NTV2_FIX_BLANK_PARENT_NAME     ) != 0 )
            fprintf(fp, "  fix: parent name blank\n");

         if ( (hdr->fixed & NTV2_FIX_BLANK_SUBFILE_NAME    ) != 0 )
            fprintf(fp, "  fix: subfile name blank\n");

         if ( (hdr->fixed & NTV2_FIX_END_REC_NOT_FOUND     ) != 0 )
            fprintf(fp, "  fix: end record not found\n");

         if ( (hdr->fixed & NTV2_FIX_END_REC_NAME_NOT_ALPHA) != 0 )
            fprintf(fp, "  fix: end record name not all alphameric\n");

         if ( (hdr->fixed & NTV2_FIX_END_REC_PAD_NOT_ZERO  ) != 0 )
            fprintf(fp, "  fix: end record pad not all zeros\n");
      }

      /* Note that we only set this if there are no other errors,
         since any other error is a lot more important.
      */
      if ( rc == NTV2_ERR_OK )
         rc = NTV2_ERR_FILE_NEEDS_FIXING;
   }

   if ( rc != NTV2_ERR_OK )
   {
      if ( fp != NTV2_NULL )
         fprintf(fp, "\n");
   }

   return rc;
}

/*------------------------------------------------------------------------
 * Fix all parent and subfile pointers.
 *
 * This creates the chain of parent pointers  (stored in hdr->first_parent)
 * and the chain of sub-files for each parent (stored in rec->sub).
 */
static int ntv2_fix_ptrs(
   NTV2_HDR *hdr)
{
   NTV2_REC * sub = NTV2_NULL;
   int i, j;

   hdr->num_parents  = 0;
   hdr->first_parent = NTV2_NULL;

   /* -------- adjust all parent pointers */

   for (i = 0; i < hdr->num_recs; i++)
   {
      NTV2_REC * rec = &hdr->recs[i];

      if ( rec->active )
      {
         rec->parent = NTV2_NULL;
         rec->sub    = NTV2_NULL;
         rec->next   = NTV2_NULL;

         if ( ntv2_strcmp_i(rec->record_name, NTV2_ALL_BLANKS) == 0 )
            hdr->fixed |= NTV2_FIX_BLANK_SUBFILE_NAME;

         if ( ntv2_strcmp_i(rec->parent_name, NTV2_NO_PARENT_NAME) == 0 )
         {
            /* This record has no parent, so it is a top-level parent.
               Bump our parent count & add it to the top-level parent chain.
            */

            hdr->num_parents++;
            if ( sub != NTV2_NULL )
               sub->next = rec;
            else
               hdr->first_parent = rec;
            sub = rec;
         }
         else
         if ( ntv2_strcmp_i(rec->record_name, rec->parent_name) == 0 )
         {
            /* A record can't be its own parent. */
            return NTV2_ERR_INVALID_PARENT_NAME;
         }
         else
         {
            /* This record has a parent.
               Add it to its parent's child chain.
               It is an error if we can't find its parent.
            */

            NTV2_REC * p = NTV2_NULL;

            /*
               I cannot find anything in the NTv2 spec that states
                  definitively that a child must appear later in the file
                  than a parent, although that is the case for all files
                  that I've seen, with sub-files immediately following
                  their parent.

               In fact, the NTv2 Developer's Guide states "the order of
                  the sub-files is of no consequence", which could be
                  interpreted as meaning that the sub-files for a particular
                  grid must follow it but may be in any order, but it doesn't
                  explicitly say that.

               Thus, we check all records (other than this one) to see if
                  it is the parent of this one.

               If we can establish that children must follow their parents,
                  then the for-loop below can go to i instead of hdr->num_recs.

               NEWS AT 11:
                  I have just found a published Australian NTv2 file
                  (australia/WA_0700.gsb) in which all the sub-files appear
                  ahead of their parent!

                  It appears that the people who wrote this file just wrote
                  all the sub-files in alphabetical order by sub-file name.

                  However, that file also doesn't blank pad its sub-file field
                  names or its end record properly, so it could be considered
                  as a bogusly-created file and they just didn't know what
                  they were doing, which it seems they didn't.  Nevertheless,
                  it is a published file, so we have to deal with it.
            */

            for (j = 0; j < hdr->num_recs; j++)
            {
               NTV2_REC * next = hdr->recs + j;

               if ( i != j && next->active )
               {
                  if ( ntv2_strcmp_i(rec->parent_name, next->record_name) == 0 )
                  {
                     p = next;
                     break;
                  }
               }
            }

            if ( p == NTV2_NULL )
            {
               /*
                  This record's parent can't be found.  Check if they
                  stoopidly left the parent name blank, instead of using
                  "NONE" like they're supposed to do.  If they did, we'll
                  just fix it.  Note that the parent name has already been
                  "cleaned up".
               */

               if ( ntv2_strcmp_i(rec->parent_name, NTV2_ALL_BLANKS) == 0 )
               {
                  strcpy(rec->parent_name, NTV2_NO_PARENT_NAME);
                  if ( hdr->subfiles != NTV2_NULL )
                  {
                     strncpy(hdr->subfiles[rec->rec_num].s_parent,
                             NTV2_NO_PARENT_NAME, NTV2_NAME_LEN);
                  }
                  hdr->fixed |= NTV2_FIX_BLANK_PARENT_NAME;
                  i--;
                  continue;
               }

               return NTV2_ERR_PARENT_NOT_FOUND;
            }

            rec->parent = p;
            p->num_subs++;
         }
      }
   }

   /* -------- make sure that we have at least one top-level parent.

      It is possible to have no top-level parents.  For example,
      assume we have only two records (A and B), and A's parent is B
      and B's parent is A.
   */

   if ( hdr->first_parent == NTV2_NULL )
   {
      return NTV2_ERR_NO_TOP_LEVEL_PARENT;
   }

   /* -------- validate all parent chains

      At this point, we have ascertained that all records
      either are a top-level parent (i.e. have no parent)
      or have a valid parent.  We have also counted the top-level
      parents and have created a chain of them to follow.

      Now we want to validate that all sub-files ultimately
      point to a top-level parent.  ie, there are no parent chain
      loops (eg, subfile A's parent is B and B's parent is A).

      Our logic here is to get the longest possible parent chain
      (# records - # parents + 1), then chase each record's parent
      back.  If we don't find a top-level parent within the
      chain length, we must have a loop.
   */

   {
      int max_chain = (hdr->num_recs - hdr->num_parents) + 1;

      for (i = 0; i < hdr->num_recs; i++)
      {
         NTV2_REC * rec = &hdr->recs[i];

         if ( rec->active )
         {
            NTV2_REC * parent = rec->parent;
            int count = 0;

            for (; parent != NTV2_NULL; parent = parent->parent)
            {
               if ( ++count > max_chain )
                  return NTV2_ERR_PARENT_LOOP;
            }
         }
      }
   }

   /* -------- adjust all sub-file pointers

      Here we create the chains of all sub-file pointers.
   */

   for (i = 0; i < hdr->num_recs; i++)
   {
      NTV2_REC * rec = &hdr->recs[i];

      if ( rec->active )
      {
         sub = NTV2_NULL;

         /* See the comment above about child records.

            We check all records (other than this one) to see if
            it is a child of this one.  If it is, we add it to
            this record's child chain.

            If we could enforce the rule that children must follow
            their parents (which we can't), then the for-loop below
            could go from i+1 instead of from 0.
         */

         for (j = 0; j < hdr->num_recs; j++)
         {
            NTV2_REC * next = hdr->recs + j;

            if ( i != j && next->active )
            {
               if ( next->parent == rec )
               {
                  if ( rec->sub == NTV2_NULL )
                     rec->sub = next;

                  if ( sub != NTV2_NULL )
                     sub->next = next;
                  sub = next;
               }
            }
         }
      }
   }

   return NTV2_ERR_OK;
}

/* ------------------------------------------------------------------------- */
/* NTv2 common routines                                                      */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Determine the file type of a name
 *
 * This is done solely by checking the filename extension.
 * No examination of the file contents (if any) is done.
 */
int ntv2_filetype(const char *ntv2file)
{
   const char *ext;

   if ( ntv2file == NTV2_NULL || *ntv2file == 0 )
      return NTV2_FILE_TYPE_UNK;

   ext = strrchr(ntv2file, '.');
   if ( ext != NTV2_NULL )
   {
      ext++;

      if ( ntv2_strcmp_i(ext, NTV2_FILE_BIN_EXTENSION) == 0 )
         return NTV2_FILE_TYPE_BIN;

      if ( ntv2_strcmp_i(ext, NTV2_FILE_ASC_EXTENSION) == 0 )
         return NTV2_FILE_TYPE_ASC;
   }

   return NTV2_FILE_TYPE_UNK;
}

/*------------------------------------------------------------------------
 * Create an empty NTv2 struct, query the file type, and open the file.
 */
static NTV2_HDR * ntv2_create(const char *ntv2file, int type, int *prc)
{
   NTV2_HDR * hdr;

   hdr = (NTV2_HDR *)ntv2_memalloc(sizeof(*hdr));
   if ( hdr == NTV2_NULL )
   {
      *prc = NTV2_ERR_NO_MEMORY;
      return NTV2_NULL;
   }

   memset(hdr, 0, sizeof(*hdr));

   strcpy(hdr->path, ntv2file);
   hdr->file_type = type;
   hdr->lon_min   =  360.0;
   hdr->lat_min   =   90.0;
   hdr->lon_max   = -360.0;
   hdr->lat_max   =  -90.0;

   hdr->fp = fopen(hdr->path, "rb");
   if ( hdr->fp == NTV2_NULL )
   {
      ntv2_memdealloc(hdr);
      *prc = NTV2_ERR_CANNOT_OPEN_FILE;
      return NTV2_NULL;
   }

   if ( type == NTV2_FILE_TYPE_BIN )
   {
      hdr->mutex = ntv2_mutex_create();
   }

   *prc = NTV2_ERR_OK;
   return hdr;
}

/*------------------------------------------------------------------------
 * Delete a NTv2 object
 */
void ntv2_delete(NTV2_HDR *hdr)
{
   if ( hdr != NTV2_NULL )
   {
      int i;

      if ( hdr->fp != NTV2_NULL )
      {
         fclose(hdr->fp);
      }

      if ( hdr->mutex != NTV2_NULL )
      {
         ntv2_mutex_delete(hdr->mutex);
      }

      for (i = 0; i < hdr->num_recs; i++)
      {
         ntv2_memdealloc(hdr->recs[i].shifts);
         ntv2_memdealloc(hdr->recs[i].accurs);
      }

      ntv2_memdealloc(hdr->overview);
      ntv2_memdealloc(hdr->subfiles);

      ntv2_memdealloc(hdr->recs);
      ntv2_memdealloc(hdr);
   }
}

/* ------------------------------------------------------------------------- */
/* NTv2 binary read routines                                                 */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Read a binary overview record.
 *
 * The original Canadian specification for NTv2 files included
 * a 4-byte pad of zeros after each int in the headers, which
 * kept the length of each record (overview field, sub-file field,
 * or grid-shift entry) to be 16 bytes.  This was probably done
 * so they could read the files in FORTRAN or on a main-frame using
 * "DCB=(RECFM=FB,LRECL=16,...)".
 *
 * However, when Australia started using this file format, they
 * misunderstood the reason for the padding and thought it was a compiler
 * issue.  The Canadians had implemented NTv2 using FORTRAN,
 * and information at the time suggested that the use of records in
 * a FORTRAN binary file would create a file that other languages
 * would have a difficult time reading and writing.  Thus, the
 * Australians decided to format the file as specified, but to
 * ignore any "auxiliary record identifiers" that FORTRAN used (i.e.
 * the padding bytes).
 *
 * To make matters worse, New Zealand copied the Australian
 * format.  Blimey!  However, it became apparent that the NTv2 binary
 * format is not compiler dependent, and since 2000 both Australia
 * and New Zealand have been writing new files correctly.
 *
 * But this means that when reading a NTv2 header record we have to
 * determine whether these pad bytes are there, since we may be reading
 * an older Australian or New Zealand NTv2 file.
 *
 * Also, there is nothing in the NTv2 specification that states
 * whether the file should be written big-endian or little-endian,
 * so we also have to determine how it was written, so we can know if
 * we have to byte-swap the numbers.  However, all files we've seen
 * so far are all little-endian.
 *
 * Also, when we cache the overview and subfile records, they are always
 * stored in native-endian format.  They get byte-swapped back, if needed,
 * when written out.  This allows us to choose how they are written out,
 * and makes it easier to dump the data.
 *
 *------------------------------------------------------------------------
 * Implementation note:
 *   In GCC, a function can be declared with the attribute "warn_unused_result",
 *   which results in a warning being issued if the return value from that
 *   function is not used.  This, of course, will cause a compilation to
 *   fail if "-Werror" (treat warnings as errors) is specified on the
 *   command line (which we like to do).  For details, see
 *   "http://gcc.gnu.org/onlinedocs/gcc-3.4.6/gcc/Function-Attributes.html".
 *
 *   Despite flames from the user community, Ubuntu Linux sets this attribute
 *   for the fread() function.  Unfortunately, we don't bother to check the
 *   result from each call to fread(), since that makes the code real ugly
 *   and it is easier just to check ferror() at the end.  But, because of
 *   Ubuntu's insistence on requiring that the return value be used, we store
 *   the result but we don't bother checking it.
 *------------------------------------------------------------------------
 */
static int ntv2_read_ov_bin(
   NTV2_HDR     *hdr,
   NTV2_FILE_OV *ov)
{
   size_t nr;

   /* -------- NUM_OREC */

   nr = fread( ov->n_num_orec, NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&ov->i_num_orec, NTV2_SIZE_INT, 1, hdr->fp);

   /* determine if byte-swapping is needed */

   if ( ov->i_num_orec != 11 )
   {
      hdr->swap_data = TRUE;
      NTV2_SWAPI(&ov->i_num_orec, 1);
      if ( ov->i_num_orec != 11 )
         return NTV2_ERR_INVALID_NUM_OREC;
   }

   /* determine if pad-bytes are present */

   nr = fread(&ov->p_num_orec, NTV2_SIZE_INT, 1, hdr->fp);
   if ( ov->p_num_orec == 0 )
   {
      hdr->pads_present = TRUE;
   }
   else
   {
      ov->p_num_orec = 0;
      fseek(hdr->fp, -NTV2_SIZE_INT, SEEK_CUR);
   }

   /* -------- NUM_SREC */

   nr = fread( ov->n_num_srec, NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread(&ov->i_num_srec, NTV2_SIZE_INT,  1, hdr->fp);
   ov->p_num_srec = 0;

   NTV2_SWAPI(&ov->i_num_srec, 1);
   if ( ov->i_num_srec != 11 )
      return NTV2_ERR_INVALID_NUM_SREC;

   if ( hdr->pads_present )
      fseek(hdr->fp, NTV2_SIZE_INT, SEEK_CUR);

   /* -------- NUM_FILE */

   nr = fread( ov->n_num_file,  NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&ov->i_num_file,  NTV2_SIZE_INT, 1, hdr->fp);
   ov->p_num_file = 0;

   NTV2_SWAPI(&ov->i_num_file, 1);
   if ( ov->i_num_file <= 0 )
      return NTV2_ERR_INVALID_NUM_FILE;
   hdr->num_recs = ov->i_num_file;

   if ( hdr->pads_present )
      fseek(hdr->fp, NTV2_SIZE_INT, SEEK_CUR);

   /* -------- GS_TYPE */

   nr = fread( ov->n_gs_type,  NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread( ov->s_gs_type,  NTV2_NAME_LEN,  1, hdr->fp);

   /* -------- VERSION */

   nr = fread( ov->n_version,  NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread( ov->s_version,  NTV2_NAME_LEN,  1, hdr->fp);

   /* -------- SYSTEM_F */

   nr = fread( ov->n_system_f, NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread( ov->s_system_f, NTV2_NAME_LEN,  1, hdr->fp);

   /* -------- SYSTEM_T */

   nr = fread( ov->n_system_t, NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread( ov->s_system_t, NTV2_NAME_LEN,  1, hdr->fp);

   /* -------- MAJOR_F */

   nr = fread( ov->n_major_f,  NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread(&ov->d_major_f,  NTV2_SIZE_DBL,  1, hdr->fp);
   NTV2_SWAPD(&ov->d_major_f, 1);

   /* -------- MINOR_F */

   nr = fread( ov->n_minor_f,  NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread(&ov->d_minor_f,  NTV2_SIZE_DBL,  1, hdr->fp);
   NTV2_SWAPD(&ov->d_minor_f, 1);

   /* -------- MAJOR_T */

   nr = fread( ov->n_major_t,  NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread(&ov->d_major_t,  NTV2_SIZE_DBL,  1, hdr->fp);
   NTV2_SWAPD(&ov->d_major_t, 1);

   /* -------- MINOR_T */

   nr = fread( ov->n_minor_t,  NTV2_NAME_LEN,  1, hdr->fp);
   nr = fread(&ov->d_minor_t,  NTV2_SIZE_DBL,  1, hdr->fp);
   NTV2_SWAPD(&ov->d_minor_t, 1);

   /* -------- check for I/O error */

   if ( ferror(hdr->fp) || feof(hdr->fp) )
   {
      return NTV2_ERR_IOERR;
   }

   /* -------- get the conversion

      So far, the only data type we've encountered in any published
      NTv2 file is "SECONDS".
   */

   ntv2_cleanup_str(hdr, hdr->gs_type, ov->s_gs_type, FALSE);

   if      ( strncmp(hdr->gs_type, "SECONDS ", NTV2_NAME_LEN) == 0 )
   {
      hdr->hdr_conv = ( 1.0 / 3600.0 );
      hdr->dat_conv = ( 1.0          );
   }
   else if ( strncmp(hdr->gs_type, "MINUTES ", NTV2_NAME_LEN) == 0 )
   {
      hdr->hdr_conv = ( 1.0 / 60.0   );
      hdr->dat_conv = ( 60.0         );
   }
   else if ( strncmp(hdr->gs_type, "DEGREES ", NTV2_NAME_LEN) == 0 )
   {
      hdr->hdr_conv = ( 1.0          );
      hdr->dat_conv = ( 3600.0       );
   }
   else
   {
      return NTV2_ERR_INVALID_GS_TYPE;
   }

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Read a binary subfile record.
 *
 * See above implementation comment.
 */
static int ntv2_read_sf_bin(
   NTV2_HDR     *hdr,
   NTV2_FILE_SF *sf)
{
   size_t nr;

   /* -------- SUB_NAME */

   nr = fread( sf->n_sub_name, NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread( sf->s_sub_name, NTV2_NAME_LEN, 1, hdr->fp);

   /* -------- PARENT */

   nr = fread( sf->n_parent,   NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread( sf->s_parent,   NTV2_NAME_LEN, 1, hdr->fp);

   /* -------- CREATED */

   nr = fread( sf->n_created,  NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread( sf->s_created,  NTV2_NAME_LEN, 1, hdr->fp);

   /* -------- UPDATED */

   nr = fread( sf->n_updated,  NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread( sf->s_updated,  NTV2_NAME_LEN, 1, hdr->fp);

   /* -------- S_LAT */

   nr = fread( sf->n_s_lat,    NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->d_s_lat,    NTV2_SIZE_DBL, 1, hdr->fp);
   NTV2_SWAPD(&sf->d_s_lat, 1);

   /* -------- N_LAT */

   nr = fread( sf->n_n_lat,    NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->d_n_lat,    NTV2_SIZE_DBL, 1, hdr->fp);
   NTV2_SWAPD(&sf->d_n_lat, 1);

   /* -------- E_LONG */

   nr = fread( sf->n_e_lon,    NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->d_e_lon,    NTV2_SIZE_DBL, 1, hdr->fp);
   NTV2_SWAPD(&sf->d_e_lon, 1);

   /* -------- W_LONG */

   nr = fread( sf->n_w_lon,    NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->d_w_lon,    NTV2_SIZE_DBL, 1, hdr->fp);
   NTV2_SWAPD(&sf->d_w_lon, 1);

   /* -------- LAT_INC */

   nr = fread( sf->n_lat_inc,  NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->d_lat_inc,  NTV2_SIZE_DBL, 1, hdr->fp);
   NTV2_SWAPD(&sf->d_lat_inc, 1);

   /* -------- LONG_INC */

   nr = fread( sf->n_lon_inc,  NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->d_lon_inc,  NTV2_SIZE_DBL, 1, hdr->fp);
   NTV2_SWAPD(&sf->d_lon_inc, 1);

   /* -------- GS_COUNT */

   nr = fread( sf->n_gs_count, NTV2_NAME_LEN, 1, hdr->fp);
   nr = fread(&sf->i_gs_count, NTV2_SIZE_INT, 1, hdr->fp);
   sf->p_gs_count = 0;

   NTV2_SWAPI(&sf->i_gs_count, 1);
   if ( sf->i_gs_count <= 0 )
      return NTV2_ERR_INVALID_GS_COUNT;

   if ( hdr->pads_present )
      fseek(hdr->fp, NTV2_SIZE_INT, SEEK_CUR);

   /* -------- check for I/O error */

   if ( ferror(hdr->fp) || feof(hdr->fp) )
   {
      return NTV2_ERR_IOERR;
   }

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Read a binary end record.
 *
 * The only NTv2 file I've seen that would generate an error here
 * is the file "italy/rer.gsb", which omits the end record completely.
 * Maybe we should make them an offer they can't refuse...
 */
static int ntv2_read_er_bin(
   NTV2_HDR      *hdr,
   NTV2_FILE_END *er)
{
   size_t nr = fread(er, sizeof(*er), 1, hdr->fp);
   return (nr == 1) ? NTV2_ERR_OK : NTV2_ERR_IOERR;
}

/*------------------------------------------------------------------------
 * Convert a subfile record to a ntv2-rec.
 *
 * The NTv2 specification uses the convention of positive-west /
 * negative-east (probably since Canada is all in the west).
 * However, this is backwards from the standard negative-west /
 * positive-east notation.
 *
 * Thus, when converting the sub-file records, we flip the sign of
 * the longitude-max (the east value) and the longitude-min
 * (the west value).  This way, the longitude-min (west) is less than
 * the longitude-max (east).  This may have to be rethought if an NTv2
 * file ever spans the dateline (+/-180 degrees).
 *
 * However, we still have to remember that the grid-shift records
 * are written (backwards) east-to-west.
 */
static int ntv2_sf_to_rec(
   NTV2_HDR     *hdr,
   NTV2_FILE_SF *sf,
   int           n)
{
   NTV2_REC * rec = hdr->recs + n;

   ntv2_cleanup_str(hdr, rec->record_name, sf->s_sub_name, TRUE);
   ntv2_cleanup_str(hdr, rec->parent_name, sf->s_parent,   TRUE);
   rec->record_name[NTV2_NAME_LEN] = 0;
   rec->parent_name[NTV2_NAME_LEN] = 0;

   rec->active     =  TRUE;
   rec->parent     =  NTV2_NULL;
   rec->sub        =  NTV2_NULL;
   rec->next       =  NTV2_NULL;
   rec->num_subs   =  0;
   rec->rec_num    =  n;

   rec->offset     =  ftell(hdr->fp);
   rec->sskip      =  0;
   rec->nskip      =  0;
   rec->wskip      =  0;
   rec->eskip      =  0;

   rec->shifts     =  NTV2_NULL;
   rec->accurs     =  NTV2_NULL;

   rec->lat_min    =  sf->d_s_lat;
   rec->lat_min   *=  hdr->hdr_conv;

   rec->lat_max    =  sf->d_n_lat;
   rec->lat_max   *=  hdr->hdr_conv;

   rec->lat_inc    =  sf->d_lat_inc;
   rec->lat_inc   *=  hdr->hdr_conv;

   rec->lon_max    =  sf->d_e_lon;
   rec->lon_max   *= -hdr->hdr_conv;      /* NOTE: reversing sign! */

   rec->lon_min    =  sf->d_w_lon;
   rec->lon_min   *= -hdr->hdr_conv;      /* NOTE: reversing sign! */

   rec->lon_inc    =  sf->d_lon_inc;
   rec->lon_inc   *=  hdr->hdr_conv;

   rec->num        =  sf->i_gs_count;

   rec->nrows      =  (int)
                      ((rec->lat_max - rec->lat_min) / rec->lat_inc + 0.5) + 1;
   rec->ncols      =  (int)
                      ((rec->lon_max - rec->lon_min) / rec->lon_inc + 0.5) + 1;

   /* collect the min/max of all the sub-files */

   if ( hdr->lon_min > rec->lon_min )  hdr->lon_min = rec->lon_min;
   if ( hdr->lat_min > rec->lat_min )  hdr->lat_min = rec->lat_min;
   if ( hdr->lon_max < rec->lon_max )  hdr->lon_max = rec->lon_max;
   if ( hdr->lat_max < rec->lat_max )  hdr->lat_max = rec->lat_max;

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Read binary header data from the file.
 */
static int ntv2_read_hdrs_bin(
   NTV2_HDR *hdr)
{
   NTV2_FILE_OV  ov_rec;
   NTV2_FILE_SF  sf_rec;
   NTV2_FILE_END en_rec;
   int i;
   int rc;

   /* -------- read in the overview record */

   rc = ntv2_read_ov_bin(hdr, &ov_rec);
   if ( rc != NTV2_ERR_OK )
   {
      return rc;
   }

   hdr->num_parents = 0;
   hdr->recs        = (NTV2_REC *)
                      ntv2_memalloc(sizeof(*hdr->recs) * hdr->num_recs);
   if ( hdr->recs == NTV2_NULL )
   {
      return NTV2_ERR_NO_MEMORY;
   }

   memset(hdr->recs, 0, sizeof(*hdr->recs) * hdr->num_recs);

   /* -------- create the file record cache if wanted */

   if ( hdr->keep_orig )
   {
      hdr->overview = (NTV2_FILE_OV *)
                      ntv2_memalloc(sizeof(*hdr->overview));
      if ( hdr->overview == NTV2_NULL )
      {
         return NTV2_ERR_NO_MEMORY;
      }

      memcpy(hdr->overview, &ov_rec, sizeof(*hdr->overview));

      hdr->subfiles = (NTV2_FILE_SF *)
                      ntv2_memalloc(sizeof(*hdr->subfiles) * hdr->num_recs);
      if ( hdr->subfiles == NTV2_NULL )
      {
         return NTV2_ERR_NO_MEMORY;
      }
   }

   /* -------- read in all subfile records (but not the actual data)

      We don't read in the data now, since we may be doing
      extent processing later, which would affect the reading
      of the data.
   */

   for (i = 0; i < hdr->num_recs; i++)
   {
      long offset;

      rc = ntv2_read_sf_bin(hdr, &sf_rec);
      if ( rc != NTV2_ERR_OK )
      {
         return rc;
      }

      if ( hdr->subfiles != NTV2_NULL )
      {
         memcpy(&hdr->subfiles[i], &sf_rec, sizeof(*hdr->subfiles));
      }

      rc = ntv2_sf_to_rec(hdr, &sf_rec, i);
      if ( rc != NTV2_ERR_OK )
      {
         return rc;
      }

      /* skip over grid-shift records */
      offset = hdr->recs[i].num * sizeof(NTV2_FILE_GS);
      fseek(hdr->fp, offset, SEEK_CUR);
   }

   /* -------- read in the end record

      Note that we don't bail if we can't read the end record,
      since it is certainly possible to use the file without it because
      we know the number of records and thus where the end should be.
      But we do note that the file "needs fixing" if it isn't there,
      or if it isn't in "standard" format.

      Interestingly enough, all the Canadian NTv2 files properly
      zero out the filler bytes in the end record, but the name
      field is "END xxxx" (ie "END " and four trailing garbage bytes).
      And it's their specification!
   */

   rc = ntv2_read_er_bin(hdr, &en_rec);
   if ( rc == NTV2_ERR_OK )
   {
      if ( strncmp(en_rec.n_end, NTV2_END_NAME,  NTV2_NAME_LEN) != 0 )
         hdr->fixed |= NTV2_FIX_END_REC_NAME_NOT_ALPHA;

      if ( memcmp(en_rec.filler, NTV2_ALL_ZEROS, NTV2_NAME_LEN) != 0 )
         hdr->fixed |= NTV2_FIX_END_REC_PAD_NOT_ZERO;
   }
   else
   {
      hdr->fixed |= NTV2_FIX_END_REC_NOT_FOUND;
   }

   /* -------- adjust all pointers */

   rc = ntv2_fix_ptrs(hdr);
   return rc;
}

/*------------------------------------------------------------------------
 * Mark a sub-file and all its children as inactive.
 *
 * This is called if a sub-file is to be skipped due to extent processing.
 */
static int ntv2_inactivate(
   NTV2_HDR *hdr,
   NTV2_REC *rec,
   int       nrecs)
{
   if ( rec->active )
   {
      NTV2_REC *sub;

      nrecs--;
      rec->active = FALSE;

      for (sub = rec->sub; sub != NTV2_NULL; sub = sub->next)
      {
         nrecs = ntv2_inactivate(hdr, sub, nrecs);
      }
   }

   return nrecs;
}

/*------------------------------------------------------------------------
 * Apply an extent to an NTv2 object.
 */
static int ntv2_process_extent(
   NTV2_HDR    *hdr,
   NTV2_EXTENT *extent)
{
   double wlon;
   double slat;
   double elon;
   double nlat;
   int    nrecs;
   int    i;
   int    rc = NTV2_ERR_OK;

   if ( ntv2_extent_is_empty(extent) )
   {
      return NTV2_ERR_OK;
   }

   nrecs = hdr->num_recs;

   /* get extent values */

   wlon = extent->wlon;
   slat = extent->slat;
   elon = extent->elon;
   nlat = extent->nlat;

   /* apply the extent to all records */

   for (i = 0; i < hdr->num_recs; i++)
   {
      NTV2_REC * rec = &hdr->recs[i];
      double d;
      int    nskip = 0;
      int    sskip = 0;
      int    wskip = 0;
      int    eskip = 0;
      int    ocols = rec->ncols;
      int    k;

      /* check if this record has already been marked as out-of-range */

      if ( !rec->active )
         continue;

      /* check if this record is out-of-range */

      if ( NTV2_GE(wlon, rec->lon_max) ||
           NTV2_GE(slat, rec->lat_max) ||
           NTV2_LE(elon, rec->lon_min) ||
           NTV2_LE(nlat, rec->lat_min) )
      {
         nrecs = ntv2_inactivate(hdr, rec, nrecs);
         continue;
      }

      /* check if this extent is a subset of the record */

      if ( NTV2_GT(wlon, rec->lon_min) ||
           NTV2_GT(slat, rec->lat_min) ||
           NTV2_LT(elon, rec->lon_max) ||
           NTV2_LT(nlat, rec->lat_max) )
      {
         /* Force our extent to be a proper multiple of the deltas.
            We move the extent edges out to the next delta
            multiple to do this.

            Note that the deltas we use are the parent's deltas if
            we have a parent.  This is to make sure we stay lined
            up on a parent grid line.
         */
         double lat_inc = rec->lat_inc;
         double lon_inc = rec->lon_inc;
         int    lat_mul = 1;
         int    lon_mul = 1;

         if ( rec->parent != NTV2_NULL )
         {
            lat_inc = rec->parent->lat_inc;
            lon_inc = rec->parent->lon_inc;
            lat_mul = (int)((lat_inc / rec->lat_inc) + 0.5);
            lon_mul = (int)((lon_inc / rec->lon_inc) + 0.5);
         }

         if ( NTV2_GT(wlon, rec->lon_min) )
         {
            d = (wlon - rec->lon_min) / lon_inc;
            k = (int)floor(d) * lon_mul;

            if ( k > 0 )
            {
               wskip = k;
               rec->lon_min += (k * rec->lon_inc);
               rec->ncols   -= k;
            }
         }

         if ( NTV2_LT(elon, rec->lon_max) )
         {
            d = (rec->lon_max - elon) / lon_inc;
            k = (int)floor(d) * lon_mul;

            if ( k > 0 )
            {
               eskip = k;
               rec->lon_max -= (k * rec->lon_inc);
               rec->ncols   -= k;
            }
         }

         if ( NTV2_GT(slat, rec->lat_min) )
         {
            d = (slat - rec->lat_min) / lat_inc;
            k = (int)floor(d) * lat_mul;

            if ( k > 0 )
            {
               sskip = k;
               rec->lat_min += (k * rec->lat_inc);
               rec->nrows   -= k;
            }
         }

         if ( NTV2_LT(nlat, rec->lat_max) )
         {
            d = (rec->lat_max - nlat) / lat_inc;
            k = (int)floor(d) * lat_mul;

            if ( k > 0 )
            {
               nskip = k;
               rec->lat_max -= (k * rec->lat_inc);
               rec->nrows   -= k;
            }
         }

         /* Check if there is any data left.
            I think that this will always be the case, but it is
            easier to do the check than to think out all
            possible scenarios.  Also, better safe than sorry.
         */

         if ( rec->ncols <= 0 || rec->nrows <= 0 )
         {
            nrecs = ntv2_inactivate(hdr, rec, nrecs);
            continue;
         }

         /* check if this record has changed */

         if ( nskip > 0 || sskip > 0 || wskip > 0 || eskip > 0 )
         {
            rec->num   = (rec->ncols * rec->nrows);
            rec->sskip = (sskip * sizeof(NTV2_FILE_GS)) * ocols;
            rec->nskip = (nskip * sizeof(NTV2_FILE_GS)) * ocols;
            rec->wskip = (wskip * sizeof(NTV2_FILE_GS));
            rec->eskip = (eskip * sizeof(NTV2_FILE_GS));

            /* adjust the subfile record if we cached it */
            if ( hdr->subfiles != NTV2_NULL )
            {
               NTV2_FILE_SF * sf = &hdr->subfiles[i];

               sf->i_gs_count = rec->num;

               sf->d_s_lat =  (rec->lat_min / hdr->hdr_conv);
               sf->d_n_lat =  (rec->lat_max / hdr->hdr_conv);

               /* Note the sign changes here! */

               sf->d_e_lon = -(rec->lon_max / hdr->hdr_conv);
               sf->d_w_lon = -(rec->lon_min / hdr->hdr_conv);
            }
         }
      }
   }

   /* check if any records have been deleted */

   if ( nrecs != hdr->num_recs )
   {
      /* error if no records are left */
      if ( nrecs == 0 )
         return NTV2_ERR_INVALID_EXTENT;

      /* fix number of files in overview record if present */
      if ( hdr->overview != NTV2_NULL )
         hdr->overview->i_num_file = nrecs;

      /* readjust all pointers */
      rc = ntv2_fix_ptrs(hdr);
   }

   return rc;
}

/*------------------------------------------------------------------------
 * Read in binary shift data for all sub-files.
 */
static int ntv2_read_data_bin(
   NTV2_HDR *hdr)
{
   int i;

   for (i = 0; i < hdr->num_recs; i++)
   {
      NTV2_REC * rec = &hdr->recs[i];
      int row;
      int col;
      int j;

      if ( !rec->active )
         continue;

      /* allocate our arrays */

      {
         rec->shifts = (NTV2_SHIFT *)
                       ntv2_memalloc(sizeof(*rec->shifts) * rec->num);
         if ( rec->shifts == NTV2_NULL )
            return NTV2_ERR_NO_MEMORY;
      }

      if ( hdr->keep_orig )
      {
         rec->accurs = (NTV2_SHIFT *)
                       ntv2_memalloc(sizeof(*rec->accurs) * rec->num);
         if ( rec->accurs == NTV2_NULL )
            return NTV2_ERR_NO_MEMORY;
      }

      /* position to start of data to read */

      fseek(hdr->fp, rec->offset + rec->sskip, SEEK_SET);

      /* now read the data */
      /* Remember that data in a latitude row goes East to West! */

      j = 0;
      for (row = 0; row < rec->nrows; row++)
      {
         if ( rec->eskip > 0 )
            fseek(hdr->fp, rec->eskip, SEEK_CUR);

         for (col = 0; col < rec->ncols; col++)
         {
            NTV2_FILE_GS gs;
            size_t nr;

            nr = fread(&gs, sizeof(gs), 1, hdr->fp);
            if ( nr != 1 )
               return NTV2_ERR_IOERR;

            NTV2_SWAPF(&gs.f_lat_shift, 1);
            NTV2_SWAPF(&gs.f_lon_shift, 1);

            rec->shifts[j][NTV2_COORD_LAT] = gs.f_lat_shift;
            rec->shifts[j][NTV2_COORD_LON] = gs.f_lon_shift;

            if ( rec->accurs != NTV2_NULL )
            {
               NTV2_SWAPF(&gs.f_lat_accuracy, 1);
               NTV2_SWAPF(&gs.f_lon_accuracy, 1);

               rec->accurs[j][NTV2_COORD_LAT] = gs.f_lat_accuracy;
               rec->accurs[j][NTV2_COORD_LON] = gs.f_lon_accuracy;
            }

            j++;
         }

         if ( rec->wskip > 0 )
            fseek(hdr->fp, rec->wskip, SEEK_CUR);
      }
   }

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Load a binary NTv2 file into memory.
 *
 * This routine combines all the lower-level read routines into one.
 */
static NTV2_HDR * ntv2_load_file_bin(
   const char *  ntv2file,
   NTV2_BOOL     keep_orig,
   NTV2_BOOL     read_data,
   NTV2_EXTENT * extent,
   int *         prc)
{
   NTV2_HDR * hdr = NTV2_NULL;
   int rc = NTV2_ERR_OK;

   hdr = ntv2_create(ntv2file, NTV2_FILE_TYPE_BIN, prc);
   if ( hdr == NTV2_NULL )
   {
      return NTV2_NULL;
   }

   hdr->keep_orig      = keep_orig;

   if ( rc == NTV2_ERR_OK )
   {
      rc = ntv2_read_hdrs_bin(hdr);
   }

   if ( rc == NTV2_ERR_OK && extent != NTV2_NULL )
   {
      rc = ntv2_process_extent(hdr, extent);
   }

   if ( rc != NTV2_ERR_OK )
   {
      fclose(hdr->fp);
      hdr->fp = NTV2_NULL;
   }
   else
   {
      if ( read_data )
      {
         rc = ntv2_read_data_bin(hdr);

         /* done with the file whether successful or not */
         fclose(hdr->fp);
         hdr->fp = NTV2_NULL;
      }
      else
      {
         if ( hdr->file_type == NTV2_FILE_TYPE_BIN )
         {
            hdr->mutex = (void *)ntv2_mutex_create();
         }
         else
         {
            /* No reading on-the-fly if it's an ascii file. */
            fclose(hdr->fp);
            hdr->fp = NTV2_NULL;
         }
      }

   }

   *prc = rc;
   return hdr;
}

/* ------------------------------------------------------------------------- */
/* NTv2 ascii read routines                                                  */
/* ------------------------------------------------------------------------- */

#define RT(n)    if ( ntv2_read_toks(hdr->fp, &tok, n) <= 0 ) \
                    return NTV2_ERR_UNEXPECTED_EOF

#define TOK(i)   tok.toks[i]

/*------------------------------------------------------------------------
 * Read a ascii overview record.
 */
static int ntv2_read_ov_asc(
   NTV2_HDR     *hdr,
   NTV2_FILE_OV *ov)
{
   NTV2_TOKEN tok;

   /* -------- NUM_OREC */

   RT(2);
   ntv2_strcpy_pad(ov->n_num_orec,  TOK(0));
   ov->i_num_orec =            atoi(TOK(1));
   ov->p_num_orec = 0;

   if ( ov->i_num_orec != 11 )
      return NTV2_ERR_INVALID_NUM_OREC;

   /* -------- NUM_SREC */

   RT(2);
   ntv2_strcpy_pad(ov->n_num_srec,  TOK(0));
   ov->i_num_srec =            atoi(TOK(1));
   ov->p_num_srec = 0;

   if ( ov->i_num_srec != 11 )
      return NTV2_ERR_INVALID_NUM_SREC;

   /* -------- NUM_FILE */

   RT(2);
   ntv2_strcpy_pad(ov->n_num_file,  TOK(0));
   ov->i_num_file =            atoi(TOK(1));
   ov->p_num_file = 0;

   if ( ov->i_num_file <= 0 )
      return NTV2_ERR_INVALID_NUM_FILE;
   hdr->num_recs = ov->i_num_file;

   /* -------- GS_TYPE */

   RT(2);
   ntv2_strcpy_pad(ov->n_gs_type,   TOK(0));
   ntv2_strcpy_pad(ov->s_gs_type,   TOK(1));

   /* -------- VERSION */

   RT(2);
   ntv2_strcpy_pad(ov->n_version,   TOK(0));
   ntv2_strcpy_pad(ov->s_version,   TOK(1));

   /* -------- SYSTEM_F */

   RT(2);
   ntv2_strcpy_pad(ov->n_system_f,  TOK(0));
   ntv2_strcpy_pad(ov->s_system_f,  TOK(1));

   /* -------- SYSTEM_T */

   RT(2);
   ntv2_strcpy_pad(ov->n_system_t,  TOK(0));
   ntv2_strcpy_pad(ov->s_system_t,  TOK(1));

   /* -------- MAJOR_F */

   RT(2);
   ntv2_strcpy_pad(ov->n_major_f,   TOK(0));
   ov->d_major_f =        ntv2_atod(TOK(1));

   /* -------- MINOR_F */

   RT(2);
   ntv2_strcpy_pad(ov->n_minor_f,   TOK(0));
   ov->d_minor_f =        ntv2_atod(TOK(1));

   /* -------- MAJOR_T */

   RT(2);
   ntv2_strcpy_pad(ov->n_major_t,   TOK(0));
   ov->d_major_t =        ntv2_atod(TOK(1));

   /* -------- MINOR_T */

   RT(2);
   ntv2_strcpy_pad(ov->n_minor_t,   TOK(0));
   ov->d_minor_t =        ntv2_atod(TOK(1));

   /* -------- get the conversion */

   ntv2_cleanup_str(hdr, hdr->gs_type, ov->s_gs_type, FALSE);

   if      ( strncmp(hdr->gs_type, "SECONDS ", NTV2_NAME_LEN) == 0 )
   {
      hdr->hdr_conv = ( 1.0 / 3600.0 );
      hdr->dat_conv = ( 1.0          );
   }
   else if ( strncmp(hdr->gs_type, "MINUTES ", NTV2_NAME_LEN) == 0 )
   {
      hdr->hdr_conv = ( 1.0 / 60.0   );
      hdr->dat_conv = ( 60.0         );
   }
   else if ( strncmp(hdr->gs_type, "DEGREES ", NTV2_NAME_LEN) == 0 )
   {
      hdr->hdr_conv = ( 1.0          );
      hdr->dat_conv = ( 3600.0       );
   }
   else
   {
      return NTV2_ERR_INVALID_GS_TYPE;
   }

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Read a ascii subfile record.
 */
static int ntv2_read_sf_asc(
   NTV2_HDR     *hdr,
   NTV2_FILE_SF *sf)
{
   NTV2_TOKEN tok;

   /* -------- SUB_NAME */

   RT(2);
   ntv2_strcpy_pad(sf->n_sub_name,  TOK(0));
   ntv2_strcpy_pad(sf->s_sub_name,  TOK(1));

   /* -------- PARENT */

   RT(2);
   ntv2_strcpy_pad(sf->n_parent,    TOK(0));
   ntv2_strcpy_pad(sf->s_parent,    TOK(1));

   /* -------- CREATED */

   RT(2);
   ntv2_strcpy_pad(sf->n_created,   TOK(0));
   ntv2_strcpy_pad(sf->s_created,   TOK(1));

   /* -------- UPDATED */

   RT(2);
   ntv2_strcpy_pad(sf->n_updated,   TOK(0));
   ntv2_strcpy_pad(sf->s_updated,   TOK(1));

   /* -------- S_LAT */

   RT(2);
   ntv2_strcpy_pad(sf->n_s_lat,     TOK(0));
   sf->d_s_lat =          ntv2_atod(TOK(1));

   /* -------- N_LAT */

   RT(2);
   ntv2_strcpy_pad(sf->n_n_lat,     TOK(0));
   sf->d_n_lat =          ntv2_atod(TOK(1));

   /* -------- E_LONG */

   RT(2);
   ntv2_strcpy_pad(sf->n_e_lon,     TOK(0));
   sf->d_e_lon =          ntv2_atod(TOK(1));

   /* -------- W_LONG */

   RT(2);
   ntv2_strcpy_pad(sf->n_w_lon,     TOK(0));
   sf->d_w_lon =          ntv2_atod(TOK(1));

   /* -------- LAT_INC */

   RT(2);
   ntv2_strcpy_pad(sf->n_lat_inc,   TOK(0));
   sf->d_lat_inc =        ntv2_atod(TOK(1));

   /* -------- LONG_INC */

   RT(2);
   ntv2_strcpy_pad(sf->n_lon_inc,   TOK(0));
   sf->d_lon_inc =        ntv2_atod(TOK(1));

   /* -------- GS_COUNT */

   RT(2);
   ntv2_strcpy_pad(sf->n_gs_count,  TOK(0));
   sf->i_gs_count =            atoi(TOK(1));
   sf->p_gs_count = 0;

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Read in ascii shift data
 *
 * Note that it is OK if only shift data is on a line (i.e. no
 * accuracy data).  In that case, the accuracies are set to 0.
 */
static int ntv2_read_data_asc(
   NTV2_HDR *hdr,
   NTV2_REC *rec,
   NTV2_BOOL read_data)
{
   NTV2_TOKEN tok;
   int i;

   /* allocate our arrays */

   if ( read_data )
   {
      {
         rec->shifts = (NTV2_SHIFT *)
                       ntv2_memalloc(sizeof(*rec->shifts) * rec->num);
         if ( rec->shifts == NTV2_NULL )
            return NTV2_ERR_NO_MEMORY;
      }

      if ( hdr->keep_orig )
      {
         rec->accurs = (NTV2_SHIFT *)
                       ntv2_memalloc(sizeof(*rec->accurs) * rec->num);
         if ( rec->accurs == NTV2_NULL )
            return NTV2_ERR_NO_MEMORY;
      }
   }

   /* now read the data */

   for (i = 0; i < rec->num; i++)
   {
      RT(4);

      if ( tok.num == 2 || tok.num == 4 )
      {
         if ( read_data )
         {
            rec->shifts[i][NTV2_COORD_LAT] = (float)ntv2_atod(TOK(0));
            rec->shifts[i][NTV2_COORD_LON] = (float)ntv2_atod(TOK(1));

            if ( rec->accurs != NTV2_NULL )
            {
               /* Note that if there are only 2 tokens in the line,
                  these token pointers will point to empty strings.
               */
               rec->accurs[i][NTV2_COORD_LAT] = (float)ntv2_atod(TOK(2));
               rec->accurs[i][NTV2_COORD_LON] = (float)ntv2_atod(TOK(3));
            }
         }
      }
      else
      {
         return NTV2_ERR_INVALID_LINE;
      }
   }

   return NTV2_ERR_OK;
}

/*------------------------------------------------------------------------
 * Read a ascii end record.
 */
static int ntv2_read_er_asc(
   NTV2_HDR *hdr)
{
   NTV2_TOKEN tok;
   int num;
   int rc = 0;

   num = ntv2_read_toks(hdr->fp, &tok, 1);
   if ( num <= 0 )
   {
      hdr->fixed |= NTV2_FIX_END_REC_NOT_FOUND;
   }
   else
   {
      if ( ntv2_strcmp_i(tok.toks[0], "END") != 0 )
         hdr->fixed |= NTV2_FIX_END_REC_NOT_FOUND;
   }

   return rc;
}

/*------------------------------------------------------------------------
 * Load a NTv2 ascii file into memory.
 */
static NTV2_HDR * ntv2_load_file_asc(
   const char *  ntv2file,
   NTV2_BOOL     keep_orig,
   NTV2_BOOL     read_data,
   NTV2_EXTENT * extent,
   int *         prc)
{
   NTV2_HDR *hdr;
   NTV2_FILE_OV  ov_rec;
   NTV2_FILE_SF  sf_rec;
   int i;
   int rc;

   NTV2_UNUSED_PARAMETER(extent);

   hdr = ntv2_create(ntv2file, NTV2_FILE_TYPE_ASC, prc);
   if ( hdr == NTV2_NULL )
   {
      return NTV2_NULL;
   }

   hdr->keep_orig = keep_orig;

   /* -------- read in the overview record */

   rc = ntv2_read_ov_asc(hdr, &ov_rec);
   if ( rc != NTV2_ERR_OK )
   {
      ntv2_delete(hdr);
      *prc = rc;
      return NTV2_NULL;
   }

   hdr->num_parents = 0;
   hdr->recs        = (NTV2_REC *)
                      ntv2_memalloc(sizeof(*hdr->recs) * hdr->num_recs);
   if ( hdr->recs == NTV2_NULL )
   {
      ntv2_delete(hdr);
      *prc = NTV2_ERR_NO_MEMORY;
      return NTV2_NULL;
   }

   memset(hdr->recs, 0, sizeof(*hdr->recs) * hdr->num_recs);

   /* -------- create the file record cache if wanted */

   if ( hdr->keep_orig )
   {
      hdr->overview = (NTV2_FILE_OV *)
                      ntv2_memalloc(sizeof(*hdr->overview));
      if ( hdr->overview == NTV2_NULL )
      {
         ntv2_delete(hdr);
         *prc = NTV2_ERR_NO_MEMORY;
         return NTV2_NULL;
      }

      memcpy(hdr->overview, &ov_rec, sizeof(*hdr->overview));

      hdr->subfiles = (NTV2_FILE_SF *)
                      ntv2_memalloc(sizeof(*hdr->subfiles) * hdr->num_recs);
      if ( hdr->subfiles == NTV2_NULL )
      {
         ntv2_delete(hdr);
         *prc = NTV2_ERR_NO_MEMORY;
         return NTV2_NULL;
      }
   }

   /* -------- read in all subfile records */

   for (i = 0; i < hdr->num_recs; i++)
   {
      rc = ntv2_read_sf_asc(hdr, &sf_rec);
      if ( rc == NTV2_ERR_OK )
      {
         if ( hdr->subfiles != NTV2_NULL )
            memcpy(&hdr->subfiles[i], &sf_rec, sizeof(*hdr->subfiles));

         rc = ntv2_sf_to_rec(hdr, &sf_rec, i);
         if ( rc == NTV2_ERR_OK )
            rc = ntv2_read_data_asc(hdr, hdr->recs + i, read_data);
      }

      if ( rc != NTV2_ERR_OK )
      {
         ntv2_delete(hdr);
         *prc = rc;
         return NTV2_NULL;
      }
   }

   /* -------- read in the end record */

   ntv2_read_er_asc(hdr);

   /* -------- adjust all pointers */

   rc = ntv2_fix_ptrs(hdr);
   if ( rc != NTV2_ERR_OK )
   {
      ntv2_delete(hdr);
      hdr = NTV2_NULL;
   }

   *prc = rc;
   return hdr;
}

/* ------------------------------------------------------------------------- */
/* NTv2 read routines                                                        */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Load a NTv2 file into memory.
 */
NTV2_HDR * ntv2_load_file(
   const char *  ntv2file,
   NTV2_BOOL     keep_orig,
   NTV2_BOOL     read_data,
   NTV2_EXTENT * extent,
   int *         prc)
{
   NTV2_HDR * hdr;
   int        type;
   int        rc;

   if ( prc == NTV2_NULL )
      prc = &rc;
   *prc = NTV2_ERR_OK;

   if ( ntv2file == NTV2_NULL || *ntv2file == 0 )
   {
      *prc = NTV2_ERR_NULL_PATH;
      return NTV2_NULL;
   }

   type = ntv2_filetype(ntv2file);
   switch (type)
   {
      case NTV2_FILE_TYPE_ASC:
         hdr = ntv2_load_file_asc(ntv2file,
                                  keep_orig,
                                  read_data,
                                  extent,
                                  prc);
         break;

      case NTV2_FILE_TYPE_BIN:
         hdr = ntv2_load_file_bin(ntv2file,
                                  keep_orig,
                                  read_data,
                                  extent,
                                  prc);
         break;

      default:
         hdr  = NTV2_NULL;
         *prc = NTV2_ERR_UNKNOWN_FILE_TYPE;
         break;
   }

   return hdr;
}

/* ------------------------------------------------------------------------- */
/* NTv2 binary write routines                                                */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Write a binary overview record
 */
static void ntv2_write_ov_bin(
   FILE     *fp,
   NTV2_HDR *hdr,
   NTV2_BOOL swap_data)
{
   NTV2_FILE_OV   ov;
   NTV2_FILE_OV * pov;

   ntv2_validate_ov(hdr, NTV2_NULL, 0);

   pov = hdr->overview;
   if ( swap_data )
   {
      memcpy(&ov, pov, sizeof(ov));
      ntv2_swap_int( &ov.i_num_orec, 1);
      ntv2_swap_int( &ov.i_num_srec, 1);
      ntv2_swap_int( &ov.i_num_file, 1);
      ntv2_swap_dbl( &ov.d_major_f,  1);
      ntv2_swap_dbl( &ov.d_minor_f,  1);
      ntv2_swap_dbl( &ov.d_major_t,  1);
      ntv2_swap_dbl( &ov.d_minor_t,  1);
      pov = &ov;
   }

   fwrite(pov, sizeof(*pov), 1, fp);
}

/*------------------------------------------------------------------------
 * Write a binary sub-file record and its children.
 */
static void ntv2_write_sf_bin(
   FILE     *fp,
   NTV2_HDR *hdr,
   NTV2_REC *rec,
   NTV2_BOOL swap_data)
{
   NTV2_REC * sub;
   int i;

   /* this shouldn't happen */
   if ( !rec->active )
      return;

   ntv2_validate_sf(hdr, NTV2_NULL, rec->rec_num, 0);

   /* write the sub-file header */
   {
      NTV2_FILE_SF   sf;
      NTV2_FILE_SF * psf;

      psf = &hdr->subfiles[rec->rec_num];
      if ( swap_data )
      {
         memcpy(&sf, psf, sizeof(sf));
         ntv2_swap_dbl( &sf.d_s_lat,    1);
         ntv2_swap_dbl( &sf.d_n_lat,    1);
         ntv2_swap_dbl( &sf.d_e_lon,    1);
         ntv2_swap_dbl( &sf.d_w_lon,    1);
         ntv2_swap_dbl( &sf.d_lat_inc,  1);
         ntv2_swap_dbl( &sf.d_lon_inc,  1);
         ntv2_swap_int( &sf.i_gs_count, 1);
         psf = &sf;
      }
      fwrite(psf, sizeof(*psf), 1, fp);
   }

   /* write all grid-shift records */
   for (i = 0; i < rec->num; i++)
   {
      NTV2_FILE_GS gs;

      gs.f_lat_shift = rec->shifts[i][NTV2_COORD_LAT];
      gs.f_lon_shift = rec->shifts[i][NTV2_COORD_LON];

      if ( rec->accurs != NTV2_NULL )
      {
         gs.f_lat_accuracy = rec->accurs[i][NTV2_COORD_LAT];
         gs.f_lon_accuracy = rec->accurs[i][NTV2_COORD_LON];
      }
      else
      {
         gs.f_lat_accuracy = 0.0;
         gs.f_lon_accuracy = 0.0;
      }

      if ( swap_data )
      {
         ntv2_swap_flt((float *)&gs, 4);
      }

      fwrite(&gs, sizeof(gs), 1, fp);
   }

   /* recursively write all its children */
   for (sub = rec->sub; sub != NTV2_NULL; sub = sub->next)
   {
      ntv2_write_sf_bin(fp, hdr, sub, swap_data);
   }
}

/*------------------------------------------------------------------------
 * Write a binary end record
 */
static void ntv2_write_end_bin(
   FILE     *fp)
{
   NTV2_FILE_END end_rec;

   strncpy(end_rec.n_end,  NTV2_END_NAME,  NTV2_NAME_LEN);
   memcpy (end_rec.filler, NTV2_ALL_ZEROS, NTV2_NAME_LEN);
   fwrite(&end_rec, sizeof(end_rec), 1, fp);
}

/*------------------------------------------------------------------------
 * Write out the NTV2_HDR object to a binary file.
 */
static int ntv2_write_file_bin(
   NTV2_HDR   *hdr,
   const char *path,
   int         byte_order)
{
   FILE *    fp;
   NTV2_BOOL swap_data;
   int       rc;

   swap_data = hdr->swap_data;
   switch (byte_order)
   {
      case NTV2_ENDIAN_BIG:    swap_data ^= !ntv2_is_big_endian(); break;
      case NTV2_ENDIAN_LITTLE: swap_data ^= !ntv2_is_ltl_endian(); break;
      case NTV2_ENDIAN_NATIVE: swap_data  = FALSE;                 break;
   }

   fp = fopen(path, "wb");
   if ( fp == NTV2_NULL )
   {
      rc = NTV2_ERR_CANNOT_OPEN_FILE;
   }
   else
   {
      NTV2_REC * sf;

      ntv2_write_ov_bin(fp, hdr, swap_data);

      for (sf = hdr->first_parent; sf != NTV2_NULL; sf = sf->next)
         ntv2_write_sf_bin(fp, hdr, sf, swap_data);

      ntv2_write_end_bin(fp);

      rc = (ferror(fp) == 0) ? NTV2_ERR_OK : NTV2_ERR_IOERR;
      fclose(fp);
   }

   return rc;
}

/* ------------------------------------------------------------------------- */
/* NTv2 ascii write routines                                                 */
/* ------------------------------------------------------------------------- */

#define T(x)   ntv2_strip( strncpy(stemp, x, NTV2_NAME_LEN) )
#define D(x)   ntv2_dtoa (         ntemp, x)

/*------------------------------------------------------------------------
 * Write a ascii overview record
 */
static void ntv2_write_ov_asc(
   FILE     *fp,
   NTV2_HDR *hdr)
{
   NTV2_FILE_OV * pov = hdr->overview;
   char stemp[NTV2_NAME_LEN+1];
   char ntemp[32];

   ntv2_validate_ov(hdr, NTV2_NULL, 0);
   stemp[NTV2_NAME_LEN] = 0;

   fprintf(fp, "NUM_OREC %d\n",   pov->i_num_orec  );
   fprintf(fp, "NUM_SREC %d\n",   pov->i_num_srec  );
   fprintf(fp, "NUM_FILE %d\n",   pov->i_num_file  );
   fprintf(fp, "GS_TYPE  %s\n", T(pov->s_gs_type)  );
   fprintf(fp, "VERSION  %s\n", T(pov->s_version)  );
   fprintf(fp, "SYSTEM_F %s\n", T(pov->s_system_f) );
   fprintf(fp, "SYSTEM_T %s\n", T(pov->s_system_t) );
   fprintf(fp, "MAJOR_F  %s\n", D(pov->d_major_f)  );
   fprintf(fp, "MINOR_F  %s\n", D(pov->d_minor_f)  );
   fprintf(fp, "MAJOR_T  %s\n", D(pov->d_major_t)  );
   fprintf(fp, "MINOR_T  %s\n", D(pov->d_minor_t)  );
}

/*------------------------------------------------------------------------
 * Write a ascii sub-file record and its children.
 */
static void ntv2_write_sf_asc(
   FILE     *fp,
   NTV2_HDR *hdr,
   NTV2_REC *rec)
{
   NTV2_REC * sub;
   char stemp[NTV2_NAME_LEN+1];
   char ntemp[32];
   int i;

   /* this shouldn't happen */
   if ( !rec->active )
      return;

   ntv2_validate_sf(hdr, NTV2_NULL, rec->rec_num, 0);
   stemp[NTV2_NAME_LEN] = 0;

   /* write the sub-file header */
   {
      NTV2_FILE_SF * psf = &hdr->subfiles[rec->rec_num];

      fprintf(fp, "\n");
      fprintf(fp, "SUB_NAME %s\n", T(psf->s_sub_name) );
      fprintf(fp, "PARENT   %s\n", T(psf->s_parent)   );
      fprintf(fp, "CREATED  %s\n", T(psf->s_created)  );
      fprintf(fp, "UPDATED  %s\n", T(psf->s_updated)  );
      fprintf(fp, "S_LAT    %s\n", D(psf->d_s_lat)    );
      fprintf(fp, "N_LAT    %s\n", D(psf->d_n_lat)    );
      fprintf(fp, "E_LONG   %s\n", D(psf->d_e_lon)    );
      fprintf(fp, "W_LONG   %s\n", D(psf->d_w_lon)    );
      fprintf(fp, "LAT_INC  %s\n", D(psf->d_lat_inc)  );
      fprintf(fp, "LONG_INC %s\n", D(psf->d_lon_inc)  );
      fprintf(fp, "GS_COUNT %d\n",   psf->i_gs_count  );
      fprintf(fp, "\n");
   }

   /* write all grid-shift records */
   for (i = 0; i < rec->num; i++)
   {
      NTV2_FILE_GS gs;

      gs.f_lat_shift    = rec->shifts[i][NTV2_COORD_LAT];
      gs.f_lon_shift    = rec->shifts[i][NTV2_COORD_LON];

      if ( rec->accurs != NTV2_NULL )
      {
         gs.f_lat_accuracy = rec->accurs[i][NTV2_COORD_LAT];
         gs.f_lon_accuracy = rec->accurs[i][NTV2_COORD_LON];
      }
      else
      {
         gs.f_lat_accuracy = 0.0;
         gs.f_lon_accuracy = 0.0;
      }

      fprintf(fp, "%-16s", D(gs.f_lat_shift)    );
      fprintf(fp, "%-16s", D(gs.f_lon_shift)    );
      fprintf(fp, "%-16s", D(gs.f_lat_accuracy) );
      fprintf(fp, "%s\n",  D(gs.f_lon_accuracy) );
   }

   /* recursively write all its children */
   for (sub = rec->sub; sub != NTV2_NULL; sub = sub->next)
   {
      ntv2_write_sf_asc(fp, hdr, sub);
   }
}

/*------------------------------------------------------------------------
 * Write a ascii end record
 */
static void ntv2_write_end_asc(
   FILE     *fp)
{
   fprintf(fp, "\n");
   fprintf(fp, "END\n");
}

/*------------------------------------------------------------------------
 * Write out the NTV2_HDR object to an ascii file.
 */
static int ntv2_write_file_asc(
   NTV2_HDR   *hdr,
   const char *path)
{
   FILE *  fp;
   int     rc;

   fp = fopen(path, "w");
   if ( fp == NTV2_NULL )
   {
      rc = NTV2_ERR_CANNOT_OPEN_FILE;
   }
   else
   {
      NTV2_REC * sf;

      ntv2_write_ov_asc(fp, hdr);

      for (sf = hdr->first_parent; sf != NTV2_NULL; sf = sf->next)
         ntv2_write_sf_asc(fp, hdr, sf);

      ntv2_write_end_asc(fp);

      rc = (ferror(fp) == 0) ? NTV2_ERR_OK : NTV2_ERR_IOERR;
      fclose(fp);
   }

   return rc;
}

/* ------------------------------------------------------------------------- */
/* NTv2 write routines                                                       */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Write out the NTV2_HDR object to a file.
 */
int ntv2_write_file(
   NTV2_HDR   *hdr,
   const char *path,
   NTV2_BOOL   byte_order)
{
   int type;
   int rc;

   if ( hdr == NTV2_NULL )
   {
      return NTV2_ERR_NULL_HDR;
   }

   if ( path == NTV2_NULL || *path == 0 )
   {
      return NTV2_ERR_NULL_PATH;
   }

   if ( hdr->num_recs == 0 )
   {
      return NTV2_ERR_HDRS_NOT_READ;
   }

   if ( !hdr->keep_orig )
   {
      return NTV2_ERR_ORIG_DATA_NOT_KEPT;
   }

   if ( (hdr->first_parent)->shifts == NTV2_NULL )
   {
      return NTV2_ERR_DATA_NOT_READ;
   }

   type = ntv2_filetype(path);
   switch (type)
   {
      case NTV2_FILE_TYPE_ASC:
         rc = ntv2_write_file_asc(hdr, path);
         break;

      case NTV2_FILE_TYPE_BIN:
         rc = ntv2_write_file_bin(hdr, path, byte_order);
         break;

      default:
         rc = NTV2_ERR_UNKNOWN_FILE_TYPE;
         break;
   }

   return rc;
}

/* ------------------------------------------------------------------------- */
/* NTv2 dump routines                                                        */
/* ------------------------------------------------------------------------- */

#define T1(x)   ntv2_strip( strncpy(stemp1, x, NTV2_NAME_LEN) )
#define T2(x)   ntv2_strip( strncpy(stemp2, x, NTV2_NAME_LEN) )
#define D1(x)   ntv2_dtoa (         ntemp1, x)
#define D2(x)   ntv2_dtoa (         ntemp2, x)
#define CV(x)   D2(x * hdr->hdr_conv)

/*------------------------------------------------------------------------
 * Dump the file overview struct to a stream.
 */
static void ntv2_dump_ov(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             mode)
{
   const NTV2_FILE_OV * ov = hdr->overview;
   char   stemp1[NTV2_NAME_LEN+1];
   char   stemp2[NTV2_NAME_LEN+1];
   char   ntemp1[32];

   if ( ov == NTV2_NULL )
      return;

   if ( (mode & NTV2_DUMP_HDRS_EXT) == 0 )
      return;

   stemp1[NTV2_NAME_LEN] = 0;
   stemp2[NTV2_NAME_LEN] = 0;

   fprintf(fp, "filename %s\n", hdr->path);
   fprintf(fp, "========  hdr ===================================\n");

   fprintf(fp, "%-8s %d\n", T1(ov->n_num_orec),    ov->i_num_orec  );
   fprintf(fp, "%-8s %d\n", T1(ov->n_num_srec),    ov->i_num_srec  );
   fprintf(fp, "%-8s %d\n", T1(ov->n_num_file),    ov->i_num_file  );
   fprintf(fp, "%-8s %s\n", T1(ov->n_gs_type ), T2(ov->s_gs_type)  );
   fprintf(fp, "%-8s %s\n", T1(ov->n_version ), T2(ov->s_version)  );
   fprintf(fp, "%-8s %s\n", T1(ov->n_system_f), T2(ov->s_system_f) );
   fprintf(fp, "%-8s %s\n", T1(ov->n_system_t), T2(ov->s_system_t) );
   fprintf(fp, "%-8s %s\n", T1(ov->n_major_f ), D1(ov->d_major_f)  );
   fprintf(fp, "%-8s %s\n", T1(ov->n_minor_f ), D1(ov->d_minor_f)  );
   fprintf(fp, "%-8s %s\n", T1(ov->n_major_t ), D1(ov->d_major_t)  );
   fprintf(fp, "%-8s %s\n", T1(ov->n_minor_t ), D1(ov->d_minor_t)  );

   fprintf(fp, "\n");
}

/*------------------------------------------------------------------------
 * Dump a file sub-file struct to a stream.
 */
static void ntv2_dump_sf(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             n,
   int             mode)
{
   const NTV2_FILE_SF * sf;
   char   stemp1[NTV2_NAME_LEN+1];
   char   stemp2[NTV2_NAME_LEN+1];
   char   ntemp1[32];
   char   ntemp2[32];

   if ( (mode & NTV2_DUMP_HDRS_EXT)     == 0 )
      return;

   if ( (mode & NTV2_DUMP_HDRS_SUMMARY) != 0 )
      return;

   if ( !hdr->recs[n].active )
      return;

   if ( hdr->subfiles == NTV2_NULL )
      return;

   sf = hdr->subfiles+n;

   stemp1[NTV2_NAME_LEN] = 0;
   stemp2[NTV2_NAME_LEN] = 0;

   fprintf(fp, "======== %4d ===================================\n", n);

   fprintf(fp, "%-8s %s\n",               T1(sf->n_sub_name),
                                          T2(sf->s_sub_name) );

   fprintf(fp, "%-8s %s\n",               T1(sf->n_parent),
                                          T2(sf->s_parent)   );

   fprintf(fp, "%-8s %s\n",               T1(sf->n_created),
                                          T2(sf->s_created)  );

   fprintf(fp, "%-8s %s\n",               T1(sf->n_updated),
                                          T2(sf->s_updated)  );

   fprintf(fp, "%-8s %-14s [ %13s ]\n",   T1(sf->n_s_lat),
                                          D1(sf->d_s_lat),
                                          CV(sf->d_s_lat)    );

   fprintf(fp, "%-8s %-14s [ %13s ]\n",   T1(sf->n_n_lat),
                                          D1(sf->d_n_lat),
                                          CV(sf->d_n_lat)    );

   fprintf(fp, "%-8s %-14s [ %13s ]\n",   T1(sf->n_e_lon),
                                          D1(sf->d_e_lon),
                                          CV(sf->d_e_lon)    );

   fprintf(fp, "%-8s %-14s [ %13s ]\n",   T1(sf->n_w_lon),
                                          D1(sf->d_w_lon),
                                          CV(sf->d_w_lon)    );

   fprintf(fp, "%-8s %-14s [ %13s ]\n",   T1(sf->n_lat_inc),
                                          D1(sf->d_lat_inc),
                                          CV(sf->d_lat_inc)  );

   fprintf(fp, "%-8s %-14s [ %13s ]\n",   T1(sf->n_lon_inc),
                                          D1(sf->d_lon_inc),
                                          CV(sf->d_lon_inc)  );

   fprintf(fp, "%-8s %d\n", T1(sf->n_gs_count), sf->i_gs_count  );

   fprintf(fp, "\n");
}

/*------------------------------------------------------------------------
 * Dump the internal overview header struct to a stream.
 */
static void ntv2_dump_hdr(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             mode)
{
   char ntemp1[32];

   if ( (mode & NTV2_DUMP_HDRS_INT    ) == 0 )
      return;

   if ( (mode & NTV2_DUMP_HDRS_SUMMARY) != 0 )
      return;

   if ( (mode & NTV2_DUMP_HDRS_EXT) == 0 )
   {
      fprintf(fp, "filename    %s\n",  hdr->path);
      fprintf(fp, "========  overview ==============\n");
   }

   fprintf(fp, "num-recs    %6d\n", hdr->num_recs);
   fprintf(fp, "num-parents %6d\n", hdr->num_parents);
   if ( hdr->num_parents > 0 )
   {
     NTV2_REC * rec = hdr->first_parent;
     const char * t = "parents  ";

     while ( rec != NTV2_NULL )
     {
        fprintf(fp, "%-11s %4d [ %s ]\n", t, rec->rec_num, rec->record_name);
        rec = rec->next;
        t = "";
     }
   }

   fprintf(fp, "conversion  %s  [ %s ]\n",
      D1(hdr->hdr_conv), hdr->gs_type);

   fprintf(fp, "\n");
}

/*------------------------------------------------------------------------
 * Dump an internal sub-file record struct to a stream.
 */
static void ntv2_dump_rec(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             n,
   int             mode)
{
   const NTV2_REC * rec = hdr->recs+n;
   char ntemp1[32];

   if ( (mode & NTV2_DUMP_HDRS_INT)     == 0 )
      return;

   if ( (mode & NTV2_DUMP_HDRS_SUMMARY) != 0 )
      return;

   if ( !rec->active )
      return;

   if ( (mode & NTV2_DUMP_HDRS_EXT) == 0 )
   {
      fprintf(fp, "======== sub-file %d ==============\n", n);
   }

   fprintf(fp, "name        %s\n",    rec->record_name);
   fprintf(fp, "parent      %s",      rec->parent_name);

   if ( rec->parent != NTV2_NULL )
      fprintf(fp, "  [ %d ]", rec->parent->rec_num);
   fprintf(fp, "\n");

   fprintf(fp, "num subs    %d\n",    rec->num_subs);

   if ( rec->sub != NTV2_NULL )
   {
     const NTV2_REC * sub = rec->sub;
     const char * t = "sub files";

     while ( sub != NTV2_NULL )
     {
        fprintf(fp, "%-11s %4d [ %s ]\n", t, sub->rec_num, sub->record_name);
        sub = sub->next;
        t = "";
     }
   }

   fprintf(fp, "num recs    %d\n",    rec->num);
   fprintf(fp, "num cols    %d\n",    rec->ncols);
   fprintf(fp, "num rows    %d\n",    rec->nrows);

   fprintf(fp, "lat min     %s\n", D1(rec->lat_min));
   fprintf(fp, "lat max     %s\n", D1(rec->lat_max));
   fprintf(fp, "lat inc     %s\n", D1(rec->lat_inc));
   fprintf(fp, "lon min     %s\n", D1(rec->lon_min));
   fprintf(fp, "lon max     %s\n", D1(rec->lon_max));
   fprintf(fp, "lon inc     %s\n", D1(rec->lon_inc));

   fprintf(fp, "\n");
}

/*------------------------------------------------------------------------
 * Dump the data for a subfile.
 */
static void ntv2_dump_data(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             n,
   int             mode)
{
   const NTV2_REC * rec = hdr->recs+n;

   if ( rec->shifts != NTV2_NULL )
   {
      /* GCC needs these casts for some reason... */
      const NTV2_SHIFT * shifts   = (const NTV2_SHIFT *)(rec->shifts);
      const NTV2_SHIFT * accurs   = (const NTV2_SHIFT *)(rec->accurs);
      NTV2_BOOL    dump_acc = FALSE;
      int row, col;

      if ( (mode & NTV2_DUMP_DATA_ACC) == NTV2_DUMP_DATA_ACC &&
           accurs != NTV2_NULL )
      {
         dump_acc = TRUE;
      }

      for (row = 0; row < rec->nrows; row++)
      {
         if ( dump_acc )
         {
            fprintf(fp, "   row     col       lat-shift       lon-shift  "
                        "  lat-accuracy    lon-accuracy\n");
            fprintf(fp, "------  ------  --------------  --------------  "
                        "--------------  --------------\n");
         }
         else
         {
            fprintf(fp, "   row     col        latitude       longitude  "
                        "     lat-shift       lon-shift\n");
            fprintf(fp, "------  ------  --------------  --------------  "
                        "--------------  --------------\n");
         }

         for (col = 0; col < rec->ncols; col++)
         {
            if ( dump_acc )
            {
               fprintf(fp, "%6d  %6d  %14.8f  %14.8f  %14.8f  %14.8f\n",
                  row, col,
                  (double)(*shifts)[NTV2_COORD_LAT],
                  (double)(*shifts)[NTV2_COORD_LON],
                  (double)(*accurs)[NTV2_COORD_LAT],
                  (double)(*accurs)[NTV2_COORD_LON]);
               shifts++;
               accurs++;
            }
            else
            {
               fprintf(fp, "%6d  %6d  %14.8f  %14.8f  %14.8f  %14.8f\n",
                  row, col,
                  rec->lat_min + (rec->lat_inc * row),
                  rec->lon_max - (rec->lon_inc * col),
                  (double)(*shifts)[NTV2_COORD_LAT],
                  (double)(*shifts)[NTV2_COORD_LON]);
               shifts++;
            }
         }

         fprintf(fp, "\n");
      }
   }
}

/*------------------------------------------------------------------------
 * Dump the contents of all headers in an NTv2 file.
 */
void ntv2_dump(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             mode)
{
   if ( hdr != NTV2_NULL && fp != NTV2_NULL )
   {
      int i;

      ntv2_dump_ov (hdr, fp, mode);
      ntv2_dump_hdr(hdr, fp, mode);

      for (i = 0; i < hdr->num_recs; i++)
      {
         ntv2_dump_sf (hdr, fp, i, mode);
         ntv2_dump_rec(hdr, fp, i, mode);

         if ( (mode & NTV2_DUMP_DATA) != 0 )
            ntv2_dump_data(hdr, fp, i, mode);
      }
   }
}

/*------------------------------------------------------------------------
 * List the contents of all headers in an NTv2 file.
 */
void ntv2_list(
   const NTV2_HDR *hdr,
   FILE           *fp,
   int             mode,
   NTV2_BOOL       do_hdr_line)
{
   if ( hdr != NTV2_NULL && fp != NTV2_NULL )
   {
      int i;

      if ( do_hdr_line )
      {
         if ( (mode & NTV2_DUMP_HDRS_SUMMARY) == 0 )
         {
            fprintf(fp, "filename\n");
            fprintf(fp, "  num sub-file par  lon-min  lat-min "
                        " lon-max  lat-max  d-lon  d-lat nrow ncol\n");
            fprintf(fp, "  --- -------- --- -------- -------- "
                        "-------- -------- ------ ------ ---- ----\n");
         }
         else
         {
            fprintf(fp,
               "filename                             num  lon-min"
               "  lat-min  lon-max  lat-max\n");
            fprintf(fp,
               "-----------------------------------  --- --------"
               " -------- -------- --------\n");
         }
      }

      if ( (mode & NTV2_DUMP_HDRS_SUMMARY) == 0 )
         fprintf(fp, "%s\n", hdr->path);

      for (i = 0; i < hdr->num_recs; i++)
      {
         const NTV2_REC * rec = &hdr->recs[i];

         if ( rec->active )
         {
            char parent[4];

            if ( rec->parent == NTV2_NULL )
            {
               strcpy(parent, " --");
            }
            else
            {
                sprintf(parent, "%3d", (rec->parent)->rec_num);
            }

            if ( (mode & NTV2_DUMP_HDRS_SUMMARY) == 0 )
            {
               fprintf(fp,
               "  %3d %-8s %3s %8.3f %8.3f %8.3f %8.3f %6.3f %6.3f %4d %4d\n",
                  i,
                  rec->record_name,
                  parent,
                  rec->lon_min,
                  rec->lat_min,
                  rec->lon_max,
                  rec->lat_max,
                  rec->lon_inc,
                  rec->lat_inc,
                  rec->nrows,
                  rec->ncols);
            }
         }
      }

      if ( (mode & NTV2_DUMP_HDRS_SUMMARY) == 0 )
      {
         if ( hdr->num_recs > 1 )
         {
            fprintf(fp, "  %3s %-8s %3s %8.3f %8.3f %8.3f %8.3f\n",
               "tot",
               "--------",
               "   ",
               hdr->lon_min,
               hdr->lat_min,
               hdr->lon_max,
               hdr->lat_max);
         }
      }
      else
      {
         fprintf(fp, "%-36s %3d %8.3f %8.3f %8.3f %8.3f\n",
            hdr->path,
            hdr->num_recs,
            hdr->lon_min,
            hdr->lat_min,
            hdr->lon_max,
            hdr->lat_max);
      }

      if ( (mode & NTV2_DUMP_HDRS_SUMMARY) == 0 )
         fprintf(fp, "\n");
   }
}

/* ------------------------------------------------------------------------- */
/* NTv2 forward / inverse routines                                           */
/* ------------------------------------------------------------------------- */

/*------------------------------------------------------------------------
 * Find the best ntv2 record containing a given point.
 *
 * The NTv2 specification states that points on the edge of a
 * grid should be processed using the parent grid or be ignored, and
 * points outside of the top-level grid should be ignored completely.
 * However, this creates a situation where a point inside can be
 * shifted to a point outside or to the edge, and then it cannot
 * be shifted back.  Not good.
 *
 * Thus, we pretend that there is a one-cell zone around each grid
 * that has a shift value of zero.  This allows for a point that was
 * shifted out to be properly shifted back, and also allows for
 * a gradual change rather than an abrupt jump.  Much better!
 *
 * However, this creates a problem when two grids are adjacent, either
 * at a corner or along an edge (or both).  In fact, a point on the
 * corner of a grid could be adjacent to up to three other grids!  In this
 * case, a point in the imaginary outside cell of one grid could then
 * magically appear inside another grid and vice-versa.  So we have a lot
 * of esoteric code to deal with those cases.
 */
const NTV2_REC * ntv2_find_rec(
   const NTV2_HDR * hdr,
   double           lon,
   double           lat,
   int            * pstatus)
{
   const NTV2_REC * ps;
   const NTV2_REC * psc      = NTV2_NULL;
   NTV2_BOOL  next_gen = TRUE;
   int n_stat_5_pinged = 0;
   int status_ps       = (NTV2_STATUS_OUTSIDE_CELL + 1);
   int status_psc      = (NTV2_STATUS_OUTSIDE_CELL + 1);
   int status_tmp;
   int npings;

   if ( pstatus == NTV2_NULL )
      pstatus = &status_tmp;

   if ( hdr == NTV2_NULL )
   {
      *pstatus = NTV2_STATUS_NOTFOUND;
      return NTV2_NULL;
   }

   /* First, find the top-level parent that contains this point.
      There can only be one, since top-level parents cannot overlap.
      But we have to deal with edge conditions.
   */

   for (ps = hdr->first_parent; ps != NTV2_NULL; ps = ps->next)
   {
      if ( NTV2_LE(lat, ps->lat_max) &&
           NTV2_GT(lat, ps->lat_min) &&
           NTV2_LE(lon, ps->lon_max) &&
           NTV2_GT(lon, ps->lon_min) )
      {
         /* Total containment */
         psc  = ps;
         status_ps = NTV2_STATUS_CONTAINED;
         break;
      }

      if ( NTV2_LE(lon, ps->lon_max) &&
           NTV2_GT(lon, ps->lon_min) &&
           NTV2_EQ(lat, ps->lat_max) &&
           status_ps > NTV2_STATUS_NORTH )
      {
         /* Border condition - on North limit */
         psc = ps;
         status_ps = NTV2_STATUS_NORTH;
      }

      else
      if ( NTV2_LE(lon, ps->lon_max) &&
           NTV2_GT(lon, ps->lon_min) &&
           NTV2_EQ(lat, ps->lat_max) &&
           status_ps > NTV2_STATUS_WEST )
      {
         /* Border condition - on West limit */
         psc = ps;
         status_ps = NTV2_STATUS_WEST;
      }

      else
      if ( NTV2_EQ(lon, ps->lon_min) &&
           NTV2_EQ(lat, ps->lat_max) &&
           status_ps > NTV2_STATUS_NORTH_WEST )
      {
         /* Border condition - on North & West limit */
         psc = ps;
         status_ps = NTV2_STATUS_NORTH_WEST;
      }

      else
      if ( NTV2_GT(lon, (ps->lon_min - ps->lon_inc)) &&
           NTV2_LT(lon, (ps->lon_max + ps->lon_inc)) &&
           NTV2_GT(lat, (ps->lat_min - ps->lat_inc)) &&
           NTV2_LT(lat, (ps->lat_max + ps->lat_inc)) &&
           status_ps > NTV2_STATUS_OUTSIDE_CELL )
      {
         /* Is it just within one cell outside of the border? */
         status_ps = NTV2_STATUS_OUTSIDE_CELL;

         /* Do a parent counter, and figure out if it is a
            one point corner or two point edge situation.
            Always take the two-pointer over a one-pointer.
            Always take the last two-pointer as the parent.
         */

         npings = 1;
         if ( NTV2_GE(lon, ps->lon_min) && NTV2_LE(lon, ps->lon_max) ) npings++;
         if ( NTV2_GE(lat, ps->lat_min) && NTV2_LE(lat, ps->lat_max) ) npings++;

         if ( npings >= n_stat_5_pinged )
         {
            psc = ps;
            n_stat_5_pinged = npings;
         }
      }
   }

   /* If no parent was found, we're done. */

   if ( psc == NTV2_NULL )
   {
      *pstatus = NTV2_STATUS_NOTFOUND;
      return NTV2_NULL;
   }

   /* If this parent has no children or the point lies outside the
      parent border, we're done. */

   if ( psc->sub == NTV2_NULL || status_ps == NTV2_STATUS_OUTSIDE_CELL )
   {
      *pstatus = status_ps;
      return psc;
   }

   /* Now run recursively through the child grids to find the
      best one.  Again, there can only be one best record, since
      children cannot overlap but can only nest.
      But here also we have to deal with edge conditions.
   */

   ps  = psc;
   psc = NTV2_NULL;
   while ( ps->sub != NTV2_NULL && next_gen )
   {
      next_gen = FALSE;

      for (psc = ps->sub; psc != NTV2_NULL; psc = psc->next)
      {
         if ( NTV2_LE(lon, psc->lon_max) &&
              NTV2_GT(lon, psc->lon_min) &&
              NTV2_GE(lat, psc->lat_min) &&
              NTV2_LT(lat, psc->lat_max) &&
              status_psc >= NTV2_STATUS_CONTAINED )
         {
            /* Total containment */
            next_gen = TRUE;
            ps = psc;
            status_psc = NTV2_STATUS_CONTAINED;
            status_ps  = NTV2_STATUS_CONTAINED;
            break;
         }

         else
         if ( NTV2_LE(lon, psc->lon_max) &&
              NTV2_GT(lon, psc->lon_min) &&
              NTV2_EQ(lat, psc->lat_max) &&
              status_psc >= NTV2_STATUS_NORTH )
         {
            /* Border condition - on North limit */
            next_gen = TRUE;
            ps = psc;
            status_psc = NTV2_STATUS_NORTH;
            status_ps  = NTV2_STATUS_NORTH;
         }

         else
         if ( NTV2_EQ(lon, psc->lon_min) &&
              NTV2_GE(lat, psc->lat_min) &&
              NTV2_LT(lat, psc->lat_max) &&
              status_psc >= NTV2_STATUS_WEST )
         {
            /* Border condition - on West limit */
            next_gen = TRUE;
            ps = psc;
            status_psc = NTV2_STATUS_WEST;
            status_ps  = NTV2_STATUS_WEST;
         }

         else
         if ( NTV2_EQ(lon, psc->lon_min) &&
              NTV2_EQ(lat, psc->lat_max) &&
              status_psc >= NTV2_STATUS_NORTH_WEST )
         {
            /* Border condition - on North & West limit */
            next_gen = TRUE;
            ps = psc;
            status_psc = NTV2_STATUS_NORTH_WEST;
            status_ps  = NTV2_STATUS_NORTH_WEST;
         }
      }
   }

   *pstatus = status_ps;
   return ps;
}

/*------------------------------------------------------------------------
 * Get a lat/lon shift value (either from a file or memory).
 *
 * Note that these routines are called only if a sub-file record
 * was found that contains the point.  Thus we know that the row
 * and column values are valid.  However, it is possible to have
 * not read the data (only the headers) and then closed the file,
 * so we have to check for that.
 */
static double ntv2_get_shift_from_file(
   const NTV2_HDR * hdr,
   const NTV2_REC * rec,
   int              icol,
   int              irow,
   int              coord_type)
{
   float shift = 0.0;
   int   offs  = rec->offset +
                 (rec->ncols * irow + icol) * sizeof(NTV2_FILE_GS) +
                 ((coord_type == NTV2_COORD_LAT) ?
                    NTV2_OFFSET_OF(NTV2_FILE_GS, f_lat_shift) :
                    NTV2_OFFSET_OF(NTV2_FILE_GS, f_lon_shift) );
   size_t nr = 0;

   if ( hdr->fp != NTV2_NULL )
   {
      ntv2_mutex_enter(hdr->mutex);
      {
         fseek(hdr->fp, offs, SEEK_SET);
         nr = fread(&shift, NTV2_SIZE_FLT, 1, hdr->fp);
      }
      ntv2_mutex_leave(hdr->mutex);
   }

   if ( nr != 1 )
   {
      shift = 0.0;
   }
   else
   {
      NTV2_SWAPF(&shift, 1);
   }

   return shift;
}

static double ntv2_get_shift_from_data(
   const NTV2_HDR * hdr,
   const NTV2_REC * rec,
   int              icol,
   int              irow,
   int              coord_type)
{
   double shift;
   int   offs = (irow * rec->ncols) + icol;

   NTV2_UNUSED_PARAMETER(hdr);

   shift = rec->shifts[offs][coord_type];

   return shift;
}

static double ntv2_get_shift(
   const NTV2_HDR * hdr,
   const NTV2_REC * rec,
   int              icol,
   int              irow,
   int              coord_type)
{
   if ( rec->shifts == NTV2_NULL )
      return ntv2_get_shift_from_file(hdr, rec, irow, icol, coord_type);
   else
      return ntv2_get_shift_from_data(hdr, rec, irow, icol, coord_type);
}

/*------------------------------------------------------------------------
 * Calculate one shift (lat or lon).
 *
 * In this routine we deal with our idea of a phantom row/col of
 * zero-shift values along each edge of the top-level-grid.
 */
static double ntv2_calculate_one_shift(
   const NTV2_HDR * hdr,
   const NTV2_REC * rec,
   int              status,
   int              icol,
   int              irow,
   int              move_shifts_horz,
   int              move_shifts_vert,
   double           x_cellfrac,
   double           y_cellfrac,
   int              coord_type)
{
   double ll_shift = 0, lr_shift = 0, ul_shift = 0, ur_shift = 0;
   double b, c, d;
   double shift;

#define GET_SHIFT(i,j)   ntv2_get_shift(hdr, rec, i, j, coord_type)

   /* get the shift values for the "corners" of the cell
      containing the point */

   switch ( status )
   {
      case NTV2_STATUS_CONTAINED:
         lr_shift = GET_SHIFT(irow,   icol  );
         ll_shift = GET_SHIFT(irow,   icol+1);
         ur_shift = GET_SHIFT(irow+1, icol  );
         ul_shift = GET_SHIFT(irow+1, icol+1);
         break;

      case NTV2_STATUS_NORTH:
         lr_shift = GET_SHIFT(irow,   icol  );
         ll_shift = GET_SHIFT(irow,   icol+1);
         ur_shift = lr_shift;
         ul_shift = ll_shift;
         break;

      case NTV2_STATUS_WEST:
         lr_shift = GET_SHIFT(irow,   icol  );
         ll_shift = lr_shift;
         ur_shift = GET_SHIFT(irow+1, icol  );
         ul_shift = ur_shift;
         break;

      case NTV2_STATUS_NORTH_WEST:
         lr_shift = GET_SHIFT(irow,   icol  );
         ll_shift = lr_shift;
         ur_shift = lr_shift;
         ul_shift = lr_shift;
         break;

      case NTV2_STATUS_OUTSIDE_CELL:
         lr_shift = GET_SHIFT(irow,   icol  );
         ll_shift = GET_SHIFT(irow,   icol+1);
         ur_shift = GET_SHIFT(irow+1, icol  );
         ul_shift = GET_SHIFT(irow+1, icol+1);

         if ( move_shifts_horz == -1 )
         {
            lr_shift = ll_shift;
            ur_shift = ul_shift;
            ll_shift = 0.0;
            ul_shift = 0.0;
         }
         if ( move_shifts_horz == +1 )
         {
            ll_shift = lr_shift;
            ul_shift = ur_shift;
            ur_shift = 0.0;
            lr_shift = 0.0;
         }
         if ( move_shifts_vert == -1 )
         {
            ul_shift = ll_shift;
            ur_shift = lr_shift;
            ll_shift = 0.0;
            lr_shift = 0.0;
         }
         if ( move_shifts_vert == +1 )
         {
            ll_shift = ul_shift;
            lr_shift = ur_shift;
            ul_shift = 0.0;
            ur_shift = 0.0;
         }
         break;

      default:
         return 0.0;
   }

#undef GET_SHIFT

   /* do the bilinear interpolation of the corner shift values */

   b = (ll_shift - lr_shift);
   c = (ur_shift - lr_shift);
   d = (ul_shift - ll_shift) - (ur_shift - lr_shift);

   shift = lr_shift + (b * x_cellfrac)
                    + (c * y_cellfrac)
                    + (d * x_cellfrac * y_cellfrac);

   /* The shift at this point is in decimal seconds, so convert to degrees. */
   return (shift * hdr->dat_conv) / 3600.0;
}

/*------------------------------------------------------------------------
 * Calculate the lon & lat shifts for a given lon/lat.
 *
 * In this routine we deal with our idea of a phantom row/col of
 * zero-shift values along each edge of the top-level-grid.
 */
static void ntv2_calculate_shifts(
   const NTV2_HDR * hdr,
   const NTV2_REC * rec,
   double           lon,
   double           lat,
   int              status,
   double *         plon_shift,
   double *         plat_shift)
{
   double xgrid_index, ygrid_index, x_cellfrac, y_cellfrac;
   int    horz = 0, vert = 0;
   int    icol, irow;

   /* lat goes S to N, lon goes E to W */
   xgrid_index = (rec->lon_max - lon) / rec->lon_inc;
   ygrid_index = (lat - rec->lat_min) / rec->lat_inc;

   icol = (int)xgrid_index;
   irow = (int)ygrid_index;

   /* If the status is NTV2_STATUS_OUTSIDE_CELL, it is within one cell of the
      edge of a parent grid.  Have to do something different.
   */
   if ( status == NTV2_STATUS_OUTSIDE_CELL )
   {
      icol = (xgrid_index < 0.0) ? -1 : (int)xgrid_index;
      irow = (ygrid_index < 0.0) ? -1 : (int)ygrid_index;
   }

   x_cellfrac = xgrid_index - icol;
   y_cellfrac = ygrid_index - irow;

   if ( status == NTV2_STATUS_OUTSIDE_CELL )
   {
      horz = (icol < 0) ? +1
                        : (icol > rec->ncols-2) ? -1 : 0;
      vert = (irow < 0) ? -1
                        : (irow > rec->nrows-2) ? +1 : 0;

      icol = (icol < 0) ? 0
                        : (icol > rec->ncols-2) ? rec->ncols-2 : icol;
      irow = (irow < 0) ? 0
                        : (irow > rec->nrows-2) ? rec->nrows-2 : irow;
   }

   /* The longitude shifts are built for (+west/-east) longitude values,
      so we flip the sign of the calculated longitude shift to make
      it standard (-west/+east).
   */

   *plon_shift = -ntv2_calculate_one_shift(hdr, rec, status,
      icol, irow, horz, vert, x_cellfrac, y_cellfrac, NTV2_COORD_LON);

   *plat_shift =  ntv2_calculate_one_shift(hdr, rec, status,
      icol, irow, horz, vert, x_cellfrac, y_cellfrac, NTV2_COORD_LAT);
}

/*------------------------------------------------------------------------
 * Perform a forward transformation on an array of points.
 *
 * Note that the return value is the number of points successfully
 * transformed, and points that can't be transformed (usually because
 * they are outside of the grid) are left unchanged.  However, there
 * is no indication of which points were changed and which were not.
 */
int ntv2_forward(
   const NTV2_HDR *hdr,
   double          deg_factor,
   int             n,
   NTV2_COORD      coord[])
{
   int num = 0;
   int i;

   if ( hdr == NTV2_NULL || coord == NTV2_NULL || n <= 0 )
      return 0;

   if ( deg_factor <= 0.0 )
      deg_factor = 1.0;

   for (i = 0; i < n; i++)
   {
      const NTV2_REC * rec;
      double lon, lat;
      int status;

      lon = (coord[i][NTV2_COORD_LON] * deg_factor);
      lat = (coord[i][NTV2_COORD_LAT] * deg_factor);

      rec = ntv2_find_rec(hdr, lon, lat, &status);
      if ( rec != NTV2_NULL )
      {
         double lon_shift, lat_shift;

         ntv2_calculate_shifts(hdr, rec, lon, lat, status,
            &lon_shift, &lat_shift);

         coord[i][NTV2_COORD_LON] = ((lon + lon_shift) / deg_factor);
         coord[i][NTV2_COORD_LAT] = ((lat + lat_shift) / deg_factor);
         num++;
      }
   }

   return num;
}

/*------------------------------------------------------------------------
 * Perform a inverse transformation on an array of points.
 *
 * Note that the return value is the number of points successfully
 * transformed, and points that can't be transformed (usually because
 * they are outside of the grid) are left unchanged.  However, there
 * is no indication of which points were changed and which were not.
 */
#ifndef   MAX_ITERATIONS
#  define MAX_ITERATIONS  50
#endif

int ntv2_inverse(
   const NTV2_HDR *hdr,
   double          deg_factor,
   int             n,
   NTV2_COORD      coord[])
{
   int max_iterations = MAX_ITERATIONS;
   int num = 0;
   int i;

   if ( hdr == NTV2_NULL || coord == NTV2_NULL || n <= 0 )
      return 0;

   if ( deg_factor <= 0.0 )
      deg_factor = 1.0;

   for (i = 0; i < n; i++)
   {
      double  lon,      lat;
      double  lon_next, lat_next;
      int num_iterations;

      lon_next = lon = (coord[i][NTV2_COORD_LON] * deg_factor);
      lat_next = lat = (coord[i][NTV2_COORD_LAT] * deg_factor);

      /* The inverse is not a simple transformation like the forward.
         We have to iteratively zero in on the answer by successively
         calculating what the forward delta is at the point, and then
         subtracting it instead of adding it.  The assumption here
         is that all the shifts are smooth, which should be the case.

         If we can't get the lat and lon deltas between two steps to be
         both within a given tolerance (NTV2_EPS) in max_iterations,
         we just give up and use the last value we calculated.
      */

      for (num_iterations = 0;
           num_iterations < max_iterations;
           num_iterations++)
      {
         const NTV2_REC * rec;
         double lon_shift, lat_shift;
         double lon_delta, lat_delta;
         double lon_est,   lat_est;
         int status;

         /* It may seem unnecessary to find the subfile this point is
            in each time, since it would seem that it *shouldn't* change.
            However, if the point was outside of a grid but within one cell
            of the edge, a shift may move it inside and then it could
            possibly be found in some subfile.  It is also possible
            that a point inside could shift to be on the edge or outside,
            in which case the record for it would be a higher-up parent or
            even a different sub-grid.
         */

         rec = ntv2_find_rec(hdr, lon_next, lat_next, &status);
         if ( rec == NTV2_NULL )
            break;

         ntv2_calculate_shifts(hdr, rec, lon_next, lat_next, status,
            &lon_shift, &lat_shift);

         lon_est   = (lon_next + lon_shift);
         lat_est   = (lat_next + lat_shift);

         lon_delta = (lon_est  - lon);
         lat_delta = (lat_est  - lat);

#if DEBUG_INVERSE
         {
            fprintf(stderr, "iteration %2d: value: %.17g %.17g\n",
               num_iterations+1,
               lon_next - lon_delta,
               lat_next - lat_delta);
            fprintf(stderr, "              delta: %.17g %.17g\n",
               lon_delta, lat_delta);
         }
#endif

         if ( NTV2_ZERO(lon_delta) && NTV2_ZERO(lat_delta) )
            break;

         lon_next  = (lon_next - lon_delta);
         lat_next  = (lat_next - lat_delta);
      }

#if DEBUG_INVERSE
         fprintf(stderr, "final         value: %.17g %.17g\n",
            lon_next, lat_next);
#endif

      if ( num_iterations > 0 )
      {
         coord[i][NTV2_COORD_LON] = (lon_next / deg_factor);
         coord[i][NTV2_COORD_LAT] = (lat_next / deg_factor);
         num++;
      }
   }

   return num;
}

/*------------------------------------------------------------------------
 * Perform a transformation (forward or inverse) on an array of points.
 */
int ntv2_transform(
   const NTV2_HDR *hdr,
   double          deg_factor,
   int             n,
   NTV2_COORD      coord[],
   int             direction)
{
   if ( direction == NTV2_CVT_INVERSE )
      return ntv2_inverse(hdr, deg_factor, n, coord);
   else
      return ntv2_forward(hdr, deg_factor, n, coord);
}
