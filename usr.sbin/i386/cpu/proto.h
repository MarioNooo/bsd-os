/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	proto.h,v 1.13 2002/04/04 20:37:33 jch Exp
 */

/* bootparam.c */
int bootparam __P((int ac, char **av));
int disp_intr __P((int ac, char **av));
int disp_mpinfo __P((int ac, char **av));
void pmp __P((struct fps *f));

/* cpu.c */
int main __P((int ac, char **av));

/* cpucheck.c */
int cpu_check __P((int ac, char **av));
int disp_cpu __P((int ac, char **av));

/* intr.c */
void setup_intr_routing __P((void));

/* intr_clock.c */
int route_clock __P((void));

/* intr_debug.c */
void intr_sanity __P((void));
void piroute __P((struct ithdinfo *hd));

/* intr_special.c */
void route_special __P((isrc_type type));

/* intr_util.c */
void add_apic_normal __P((struct ithdinfo *itp, int apic, int pin, int level, int pol, int excl));
isrc_apic_t *add_apic_src __P((struct ithdinfo *itp, isrc_type type, int apic, int pin));
void check_local_intrs __P((void));
struct ithdinfo *find_ithd __P((int irq));
int find_pci_irq __P((struct mce_io_intr *ip, struct mce_bus *busp));
int isa_lvlmode __P((struct ithdinfo *itp));
struct ithdinfo *newthd __P((struct ithdinfo **hdp));
void upload_itree __P((struct ithdinfo *ihd));

/* kerninfo.c */
void get_kern_info __P((void));
void load_info __P((char *filename));
int save_info __P((int ac, char **av));

/* map.c */
int map_apic_id __P((int));
void map_ioapics __P((void));
void map_lapics __P((void));

/* misc.c */
void hexdump __P((void *b, int len));
void phex __P((void *data, int len));

/* mpcpu.c */
int cpu_start __P((int ac, char **av));
int cpu_stop __P((int ac, char **av));
int cpu_stat __P((int ac, char **av));
void find_valid_cpus __P((void));
int go_mp __P((int ac, char **av));

/* mpinfo.c */
int check_for_imcr __P((void));
struct mce_io_intr *get_apic_extint __P((int n));
int get_boot_processor __P((void));
struct mce_bus *get_bus __P((int n));
void *get_ent_bytype __P((int type, int num));
struct mce_io_intr *get_intr __P((int n));
struct mce_io_intr *get_isa_intr __P((int n));
struct mce_io_apic *get_io_apic __P((int id));
struct mce_lc_intr *get_lcl_extint __P((int n));
u_long get_local_apic_addr __P((int cno));
struct mce_lc_intr *get_local_intr __P((int n));
void get_mptable __P((void));
struct mce_io_intr *get_pci_intr __P((int n, struct mce_bus **buspp));
struct mce_proc *get_cpu __P((int n));
int mp_capable __P((void));

/* override.c */
int ovr __P((char *key));
char *ovrn __P((char *re, int which, int *nums));
int ovr_chk __P((char *re, char *str));
void setup_overrides __P((char **av, int ac));
int test_ovr __P((int ac, char **av));

/* pci.c */
int check_clkhack __P((void));
int disp_pci __P((int ac, char **av));

/* siomode.c */
int do_siomode __P((void));
int siomode __P((int ac, char **av));
