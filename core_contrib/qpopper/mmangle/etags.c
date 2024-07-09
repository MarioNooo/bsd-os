/* C code produced by gperf version 2.5 (GNU C++ version) */
/* Command-line: gperf -tpacCg -k* -N etags_lookup -H etags_hash -K enriched_tag etags.gperf  */ /* etags.gperf -*- C -*- Map enriched to html tags
    * Praveen Yaramada
    * gperf -tpacCg -k "*" -N etags_lookup -H etags_hash -K enriched_tag etags.gperf > etags.c
    */

#include "string.h"
#include "enriched.h"

struct etags_rec { const char *enriched_tag; const char *html_tag; int html_tag_len; EnrichedTag et; };

#define TOTAL_KEYWORDS 35
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 118
/* maximum key range = 118, duplicates = 0 */

#ifdef __GNUC__
inline
#endif
static unsigned int
etags_hash (register const char *str, register int len)
{
  static const unsigned char asso_values[] =
    {
     119, 119, 119, 119, 119, 119, 119, 119, 119, 119,
     119, 119, 119, 119, 119, 119, 119, 119, 119, 119,
     119, 119, 119, 119, 119, 119, 119, 119, 119, 119,
     119, 119, 119, 119, 119, 119, 119, 119,   5, 119,
     119, 119, 119, 119, 119, 119, 119,   0, 119, 119,
     119, 119, 119, 119, 119, 119, 119, 119, 119, 119,
       0, 119,   0, 119, 119, 119, 119, 119, 119, 119,
     119, 119, 119, 119, 119, 119, 119, 119, 119, 119,
     119, 119, 119, 119, 119, 119, 119, 119, 119, 119,
     119, 119, 119, 119, 119, 119, 119,  10,  35,  10,
      20,   0,   0,  10,   0,   0, 119, 119,   0,  20,
       0,  50,   0, 119,   0,   0,   0,   0, 119, 119,
      15,  15, 119, 119, 119, 119, 119, 119,
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 13:
        hval += asso_values[ (int) str [12] ];
      case 12:
        hval += asso_values[ (int) str [11] ];
      case 11:
        hval += asso_values[ (int) str [10] ];
      case 10:
        hval += asso_values[ (int) str [9] ];
      case 9:
        hval += asso_values[ (int) str [8] ];
      case 8:
        hval += asso_values[ (int) str [7] ];
      case 7:
        hval += asso_values[ (int) str [6] ];
      case 6:
        hval += asso_values[ (int) str [5] ];
      case 5:
        hval += asso_values[ (int) str [4] ];
      case 4:
        hval += asso_values[ (int) str [3] ];
      case 3:
        hval += asso_values[ (int) str [2] ];
      case 2:
        hval += asso_values[ (int) str [1] ];
      case 1:
        hval += asso_values[ (int) str [0] ];
    }
  return hval;
}

#ifdef __GNUC__
inline
#endif
const struct etags_rec *
etags_lookup (register const char *str, register int len)
{
  static const struct etags_rec wordlist[] =
    {
      {"",               "",                        0, 0}, 
      {">",              "&gt;",                    4, CHARTAG},
      {"<<",             "&lt;",                    4, CHARTAG},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"&",              "&amp;",                   5, CHARTAG},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<flushleft>",    "<div align=left>",       16, FLUSHLEFT},
      {"</flushleft>",   "</div>",                  6, XFLUSHLEFT},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<center>",       "<center>",                8, CENTER},
      {"</center>",      "</center>",               9, XCENTER},
      {"","",0,0}, {"","",0,0}, 
      {"<flushright>",   "<div align=right>",      17, FLUSHRIGHT},
      {"</flushright>",  "</div>",                  6, XFLUSHRIGHT},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<italic>",       "<i>",                     3, ITALIC},
      {"</italic>",      "</i>",                    4, XITALIC},
      {"","",0,0}, 
      {"<underline>",    "<u>",                     3, UNDERLINE},
      {"</underline>",   "</u>",                    4, XUNDERLINE},
      {"","",0,0}, 
      {"<excerpt>",      "<blockquote type=cite>", 22, EXCERPT},
      {"</excerpt>",     "</blockquote>",          13, XEXCERPT},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<smaller>",      "<small>",                 7, SMALLER},
      {"</smaller>",     "</small>",                8, XSMALLER},
      {"","",0,0}, 
      {"<fixed>",        "<tt>",                    4, FIXED},
      {"</fixed>",       "</tt>",                   5, XFIXED},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<param>",        "<?param >",               6, PARAM},
      {"</param>",       "\">",                     2, XPARAM},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<paraindent>",   "",                        0, PARAINDENT},
      {"</paraindent>",  "",                        0, XPARAINDENT},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<nofill>",       "<pre>",                   5, NOFILLV},
      {"</nofill>",      "</pre>",                  6, XNOFILLV},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<bigger>",       "<big>",                   5, BIGGER},
      {"</bigger>",      "</big>",                  6, XBIGGER},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<flushboth>",    "<div align=justify>",    19, FLUSHBOTH},
      {"</flushboth>",   "</div>",                  6, XFLUSHBOTH},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<fontfamily>",   "<font face=\"",          12, FONTFAMILY},
      {"</fontfamily>",  "</font>",                 7, XFONTFAMILY},
      {"","",0,0}, {"","",0,0}, 
      {"<bold>",         "<b>",                     3, BOLD},
      {"</bold>",        "</b>",                    4, XBOLD},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0}, 
      {"<color>",        "<font color=\"",         13, COLOR},
      {"</color>",       "</font>",                 7, XCOLOR},
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = etags_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].enriched_tag;

          if (*s == *str && !strncmp (str + 1, s + 1, len - 1))
            return &wordlist[key];
        }
    }
  return 0;
}
