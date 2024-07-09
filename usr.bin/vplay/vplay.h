/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI vplay.h,v 1.6 1997/08/18 18:07:16 ewv Exp
 */

/*
 * Prototypes
 */
/* vplay.c */
int main __P((int argc, char **argv));
void play_file __P((char *name));
void record_file __P((char *name));
void audio_out __P((FILE *fp, u_long count));
void audio_in __P((FILE *fp, u_long count));
void set_stereo __P((int new));
void set_bits __P((int new));
void set_speed __P((int new));
void sync_dsp __P((void));
u_long calc_count __P((void));
int cvt_to_mono __P((u_char *buf, int count, int bits));
int cvt_to_8 __P((void *buf, int count));
void usage __P((void));

/* wav.c */
int wav_chkhdr __P((void *buffer, char *name));
u_long wav_finddata __P((FILE *fp, char *name));
void wr_wav_hdr __P((FILE *fp, u_long cnt));
void set_wavparms __P((void *buf));

/* voc.c */
int voc_chkhdr __P((void *buffer));
void play_voc __P((FILE *fp, char *name));
void wr_voc_hdr __P((FILE *fp, u_long cnt));
void wr_voc_trail __P((FILE *fp));
void write_zeros __P((unsigned int len));

typedef enum {
	Voc,
	Wav,
	Raw,
	Unk
} AudioType;

typedef enum {
	Play,
	Record
} Direction;

/* Size of zero buffer used for voc silence */
#define ZBUF_SIZE		128
#define DEFAULT_DSP_SPEED	8000
#define DEFAULT_DEV		"/dev/dsp"

#define	d_printf(x)		if (debug) fprintf x

typedef struct {
	int speed;
	int bits;
	int stereo;
} DspSettings;

extern DspSettings dsp;
extern DspSettings user;
extern DspSettings fset;
extern int verbose;
extern int debug;
extern u_char *audiobuf;
extern int abuf_size;
extern int audio;
extern int timelimit;

/* Definitions for .VOC files */

/* Max size of a voc audio block */
#define VOC_MAXSEG	(16 * 1024 * 1024 - 1)

#define MAGIC_STRING	"Creative Voice File\0x1A"
#define ACTUAL_VERSION	0x010A
#define VOC_SAMPLESIZE	8

#define MODE_MONO	0
#define MODE_STEREO	1

#define DATALEN(bp)	((u_long)(bp->datalen) | \
                         ((u_long)(bp->datalen_m) << 8) | \
                         ((u_long)(bp->datalen_h) << 16) )

typedef struct _vocheader {
  u_char  magic[20];	/* must be MAGIC_STRING */
  u_short headerlen;	/* Headerlength, should be 0x1A */
  u_short version;	/* VOC-file version */
  u_short coded_ver;	/* 0x1233-version */
} VocHeader;

typedef struct _voc_b1 {
	u_char	len[3];
	u_char	tc;
	u_char	pack;
} Voc_B1;

typedef struct _voc_b3 {
	u_char	len[3];
	u_char	period[2];
	u_char	tc;
} Voc_B3;

typedef struct _voc_b4 {
	u_char	len[3];
	u_char	mark[2];
} Voc_B4;

typedef struct _voc_b6 {
	u_char	len[3];
	u_char	cnt[2];
} Voc_B6;

typedef struct _voc_b8 {
	u_char	len[3];
	u_char	tc[2];
	u_char	pack;
	u_char	mode;
} Voc_B8;

typedef struct _voc_b9 {
	u_char	len[3];
	u_char	rate[4];
	u_char	bits;
	u_char	chan;
	u_char	fmt[2];
	u_char	res[4];
} Voc_B9;

typedef struct _ext_block {
  u_short tc;
  u_char  pack;
  u_char  mode;
} Ext_Block;


/* Definitions for Microsoft WAVE format */

#define RIFF		0x46464952	
#define WAVE		0x45564157
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* Simplification but good enough for everyday use */
typedef struct _waveheader {
	u_long	main_chunk;	/* 'RIFF' */
	u_long	length;		/* filelen */
	u_long	chunk_type;	/* 'WAVE' */

	u_long	sub_chunk;	/* 'fmt ' */
	u_long	sc_len;		/* length of sub_chunk, =16 */
	u_short	format;		/* should be 1 for PCM-code */
	u_short	modus;		/* 1 Mono, 2 Stereo */
	u_long	sample_fq;	/* frequence of sample */
	u_long	byte_p_sec;
	u_short	byte_p_spl;	/* samplesize; 1 or 2 bytes */
	u_short	bit_p_spl;	/* 8, 12 or 16 bit */ 
} WaveHeader;
