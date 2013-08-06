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
/* This program uses an NTv2 file to transform (forward or inverse)          */
/* lon/lat points from one datum to another.                                 */
/* ------------------------------------------------------------------------- */

#if _WIN32
#  pragma warning (disable: 4996) /* _CRT_SECURE_NO_WARNINGS */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libntv2.h"

/*------------------------------------------------------------------------
 * program options and variables
 */
static const char *    pgm         = NTV2_NULL;
static const char *    ntv2file    = NTV2_NULL;        /* NTv2 filename */
static const char *    datafile    = "-";              /* -p file       */
static const char *    separator   = " ";              /* -s str        */

static NTV2_BOOL       direction   = NTV2_CVT_FORWARD; /* -f | -i       */
static NTV2_BOOL       reversed    = FALSE;            /* -r            */
static NTV2_BOOL       read_on_fly = FALSE;            /* -d            */

static NTV2_EXTENT     extent      = { 0 };            /* -e ...        */
static NTV2_EXTENT   * extptr      = NTV2_NULL;        /* -e ...        */

static double          deg_factor  = 1.0;              /* -c factor     */

/*------------------------------------------------------------------------
 * output usage
 */
static void display_usage(int level)
{
   if ( level )
   {
      printf("Usage: %s [options] ntv2file [lat lon] ...\n", pgm);
      printf("Options:\n");
      printf("  -?, -help  Display help\n");
      printf("  -r         Reversed data (lon lat) instead of (lat lon)\n");
      printf("  -d         Read shift data on the fly (no load of data)\n");
      printf("  -i         Inverse transformation\n");
      printf("  -f         Forward transformation       "
                           "(default)\n");
      printf("\n");
      printf("  -c val     Conversion: degrees-per-unit "
                           "(default is %.17g)\n", deg_factor);
      printf("  -s str     Use str as output separator  "
                           "(default is \" \")\n");
      printf("  -p file    Read points from file        "
                           "(default is \"-\" or stdin)\n");
      printf("  -e wlon slat elon nlat   Specify extent\n");
      printf("\n");

      printf("If no coordinate pairs are specified on the command line, then\n");
      printf("they are read one per line from the specified data file.\n");
   }
   else
   {
      fprintf(stderr,
         "Usage: %s [-r] [-d] [-i|-f] [-c val] [-s str] [-p file]\n",
         pgm);
      fprintf(stderr,
         "       %*s [-e wlon slat elon nlat]\n",
         (int)strlen(pgm), "");
      fprintf(stderr,
         "       %*s ntv2file [lat lon] ...\n",
         (int)strlen(pgm), "");
   }
}

/*------------------------------------------------------------------------
 * process all command-line options
 */
static int process_options(int argc, const char **argv)
{
   int optcnt;

   if ( pgm == NTV2_NULL )  pgm = strrchr(argv[0], '/');
   if ( pgm == NTV2_NULL )  pgm = strrchr(argv[0], '\\');
   if ( pgm == NTV2_NULL )  pgm = argv[0];
   else                     pgm++;

   for (optcnt = 1; optcnt < argc; optcnt++)
   {
      const char *arg = argv[optcnt];

      if ( *arg != '-' )
         break;

      while ( *arg == '-' )
         arg++;
      if ( !*arg )
      {
         optcnt++;
         break;
      }

      if ( strcmp(arg, "?")    == 0 ||
           strcmp(arg, "help") == 0 )
      {
         display_usage(1);
         exit(EXIT_SUCCESS);
      }

      else if ( strcmp(arg, "f") == 0 )  direction   = NTV2_CVT_FORWARD;
      else if ( strcmp(arg, "i") == 0 )  direction   = NTV2_CVT_INVERSE;
      else if ( strcmp(arg, "r") == 0 )  reversed    = TRUE;
      else if ( strcmp(arg, "d") == 0 )  read_on_fly = TRUE;

      else if ( strcmp(arg, "s") == 0 )
      {
         if ( ++optcnt >= argc )
         {
            fprintf(stderr, "%s: Option needs an argument -- -%s\n",
               pgm, "s");
            display_usage(0);
            exit(EXIT_FAILURE);
         }
         separator = argv[optcnt];
      }

      else if ( strcmp(arg, "c") == 0 )
      {
         if ( ++optcnt >= argc )
         {
            fprintf(stderr, "%s: Option needs an argument -- -%s\n",
               pgm, "c");
            display_usage(0);
            exit(EXIT_FAILURE);
         }
         deg_factor = atof( argv[optcnt] );
      }

      else if ( strcmp(arg, "p") == 0 )
      {
         if ( ++optcnt >= argc )
         {
            fprintf(stderr, "%s: Option needs an argument -- -%s\n",
               pgm, "p");
            display_usage(0);
            exit(EXIT_FAILURE);
         }
         datafile = argv[optcnt];
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
         fprintf(stderr, "%s: Invalid option -- %s\n", pgm, argv[optcnt]);
         display_usage(0);
         exit(EXIT_FAILURE);
      }
   }

   return optcnt;
}

/*------------------------------------------------------------------------
 * strip a string of all leading/trailing whitespace
 */
static char * strip(char *str)
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

/*------------------------------------------------------------------------
 * process a lat/lon value
 */
static void process_lat_lon(
   NTV2_HDR * hdr,
   double     lat,
   double     lon)
{
   NTV2_COORD coord[1];

   coord[0][NTV2_COORD_LAT] = lat;
   coord[0][NTV2_COORD_LON] = lon;

   if ( ntv2_transform(hdr, deg_factor, 1, coord, direction) == 1 )
   {
      lat = coord[0][NTV2_COORD_LAT];
      lon = coord[0][NTV2_COORD_LON];
   }

   if ( reversed )
      printf("%.16g%s%.16g\n", lon, separator, lat);
   else
      printf("%.16g%s%.16g\n", lat, separator, lon);
   fflush(stdout);
}

/*------------------------------------------------------------------------
 * process all arguments
 */
static int process_args(
   NTV2_HDR *   hdr,
   int          optcnt,
   int          argc,
   const char **argv)
{
   while ( (argc - optcnt) >= 2 )
   {
      double lon, lat;

      if ( reversed )
      {
        lon = atof( argv[optcnt++] );
        lat = atof( argv[optcnt++] );
      }
      else
      {
        lat = atof( argv[optcnt++] );
        lon = atof( argv[optcnt++] );
      }

      process_lat_lon(hdr, lat, lon);
   }

   return 0;
}

/*------------------------------------------------------------------------
 * process a stream of lon/lat values
 */
static int process_file(
   NTV2_HDR *  hdr,
   const char *file)
{
   FILE * fp;

   if ( strcmp(file, "-") == 0 )
   {
      fp = stdin;
   }
   else
   {
      fp = fopen(file, "r");
      if ( fp == NTV2_NULL )
      {
         fprintf(stderr, "%s: Cannot open data file %s\n", pgm, file);
         return -1;
      }
   }

   for (;;)
   {
      char   line[128];
      char * lp;
      double lon, lat;
      int n;

      if ( fgets(line, sizeof(line), fp) == NTV2_NULL )
         break;

      /* Strip all whitespace to check for an empty line.
         Lines starting with a # are considered comments.
      */
      lp = strip(line);
      if ( *lp == 0 || *lp == '#' )
         continue;

      /* parse lat/lon or lon/lat & process it */
      if ( reversed )
         n = sscanf(lp, "%lf %lf", &lon, &lat);
      else
         n = sscanf(lp, "%lf %lf", &lat, &lon);

      if ( n != 2 )
         printf("invalid: %s\n", lp);
      else
         process_lat_lon(hdr, lat, lon);
   }

   fclose(fp);
   return 0;
}

/*------------------------------------------------------------------------
 * main
 */
int main(int argc, const char **argv)
{
   NTV2_HDR * hdr;
   int optcnt;
   int rc;

   /*---------------------------------------------------------
    * process options
    */
   optcnt = process_options(argc, argv);

   /*---------------------------------------------------------
    * get the filename
    */
   if ( (argc - optcnt) == 0 )
   {
      fprintf(stderr, "%s: Missing NTv2 filename\n", pgm);
      display_usage(0);
      return EXIT_FAILURE;
   }
   ntv2file = argv[optcnt++];

   /*---------------------------------------------------------
    * load the file
    */
   hdr = ntv2_load_file(
      ntv2file,           /* in:  filename                 */
      FALSE,              /* in:  don't keep original hdrs */
      !read_on_fly,       /* in:  read    shift data?      */
      extptr,             /* in:  extent pointer           */
      &rc);               /* out: result code              */

   if ( hdr == NTV2_NULL )
   {
      char msg_buf[NTV2_MAX_ERR_LEN];

      fprintf(stderr, "%s: %s: %s\n", pgm, ntv2file, ntv2_errmsg(rc, msg_buf));
      return EXIT_FAILURE;
   }

   /*---------------------------------------------------------
    * Either process lon/lat pairs from the cmd line or
    * process all points in the input file.
    */
   if ( optcnt < argc )
      rc = process_args(hdr, optcnt, argc, argv);
   else
      rc = process_file(hdr, datafile);

   /*---------------------------------------------------------
    * done - close out the ntv2 object and exit
    */
   ntv2_delete(hdr);

   return (rc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
