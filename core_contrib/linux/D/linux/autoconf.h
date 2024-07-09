/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED

/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1

/*
 * Loadable module support
 */
#define CONFIG_MODULES 1
#define CONFIG_MODVERSIONS 1
#define CONFIG_KERNELD 1

/*
 * General setup
 */
#define CONFIG_MATH_EMULATION 1
#define CONFIG_NET 1
#undef  CONFIG_MAX_16M
#define CONFIG_PCI 1
#undef  CONFIG_PCI_OPTIMIZE
#define CONFIG_SYSVIPC 1
#define CONFIG_BINFMT_AOUT 1
#define CONFIG_BINFMT_ELF 1
#undef  CONFIG_BINFMT_JAVA
#define CONFIG_KERNEL_ELF 1
#define CONFIG_M386 1
#undef  CONFIG_M486
#undef  CONFIG_M586
#undef  CONFIG_M686

/*
 * Floppy, IDE, and other block devices
 */
#define CONFIG_BLK_DEV_FD 1
#define CONFIG_BLK_DEV_IDE 1

/*
 * Please see Documentation/ide.txt for help/info on IDE drives
 */
#undef  CONFIG_BLK_DEV_HD_IDE
#define CONFIG_BLK_DEV_IDECD 1
#define CONFIG_BLK_DEV_IDETAPE 1
#define CONFIG_BLK_DEV_IDEFLOPPY 1
#undef  CONFIG_BLK_DEV_IDESCSI
#define CONFIG_BLK_DEV_IDE_PCMCIA 1
#define CONFIG_BLK_DEV_CMD640 1
#define CONFIG_BLK_DEV_CMD640_ENHANCED 1
#define CONFIG_BLK_DEV_RZ1000 1
#define CONFIG_BLK_DEV_TRITON 1
#undef  CONFIG_IDE_CHIPSETS

/*
 * Additional Block Devices
 */
#undef  CONFIG_BLK_DEV_LOOP
#define CONFIG_BLK_DEV_LOOP_MODULE 1
#define CONFIG_BLK_DEV_MD 1
#undef  CONFIG_MD_LINEAR
#define CONFIG_MD_LINEAR_MODULE 1
#undef  CONFIG_MD_STRIPED
#define CONFIG_MD_STRIPED_MODULE 1
#define CONFIG_BLK_DEV_RAM 1
#define CONFIG_BLK_DEV_INITRD 1
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_BLK_DEV_HD

/*
 * Networking options
 */
#define CONFIG_FIREWALL 1
#define CONFIG_NET_ALIAS 1
#define CONFIG_INET 1
#define CONFIG_IP_FORWARD 1
#define CONFIG_IP_MULTICAST 1
#define CONFIG_SYN_COOKIES 1
#define CONFIG_IP_FIREWALL 1
#define CONFIG_IP_FIREWALL_VERBOSE 1
#define CONFIG_IP_MASQUERADE 1

/*
 * Protocol-specific masquerading support will be built as modules.
 */
#undef  CONFIG_IP_MASQUERADE_IPAUTOFW
#define CONFIG_IP_MASQUERADE_ICMP 1
#define CONFIG_IP_TRANSPARENT_PROXY 1
#undef  CONFIG_IP_ALWAYS_DEFRAG
#define CONFIG_IP_ACCT 1
#undef  CONFIG_IP_ROUTER
#undef  CONFIG_NET_IPIP
#define CONFIG_NET_IPIP_MODULE 1
#undef  CONFIG_IP_MROUTE
#undef  CONFIG_IP_ALIAS
#define CONFIG_IP_ALIAS_MODULE 1

/*
 * (it is safe to leave these untouched)
 */
#undef  CONFIG_INET_PCTCP
#undef  CONFIG_INET_RARP
#define CONFIG_INET_RARP_MODULE 1
#undef  CONFIG_NO_PATH_MTU_DISCOVERY
#define CONFIG_IP_NOSR 1
#define CONFIG_SKB_LARGE 1

/*
 *  
 */
#undef  CONFIG_IPX
#define CONFIG_IPX_MODULE 1
#undef  CONFIG_ATALK
#define CONFIG_ATALK_MODULE 1
#undef  CONFIG_AX25
#undef  CONFIG_BRIDGE
#undef  CONFIG_NETLINK

/*
 * SCSI support
 */
#define CONFIG_SCSI 1

/*
 * SCSI support type (disk, tape, CD-ROM)
 */
#define CONFIG_BLK_DEV_SD 1
#undef  CONFIG_CHR_DEV_ST
#define CONFIG_CHR_DEV_ST_MODULE 1
#define CONFIG_BLK_DEV_SR 1
#undef  CONFIG_CHR_DEV_SG
#define CONFIG_CHR_DEV_SG_MODULE 1

/*
 * Some SCSI devices (e.g. CD jukebox) support multiple LUNs
 */
#undef  CONFIG_SCSI_MULTI_LUN
#define CONFIG_SCSI_CONSTANTS 1

/*
 * SCSI low-level drivers
 */
#undef  CONFIG_SCSI_7000FASST
#define CONFIG_SCSI_7000FASST_MODULE 1
#undef  CONFIG_SCSI_AHA152X
#define CONFIG_SCSI_AHA152X_MODULE 1
#undef  CONFIG_SCSI_AHA1542
#define CONFIG_SCSI_AHA1542_MODULE 1
#undef  CONFIG_SCSI_AHA1740
#define CONFIG_SCSI_AHA1740_MODULE 1
#undef  CONFIG_SCSI_AIC7XXX
#define CONFIG_SCSI_AIC7XXX_MODULE 1
#undef  CONFIG_OVERRIDE_CMDS
#undef  CONFIG_AIC7XXX_PROC_STATS
#define CONFIG_AIC7XXX_RESET_DELAY (15)
#undef  CONFIG_SCSI_ADVANSYS
#define CONFIG_SCSI_ADVANSYS_MODULE 1
#undef  CONFIG_SCSI_IN2000
#define CONFIG_SCSI_IN2000_MODULE 1
#define CONFIG_SCSI_AM53C974 1
#undef  CONFIG_SCSI_BUSLOGIC
#define CONFIG_SCSI_BUSLOGIC_MODULE 1
#undef  CONFIG_SCSI_OMIT_FLASHPOINT
#undef  CONFIG_SCSI_DTC3280
#define CONFIG_SCSI_DTC3280_MODULE 1
#undef  CONFIG_SCSI_EATA_DMA
#define CONFIG_SCSI_EATA_DMA_MODULE 1
#undef  CONFIG_SCSI_EATA_PIO
#define CONFIG_SCSI_EATA_PIO_MODULE 1
#undef  CONFIG_SCSI_EATA
#define CONFIG_SCSI_EATA_MODULE 1
#undef  CONFIG_SCSI_EATA_TAGGED_QUEUE
#undef  CONFIG_SCSI_EATA_LINKED_COMMANDS
#define CONFIG_SCSI_EATA_MAX_TAGS (16)
#undef  CONFIG_SCSI_FUTURE_DOMAIN
#define CONFIG_SCSI_FUTURE_DOMAIN_MODULE 1
#undef  CONFIG_SCSI_GENERIC_NCR5380
#define CONFIG_SCSI_GENERIC_NCR5380_MODULE 1
#undef  CONFIG_SCSI_GENERIC_NCR53C400
#define CONFIG_SCSI_G_NCR5380_PORT 1
#undef  CONFIG_SCSI_G_NCR5380_MEM
#undef  CONFIG_SCSI_NCR53C406A
#define CONFIG_SCSI_NCR53C406A_MODULE 1
#undef  CONFIG_SCSI_NCR53C7xx
#define CONFIG_SCSI_NCR53C7xx_MODULE 1
#undef  CONFIG_SCSI_NCR53C7xx_sync
#undef  CONFIG_SCSI_NCR53C7xx_FAST
#undef  CONFIG_SCSI_NCR53C7xx_DISCONNECT
#undef  CONFIG_SCSI_NCR53C8XX
#define CONFIG_SCSI_NCR53C8XX_MODULE 1
#define CONFIG_SCSI_NCR53C8XX_NVRAM_DETECT 1
#undef  CONFIG_SCSI_NCR53C8XX_TAGGED_QUEUE
#undef  CONFIG_SCSI_NCR53C8XX_IOMAPPED
#define CONFIG_SCSI_NCR53C8XX_MAX_TAGS (4)
#define CONFIG_SCSI_NCR53C8XX_SYNC (5)
#undef  CONFIG_SCSI_NCR53C8XX_NO_DISCONNECT
#undef  CONFIG_SCSI_NCR53C8XX_SYMBIOS_COMPAT
#undef  CONFIG_SCSI_PPA
#define CONFIG_SCSI_PPA_MODULE 1
#undef  CONFIG_SCSI_PAS16
#define CONFIG_SCSI_PAS16_MODULE 1
#undef  CONFIG_SCSI_QLOGIC_FAS
#define CONFIG_SCSI_QLOGIC_FAS_MODULE 1
#undef  CONFIG_SCSI_QLOGIC_ISP
#define CONFIG_SCSI_QLOGIC_ISP_MODULE 1
#undef  CONFIG_SCSI_SEAGATE
#define CONFIG_SCSI_SEAGATE_MODULE 1
#undef  CONFIG_SCSI_T128
#define CONFIG_SCSI_T128_MODULE 1
#undef  CONFIG_SCSI_U14_34F
#define CONFIG_SCSI_U14_34F_MODULE 1
#undef  CONFIG_SCSI_U14_34F_LINKED_COMMANDS
#define CONFIG_SCSI_U14_34F_MAX_TAGS (8)
#undef  CONFIG_SCSI_ULTRASTOR
#define CONFIG_SCSI_ULTRASTOR_MODULE 1
#undef  CONFIG_SCSI_GDTH
#define CONFIG_SCSI_GDTH_MODULE 1

/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1
#undef  CONFIG_DUMMY
#define CONFIG_DUMMY_MODULE 1
#undef  CONFIG_EQUALIZER
#define CONFIG_EQUALIZER_MODULE 1
#undef  CONFIG_DLCI
#undef  CONFIG_PLIP
#define CONFIG_PLIP_MODULE 1
#undef  CONFIG_PPP
#define CONFIG_PPP_MODULE 1

/*
 * CCP compressors for PPP are only built as modules.
 */
#undef  CONFIG_SLIP
#define CONFIG_SLIP_MODULE 1
#define CONFIG_SLIP_COMPRESSED 1
#define CONFIG_SLIP_SMART 1
#undef  CONFIG_SLIP_MODE_SLIP6
#undef  CONFIG_NET_RADIO
#define CONFIG_NET_ETHERNET 1
#define CONFIG_NET_VENDOR_3COM 1
#undef  CONFIG_EL1
#define CONFIG_EL1_MODULE 1
#undef  CONFIG_EL2
#define CONFIG_EL2_MODULE 1
#undef  CONFIG_ELPLUS
#undef  CONFIG_EL16
#undef  CONFIG_EL3
#define CONFIG_EL3_MODULE 1
#undef  CONFIG_3C515
#define CONFIG_3C515_MODULE 1
#undef  CONFIG_VORTEX
#define CONFIG_VORTEX_MODULE 1
#define CONFIG_NET_VENDOR_SMC 1
#undef  CONFIG_WD80x3
#define CONFIG_WD80x3_MODULE 1
#undef  CONFIG_ULTRA
#define CONFIG_ULTRA_MODULE 1
#undef  CONFIG_ULTRA32
#define CONFIG_ULTRA32_MODULE 1
#undef  CONFIG_SMC9194
#define CONFIG_SMC9194_MODULE 1
#define CONFIG_NET_PCI 1
#undef  CONFIG_PCNET32
#define CONFIG_PCNET32_MODULE 1
#undef  CONFIG_EEXPRESS_PRO100B
#define CONFIG_EEXPRESS_PRO100B_MODULE 1
#undef  CONFIG_DE4X5
#define CONFIG_DE4X5_MODULE 1
#undef  CONFIG_DEC_ELCP
#define CONFIG_DEC_ELCP_MODULE 1
#undef  CONFIG_DGRS
#define CONFIG_DGRS_MODULE 1
#undef  CONFIG_NE2K_PCI
#define CONFIG_NE2K_PCI_MODULE 1
#undef  CONFIG_YELLOWFIN
#define CONFIG_YELLOWFIN_MODULE 1
#undef  CONFIG_RTL8139
#define CONFIG_RTL8139_MODULE 1
#undef  CONFIG_EPIC
#define CONFIG_EPIC_MODULE 1
#undef  CONFIG_TLAN
#define CONFIG_TLAN_MODULE 1
#define CONFIG_NET_ISA 1
#undef  CONFIG_LANCE
#define CONFIG_LANCE_MODULE 1
#undef  CONFIG_AT1700
#define CONFIG_AT1700_MODULE 1
#undef  CONFIG_E2100
#define CONFIG_E2100_MODULE 1
#undef  CONFIG_DEPCA
#define CONFIG_DEPCA_MODULE 1
#undef  CONFIG_EWRK3
#define CONFIG_EWRK3_MODULE 1
#undef  CONFIG_EEXPRESS
#define CONFIG_EEXPRESS_MODULE 1
#undef  CONFIG_EEXPRESS_PRO
#define CONFIG_EEXPRESS_PRO_MODULE 1
#undef  CONFIG_FMV18X
#undef  CONFIG_HPLAN_PLUS
#define CONFIG_HPLAN_PLUS_MODULE 1
#undef  CONFIG_HPLAN
#define CONFIG_HPLAN_MODULE 1
#undef  CONFIG_HP100
#define CONFIG_HP100_MODULE 1
#undef  CONFIG_ETH16I
#define CONFIG_ETH16I_MODULE 1
#undef  CONFIG_NE2000
#define CONFIG_NE2000_MODULE 1
#undef  CONFIG_NI52
#define CONFIG_NI52_MODULE 1
#undef  CONFIG_NI65
#define CONFIG_NI65_MODULE 1
#undef  CONFIG_SEEQ8005
#undef  CONFIG_SK_G16
#define CONFIG_NET_EISA 1
#undef  CONFIG_AC3200
#define CONFIG_AC3200_MODULE 1
#undef  CONFIG_APRICOT
#define CONFIG_APRICOT_MODULE 1
#undef  CONFIG_ZNET
#define CONFIG_NET_POCKET 1
#undef  CONFIG_ATP
#define CONFIG_ATP_MODULE 1
#undef  CONFIG_DE600
#define CONFIG_DE600_MODULE 1
#undef  CONFIG_DE620
#define CONFIG_DE620_MODULE 1
#define CONFIG_TR 1
#undef  CONFIG_IBMTR
#define CONFIG_IBMTR_MODULE 1
#undef  CONFIG_FDDI
#undef  CONFIG_ARCNET
#define CONFIG_ARCNET_MODULE 1
#define CONFIG_ARCNET_ETH 1
#define CONFIG_ARCNET_1051 1

/*
 * ISDN subsystem
 */
#undef  CONFIG_ISDN
#define CONFIG_ISDN_MODULE 1
#define CONFIG_ISDN_PPP 1
#define CONFIG_ISDN_PPP_VJ 1
#define CONFIG_ISDN_MPP 1
#define CONFIG_ISDN_AUDIO 1
#undef  CONFIG_ISDN_DRV_ICN
#define CONFIG_ISDN_DRV_ICN_MODULE 1
#undef  CONFIG_ISDN_DRV_PCBIT
#define CONFIG_ISDN_DRV_PCBIT_MODULE 1
#undef  CONFIG_ISDN_DRV_HISAX
#define CONFIG_ISDN_DRV_HISAX_MODULE 1
#define CONFIG_HISAX_EURO 1
#define CONFIG_HISAX_1TR6 1
#define CONFIG_HISAX_16_0 1
#define CONFIG_HISAX_16_3 1
#define CONFIG_HISAX_AVM_A1 1
#define CONFIG_HISAX_ELSA_PCC 1
#undef  CONFIG_HISAX_ELSA_PCMCIA
#define CONFIG_HISAX_IX1MICROR2 1
#undef  CONFIG_ISDN_DRV_SC
#define CONFIG_ISDN_DRV_SC_MODULE 1
#undef  CONFIG_ISDN_DRV_AVMB1
#define CONFIG_ISDN_DRV_AVMB1_MODULE 1
#define CONFIG_ISDN_DRV_AVMB1_VERBOSE_REASON 1

/*
 * CD-ROM drivers (not for SCSI or IDE/ATAPI drives)
 */
#define CONFIG_CD_NO_IDESCSI 1
#undef  CONFIG_AZTCD
#define CONFIG_AZTCD_MODULE 1
#undef  CONFIG_GSCD
#define CONFIG_GSCD_MODULE 1
#undef  CONFIG_SBPCD
#define CONFIG_SBPCD_MODULE 1
#undef  CONFIG_MCD
#define CONFIG_MCD_MODULE 1
#undef  CONFIG_MCDX
#define CONFIG_MCDX_MODULE 1
#undef  CONFIG_OPTCD
#define CONFIG_OPTCD_MODULE 1
#undef  CONFIG_CM206
#define CONFIG_CM206_MODULE 1
#undef  CONFIG_SJCD
#define CONFIG_SJCD_MODULE 1
#define CONFIG_CDI_INIT 1
#undef  CONFIG_ISP16_CDI
#define CONFIG_ISP16_CDI_MODULE 1
#undef  CONFIG_CDU31A
#define CONFIG_CDU31A_MODULE 1
#undef  CONFIG_CDU535
#define CONFIG_CDU535_MODULE 1

/*
 * Filesystems
 */
#define CONFIG_QUOTA 1
#undef  CONFIG_MINIX_FS
#define CONFIG_MINIX_FS_MODULE 1
#undef  CONFIG_EXT_FS
#define CONFIG_EXT_FS_MODULE 1
#define CONFIG_EXT2_FS 1
#undef  CONFIG_XIA_FS
#define CONFIG_XIA_FS_MODULE 1
#define CONFIG_NLS 1
#undef  CONFIG_ISO9660_FS
#define CONFIG_ISO9660_FS_MODULE 1
#define CONFIG_FAT_FS 1
#define CONFIG_MSDOS_FS 1
#undef  CONFIG_UMSDOS_FS
#define CONFIG_UMSDOS_FS_MODULE 1
#undef  CONFIG_VFAT_FS
#define CONFIG_VFAT_FS_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_437
#define CONFIG_NLS_CODEPAGE_437_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_737
#define CONFIG_NLS_CODEPAGE_737_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_775
#define CONFIG_NLS_CODEPAGE_775_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_850
#define CONFIG_NLS_CODEPAGE_850_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_852
#define CONFIG_NLS_CODEPAGE_852_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_855
#define CONFIG_NLS_CODEPAGE_855_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_857
#define CONFIG_NLS_CODEPAGE_857_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_860
#define CONFIG_NLS_CODEPAGE_860_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_861
#define CONFIG_NLS_CODEPAGE_861_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_862
#define CONFIG_NLS_CODEPAGE_862_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_863
#define CONFIG_NLS_CODEPAGE_863_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_864
#define CONFIG_NLS_CODEPAGE_864_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_865
#define CONFIG_NLS_CODEPAGE_865_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_866
#define CONFIG_NLS_CODEPAGE_866_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_869
#define CONFIG_NLS_CODEPAGE_869_MODULE 1
#undef  CONFIG_NLS_CODEPAGE_874
#define CONFIG_NLS_CODEPAGE_874_MODULE 1
#undef  CONFIG_NLS_ISO8859_1
#define CONFIG_NLS_ISO8859_1_MODULE 1
#undef  CONFIG_NLS_ISO8859_2
#define CONFIG_NLS_ISO8859_2_MODULE 1
#undef  CONFIG_NLS_ISO8859_3
#define CONFIG_NLS_ISO8859_3_MODULE 1
#undef  CONFIG_NLS_ISO8859_4
#define CONFIG_NLS_ISO8859_4_MODULE 1
#undef  CONFIG_NLS_ISO8859_5
#define CONFIG_NLS_ISO8859_5_MODULE 1
#undef  CONFIG_NLS_ISO8859_6
#define CONFIG_NLS_ISO8859_6_MODULE 1
#undef  CONFIG_NLS_ISO8859_7
#define CONFIG_NLS_ISO8859_7_MODULE 1
#undef  CONFIG_NLS_ISO8859_8
#define CONFIG_NLS_ISO8859_8_MODULE 1
#undef  CONFIG_NLS_ISO8859_9
#define CONFIG_NLS_ISO8859_9_MODULE 1
#undef  CONFIG_NLS_KOI8_R
#define CONFIG_NLS_KOI8_R_MODULE 1
#define CONFIG_PROC_FS 1
#undef  CONFIG_NFS_FS
#define CONFIG_NFS_FS_MODULE 1
#undef  CONFIG_SMB_FS
#define CONFIG_SMB_FS_MODULE 1
#define CONFIG_SMB_WIN95 1
#undef  CONFIG_NCP_FS
#define CONFIG_NCP_FS_MODULE 1
#undef  CONFIG_HPFS_FS
#define CONFIG_HPFS_FS_MODULE 1
#undef  CONFIG_SYSV_FS
#define CONFIG_SYSV_FS_MODULE 1
#undef  CONFIG_AUTOFS_FS
#define CONFIG_AUTOFS_FS_MODULE 1
#undef  CONFIG_AFFS_FS
#undef  CONFIG_UFS_FS
#define CONFIG_UFS_FS_MODULE 1
#undef  CONFIG_BSD_DISKLABEL
#undef  CONFIG_SMD_DISKLABEL

/*
 * Character devices
 */
#define CONFIG_SERIAL 1
#undef  CONFIG_DIGI
#undef  CONFIG_CYCLADES
#define CONFIG_CYCLADES_MODULE 1
#define CONFIG_STALDRV 1
#undef  CONFIG_STALLION
#define CONFIG_STALLION_MODULE 1
#undef  CONFIG_ISTALLION
#define CONFIG_ISTALLION_MODULE 1
#undef  CONFIG_RISCOM8
#define CONFIG_RISCOM8_MODULE 1
#undef  CONFIG_PRINTER
#define CONFIG_PRINTER_MODULE 1
#undef  CONFIG_SPECIALIX
#define CONFIG_SPECIALIX_MODULE 1
#undef  CONFIG_SPECIALIX_RTSCTS
#define CONFIG_MOUSE 1
#define CONFIG_ATIXL_BUSMOUSE 1
#define CONFIG_BUSMOUSE 1
#define CONFIG_MS_BUSMOUSE 1
#define CONFIG_PSMOUSE 1
#define CONFIG_82C710_MOUSE 1
#undef  CONFIG_UMISC
#undef  CONFIG_QIC02_TAPE
#undef  CONFIG_FTAPE
#define CONFIG_FTAPE_MODULE 1

/*
 * Set IObase/IRQ/DMA for ftape in ./drivers/char/ftape/Makefile
 */
#undef  CONFIG_APM
#undef  CONFIG_WATCHDOG
#define CONFIG_RTC 1

/*
 * Sound
 */
#undef  CONFIG_SOUND
#define CONFIG_SOUND_MODULE 1
#define CONFIG_AUDIO 1
#define CONFIG_MIDI 1
#undef  CONFIG_YM3812
#define CONFIG_YM3812_MODULE 1
#undef  CONFIG_PAS
#define CONFIG_PAS_MODULE 1
#undef  CONFIG_SB
#define CONFIG_SB_MODULE 1
#undef  CONFIG_ADLIB
#define CONFIG_ADLIB_MODULE 1
#undef  CONFIG_GUS
#define CONFIG_GUS_MODULE 1
#define CONFIG_GUS16 1
#define CONFIG_GUSMAX 1
#undef  CONFIG_PSS
#define CONFIG_PSS_MODULE 1
#undef  CONFIG_MPU401
#define CONFIG_MPU401_MODULE 1
#undef  CONFIG_UART6850
#define CONFIG_UART6850_MODULE 1
#undef  CONFIG_UART401
#define CONFIG_UART401_MODULE 1
#undef  CONFIG_MSS
#define CONFIG_MSS_MODULE 1
#undef  CONFIG_SSCAPE
#define CONFIG_SSCAPE_MODULE 1
#undef  CONFIG_TRIX
#define CONFIG_TRIX_MODULE 1
#undef  CONFIG_MAD16
#define CONFIG_MAD16_MODULE 1
#undef  CONFIG_CS4232
#define CONFIG_CS4232_MODULE 1
#undef  CONFIG_MAUI
#define CONFIG_MAUI_MODULE 1
#define DSP_BUFFSIZE (65536)
#undef  CONFIG_LOWLEVEL_SOUND

/*
 * Kernel hacking
 */
#undef  CONFIG_PROFILE