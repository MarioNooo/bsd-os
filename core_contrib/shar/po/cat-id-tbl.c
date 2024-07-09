/* Automatically generated by po2tbl.sed from sharutils.pot.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "libgettext.h"

const struct _msg_ent _msg_tbl[] = {
  {"", 1},
  {"Unknown system error", 2},
  {"%s: option `%s' is ambiguous\n", 3},
  {"%s: option `--%s' doesn't allow an argument\n", 4},
  {"%s: option `%c%s' doesn't allow an argument\n", 5},
  {"%s: option `%s' requires an argument\n", 6},
  {"%s: unrecognized option `--%s'\n", 7},
  {"%s: unrecognized option `%c%s'\n", 8},
  {"%s: illegal option -- %c\n", 9},
  {"%s: invalid option -- %c\n", 10},
  {"%s: option requires an argument -- %c\n", 11},
  {"memory exhausted", 12},
  {"WARNING: not restoring timestamps.  Consider getting and", 13},
  {"installing GNU \\`touch', distributed in GNU File Utilities...", 14},
  {"creating lock directory", 15},
  {"failed to create lock directory", 16},
  {"Too many directories for mkdir generation", 17},
  {"creating directory", 18},
  {"Cannot access %s", 19},
  {"-C is being deprecated, use -Z instead", 20},
  {"Cannot get current directory name", 21},
  {"Must unpack archives in sequence!", 22},
  {"Please unpack part", 23},
  {"next!", 24},
  {"%s: Not a regular file", 25},
  {"In shar: remaining size %ld\n", 26},
  {"Newfile, remaining %ld, ", 27},
  {"Limit still %d\n", 28},
  {"restore of", 29},
  {"failed", 30},
  {"End of part", 31},
  {"continue with part", 32},
  {"Starting file %s\n", 33},
  {"empty", 34},
  {"(empty)", 35},
  {"Cannot open file %s", 36},
  {"compressed", 37},
  {"gzipped", 38},
  {"binary", 39},
  {"(compressed)", 40},
  {"(gzipped)", 41},
  {"(binary)", 42},
  {"Could not fork", 43},
  {"File %s (%s)", 44},
  {"text", 45},
  {"(text)", 46},
  {"overwriting", 47},
  {"overwrite", 48},
  {"[no, yes, all, quit] (no)?", 49},
  {"extraction aborted", 50},
  {"SKIPPING", 51},
  {"(file already exists)", 52},
  {"Saving %s (%s)", 53},
  {"extracting", 54},
  {"End of", 55},
  {"archive", 56},
  {"part", 57},
  {"File", 58},
  {"is continued in part", 59},
  {"Please unpack part 1 first!", 60},
  {"STILL SKIPPING", 61},
  {"continuing file", 62},
  {"is complete", 63},
  {"uudecoding file", 64},
  {"uncompressing file", 65},
  {"gunzipping file", 66},
  {"MD5 check failed", 67},
  {"original size", 68},
  {"current size", 69},
  {"Opening `%s'", 70},
  {"Closing `%s'", 71},
  {"Try `%s --help' for more information.\n", 72},
  {"Usage: %s [OPTION]... [FILE]...\n", 73},
  {"Mandatory arguments to long options are mandatory for short options too.\n", 74},
  {"\
\n\
Giving feedback:\n\
      --help              display this help and exit\n\
      --version           output version information and exit\n\
  -q, --quiet, --silent   do not output verbose messages locally\n\
\n\
Selecting files:\n\
  -p, --intermix-type     allow -[BTzZ] in file lists to change mode\n\
  -S, --stdin-file-list   read file list from standard input\n\
\n\
Splitting output:\n\
  -o, --output-prefix=PREFIX    output to file PREFIX.01 through PREFIX.NN\n\
  -l, --whole-size-limit=SIZE   split archive, not files, to SIZE kilobytes\n\
  -L, --split-size-limit=SIZE   split archive, or files, to SIZE kilobytes\n", 75},
  {"\
\n\
Controlling the shar headers:\n\
  -n, --archive-name=NAME   use NAME to document the archive\n\
  -s, --submitter=ADDRESS   override the submitter name\n\
  -a, --net-headers         output Submitted-by: & Archive-name: headers\n\
  -c, --cut-mark            start the shar with a cut line\n\
\n\
Selecting how files are stocked:\n\
  -M, --mixed-uuencode         dynamically decide uuencoding (default)\n\
  -T, --text-files             treat all files as text\n\
  -B, --uuencode               treat all files as binary, use uuencode\n\
  -z, --gzip                   gzip and uuencode all files\n\
  -g, --level-for-gzip=LEVEL   pass -LEVEL (default 9) to gzip\n\
  -Z, --compress               compress and uuencode all files\n\
  -b, --bits-per-code=BITS     pass -bBITS (default 12) to compress\n", 76},
  {"\
\n\
Protecting against transmission:\n\
  -w, --no-character-count      do not use `wc -c' to check size\n\
  -D, --no-md5-digest           do not use `md5sum' digest to verify\n\
  -F, --force-prefix            force the prefix character on every line\n\
  -d, --here-delimiter=STRING   use STRING to delimit the files in the shar\n\
\n\
Producing different kinds of shars:\n\
  -V, --vanilla-operation   produce very simple and undemanding shars\n\
  -P, --no-piping           exclusively use temporary files at unshar time\n\
  -x, --no-check-existing   blindly overwrite existing files\n\
  -X, --query-user          ask user before overwriting files (not for Net)\n\
  -m, --no-timestamp        do not restore file modification dates & times\n\
  -Q, --quiet-unshar        avoid verbose messages at unshar time\n\
  -f, --basename            restore in one directory, despite hierarchy\n\
      --no-i18n             do not produce internationalized shell script\n", 77},
  {"\
\n\
Option -o is required with -l or -L, option -n is required with -a.\n\
Option -g implies -z, option -b implies -Z.\n", 78},
  {"DEBUG was not selected at compile time", 79},
  {"Hard limit %dk\n", 80},
  {"Soft limit %dk\n", 81},
  {"WARNING: No user interaction in vanilla mode", 82},
  {"WARNING: Non-text storage options overridden", 83},
  {"No input files", 84},
  {"Cannot use -a option without -n", 85},
  {"Cannot use -l or -L option without -o", 86},
  {"PLEASE avoid -X shars on Usenet or public networks", 87},
  {"You have unpacked the last part", 88},
  {"Created %d files\n", 89},
  {"Found no shell commands in %s", 90},
  {"%s looks like raw C code, not a shell archive", 91},
  {"Found no shell commands after `cut' in %s", 92},
  {"%s is probably not a shell archive", 93},
  {"The `cut' line was followed by: %s", 94},
  {"Starting `sh' process", 95},
  {"\
Mandatory arguments to long options are mandatory for short options too.\n\
\n\
  -d, --directory=DIRECTORY   change to DIRECTORY before unpacking\n\
  -c, --overwrite             pass -c to shar script for overwriting files\n\
  -e, --exit-0                same as `--split-at=\"exit 0\"'\n\
  -E, --split-at=STRING       split concatenated shars after STRING\n\
  -f, --force                 same as `-c'\n\
      --help                  display this help and exit\n\
      --version               output version information and exit\n\
\n\
If no FILE, standard input is read.\n", 96},
  {"Cannot chdir to `%s'", 97},
  {"standard input", 98},
  {"%s: Short file", 99},
  {"%s: No `end' line", 100},
  {"%s: data following `=' padding character", 101},
  {"%s: illegal line", 102},
  {"%s: No `begin' line", 103},
  {"%s: Illegal ~user", 104},
  {"%s: No user `%s'", 105},
  {"Usage: %s [FILE]...\n", 106},
  {"\
Mandatory arguments to long options are mandatory to short options too.\n\
  -h, --help               display this help and exit\n\
  -v, --version            output version information and exit\n\
  -o, --output-file=FILE   direct output to FILE\n", 107},
  {"Read error", 108},
  {"Usage: %s [INFILE] REMOTEFILE\n", 109},
  {"\
\n\
  -h, --help      display this help and exit\n\
  -m, --base64    use base64 encoding as of RFC1521\n\
  -v, --version   output version information and exit\n", 110},
  {"Write error", 111},
};
int _msg_tbl_length = 111;