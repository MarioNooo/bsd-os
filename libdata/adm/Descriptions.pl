$desc{"ether"} = "Ethernet";
$desc{"token_ring"} = "Token Ring";
$desc{"fddi"} = "FDDI";
$desc{"chdlc"} = "Cisco HDLC";
$desc{"ppp"} = "Point-to-Point Protocol";
$desc{"slip"} = "Serial Line IP";
$desc{"loop"} = "Loopback";
$desc{"frelay"} = "Frame Relay";

$desc{"aui"} = "(10base5) external transceiver at 10Mb (a.ka. thick-net).
             Uses a DB15 connector to attach to an external transceiver.";
$desc{"coax"} = "(10base2, BNC) COAXial cable at 10Mb (a.k.a. thin-net).
             Uses a twist on barrel connector.";
$desc{"bnc"} = $desc{"coax"};
$desc{"10baseT"} = "(TP) twisted pair at 10Mb.
             Uses an RJ45 wide modular phone connector.";
$desc{"100baseTX"} = "Ethernet using two category 5 twisted pairs at 100Mb.";
$desc{"100baseFX"} = "Ethernet using multimode fiber at 100Mb.";
$desc{"100baseT4"} = "Ethernet using four category 3 twisted pairs at 100Mb.";
$desc{"100VgAnyLan"} = "Ethernet using four category 3 twisted pairs at 100Mb.
             An alternative standard to 100baseT4.";
$desc{"100baseT2"} = "Ethernet using two category 5 twisted pairs at 100Mb.
             An alternative standard to 100baseTX.";
$desc{"utp16"} = "Token Ring on unshielded twisted pair at 16Mb.
             Uses an RJ454 connector.";
$desc{"utp4"} = "Token Ring on unshielded twisted pair at 4Mb.
             Uses an RJ454 connector.";
$desc{"stp16"} = "Token Ring on shielded twisted pair at 16Mb.
             Uses a DB9 connector.";
$desc{"stp4"} = "Token Ring on shielded twisted pair at 4Mb.
             Uses a DB9 connector.";
$desc{"multimode"} = "FDDI using a pair of multimode fibers at 100Mb.";
$desc{"singlemode"} = "FDDI using a pair of singlemode fibers at 100Mb.";
$desc{"cddi"} = "CDDI using two category 5 twisted pairs at 100Mb.";

$desc{"etr"} = "Early token release feature on 16Mb Token Ring.
             Increases performance for adapters that support it.";
$desc{"srt"} = "Enable IP source route bridging.
             Causes ARP to send requests across source route bridges.";
$desc{"allbc"} = "Enable All Rings Broadcast when using source route briding.
             Causes broadcasts to be sent ``all-routes'' instead of XXX.";
$desc{"da"} = "Enables the use of the dual attach feature.
             Causes dual counter rotating rings to be used.";
$desc{"fdx"} = "Forces Full Duplex operation.
             Enables simultaneous transmission and reception.";
$desc{"hdx"} = "Forces Half Duplex operation.
             Transmission and reception can not occur at the same time.";
$desc{"inst 0"} = "Selects the first instance of a given media type";
$desc{"inst 1"} = "Selects the first instance of a given media type";
$desc{"inst 2"} = "Selects the first instance of a given media type";

$desc{"auto"} = "Enables media Auto-Select.
             Causes the active or best media to be selected automatically";
$desc{"nomedia"} = "Disconnects the card from it's media completely.";
$desc{"manual"} = "Choses the manually configured media.
             This is usually configured by jumpers or dip switches.";

$cl{"bus"} = "eisa isa pci pcmcia";
$desc{"eisa"} = "Extended ISA bus";
$desc{"isa"} = "ISA bus";
$desc{"pci"} = "PCI bus";
$desc{"pcmcia"} = "PCMCIA bus";

$cl{"cdrom"} = "mcd sr";
$desc{"mcd"} = "Mitsumi CD-ROM";
$desc{"sr"} = "Removable SCSI disk";

$cl{"disk"} = "aacr amir cr sd sr wd";
$desc{"amir"} = "AMI MegaRaid disk";
$desc{"aacr"} = "AACR Raid disk";
$desc{"cr"} = "Compaq RAID disk";
$desc{"sd"} = "Fixed SCSI disk";
$desc{"sr"} = "Removable SCSI disk";
$desc{"wd"} = "IDE/ESDI/RLL/ST-506 disk";

$cl{"scsig"} = "sg";
$desc{"sg"} = "SCSI Generic";

$cl{"network"} = "ar cce cnw de df di eb ef el em eo ep ex exp fea fpa hpp"
	       . "mz ne pcn re red rtl se sym te tl tn tnic tr we wl";
$desc{"ar"} = "Arlan Wireless Communications radio ethernet";
$desc{"an"} = "Aironet 802.11 Wireless Communications radio ethernet";
$desc{"cce"} = "Fujitsu MB86960 based PCMCIA Ethernet";
$desc{"cnw"} = "Netwave AirSurfer Wireless LAN";
$desc{"de"} = "Digital DC21040/21140/21041";
$desc{"df"} = "Digital DC21143";
$desc{"di"} = "Digital EtherWorks III";
$desc{"eb"} = "3Com Fast EtherLink XL (3C900/905)";
$desc{"ef"} = "3Com Etherlink III (3C509/B/579/595/597/592)";
$desc{"eg"} = "3Com Gigabit Ethernet";
$desc{"el"} = "3Com Etherlink 16 (3C507)";
$desc{"eo"} = "3Com Etherlink (3C501)";
$desc{"em"} = "Intel Gigabit Ethernet";
$desc{"ep"} = "3Com Etherlink Plus (3C505)";
$desc{"ex"} = "Intel EtherExpress 16";
$desc{"exp"} = "Intel EtherExpress Pro100B/Pro100+";
$desc{"fea"} = "Digital DEFEA DA/SA/UA FDDI";
$desc{"fpa"} = "Digital DEFPA DA/SA/UA FDDI";
$desc{"hpp"} = "HP EtherTwist";
$desc{"mz"} = "MegaHertz PCMCIA Ethernet XJ10BT/XJ10BC/(CC10BT)";
$desc{"ne"} = "Novell NE-1000/NE-2000";
$desc{"pcn"} = "AMD PCnet-PCI Ethernet";
$desc{"re"} = "Allied Telesis RE2000/AT-1700";
$desc{"red"} = "3Com Red (3C508)";
$desc{"rtl"} = "Realtek 8139 ethernet";
$desc{"se"} = "SMC 9432TX EtherPower II";
$desc{"sym"} = "Symbios Ethernet";
$desc{"te"} = "SMC TokenCard Elite";
$desc{"ti"} = "Alteon Gigabit Ethernet";
$desc{"tl"} = "Compaq NetFlex/netelligent, RACORE M8165/M8148";
$desc{"tn"} = "TNIC-1500";
$desc{"tnic"} = "TNIC 1500 Transition Eng ethernet";
$desc{"tr"} = "IBM TRA 16/4 ISA & 3COM 3C619B/C Token Ring";
$desc{"we"} = "WD, SMC Elite/Ultra/EtherEZ, & 3Com 3C503";
$desc{"wi"} = "Lucent 802.11 WaveLan";
$desc{"wl"} = "NCR WaveLan";
$desc{"wx"} = "Intel Gigabit Ethernet";

$cl{"p2p"} = "ntwo rh rn";
$desc{"lmc"} = "Lan Media WAN serial card";
$desc{"ntwo"} = "SDL Communications RISCom/N2 dual sync serial port driver";
$desc{"rh"} = "SDL Communications RISCom/H2 HDLC serial card";
$desc{"rn"} = "SDL Communications RISCom/N1 HDLC serial card";
$desc{"wf"} = "SDL Communications WANic 500 serial card";

$cl{"fddi"} = "fea fpa";
$desc{"fea"} = "DEC EISA FDDI (DEFEA-DA, DEFEA-SA, DEFEA-UA)";
$desc{"fpa"} = "DEC PCI FDDI (DEFPA-DA, DEFPA-SA, DEFPA-UA)";

$cl{"pc"} = "bms fd lms lp mc npx pcaux pccons saturn vga wdc wdpi";
$desc{"bms"} = "Microsoft and ATI Ultra Bus Mouse driver";
$desc{"fd"} = "Floppy disk controller";
$desc{"lms"} = "Logitec and AIT Ultra Pro Bus Mouse driver";
$desc{"lp"} = "parallel printer port";
$desc{"mc"} = "PC Card Memory Card";
$desc{"npx"} = "Numeric coprocessor";
$desc{"pcaux"} = "PS/2 mouse port";
$desc{"pccons"} = "PC text mode console and keyboard";
$desc{"saturn"} = "Saturn PCI chipset workaround";
$desc{"vga"} = "VGA display adapter";
$desc{"wdc"} = "IDE/ESDI/RLL/ST-506/ATAPI controller";
$desc{"wdpi"} = "ATAPI HBA (IDE-CDROM)";

$cl{"pocket"} = "pe xir";
$desc{"pe"} = "Xircom PE-2";
$desc{"xir"} = "Xircom PE-3";

$cl{"scsi"} = "aha aic bfp bha dpt eaha eha ncr sa";
$desc{"aha"} = "Adaptec 1540/1542 SCSI controllers";
$desc{"aic"} = "Adaptec AIC-7770/7780 SCSI controllers";
$desc{"bfp"} = "Buslogic Flashpoint 930/950 SCSI controllers";
$desc{"bha"} = "All BusLogic SCSI controllers";
$desc{"dpt"} = "Distributed Processing Technology SCSI controllers";
$desc{"eaha"} = "Adaptec 1740/1742 SCSI controllers";
$desc{"eha"} = "???";
$desc{"ncr"} = "NCR 810/815/820/835 SCSI controllers";
$desc{"sa"} = "Adaptec 1520/1522 (AIC-6260) SCSI controllers";

$cl{"ser"} = "aimc bheat com cy digi eqnx ms rc rp si stl stli";
$desc{"aimc"} = "Chase Research IOPRO control driver";
$desc{"bheat"} = "Connect Tech Blue Heat serial";
$desc{"com"} = "Standard PC serial port";
$desc{"cy"} = "Cyclades multiport serial card";
$desc{"digi"} = "DigiBoard terminal multiplexor";
$desc{"eqnx"} = "Equinox SST multiport serial card";
$desc{"ms"} = "Maxpeed intelligent serial controller";
$desc{"rc"} = "SDL RISCom async mux";
$desc{"rp"} = "Comtrol Rocketport serial card";
$desc{"si"} = "Specialix terminal multiplexor";
$desc{"stl"} = "Stallion EasyIO serial";
$desc{"stli"} = "Stallion EasyConnection 8/64 serial";

$cl{"sound"} = "gus gusxvi mpu mss opl pas sb sbmidi sbxvi uart";
$desc{"gus"} = "Gravis UltraSound";
$desc{"gusxvi"} = "Gravis UltraSound 16";
$desc{"mpu"} = "Generic MPU-401 midi interface";
$desc{"mss"} = "MS Sound System";
$desc{"opl"} = "Yamaha OPL-2/OPL-3 FM synth";
$desc{"pas"} = "Pro Audio Spectrum 16";
$desc{"sb"} = "Sound Blaster";
$desc{"sbmidi"} = "Sound Blaster 16 MIDI port";
$desc{"sbxvi"} = "Sound Blaster 16";
$desc{"uart"} = "6850 uart midi interface";

$cl{"tape"} = "st wt";
$desc{"st"} = "SCSI tape";
$desc{"wt"} = "QIC-02 tape";

$cl{"pccardc"} = "pcic tcic";
$desc{"pcic"} = "Intel PC Card Interface Controller";
$desc{"tcic"} = "Databook PC Card Interface Controller";

$cl{"video"} = "fvc qcam";
$desc{"fvc"} = "I/O Magic Focus Video Capture";
$desc{"qcam"} = "Connectix QuickCam (mono)";

1;
