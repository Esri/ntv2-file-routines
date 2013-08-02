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
/* public header for the libntv2 library                                     */
/* ------------------------------------------------------------------------- */

#ifndef LIBNTV2_INCLUDED
#define LIBNTV2_INCLUDED

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */
/* version info                                                           */
/* ---------------------------------------------------------------------- */

#define NTV2_VERSION_MAJOR     1
#define NTV2_VERSION_MINOR     0
#define NTV2_VERSION_RELEASE   0
#define NTV2_VERSION_STR       "1.0.0"

/*------------------------------------------------------------------------*/
/* external definitions & structs                                         */
/*------------------------------------------------------------------------*/

#define FALSE              0
#define TRUE               1

#define NTV2_NULL          0             /*!< NULL pointer         */

#define NTV2_COORD_LON     0             /*!< NTV2_COORD longitude */
#define NTV2_COORD_LAT     1             /*!< NTV2_COORD latitude  */

#define NTV2_MAX_PATH_LEN  256           /*!< Max pathname length  */
#define NTV2_MAX_ERR_LEN   32            /*!< Max err msg  length  */

typedef int            NTV2_BOOL;        /*!< Boolean variable     */
typedef double         NTV2_COORD [2];   /*!< Lon/lat coordinate   */

/*---------------------------------------------------------------------*/
/**
 * NTv2 Extent struct
 *
 * <p>This struct defines the lower-left and the upper-right
 * corners of an extent to use to cut down the area defined by
 * a grid file.
 *
 * <p>Unlike NTv2 files, this struct uses the standard notation of
 * negative-west / positive-east and values are always in degrees.
 *
 * <p>Since shifts are usually very small (on the order of fractions
 * of a second), it doesn't matter which datum the values are on.
 *
 * <p>Note that this struct is used only by this API, and is not part of
 * the NTv2 specification.
 */
typedef struct ntv2_extent NTV2_EXTENT;
struct ntv2_extent
{
   /* lower-left  corner */
   double  wlon;                 /*!< West  longitude (degrees) */
   double  slat;                 /*!< South latitude  (degrees) */

   /* upper-right corner */
   double  elon;                 /*!< East  longitude (degrees) */
   double  nlat;                 /*!< North latitude  (degrees) */
};

/*------------------------------------------------------------------------*/
/* NTv2 file layout                                                       */
/*                                                                        */
/* A NTv2 file (either binary or text) is laid out as follows:            */
/*                                                                        */
/*    overview record                                                     */
/*                                                                        */
/*    sub-file record 1                                                   */
/*       gs_count grid-shift records                                      */
/*    ...                                                                 */
/*    sub-file record n (if present)                                      */
/*       gs_count grid-shift records                                      */
/*                                                                        */
/*    end record                                                          */
/*------------------------------------------------------------------------*/

#define NTV2_NAME_LEN            8      /*!< Blank-padded, no null */

#define NTV2_FILE_BIN_EXTENSION  "gsb"  /*!< "Grid Shift Binary"   */
#define NTV2_FILE_ASC_EXTENSION  "gsa"  /*!< "Grid Shift Ascii"    */

#define NTV2_FILE_TYPE_UNK       0      /*!< File type is unknown  */
#define NTV2_FILE_TYPE_BIN       1      /*!< File type is binary   */
#define NTV2_FILE_TYPE_ASC       2      /*!< File type is ascii    */

/*---------------------------------------------------------------------*/
/**
 * NTv2 binary file overview record
 *
 * Notes:
 *   <ul>
 *     <li>The pad words (p_* fields) may not be present in the file,
 *         although any files created after 2000 should have them.
 *     <li>The strings (n_* and s_* fields) are blank-padded and are not
 *         null-terminated.
 *     <li>The name strings (n_* fields) should appear exactly as in the
 *         comments.
 *   </ul>
 */
typedef struct ntv2_file_ov NTV2_FILE_OV;
struct ntv2_file_ov
{
   char   n_num_orec [NTV2_NAME_LEN];  /*!< "NUM_OREC"                */
   int    i_num_orec;                  /*!< Should be 11              */
   int    p_num_orec;                  /*!< Zero pad ?                */

   char   n_num_srec [NTV2_NAME_LEN];  /*!< "NUM_SREC"                */
   int    i_num_srec;                  /*!< Should be 11              */
   int    p_num_srec;                  /*!< Zero pad ?                */

   char   n_num_file [NTV2_NAME_LEN];  /*!< "NUM_FILE"                */
   int    i_num_file;                  /*!< Num of sub-grid files     */
   int    p_num_file;                  /*!< Zero pad ?                */

   char   n_gs_type  [NTV2_NAME_LEN];  /*!< "GS_TYPE "                */
   char   s_gs_type  [NTV2_NAME_LEN];  /*!< e.g. "SECONDS "           */

   char   n_version  [NTV2_NAME_LEN];  /*!< "VERSION "                */
   char   s_version  [NTV2_NAME_LEN];  /*!< ID of distortion model    */

   char   n_system_f [NTV2_NAME_LEN];  /*!< "SYSTEM_F"                */
   char   s_system_f [NTV2_NAME_LEN];  /*!< Reference system from     */

   char   n_system_t [NTV2_NAME_LEN];  /*!< "SYSTEM_T"                */
   char   s_system_t [NTV2_NAME_LEN];  /*!< Reference system to       */

   char   n_major_f  [NTV2_NAME_LEN];  /*!< "MAJOR_F "                */
   double d_major_f;                   /*!< Semi-major axis from      */

   char   n_minor_f  [NTV2_NAME_LEN];  /*!< "MINOR_F "                */
   double d_minor_f;                   /*!< Semi-minor axis from      */

   char   n_major_t  [NTV2_NAME_LEN];  /*!< "MAJOR_T "                */
   double d_major_t;                   /*!< Semi-major axis to        */

   char   n_minor_t  [NTV2_NAME_LEN];  /*!< "MINOR_T "                */
   double d_minor_t;                   /*!< Semi-minor axis to        */
};

/*---------------------------------------------------------------------*/
/**
 * NTv2 binary file subfile record
 *
 * Notes:
 *   <ul>
 *     <li>The pad words (p_* fields) may not be present in the file,
 *         although any files created after 2000 should have them.
 *     <li>The strings (n_* and s_* fields) are blank-padded and are not
 *         null-terminated.
 *     <li>The name strings (n_* fields) should appear exactly as in the
 *         comments.
 *     <li>There will always be at least one subfile record which is a parent.
 *     <li>Parent records should have a parent-name of "NONE".
 *     <li>Longitude values are specified as positive-west / negative-east.
 *   </ul>
 */
typedef struct ntv2_file_sf NTV2_FILE_SF;
struct ntv2_file_sf
{
   char   n_sub_name [NTV2_NAME_LEN];  /*!< "SUB_NAME"                */
   char   s_sub_name [NTV2_NAME_LEN];  /*!< Sub-file name             */

   char   n_parent   [NTV2_NAME_LEN];  /*!< "PARENT  "                */
   char   s_parent   [NTV2_NAME_LEN];  /*!< Parent name or "NONE"     */

   char   n_created  [NTV2_NAME_LEN];  /*!< "CREATED "                */
   char   s_created  [NTV2_NAME_LEN];  /*!< Creation date             */

   char   n_updated  [NTV2_NAME_LEN];  /*!< "UPDATED "                */
   char   s_updated  [NTV2_NAME_LEN];  /*!< Last revision date        */

   char   n_s_lat    [NTV2_NAME_LEN];  /*!< "S_LAT   "                */
   double d_s_lat;                     /*!< South lat (in gs-units)   */

   char   n_n_lat    [NTV2_NAME_LEN];  /*!< "N_LAT   "                */
   double d_n_lat;                     /*!< North lat (in gs-units)   */

   char   n_e_lon    [NTV2_NAME_LEN];  /*!< "E_LONG  "                */
   double d_e_lon;                     /*!< East  lon (in gs-units)   */

   char   n_w_lon    [NTV2_NAME_LEN];  /*!< "W_LONG  "                */
   double d_w_lon;                     /*!< West  lon (in gs-units)   */

   char   n_lat_inc  [NTV2_NAME_LEN];  /*!< "LAT_INC "                */
   double d_lat_inc;                   /*!< Lat inc   (in gs-units)   */

   char   n_lon_inc  [NTV2_NAME_LEN];  /*!< "LONG_INC"                */
   double d_lon_inc;                   /*!< Lon inc   (in gs-units)   */

   char   n_gs_count [NTV2_NAME_LEN];  /*!< "GS_COUNT"                */
   int    i_gs_count;                  /*!< Num grid-shift records    */
   int    p_gs_count;                  /*!< Zero pad ?                */
};

/*---------------------------------------------------------------------*/
/**
 * NTv2 binary file end record
 *
 * <p>There are files (such as all Canadian files) that have only
 * the first four bytes correct (i.e. "END xxxx"), and there are
 * files (such as italy/rer.gsb) that omit this record completely.
 * Fortunately, we don't rely on the end-record when reading a file,
 * but depend on the num-subfiles field in the overview record.
 *
 * <p>Also, very few files that I've seen actually zero out the filler field.
 * Some files fill the field with spaces.  Most files just contain garbage
 * bytes (which is bogus, IMNSHO, since you can't then compare or checksum
 * files properly).
 */
typedef struct ntv2_file_end NTV2_FILE_END;
struct ntv2_file_end
{
   char   n_end      [NTV2_NAME_LEN];  /*!< "END     "                */
   char   filler     [NTV2_NAME_LEN];  /*!< Totally unused
                                            (should be zeroed out)    */
};

/*---------------------------------------------------------------------*/
/**
 * NTv2 file grid-shift record
 *
 * For a particular sub-file, the grid-shift records consist of
 * <i>nrows</i> sets of <i>ncols</i> records, with the latitudes going
 * South to North, and longitudes going (backwards) East to West.
 */
typedef struct ntv2_file_gs NTV2_FILE_GS;
struct ntv2_file_gs
{
   float  f_lat_shift;                 /*!< Lat shift (in gs-units)   */
   float  f_lon_shift;                 /*!< Lon shift (in gs-units)   */
   float  f_lat_accuracy;              /*!< Lat accuracy (in meters)  */
   float  f_lon_accuracy;              /*!< Lon accuracy (in meters)  */
};

/*------------------------------------------------------------------------*/
/* NTv2 internal structs                                                  */
/*------------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/

typedef float    NTV2_SHIFT [2];       /*!< Lat/lon shift/accuracy pair */

/**
 * NTv2 sub-file record
 *
 * This struct describes one sub-grid in the file. There may be many
 * of these, and they may be (logically) nested, although they are
 * all stored in an array in the order they appear in the file.
 */
typedef struct ntv2_rec NTV2_REC;
struct ntv2_rec
{
   char  record_name[NTV2_NAME_LEN+4]; /*!< Record name               */
   char  parent_name[NTV2_NAME_LEN+4]; /*!< Parent name or "NONE"     */

   NTV2_REC *     parent;              /*!< Ptr to parent or NULL     */
   NTV2_REC *     sub;                 /*!< Ptr to first sub-file     */
   NTV2_REC *     next;                /*!< Ptr to next  sibling      */

   NTV2_BOOL      active;              /*!< TRUE if this rec is used  */

   int            num_subs;            /*!< Number of child sub-files */
   int            rec_num;             /*!< Record number             */

   int            num;                 /*!< Number of records         */
   int            nrows;               /*!< Number of rows            */
   int            ncols;               /*!< Number of columns         */

   double         lat_min;             /*!< Latitude  min (degrees)   */
   double         lat_max;             /*!< Latitude  max (degrees)   */
   double         lat_inc;             /*!< Latitude  inc (degrees)   */

   double         lon_min;             /*!< Longitude min (degrees)   */
   double         lon_max;             /*!< Longitude max (degrees)   */
   double         lon_inc;             /*!< Longitude inc (degrees)   */

   /* These fields are only used for binary files. */

   long           offset;              /*!< File offset of shifts     */
   int            sskip;               /*!< Bytes to skip South       */
   int            nskip;               /*!< Bytes to skip North       */
   int            wskip;               /*!< Bytes to skip West        */
   int            eskip;               /*!< Bytes to skip East        */

   /* This may be null if data is to be read on-the-fly. */

   NTV2_SHIFT *   shifts;              /*!< Lat/lon grid-shift array  */

   /* This may be null if not wanted. Always null if shifts is null. */

   NTV2_SHIFT *   accurs;              /*!< Lat/lon accuracies array  */
};

/*---------------------------------------------------------------------*/
/**
 * NTv2 top-level struct
 *
 * This struct defines the entire NTv2 file as it is stored in memory.
 */
typedef struct ntv2_hdr NTV2_HDR;
struct ntv2_hdr
{
   char           path [NTV2_MAX_PATH_LEN];
                                       /*!< Pathname of file          */
   int            file_type;           /*!< File type (bin or asc)    */

   int            num_recs;            /*!< Number of subfile records */
   int            num_parents;         /*!< Number of parent  records */

   NTV2_BOOL      keep_orig;           /*!< TRUE if orig data is kept */
   NTV2_BOOL      pads_present;        /*!< TRUE if pads are in file  */
   NTV2_BOOL      swap_data;           /*!< TRUE if must swap data    */
   NTV2_BOOL      data_converted;      /*!< TRUE if data is converted */
   int            fixed;               /*!< Mask of NTV2_FIX_* codes  */

   double         hdr_conv;            /*!< Header conversion factor  */
   double         dat_conv;            /*!< Data   conversion factor  */
   char           gs_type [NTV2_NAME_LEN+4];
                                       /*!< Conversion type
                                            (e.g. "DEGREES")          */

   /* These are the mins and maxes across all sub-files */

   double         lat_min;             /*!< Latitude  min (degrees)   */
   double         lat_max;             /*!< Latitude  max (degrees)   */
   double         lon_min;             /*!< Longitude min (degrees)   */
   double         lon_max;             /*!< Longitude max (degrees)   */

   NTV2_REC *     recs;                /*!< Array of ntv2 records     */
   NTV2_REC *     first_parent;        /*!< Pointer to first parent   */

   /* This will be null if data is in memory. */

   FILE *         fp;                  /*!< Data stream               */

   /* This should be used if mutex control is needed   */
   /* for multi-threaded access to the file when       */
   /* transforming points and reading data on-the-fly. */
   /* This mutex does not need to be recursive.        */
   /* This is currently not implemented.               */

   void *         mutex;               /*!< Ptr to OS-specific mutex  */

   /* These may be null if not wanted. */

   NTV2_FILE_OV * overview;            /*!< Overview record           */
   NTV2_FILE_SF * subfiles;            /*!< Array of sub-file records */
};

/*------------------------------------------------------------------------*/
/* NTv2 error codes                                                       */
/*------------------------------------------------------------------------*/

#define NTV2_ERR_OK                         0

/* generic errors */
#define NTV2_ERR_NO_MEMORY                  1
#define NTV2_ERR_IOERR                      2
#define NTV2_ERR_NULL_HDR                   3

/* warnings */
#define NTV2_ERR_FILE_NEEDS_FIXING        101

/* read errors that may be ignored */
#define NTV2_ERR_START                    200

#define NTV2_ERR_INVALID_LAT_MIN_MAX      201
#define NTV2_ERR_INVALID_LON_MIN_MAX      202
#define NTV2_ERR_INVALID_LAT_MIN          203
#define NTV2_ERR_INVALID_LAT_MAX          204
#define NTV2_ERR_INVALID_LAT_INC          205
#define NTV2_ERR_INVALID_LON_INC          206
#define NTV2_ERR_INVALID_LON_MIN          207
#define NTV2_ERR_INVALID_LON_MAX          208

/* unrecoverable errors */
#define NTV2_ERR_UNRECOVERABLE_START      300

#define NTV2_ERR_INVALID_NUM_OREC         301
#define NTV2_ERR_INVALID_NUM_SREC         302
#define NTV2_ERR_INVALID_NUM_FILE         303
#define NTV2_ERR_INVALID_GS_TYPE          304
#define NTV2_ERR_INVALID_GS_COUNT         305
#define NTV2_ERR_INVALID_DELTA            306
#define NTV2_ERR_INVALID_PARENT_NAME      307
#define NTV2_ERR_PARENT_NOT_FOUND         308
#define NTV2_ERR_NO_TOP_LEVEL_PARENT      309
#define NTV2_ERR_PARENT_LOOP              310
#define NTV2_ERR_PARENT_OVERLAP           311
#define NTV2_ERR_SUBFILE_OVERLAP          312
#define NTV2_ERR_INVALID_EXTENT           313
#define NTV2_ERR_HDRS_NOT_READ            314
#define NTV2_ERR_UNKNOWN_FILE_TYPE        315
#define NTV2_ERR_FILE_NOT_BINARY          316
#define NTV2_ERR_FILE_NOT_ASCII           317
#define NTV2_ERR_NULL_PATH                318
#define NTV2_ERR_ORIG_DATA_NOT_KEPT       319
#define NTV2_ERR_DATA_NOT_READ            320
#define NTV2_ERR_CANNOT_OPEN_FILE         321
#define NTV2_ERR_UNEXPECTED_EOF           322
#define NTV2_ERR_INVALID_LINE             323

/* fix header reasons (bit-mask) */

#define NTV2_FIX_UNPRINTABLE_CHAR         0x01
#define NTV2_FIX_NAME_LOWERCASE           0x02
#define NTV2_FIX_NAME_NOT_ALPHA           0x04
#define NTV2_FIX_BLANK_PARENT_NAME        0x08
#define NTV2_FIX_BLANK_SUBFILE_NAME       0x10
#define NTV2_FIX_END_REC_NOT_FOUND        0x20
#define NTV2_FIX_END_REC_NAME_NOT_ALPHA   0x40
#define NTV2_FIX_END_REC_PAD_NOT_ZERO     0x80

/**
 * Convert an NTV2 error code to a string.
 *
 * <p>Currently, these messages are in English, but this call is
 * designed so a user could implement messages in other languages.
 *
 * @param err_num  The error code to convert.
 * @param msg_buf  Buffer to store error message in.
 *
 * @return A pointer to a error-message string.
 */
extern const char * ntv2_errmsg(
   int  err_num,
   char msg_buf[NTV2_MAX_ERR_LEN]);

/*------------------------------------------------------------------------*/
/* NTv2 file methods                                                      */
/*------------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/**
 * Determine whether a filename is for a binary or a text file.
 *
 * <p>This is done solely by checking the filename extension.
 * No examination of the file contents (if any) is done.
 *
 * @param ntv2file The filename to query.
 *
 * @return One of the following:
 *         <ul>
 *           <li>NTV2_FILE_TYPE_UNK  file type is unknown
 *           <li>NTV2_FILE_TYPE_BIN  file type is binary
 *           <li>NTV2_FILE_TYPE_ASC  file type is ascii
 *         </ul>
 */
extern int ntv2_filetype(
   const char *ntv2file);

/*---------------------------------------------------------------------*/
/**
 * Load a NTv2 file into memory.
 *
 * @param ntv2file     The name of the NTv2 file to load.
 *
 * @param keep_orig    TRUE to keep copies of all external records.
 *                     A TRUE value will also cause accuracy values to be
 *                     read in when reading shift values.
 *
 * @param read_data    TRUE to read in shift (and optionally accuracy) data.
 *                     A TRUE value will also result in closing the file
 *                     after reading, since there is no need to keep it open.
 *
 * @param convert_data TRUE to convert shift data to decimal seconds.
 *                     If you are only loading the file in order to copy
 *                     it to another file, it is best to not convert the
 *                     shift data, since converting and then unconverting
 *                     may result in a loss of precision. However, if
 *                     you are using the file to do data conversion, it
 *                     is better to do the conversion once than each time
 *                     it is used.
 *
 * @param extent       A pointer to an NTV2_EXTENT struct.
 *                     This pointer may be NULL.
 *                     This is ignored for text files.
 *
 * @param prc          A pointer to a result code.
 *                     This pointer may be NULL.
 *                     <ul>
 *                       <li>If successful,   it will be set to NTV2_ERR_OK (0).
 *                       <li>If unsuccessful, it will be set to NTV2_ERR_*.
 *                     </ul>
 *
 * @return A pointer to a NTV2_HDR object or NULL if unsuccessful.
 */
extern NTV2_HDR * ntv2_load_file(
   const char *  ntv2file,
   NTV2_BOOL     keep_orig,
   NTV2_BOOL     read_data,
   NTV2_BOOL     convert_data,
   NTV2_EXTENT * extent,
   int *         prc);

/*---------------------------------------------------------------------*/
/**
 * Delete a NTv2 object
 *
 * This method will also close any open stream in the object.
 *
 * @param hdr A pointer to a NTV2_HDR object.
 */
extern void ntv2_delete(
   NTV2_HDR *hdr);

/*---------------------------------------------------------------------*/

#define NTV2_ENDIAN_INP_FILE 0    /*!< Input-file    byte-order */
#define NTV2_ENDIAN_BIG      1    /*!< Big-endian    byte-order */
#define NTV2_ENDIAN_LITTLE   2    /*!< Little-endian byte-order */
#define NTV2_ENDIAN_NATIVE   3    /*!< Native        byte-order */

/**
 * Write out a NTV2_HDR object to a file.
 *
 * <p>Rules:
 *    <ul>
 *      <li>A binary file is always written with the zero-pads in it.
 *      <li>Sub-files are written recursively just after their parents.
 *      <li>Parents are written in the order they appeared in the original file.
 *    </ul>
 *
 * <p>One advantage of this routine is to be able to write out a file
 * that has been "cut down" by an extent specification.  Presumably,
 * this could help keep down memory usage in mobile environments.
 *
 * <p>This also provides the ability to rewrite a file in order to
 * cleanup all its header info into "standard" form.  This could be
 * useful if the file will be used by a different program that isn't as
 * tolerant and forgiving as we are.
 *
 * <p>This call can also be used to write out a binary file for an object
 * that was read from a text file, and vice-versa.
 *
 * @param hdr         A pointer to a NTV2_HDR object.
 *
 * @param path        The pathname of the file to write.
 *                    This can name either a binary or a text NTv2 file.
 *
 * @param byte_order  Byte order of the output file (NTV2_ENDIAN_*) if
 *                    binary.  A value of NTV2_ENDIAN_INP_FILE means to
 *                    write the file using the same byte-order as the
 *                    input file if binary or native byte-order if the
 *                    input file was a text file.
 *                    This parameter is ignored when writing text files.
 *
 * @return If successful,   NTV2_ERR_OK (0).
 *         If unsuccessful, NTV2_ERR_*.
 */
extern int ntv2_write_file(
   NTV2_HDR   *hdr,
   const char *path,
   int         byte_order);

/*---------------------------------------------------------------------*/
/**
 * Validate all headers in a NTv2 file.
 *
 * Some unrecoverable errors are found when reading the headers,
 * but this method does an in-depth analysis of all headers and
 * their parent-child relationships.
 *
 * <p>Note that all validation messages are in English, and no
 * mechanism was setup to localize them.
 *
 * <p>Rules:
 *
 *   <ul>
 *     <li>Each grid is rectangular, and its dimensions must be an integral
 *         number of grid intervals.
 *     <li>Densified grid intervals must be an integral division of its
 *         parent grid intervals.
 *     <li>Densified boundaries must be coincident with its parent grid,
 *         enclosing complete cells.
 *     <li>Densified areas may not overlap each other.
 *   </ul>
 *
 * @param hdr A pointer to a NTV2_HDR object.
 *
 * @param fp  A stream to write all messages to.
 *            If NULL, no messages will be written, but validation
 *            will still be done. Note that in this case, only the last
 *            error encountered will be returned, although there may
 *            be numerous errors in the file.
 *
 * @return If successful,   NTV2_ERR_OK (0).
 *         If unsuccessful, NTV2_ERR_*.
 */
extern int ntv2_validate(
   NTV2_HDR *hdr,
   FILE     *fp);

/*---------------------------------------------------------------------*/

#define NTV2_DUMP_HDRS_EXT     0x01   /*!< Dump hdrs as they are in file   */
#define NTV2_DUMP_HDRS_INT     0x02   /*!< Dump hdrs as they are in memory */
#define NTV2_DUMP_HDRS_BOTH    0x03   /*!< Dump hdrs both ways             */
#define NTV2_DUMP_HDRS_SUMMARY 0x04   /*!< Dump hdr  summaries             */
#define NTV2_DUMP_DATA         0x10   /*!< Dump data for shifts            */
#define NTV2_DUMP_DATA_ACC     0x30   /*!< Dump data for shifts & accuracy */

/**
 * Dump the contents of all headers in a NTv2 file.
 *
 * @param hdr   A pointer to a NTV2_HDR object.
 *
 * @param fp    The stream to dump it to, typically stdout.
 *              If NULL, no dump will be done.
 *
 * @param mode  The dump mode.
 *              This consists of a bit mask of NTV2_DUMP_* values.
 */
extern void ntv2_dump(
   NTV2_HDR *hdr,
   FILE     *fp,
   int       mode);

/*---------------------------------------------------------------------*/
/**
 * List the contents of all headers in a NTv2 file.
 *
 * This method dumps all headers in a concise list format.
 *
 * @param hdr          A pointer to a NTV2_HDR object.
 *
 * @param fp           The stream to dump it to, typically stdout.
 *                     If NULL, no dump will be done.
 *
 * @param mode         The dump mode.
 *                     This consists of a bit mask of NTV2_DUMP_* values.
 *
 * @param do_hdr_line  TRUE to output a header line.
 */
extern void ntv2_list(
   NTV2_HDR *hdr,
   FILE     *fp,
   int       mode,
   NTV2_BOOL do_hdr_line);

/*------------------------------------------------------------------------*/
/* NTv2 transformation methods                                            */
/*------------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/

#define NTV2_STATUS_NOTFOUND      0  /*!< Point not found in any record   */
#define NTV2_STATUS_CONTAINED     1  /*!< Point is totally contained      */
#define NTV2_STATUS_NORTH         2  /*!< Point is on North border        */
#define NTV2_STATUS_WEST          3  /*!< Point is on West border         */
#define NTV2_STATUS_NORTH_WEST    4  /*!< Point is on North & West border */
#define NTV2_STATUS_OUTSIDE_CELL  5  /*!< Point is in cell just outside   */

/**
 * Find the best NTv2 sub-file record containing a given point.
 *
 * <p>This is an internal routine, and is exposed for debugging purposes.
 * Normally, users have no need to call this routine.
 *
 * @param hdr         A pointer to a NTV2_HDR object.
 * @param lon         Longitude of point (in degrees)
 * @param lat         Latitude  of point (in degrees)
 * @param pstatus     Returned status (NTV2_STATUS_*).
 *
 * @return A pointer to a NTV2_REC object or NULL.
 */
extern NTV2_REC * ntv2_find_rec(
   NTV2_HDR *hdr,
   double    lon,
   double    lat,
   int      *pstatus);

/*---------------------------------------------------------------------*/
/**
 * Perform a forward transformation on an array of points.
 *
 * @param hdr         A pointer to a NTV2_HDR object.
 *
 * @param deg_factor  The conversion factor to convert the given coordinates
 *                    to decimal degrees.
 *                    The value is degrees-per-unit.
 *
 * @param n           Number of points in the array to be transformed.
 *
 * @param coord       An array of NTV2_COORD values to be transformed.
 *
 * @return The number of points successfully transformed.
 *
 * <p>Note that this routine just calls ntv2_transform() with a direction
 * value of NTV2_CVT_FORWARD.
 */
extern int ntv2_forward(
   NTV2_HDR  *hdr,
   double     deg_factor,
   int        n,
   NTV2_COORD coord[]);

/*---------------------------------------------------------------------*/
/**
 * Perform an inverse transformation on an array of points.
 *
 * @param hdr         A pointer to a NTV2_HDR object.
 *
 * @param deg_factor  The conversion factor to convert the given coordinates
 *                    to decimal degrees.
 *                    The value is degrees-per-unit.
 *
 * @param n           Number of points in the array to be transformed.
 *
 * @param coord       An array of NTV2_COORD values to be transformed.
 *
 * @return The number of points successfully transformed.
 *
 * <p>Note that this routine just calls ntv2_transform() with a direction
 * value of NTV2_CVT_INVERSE.
 */
extern int ntv2_inverse(
   NTV2_HDR  *hdr,
   double     deg_factor,
   int        n,
   NTV2_COORD coord[]);

/*---------------------------------------------------------------------*/

#define NTV2_CVT_FORWARD    1        /*!< Convert data forward  */
#define NTV2_CVT_INVERSE    0        /*!< Convert data inverse  */
#define NTV2_CVT_REVERSE(n) (1 - n)  /*!< Reverse the direction */

/**
 * Perform a transformation on an array of points.
 *
 * @param hdr         A pointer to a NTV2_HDR object.
 *
 * @param deg_factor  The conversion factor to convert the given coordinates
 *                    to decimal degrees.
 *                    The value is degrees-per-unit.
 *
 * @param n           Number of points in the array to be transformed.
 *
 * @param coord       An array of NTV2_COORD values to be transformed.
 *
 * @param direction   The direction of the transformation
 *                    (NTV2_CVT_FORWARD or NTV2_CVT_INVERSE).
 *
 * @return The number of points successfully transformed.
 */
extern int ntv2_transform(
   NTV2_HDR  *hdr,
   double     deg_factor,
   int        n,
   NTV2_COORD coord[],
   int        direction);

/*---------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* LIBNTV2_INCLUDED */
