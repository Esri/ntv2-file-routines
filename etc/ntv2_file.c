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
/* This program will dump and/or copy NTv2 files with optional error         */
/* checking and/or extent processing.                                        */
/* ------------------------------------------------------------------------- */

#if _WIN32
#  pragma warning (disable: 4996) /* _CRT_SECURE_NO_WARNINGS */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libntv2.h"

/*------------------------------------------------------------------------
 * program options and variables
 */
static const char *  pgm       = NTV2_NULL;
static const char *  outfile   = NTV2_NULL;            /* -o file      */

static NTV2_BOOL     dump_hdr  = FALSE;                /* -h           */
static NTV2_BOOL     list_hdr  = FALSE;                /* -l           */
static NTV2_BOOL     dump_data = FALSE;                /* -d           */
static NTV2_BOOL     read_data = FALSE;                /* -d | -o file */
static NTV2_BOOL     validate  = FALSE;                /* -v           */
static NTV2_BOOL     ignore    = FALSE;                /* -i           */
static NTV2_EXTENT   extent    = { 0 };                /* -e ...       */
static NTV2_EXTENT * extptr    = NTV2_NULL;            /* -e ...       */
static int           endian    = NTV2_ENDIAN_INP_FILE; /* -B | -L | -N */
static int           dump_mode = NTV2_DUMP_HDRS_BOTH;  /* -s | -d | -a */

static NTV2_BOOL     do_title  = TRUE;

/*------------------------------------------------------------------------
 * display usage
 */
static void display_usage(int level)
{
   if ( level )
   {
      printf("Usage: %s [options] file ...\n", pgm);
      printf("Options:\n");
      printf("  -?, -help  Display help\n");
      printf("  -v         Validate files\n");
      printf("  -i         Ignore errors\n");
      printf("\n");

      printf("  -l         List header info\n");
      printf("  -h         Dump header info\n");
      printf("  -s         Dump header summaries\n");
      printf("  -d         Dump shift data\n");
      printf("  -a         Dump shift data and accuracies\n");
      printf("\n");

      printf("  -B         Write    big-endian binary file\n");
      printf("  -L         Write little-endian binary file\n");
      printf("  -N         Write native-endian binary file\n");
      printf("             (default is same as input  file)\n");
      printf("\n");

      printf("  -o file    Specify output file\n");
      printf("  -e wlon slat elon nlat   Specify extent\n");
   }
   else
   {
      fprintf(stderr,
         "Usage: %s [-v] [-h|-l] [-s] [-d|-a] [-B|-L|-N] [-o file]\n",
         pgm);
      fprintf(stderr,
         "       %*s [-e wlon slat elon nlat] file ...\n",
         (int)strlen(pgm), "");
   }
}

/*------------------------------------------------------------------------
 * process command-line options
 */
static int process_options(int argc, const char **argv)
{
   int optcnt;

   if ( pgm == NTV2_NULL )  pgm = strrchr(argv[0], '/');
   if ( pgm == NTV2_NULL )  pgm = strrchr(argv[0], '\\');
   if ( pgm == NTV2_NULL )  pgm = argv[0];
   else                     pgm++;

   memset(&extent, 0, sizeof(extent));

   for (optcnt = 1; optcnt < argc; optcnt++)
   {
      const char *arg = argv[optcnt];

      if ( *arg != '-' )
         break;

      while (*arg == '-') arg++;

      if ( strcmp(arg, "?")    == 0 ||
           strcmp(arg, "help") == 0 )
      {
         display_usage(1);
         exit(EXIT_SUCCESS);
      }

      else if ( strcmp(arg, "v") == 0 ) validate   = TRUE;
      else if ( strcmp(arg, "i") == 0 ) ignore     = TRUE;

      else if ( strcmp(arg, "l") == 0 ) list_hdr   = TRUE;
      else if ( strcmp(arg, "h") == 0 ) dump_hdr   = TRUE;

      else if ( strcmp(arg, "s") == 0 ) dump_mode |= NTV2_DUMP_HDRS_SUMMARY;
      else if ( strcmp(arg, "d") == 0 ) dump_mode |= NTV2_DUMP_DATA;
      else if ( strcmp(arg, "a") == 0 ) dump_mode |= NTV2_DUMP_DATA_ACC;

      else if ( strcmp(arg, "B") == 0 ) endian     = NTV2_ENDIAN_BIG;
      else if ( strcmp(arg, "L") == 0 ) endian     = NTV2_ENDIAN_LITTLE;
      else if ( strcmp(arg, "N") == 0 ) endian     = NTV2_ENDIAN_NATIVE;

      else if ( strcmp(arg, "o") == 0 )
      {
         if ( ++optcnt >= argc )
         {
            fprintf(stderr, "%s: Option needs an argument -- -%s\n",
               pgm, "o");
            display_usage(0);
            exit(EXIT_FAILURE);
         }
         outfile   = argv[optcnt];
      }

      else if ( strcmp(arg, "e") == 0 )
      {
         if ( (optcnt+4) >= argc )
         {
            fprintf(stderr, "%s: Option needs 4 arguments -- -%s\n",
               pgm, "e");
            display_usage(0);
            exit(EXIT_FAILURE);
         }
         extent.wlon = atof( argv[++optcnt] );
         extent.slat = atof( argv[++optcnt] );
         extent.elon = atof( argv[++optcnt] );
         extent.nlat = atof( argv[++optcnt] );
         extptr = &extent;
      }

      else
      {
         fprintf(stderr, "Invalid option -- %s\n", argv[optcnt]);
         display_usage(0);
         exit(EXIT_FAILURE);
      }
   }

   dump_data  = ( (dump_mode & NTV2_DUMP_DATA) != 0 );
   read_data  = ( dump_data || (outfile != NTV2_NULL) );
   dump_hdr  |= dump_data;

   if ( argc == optcnt )
   {
      fprintf(stderr, "%s: No files specified.\n", pgm);
      display_usage(0);
      exit(EXIT_FAILURE);
   }

   if ( list_hdr && dump_hdr )
   {
      fprintf(stderr, "%s: Both -l and -h specified. -h ignored.\n", pgm);
      dump_hdr  = FALSE;
   }

   if ( list_hdr && dump_data )
   {
      fprintf(stderr, "%s: Both -l and -d specified. -d ignored.\n", pgm);
      dump_data = FALSE;
   }

   if ( outfile != NTV2_NULL && dump_data )
   {
      fprintf(stderr, "%s: Both -o and -d specified. -d ignored.\n", pgm);
      dump_data = FALSE;
   }

   if ( outfile != NTV2_NULL && (optcnt+1) < argc )
   {
      fprintf(stderr, "%s: Too many files specified.\n", pgm);
      display_usage(0);
      exit(EXIT_FAILURE);
   }

   return optcnt;
}

/*------------------------------------------------------------------------
 * process a file
 */
static int process_file(const char *inpfile)
{
   NTV2_HDR * hdr;
   int rc;

   /* -------- load the file */

   hdr = ntv2_load_file(
      inpfile,            /* in:  input file          */
      TRUE,               /* in:  keep original data  */
      read_data,          /* in:  read    shift data? */
      extptr,             /* in:  extent pointer?     */
      &rc);               /* out: error code          */

   if ( hdr == NTV2_NULL )
   {
      char msg_buf[NTV2_MAX_ERR_LEN];

      printf("%s: Cannot read input file: %s\n",
         inpfile, ntv2_errmsg(rc, msg_buf));
      return -1;
   }

   /* -------- validate the headers if requested */

   if ( validate )
   {
      rc = ntv2_validate(hdr, stdout);
      if ( rc != NTV2_ERR_OK )
      {
         if ( (rc >= NTV2_ERR_UNRECOVERABLE_START) ||
              (rc >= NTV2_ERR_START && !ignore)    )
         {
            ntv2_delete(hdr);
            return -1;
         }
      }
   }

   /* -------- dump the headers and/or data if requested */

   if ( list_hdr )
   {
      ntv2_list(hdr, stdout, dump_mode, do_title);
      do_title = FALSE;
   }

   if ( dump_hdr )
   {
      ntv2_dump(hdr, stdout, dump_mode);
   }

   /* -------- write out a new file if requested */

   if ( outfile != NTV2_NULL )
   {
      rc = ntv2_write_file(hdr, outfile, endian);
      if ( rc != NTV2_ERR_OK )
      {
         char msg_buf[NTV2_MAX_ERR_LEN];

         printf("%s: Cannot write output file: %s\n",
            outfile, ntv2_errmsg(rc, msg_buf));
      }
   }

   ntv2_delete(hdr);
   return (rc == NTV2_ERR_OK) ? 0 : -1;
}

/*------------------------------------------------------------------------
 * main
 */
int main(int argc, const char **argv)
{
   int optcnt = process_options(argc, argv);
   int rc = 0;

   while (optcnt < argc)
   {
      rc |= process_file(argv[optcnt++]);
   }

   return (rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
