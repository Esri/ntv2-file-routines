## ntv2-file-routines

### Introduction

The NTv2 (National Transformation Version 2) file standard was developed
by the Canadian government in 1995 to describe shift data for
transformations of geographic points from one datum to another
(originally from NAD27 to NAD83). This file format has since then
come into world-wide use, with over a dozen countries publishing NTv2
files for their areas. Unfortunately, there is a dearth of documentation
on the contents and syntax of NTv2 files, and there are no easily available
programs for creating and/or dumping them. This distribution is intended
to address this need.

This package contains a C library, some sample programs to create, dump,
and use NTv2 files, and some sample NTv2 files to test with.

For further information, see the Canadian document
[NTv2 (National Transformation Version 2)](
http://www.mgs.gov.on.ca/stdprodconsume/groups/content/@mgs/@iandit/documents/resourcelist/stel02_047447.pdf).
The relevant section is "Appendix B: NTv2 Developers Guide".

### NTv2 file formats

The original NTv2 file format, as documented by the Canadian government,
is for a binary file. However, unlike ascii files, writing a binary file
requires a specific program designed for that particular data layout.
The most useful type of such a program would be one that would convert
a ascii file to its binary equivalent.

Therefore, this package describes a proposed ascii file syntax for
NTv2 files that corresponds to the binary format. Using this ascii
format, it is easy to create a file that contains all the data, which
can then be converted to a standard binary file.

All binary NTv2 files have the extension ".gsb", for "Grid Shift Binary".
Correspondingly, for the ascii versions of the file, we propose the
extension ".gsa", for "Grid Shift Ascii". We will refer to these files
as GSB and GSA files. The sample programs included here automatically
determine whether a NTv2 file is binary or ascii by inspecting the
extension of the file name (not the contents of the file).

Both binary and ascii NTv2 files are laid out as follows:

<pre>
   overview record

   sub-file record 1
      grid-shift records for sub-file 1
   ...
   sub-file record n (if present)
      grid-shift records for sub-file n

   end record
</pre>

There must be at least one sub-file record, and it must be a parent
record. There can be any number of other parent records, and the parent
records can have children, which may also have children themselves
(and so on). The sub-file records can appear in any order; the parent/child
relationships are denoted by each record identifying its parent (if one)
by name.

See the section "GSA file syntax" below for details specific to
the proposed ascii format.

### The "ntv2_file" program

The "ntv2_file" program provides the ability to copy NTv2 files, dump
the contents of NTv2 files, and validate the headers of NTv2 files.
Once a user has created a GSA containing his data, he can then easily
use this program to create a standard GSB file from it for distribution.

The usage (from executing "ntv2_file -help") is:

    Usage: ntv2_file [options] file ...
    Options:
      -?, -help  Display help
      -v         Validate files
      -i         Ignore errors

      -l         List header info
      -h         Dump header info
      -s         Dump header summaries
      -d         Dump shift data
      -a         Dump shift data and accuracies

      -B         Write    big-endian binary file
      -L         Write little-endian binary file
      -N         Write native-endian binary file
                   (default is same as input file)

      -o file    Specify output file
      -e wlon slat elon nlat   Specify extent

A very important option is the "-v" (validate) option, which tells the
program to perform an extensive analysis of the files for consistency
and adherence with the NTv2 standard. It is highly recommended that this
option be used prior to creating a file for distribution to insure it
is valid.

Note that some NTv2 files are quite large, as they cover an entire country.
For example, Australia's "National_84_02_07_01.gsb" file is 12MB,
Canada's "Ntv2_0.gsb" file is 13MB, and Japan's "tky2jgd.gsb" file is 130MB.
Thus, this program also includes the option of creating a new smaller file
that is cut down by a specified extent. This may be of significant use
for inclusion in mobile environments where available memory may be at a
premium.

This program can be used to create a binary file from an ascii file
(GSA -> GSB) or to create an ascii file from a binary file (GSB -> GSA).
In fact, the sample ascii file included here (mne.gsa) was created
with the command:

    ntv2_file -o mne.gsa mne.gsb

If the program is used to copy one file to another, then only one input
file can be named on the command line. If it is used to list, dump,
or validate files, then multiple files can be specified.

Note that when writing a new file, the input and output filenames
may refer to the same file. This is convienient for "cleaning up"
a file for distribution.

### The "ntv2_cvt" program

The "ntv2_cvt" program is a sample program illustrating how to use
NTv2 files and the ntv2 transform routines to do either a forward
or an inverse transformation.

The usage (from executing "ntv2_cvt -help") is:

    Usage: ntv2_cvt [options] ntv2file [lat lon] ...
    Options:
      -?, -help  Display help
      -r         Reversed data (lon lat) instead of (lat lon)
      -d         Read shift data on the fly (no load of data)
      -i         Inverse transformation
      -f         Forward transformation       (default)

      -c val     Conversion: degrees-per-unit (default is 1)
      -s str     Use str as output separator  (default is " ")
      -p file    Read points from file        (default is "-" or stdin)
      -e wlon slat elon nlat   Specify extent

    If no coordinate pairs are specified on the command line, then
    they are read one per line from the specified data file.

This program can use either a GSB or a GSA file, so it can be useful
in testing an ascii file prior to converting it to a binary file
for distribution.

Examples of use:

    ntv2_cvt -f mne.gsb 42 19
    >> 42.00029960410205 18.99494761687853

    ntv2_cvt -i mne.gsb 42 19
    >> 41.99970017877432 19.00505294762521

    ntv2_cvt -f mne.gsb 42 19 | ntv2_cvt -i mne.gsb
    >> 42 19

    ntv2_cvt -i mne.gsb 42 19 | ntv2_cvt -f mne.gsb
    >> 42 19

### GSA file syntax

This section describes the syntax of an ascii NTv2 file (a GSA file).

Rules for ascii files are:

   1.  Anything following a # will be considered a comment and will
       be discarded.

   2.  Whitespace is used to delimit tokens in each line, but extraneous
       whitespace outside of quotes is ignored.

   3.  Blank lines are discarded. This is done after removing any comments
       and all leading and trailing whitespace.

   4.  Overview and sub-file records each consist of 11 lines, and each
       line contains a name and a value. The names must be as specified,
       and the lines must appear in the order specified.

   5.  Grid-shift records contain either two or four floating point numbers:
          lat-shift lon-shift [ lat-accuracy lon-accuracy ]
       If the accuracies are not specified, they are assumed to be 0.

   6.  All names and string values have a maximum length of 8 characters,
       and will be silently truncated if longer.

   7.  All floating point numbers are displayed with a '.' as the
       decimal separator. This is because these files could be used
       anywhere, and so must be locale-neutral.

       Text files will be read in properly even if decimal separators
       other than a '.' are used (and they match the separators for
       the current locale), but they will always be written out using a
       decimal point. This allows ease in creating an ascii file using
       programs that can't output locale-neutral number strings.

A description of the fields in each record type is as follows.

Overview record fields:

<pre>
   name      type    description
   --------  ------  ---------------------------------------------
   NUM_OREC  number  Number of overview record fields - must be 11
   NUM_SREC  number  Number of sub-file record fields - must be 11
   NUM_FILE  number  Number of sub-files
   GS_TYPE   string  Grid-shift units: "SECONDS", "MINUTES", or "DEGREES"
   VERSION   string  Version ID of distortion model
   SYSTEM_F  string  From reference system
   SYSTEM_T  string  To   reference system
   MAJOR_F   double  From semi-major axis (in meters)
   MINOR_F   double  From semi-minor axis (in meters)
   MAJOR_T   double  To   semi-major axis (in meters)
   MINOR_T   double  To   semi-minor axis (in meters)
</pre>

   Notes:

      1. There is nothing in the Canadian documentation describing
         the content of the VERSION, SYSTEM_F, and SYSTEM_T fields.
         However, all the Canadian NTv2 files specify "VERSION NTv2.0".

      2. Interestingly, all Canadian files use DATUM_F and DATUM_T
         instead of SYSTEM_F and SYSTEM_T, even though it is their
         specification. Don't do that.

      3. All published NTv2 files that we have seen have a GS_TYPE of
         "SECONDS". This has the advantage that the latitude, longitude,
         and increment values in the sub-file records are then usually
         whole numbers.

Sub-file record fields:

<pre>
   name      type    description
   --------  ------  ---------------------------------------------
   SUB_NAME  string  Sub-file name
   PARENT    string  Parent file name or "NONE"
   CREATED   string  Creation date
   UPDATED   string  Last revision date (may be blank)
   S_LAT     double  South latitude      (in gs-units)
   N_LAT     double  North latitude      (in gs-units)
   E_LONG    double  East  longitude     (in gs-units) Note: +west/-east
   W_LONG    double  West  longitude     (in gs-units) Note: +west/-east
   LAT_INC   double  Latitude  increment (in gs-units)
   LONG_INC  double  Longitude increment (in gs-units)
   GS_COUNT  number  Number of grid-shift records following
</pre>

   Notes:

      1. There is no description of the format of the CREATED and
         UPDATED fields. However, for consistency, we recommend using
         "YYYYMMDD", with the UPDATED field being blank if the data
         has not been updated since the creation date. (Many published
         NTv2 files have their UPDATED value the same as their CREATED
         value.)

      2. Longitudes are reversed from normal usage, with values
         west of Greenwich being positive, and values east of Greenwich
         being negative. This is probably because all of Canada (the
         creators of this file format) is west of Greenwich.

      3. Parent files may not overlap, but they may touch at an edge
         or a corner. The same rule applies to each generation of
         sub-files. Sub-files must properly nest inside their parents.

Grid-shift record:

<pre>
   Two or four numbers per line:
      Latitude  shift    (in gs-units)
      Longitude shift    (in gs-units)
      Latitude  accuracy (in meters  ) - optional
      Longitude accuracy (in meters  ) - optional
</pre>

   Notes:

      1. Grid-shift records are stored in latitude rows ranging from South
         to North, with the longitude entries in each row ranging from
         East to West:
            NROWS = ( (N_LAT  - S_LAT ) / LAT_INC ) + 1
            NCOLS = ( (W_LONG - E_LONG) / LONG_INC) + 1
         GS_COUNT should be equal to (NROWS * NCOLS).

      2. The longitude shift value is positive-west / negative-east.

      3. Many published NTv2 files have all their accuracy values set
         to either 0 or -1, which we assume indicates that no accuracy
         data is provided. No GIS software that we know of makes use of
         the accuracy data.

      4. If accuracy values are not specified, they will be set to 0.

End record:

<pre>
   END
</pre>

### Sample GSA file contents

Example (from the mne.gsa file):

<pre>
    NUM_OREC 11
    NUM_SREC 11
    NUM_FILE 1
    GS_TYPE  SECONDS
    VERSION  NTv2.0
    SYSTEM_F RS_MNE
    SYSTEM_T ETRS89
    MAJOR_F  6377397.155
    MINOR_F  6356078.963
    MAJOR_T  6378137.0
    MINOR_T  6356752.314

    SUB_NAME RS_MNE
    PARENT   NONE
    CREATED  09-02-12
    UPDATED  09-02-12
    S_LAT    150585.0
    N_LAT    156855.0
    E_LONG   -73410.0
    W_LONG   -66270.0
    LAT_INC  165.0
    LONG_INC 210.0
    GS_COUNT 1365

    1.28485203 18.76392555 0.0 0.0
    1.279158   18.73803139 0.0 0.0
    ...

    END
</pre>

### The NTv2 library API

This library contains the following API methods:

<pre>
   ntv2_filetype()     Determine the filetype of an NTv2 file (binary or ascii)
   ntv2_errmsg()       Convert an error code to a string

   ntv2_load_file()    Load   an NTv2 file into memory
   ntv2_write_file()   Write  an NTv2 object to a file
   ntv2_delete()       Delete an NTv2 object

   ntv2_validate()     Validate the contents of an NTv2 file
   ntv2_dump()         Dump     the contents of an NTv2 file
   ntv2_list()         List     the headers  in an NTv2 file

   ntv2_find_rec()     Find an NTv2 record for a point (debugging call)
   ntv2_forward()      Do a  forward transformation on an array of points
   ntv2_inverse()      Do an inverse transformation on an array of points
   ntv2_transform()    Do a  fwd/inv transformation on an array of points
</pre>

This library is documented in detail [here](
https://raw.github.com/Esri/ntv2-file-routines/master/doc/html/index.html).

### Issues

Find a bug or want to request a new feature?  Please let us know by submitting
an issue.

### Contributing

Esri welcomes contributions from anyone and everyone. Please see our [guidelines for contributing](https://github.com/esri/contributing).

### Licensing

Copyright 2010-2013 Esri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

A copy of the license is available in the repository's [license.txt](
https://raw.github.com/Esri/ntv2-file-routines/master/license.txt) file.

[](Esri Tags: NTV2 transformation)
[](Esri Language: C)
