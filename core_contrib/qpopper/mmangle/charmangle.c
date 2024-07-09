/* 
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file license.txt specifies the terms for use, modification,
 * and redistribution.
 *
 *
 * Revisions:
 *  03/09/00 [rcg]
 *          -  Cleaned up nits to appease compilers.
 *
 *  07/22/98 [py]
 *          -  Created.
 *
 */

#include "config.h"

#include <mime.h>
#include <charmangle.h>
#include <mangle.h>

typedef struct tagCHMangle {
    OutputFn        *m_pfOut;
    void            *m_pvOut;
    TextCharSetType m_partCharSet;
    TextCharSetType m_reqCharSet;
} CHMangle;

static CHMangle hm;

void *CharManglerInit ( OutputFn oFn, void *oFnState, TextCharSetType partCharSet,
                        TextCharSetType reqCharSet )
{
    hm.m_pfOut = oFn;
    hm.m_pvOut = oFnState;
    hm.m_partCharSet = partCharSet;
    hm.m_reqCharSet = reqCharSet;
    return &hm;
}

#define BLAT '_'

/* Define mappings from the ISO-8859-x charsets to Unicode.  Since all are
 * the same from 0x00 - 0xA0, we start at 0xA1, unless otherwise noted.
 * -1 is the same as Unicode, so there is no need for a mapping for it.
 * That leaves -2 through -10.
 */

static const unsigned int ISO8859_2[] = {
/* --A1--, --A2--, --A3--, --A4--, --A5--, --A6--, --A7--, --A8--, --A9--, --AA-- */
   0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7, 0x00A8, 0x0160, 0x015E,
   0x0164, 0x0179, 0x00AD, 0x017D, 0x017B, 0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4,
   0x013E, 0x015B, 0x02C7, 0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E,
   0x017C, 0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7, 0x010C,
   0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E, 0x0110, 0x0143, 0x0147,
   0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, 0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC,
   0x00DD, 0x0162, 0x00DF, 0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107,
   0x00E7, 0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F, 0x0111,
   0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7, 0x0159, 0x016F, 0x00FA,
   0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9 
};

static const unsigned int ISO8859_3[] = { 
/* --A1--, --A2--, --A3--, --A4--, --A5--, --A6--, --A7--, --A8--, --A9--, --AA-- */
   0x0126, 0x02D8, 0x00A3, 0x00A4, ' ' ,   0x0124, 0x00A7, 0x00A8, 0x0130, 0x015E, 
   0x011E, 0x0134, 0x00AD, ' ' ,   0x017B, 0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 
   0x00B5, 0x0125, 0x00B7, 0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, ' ',    
   0x017C, 0x00C0, 0x00C1, 0x00C2, ' ',    0x00C4, 0x010A, 0x0108, 0x00C7, 0x00C8, 
   0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF, ' ',    0x00D1, 0x00D2, 
   0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7, 0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 
   0x016C, 0x015C, 0x00DF, 0x00E0, 0x00E1, 0x00E2, ' ',    0x00E4, 0x010B, 0x0109, 
   0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, ' ',    
   0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7, 0x011D, 0x00F9, 0x00FA, 
   0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9
};

static const unsigned int ISO8859_4[] = {
/* --A1--, --A2--, --A3--, --A4--, --A5--, --A6--, --A7--, --A8--, --A9--, --AA-- */
   0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7, 0x00A8, 0x0160, 0x0112,
   0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF, 0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4,
   0x0129, 0x013C, 0x02C7, 0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E,
   0x014B, 0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E, 0x010C,
   0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A, 0x0110, 0x0145, 0x014C,
   0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC,
   0x0168, 0x016A, 0x00DF, 0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6,
   0x012F, 0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B, 0x0111,
   0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x0173, 0x00FA,
   0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9
};

static const unsigned int ISO8859_5[] = {
/* --A1--, --A2--, --A3--, --A4--, --A5--, --A6--, --A7--, --A8--, --A9--, --AA-- */
   0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x040A,
   0x040B, 0x040C, 0x00AD, 0x040E, 0x040F, 0x0410, 0x0411, 0x0412, 0x0413, 0x0414,
   0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
   0x041F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428,
   0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F, 0x0430, 0x0431, 0x0432,
   0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C,
   0x043D, 0x043E, 0x043F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446,
   0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F, 0x2116,
   0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045A,
   0x045B, 0x045C, 0x00A7, 0x045E, 0x045F
};

/* 0xA1 - 0xFF */
static const unsigned int ISO8859_6[] = {
/* --A1--, --A2--, --A3--, --A4--, --A5--, --A6--, --A7--, --A8--, --A9--, --AA-- */
   ' ',    ' ',    ' ',    0x00A4, ' ',    ' ',    ' ',    ' ',    ' ',    ' ',
   ' ',    0x060C, 0x00AD, ' ',    ' ',    ' ',    ' ',    ' ',    ' ',    ' ',
   ' ',    ' ',    ' ',    ' ',    ' ',    ' ',    0x061B, ' ',    ' ',    ' ',
   0x061F, ' ',    0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628,
   0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F, 0x0630, 0x0631, 0x0632,
   0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A, ' ',    ' ',
   ' ',    ' ',    ' ',    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 
   0x0647, 0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F, 0x0650, 
   0x0651, 0x0652, ' ',    ' ',    ' ',    ' ',    ' ',    ' ',    ' ',    ' ',    
   ' ',    ' ',    ' ',    ' ',    ' '
};

static const unsigned int ISO8859_7[] = {
/* --A1--, --A2--, --A3--, --A4--, --A5--, --A6--, --A7--, --A8--, --A9--, --AA-- */
   0x02BD, 0x02BC, 0x00A3, ' ',    ' ',    0x00A6, 0x00A7, 0x00A8, 0x00A9, ' ',
   0x00AB, 0x00AC, 0x00AD, ' ',    0x2015, 0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 
   0x0385, 0x0386, 0x00B7, 0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E,
   0x038F, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398,
   0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F, 0x03A0, 0x03A1, ' ',
   0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC,
   0x03AD, 0x03AE, 0x03AF, 0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6,
   0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF, 0x03C0,
   0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA,
   0x03CB, 0x03CC, 0x03CD, 0x03CE
};

/* 0x80 - 0x9F */
static const unsigned int CP1252[] = {
   0x20AC, ' '   , 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030,
   0x0160, 0x2039, 0x0152, ' '   , 0x017D, ' '   , ' '   , 0x2018, 0x2019, 0x201C,
   0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, ' '  ,
   0x017E, 0x0178
};


static unsigned int Iso8859_to_Unicode( TextCharSetType CharSet, unsigned char character )
{
    switch( CharSet ) {
    case us_ascii:
    return (character & 0x7F);
    case iso_8859_1:
    return (character & 0xFF);
    case iso_8859_2:
    return ( character < 0xA1 ) ? character : ISO8859_2[character - 0xA1];
    case iso_8859_3:
    return ( character < 0xA1 ) ? character : ISO8859_3[character - 0xA1];
    case iso_8859_4:
    return ( character < 0xA1 ) ? character : ISO8859_4[character - 0xA1];
    case iso_8859_5:
    return ( character < 0xA1 ) ? character : ISO8859_4[character - 0xA1];
    case iso_8859_6:
    return ( character < 0xA1 ) ? character : ISO8859_4[character - 0xA1];
    case cp1252:
    return ( character >= 0x80 && character <= 0x9F ) ?  CP1252[character - 0x80] 
        : character;
    default:
    /* todo for the rest of ISO8859. */
    break;
    }
    return character;
}


/*
 * Actually UCS-4
 */
static void Unicode_to_Utf8( unsigned int unicode, unsigned char *buf, int *nSize )
{
    int i;
    if( unicode <= 0x0000007F ) {
    *nSize = 1;
    buf[0] = unicode & 0x0000007F;
    }
    else if( unicode >= 0x00000080 && unicode <= 0x000007FF ){
    *nSize = 2;
    buf[0] = 0xC0 | ( (unicode >> 0x06) & 0x1F );
    }
    else if( unicode >= 0x00000800 && unicode <= 0x0000FFFF ){
    *nSize = 3;
    buf[0] = 0xE0 | ( (unicode >> 0x0C) & 0x0F );
    }
    else if( unicode >= 0x00010000 && unicode <= 0x001FFFFF ){
    *nSize = 4;
    buf[0] = 0xF0 | ( (unicode >> 0x12) & 0x07 );
    }
    else if( unicode >= 0x00200000 && unicode <= 0x03FFFFFF ){
    *nSize = 5;
    buf[0] = 0xF8 | ( (unicode >> 0x18) & 0x03 );
    }
    else if( unicode >= 0x04000000 && unicode <= 0x7FFFFFFF ){
    *nSize = 6;
    buf[0] = 0xFC | ( (unicode >> 0x1E) & 0x01 );
    }
    for( i = 1; i < *nSize; i++ )
    buf[i] = 0x80 | ( (unicode >> (0x06 * (*nSize - i - 1))) & 0x3F );
}

void CharMangler ( void *state, char *szText, long len )
{
    int i;
    (void) len;
    (void) state;


    switch ( hm.m_reqCharSet ) {

        case us_ascii:
            switch ( hm.m_partCharSet ) {
                case utf_8:
                    for ( i = 0; i < len; i++ ) {
                        switch ( szText[i] & 0xC0 ) {
                            case 0xC0:
                                szText[i] = BLAT;
                                break;
                            case 0x80:
                                break;
                            default:
                                hm.m_pfOut ( hm.m_pvOut, &szText[i], 1 );
                                break;
                        } /* switch ( szText[i] & 0xC0 ) */
                    } /* for loop */
                    break;
                case us_ascii: /* Pass */
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
                    break;  
                default:
                    /* Filter the octets with b7 set */
                    for ( i = 0; i < len; i++ ) {
                        if ( ( (unsigned char) szText[i] != 0xA0 ) && 
                             ( (unsigned char) szText[i]  & 0x80 ) ) {
                            szText [ i ] = BLAT;
                        }
                    } /* for loop */
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
            } /* switch ( hm.m_partCharSet ) */
            break;

        case iso_8859_1:
        case iso_8859_2:
        case iso_8859_3:
        case iso_8859_4:
        case iso_8859_9:
        case iso_8859_10:  /* Latin 1 thru 5 */
            switch ( hm.m_partCharSet ) {
                case us_ascii:
                case iso_8859_1:
                case iso_8859_2:
                case iso_8859_3:
                case iso_8859_4:
                case iso_8859_9:
                case iso_8859_10: /* Pass */
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
                    break;
                case utf_8:
                    /* todo */
                    break;
                case iso_8859_5:
                case iso_8859_6:
                case iso_8859_7:
                case iso_8859_8: /* Octets with b7 set replaced with 'blat' */
                default:
                    for ( i = 0; i < len; i++ ) {
                        if ( szText[i] & 0x80 ) {
                            szText[i] = '_';
                        }
                    } /* for loop */
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
                    break;
            } /* switch ( hm.m_partCharSet ) */
            break;

        case iso_8859_5:
        case iso_8859_6:
        case iso_8859_7:
        case iso_8859_8:
            switch ( hm.m_partCharSet ) {
                case utf_8:
                    break;
                default:
                    /* Filter the octets with b7 set */
                    for ( i = 0; i < len; i++ ) {
                        if ( szText[i] & 0x80 ) {
                            szText[i] = '_';
                        }
                    } /* for loop */
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
            } /* switch ( hm.m_partCharSet ) */
            break;

        case cp1252:
            switch ( hm.m_partCharSet ) {
                case us_ascii:
                case iso_8859_1:
                case iso_8859_2:
                case iso_8859_3:
                case iso_8859_4:
                case iso_8859_9:
                case iso_8859_10:
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
                    break;
                default:
                    /* Filter the octets with b7 set */
                    for ( i = 0; i < len; i++ ) {
                        if ( szText[i] & 0x80 ) {
                            szText[i] = '_';
                        }
                    } /* for loop */
                    hm.m_pfOut ( hm.m_pvOut, szText, len) ;
            } /* switch ( hm.m_partCharSet ) */
            break;

        case iso_2022_jp:
            break;

        case utf_8:
            switch ( hm.m_partCharSet ) {
                case utf_8:
                case us_ascii:
                    /* Pass */
                    hm.m_pfOut ( hm.m_pvOut, szText, len );
                    break;
                case iso_8859_1:
                case iso_8859_2:
                case iso_8859_3:
                case iso_8859_4:
                case iso_8859_5:
                case iso_8859_6:
                case iso_8859_7:
                case iso_8859_8:
                case iso_8859_9:
                    { /* local block */
                    unsigned char szBuf[6];
                    int  nS = 6;
                    unsigned int unicode;

                    for ( i = 0; i < len; i++ ) {
                        if ( ( (unsigned char) szText[i] ) > 0xA0 ) {
                            unicode = Iso8859_to_Unicode ( hm.m_partCharSet, szText[i] );
                            Unicode_to_Utf8 ( unicode, szBuf, &nS );
                            hm.m_pfOut ( hm.m_pvOut, (char *)szBuf, nS );
                        }
                        else
                            hm.m_pfOut ( hm.m_pvOut, &szText[i], 1 );
                    } /* for loop */
                    } /* local block */
                    break;
                case iso_8859_10:
                    /* to do */
                    break;
                case iso_8859_11:
                    /* to do */
                    break;
                case iso_8859_12:
                    /* to do */
                    break;
                case iso_8859_13:
                    /* to do */
                    break;
                case iso_8859_14:
                    /* to do */
                    break;
                case iso_8859_15:
                    /* to do */
                    break;
                case cp1252:
                    /* to do */
                    break;
                case iso_2022_jp:
                    /* to do */
                    break;
            } /* switch ( hm.m_partCharSet ) */
            break;

        default:
            break;
    } /* switch ( hm.m_reqCharSet ) */
}

