/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI teucode.c,v 1.1 1995/09/22 22:21:28 ewv Exp
 */

unsigned short smc_ucode[] = {
0xdead, 0xbeef, 0x0000,
0xe920, 0x708e, 0x102c, 0x172d, 0x162e, 0x152f, 0x00db, 0x69c0, 0x0010, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7958, 0x7458, 0x72aa, 0x7744, 
0x7744, 0x77bc, 0x7958, 0x781f, 0x7958, 0x780b, 0x774d, 0x77a5, 0x7958, 0x7900, 
0x7958, 0x7748, 0x1ff4, 0x6dc0, 0x0025, 0x0fe8, 0x708e, 0x5e3a, 0x060b, 0x5e3d, 
0x0802, 0x5ea2, 0x0001, 0x1f64, 0x1f74, 0x1fa0, 0x1fa1, 0x1ff2, 0x5ebe, 0x0000, 
0x1f1b, 0x1f5a, 0x1f29, 0x1f3b, 0x1f3c, 0x1f4b, 0x1f4c, 0x1f4d, 0x1f4e, 0x1f4f, 
0x1f50, 0x1f51, 0x1f52, 0x1f53, 0x1f54, 0x1f55, 0x1f56, 0x1f57, 0x1f58, 0x1f59, 
0x1f80, 0x1f81, 0x1f82, 0x1f83, 0x1f84, 0x1f85, 0x6abf, 0x004c, 0x1f77, 0x5edc, 
0xffff, 0x0f75, 0x5e11, 0x0000, 0x1f74, 0x1f3e, 0x1f3f, 0x1f40, 0x1f5d, 0x1f5e, 
0x1f5f, 0x5e9a, 0xffff, 0x5e65, 0xffff, 0x5ec7, 0xfff5, 0x541b, 0x080f, 0x101b, 
0x5429, 0xfff7, 0x1029, 0x1fd5, 0x1f1c, 0x1f87, 0x6ae5, 0x0076, 0x00db, 0x54d1, 
0x2000, 0xc46a, 0x5ede, 0x00ff, 0x61e5, 0x00ca, 0x00ca, 0x706a, 0x5e9b, 0xffff, 
0x67c0, 0x1fe8, 0x1ff2, 0x5ebe, 0x0002, 0x6abf, 0x007d, 0x1f77, 0x0f75, 0x5774, 
0x001d, 0x1074, 0x5474, 0x001f, 0x1074, 0x5ea3, 0x0fa0, 0x5e61, 0x0100, 0x6aa3, 
0x008b, 0x704f, 0x0fe8, 0x6bca, 0x02aa, 0x6be6, 0x0458, 0x1fe8, 0x541b, 0x8000, 
0x6de2, 0x0079, 0x544d, 0x0008, 0x6be2, 0x0be4, 0x0fe8, 0x1fe8, 0x00d5, 0xc4c0, 
0x69c0, 0x00b0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x726a, 0x7291, 0x70c0, 0x7228, 
0x7209, 0x719d, 0x7184, 0x7169, 0x7246, 0x70c0, 0x70c0, 0x70c0, 0x70c0, 0x714b, 
0x7123, 0x7105, 0x0fe8, 0x008b, 0x978e, 0x541b, 0x0800, 0xc58e, 0x545a, 0x0040, 
0xc5dd, 0x5311, 0x0006, 0xc5e7, 0x1fe8, 0x00b0, 0x4d3e, 0xc5e7, 0x00b1, 0x4d3f, 
0xc5e7, 0x00b2, 0x4d40, 0xc5e7, 0x575a, 0x0040, 0x105a, 0x5729, 0x0002, 0x1029, 
0x70e7, 0x5311, 0x0004, 0xc5e7, 0x1fe8, 0x545a, 0xffbf, 0x105a, 0x5729, 0x0002, 
0x1029, 0x0fe8, 0x541b, 0x2000, 0xc4fb, 0x544d, 0x0004, 0xc4fb, 0x1fe8, 0x544d, 
0xfffb, 0x104d, 0x575a, 0x8000, 0x105a, 0x5729, 0x0002, 0x1029, 0x0fe8, 0x1fe8, 
0x70fd, 0x0029, 0xc48e, 0x0fe8, 0x5e8b, 0x8000, 0x1fe8, 0x571b, 0x0800, 0x101b, 
0x708e, 0x0fe8, 0x5e30, 0x0020, 0x5e31, 0x1004, 0x5e32, 0xc000, 0x5e33, 0xffff, 
0x5e34, 0xffff, 0x5e38, 0x0012, 0x5e39, 0x0004, 0x5e1c, 0x8000, 0x5ee6, 0x8000, 
0x6bc4, 0x0118, 0x5e87, 0x8000, 0x5e69, 0x000c, 0x5ec4, 0x3022, 0x5ee5, 0x0008, 
0x708e, 0x575a, 0x5000, 0x105a, 0x5729, 0x0002, 0x1029, 0x0fe8, 0x5e30, 0x0024, 
0x5e31, 0x1002, 0x5e32, 0xc000, 0x5e33, 0xffff, 0x5e34, 0xffff, 0x5e38, 0x0016, 
0x5e39, 0x0002, 0x5e41, 0x0401, 0x005b, 0x1042, 0x6bc4, 0x013c, 0x5e87, 0x8000, 
0x5e69, 0x000c, 0x5ec4, 0x3026, 0x5e1c, 0x4000, 0x5ee6, 0x4000, 0x5ee5, 0x0008, 
0x708e, 0x0fe8, 0x5e30, 0x0020, 0x5e31, 0x1003, 0x5e32, 0xc000, 0x5e33, 0xffff, 
0x5e34, 0xffff, 0x5e38, 0x0012, 0x5e39, 0x0003, 0x6bc4, 0x015a, 0x5e87, 0x8000, 
0x5e1c, 0x2000, 0x5ee6, 0x2000, 0x5ec4, 0x3022, 0x5e69, 0x000c, 0x5ee5, 0x0008, 
0x708e, 0x5587, 0x8000, 0x6bea, 0x00c0, 0x6bc4, 0x0182, 0x5e1c, 0x0080, 0x5e87, 
0x8000, 0x5e30, 0x0020, 0x5e31, 0xf005, 0x5e32, 0xc000, 0x5e33, 0xffff, 0x5e34, 
0xffff, 0x5e38, 0x0012, 0x5e39, 0x0005, 0x72a7, 0x0ff2, 0x70c0, 0x5587, 0x0800, 
0x6bea, 0x00c0, 0x6bc4, 0x0182, 0x5e1c, 0x0040, 0x5e87, 0x0800, 0x5e30, 0x0020, 
0x5e31, 0x7006, 0x5e32, 0xc000, 0x5e33, 0xffff, 0x5e34, 0xffff, 0x5e38, 0x0012, 
0x5e39, 0x0006, 0x72a7, 0x541b, 0x2000, 0xc5aa, 0x0fe8, 0x5ee6, 0x0020, 0x007a, 
0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 0x70c0, 0x5587, 0x0800, 0x6bea, 0x00c0, 
0x6bc4, 0x0182, 0x575a, 0x2000, 0x105a, 0x5729, 0x0003, 0x1029, 0x0fe8, 0x5e2a, 
0x0054, 0x5e2b, 0x0080, 0x6dc0, 0x01e5, 0x6dc0, 0x01e5, 0x6dc0, 0x01e5, 0x6dc0, 
0x01e5, 0x6dc0, 0x01e5, 0x6dc0, 0x01e5, 0x5e1c, 0x0020, 0x5e87, 0x0800, 0x5e30, 
0x0030, 0x5e31, 0x7000, 0x5e32, 0xc000, 0x5e33, 0x0000, 0x5e34, 0x0008, 0x5e38, 
0x0022, 0x5e39, 0x6029, 0x5e41, 0x082d, 0x5ed3, 0x427a, 0x5b03, 0x5ed1, 0x082e, 
0x6300, 0x5ede, 0x007d, 0x5b03, 0x72a7, 0x5c2a, 0x05d1, 0x5c2b, 0x44ff, 0x4acf, 
0x10cf, 0x54d1, 0xff00, 0x6ae2, 0x01f6, 0x5ecf, 0x00ff, 0x1fe8, 0x575a, 0x0080, 
0x105a, 0x0fe8, 0x5c2b, 0x162b, 0x54d1, 0xff00, 0x4ace, 0x10ce, 0x6aea, 0x0205, 
0x5ece, 0xff00, 0x1fe8, 0x575a, 0x0080, 0x105a, 0x0fe8, 0x00d2, 0x5d2a, 0x162a, 
0x67c0, 0x5587, 0x0800, 0x6bea, 0x00c0, 0x6bc4, 0x0182, 0x5e1c, 0x0010, 0x5e87, 
0x0800, 0x5e30, 0x0024, 0x5e31, 0x7000, 0x5e32, 0xc000, 0x5e33, 0x0000, 0x5e34, 
0x0008, 0x5e38, 0x0016, 0x5e39, 0x6028, 0x5e41, 0x0430, 0x005c, 0x1042, 0x72a7, 
0x0ff2, 0x70c0, 0x5587, 0x0800, 0x6bea, 0x00c0, 0x6bc4, 0x0226, 0x5e1c, 0x0008, 
0x5e87, 0x0800, 0x5e30, 0x0028, 0x5e31, 0x7000, 0x5e32, 0xc000, 0x5e33, 0x0000, 
0x5e34, 0x0008, 0x5e38, 0x001a, 0x5e39, 0x6027, 0x5e41, 0x080a, 0x5ed3, 0x425d, 
0x5b03, 0x72a7, 0x5587, 0x8000, 0x6bea, 0x00c0, 0x6bc4, 0x0226, 0x5e1c, 0x0100, 
0x5e87, 0x8000, 0x5e30, 0x0034, 0x5e31, 0x7000, 0x5e32, 0xc000, 0x5e33, 0x0000, 
0x5e34, 0x0010, 0x5e38, 0x0026, 0x5e39, 0x4025, 0x5e41, 0x1422, 0x1f42, 0x1f43, 
0x1f44, 0x1f45, 0x1f46, 0x1f47, 0x1f48, 0x1f49, 0x1f4a, 0x72a7, 0x5587, 0x0800, 
0x6bea, 0x00c0, 0x6bc4, 0x0226, 0x5e1c, 0x0001, 0x5e87, 0x0800, 0x5e30, 0x0020, 
0x5e31, 0x7000, 0x5e32, 0xc000, 0x5e33, 0x0000, 0x5e34, 0x0010, 0x0fe8, 0x5e38, 
0x0012, 0x5e39, 0x4026, 0x1fe8, 0x541b, 0x2000, 0xc58d, 0x571b, 0x2000, 0x101b, 
0x5729, 0x0008, 0x1029, 0x5729, 0x0004, 0x1029, 0x72a7, 0x551c, 0x0002, 0x6ae2, 
0x00c0, 0x5e1c, 0x0002, 0x5e87, 0x8000, 0x5e30, 0x0012, 0x5e31, 0x7001, 0x0035, 
0x1032, 0x0036, 0x1033, 0x0037, 0x1034, 0x5e38, 0x0004, 0x5e39, 0x0007, 0x5ee9, 
0x0020, 0x708e, 0x0ff0, 0x540c, 0x000f, 0x52d1, 0x02b0, 0x6fc0, 0x72c3, 0x7463, 
0x72cb, 0x7357, 0x7369, 0x740b, 0x7364, 0x73c6, 0x72c3, 0x72c3, 0x72c3, 0x72c3, 
0x72c3, 0x72c3, 0x72c3, 0x72c9, 0x5e0b, 0x8001, 0x72c5, 0x5e0b, 0x8000, 0x5e8e, 
0x0010, 0x1ff0, 0x708e, 0x0ff5, 0x72ca, 0x0011, 0x6de2, 0x0079, 0x1fe8, 0x1ff2, 
0x5ebe, 0x0001, 0x5774, 0x0001, 0x1074, 0x5e9b, 0xffff, 0x5ea3, 0xea60, 0x5e61, 
0x0100, 0x6abf, 0x02db, 0x6aa3, 0x02dd, 0x5e9b, 0xffff, 0x5ea3, 0x03e8, 0x5e61, 
0x0100, 0x6aa3, 0x02e5, 0x549b, 0x0018, 0x6be2, 0x02c0, 0x5ebe, 0x0002, 0x6abf, 
0x02ed, 0x1f77, 0x5e74, 0x025d, 0x1f75, 0x5e11, 0x0001, 0x5e89, 0x0100, 0x5e61, 
0x0001, 0x5e65, 0xff7f, 0x5e9b, 0xffbf, 0x5e6e, 0x1010, 0x5edc, 0xd1ff, 0x5e0b, 
0x8000, 0x5e8e, 0x0010, 0x1ff0, 0x1f1b, 0x00b0, 0x1035, 0x00b1, 0x1036, 0x00b2, 
0x1037, 0x6abf, 0x030d, 0x1f84, 0x5e9b, 0x0010, 0x1f77, 0x708e, 0x0011, 0x6de2, 
0x0079, 0x1fe8, 0x5774, 0x0001, 0x1074, 0x5ea3, 0x9c40, 0x5e61, 0x0100, 0x60ff, 
0x6aa3, 0x0320, 0x5e9b, 0xffff, 0x5ea3, 0x03e8, 0x5e61, 0x0100, 0x6aa3, 0x0328, 
0x549b, 0x0018, 0x6be2, 0x02c0, 0x5e74, 0x023b, 0x5ea3, 0x03e8, 0x5e61, 0x0100, 
0x6aa3, 0x0334, 0x5774, 0x00c0, 0x1074, 0x5e9b, 0xffff, 0x7343, 0x1fe8, 0x5e74, 
0x02db, 0x7343, 0x1fe8, 0x5e74, 0x02cb, 0x1ff2, 0x5ebe, 0x0001, 0x5e11, 0x0008, 
0x5e89, 0x0700, 0x5ea5, 0x03e8, 0x5e61, 0x0018, 0x5e65, 0xffcf, 0x5ed5, 0x8100, 
0x1f1c, 0x1f87, 0x6abf, 0x0354, 0x72fb, 0x1fe8, 0x0011, 0xc45c, 0x5e89, 0x0000, 
0x5e0b, 0x8000, 0x5e8e, 0x0010, 0x1ff0, 0x6dc0, 0x0079, 0x708e, 0x5e12, 0xa000, 
0x5ed1, 0x002e, 0x7376, 0x000d, 0x1012, 0x1fe8, 0x540d, 0x00ff, 0x00cb, 0x1013, 
0x450c, 0x0fe8, 0x6ae2, 0x0383, 0x6aea, 0x0383, 0x1013, 0x5e0d, 0x150c, 0x1fe8, 
0x541b, 0xff00, 0x4701, 0x101b, 0x0fe8, 0x5e8e, 0x0001, 0x1ff0, 0x708e, 0x5713, 
0x1500, 0x100d, 0x1fe8, 0x541b, 0xff00, 0x4703, 0x101b, 0x0fe8, 0x5e8e, 0x0003, 
0x1ff0, 0x708e, 0x1fe8, 0x5513, 0x000c, 0x0fe8, 0x6ae2, 0x03a8, 0x6aea, 0x03a8, 
0x1013, 0x0612, 0x5ede, 0x0015, 0x5b06, 0x1612, 0x1fe8, 0x541b, 0xff00, 0x4701, 
0x101b, 0x0fe8, 0x5e8e, 0x0001, 0x1ff0, 0x708e, 0x5713, 0x1500, 0x100d, 0x0612, 
0x5ede, 0x0015, 0x5b06, 0x1612, 0x1fe8, 0x541b, 0xff00, 0x4703, 0x101b, 0x0fe8, 
0x5e8e, 0x0003, 0x1ff0, 0x708e, 0x0612, 0x5ede, 0x0015, 0x0013, 0x19cf, 0x5b00, 
0x5e0b, 0x8000, 0x5e8e, 0x0010, 0x1ff0, 0x708e, 0x5ed3, 0x1554, 0x5b06, 0x1fe8, 
0x5429, 0xfffe, 0x1029, 0x5e0d, 0x150c, 0x541b, 0xff00, 0x4704, 0x101b, 0x0fe8, 
0x5e8e, 0x0002, 0x1ff0, 0x1f54, 0x1f55, 0x1f56, 0x1f57, 0x1f58, 0x1f59, 0x708e, 
0x5ed3, 0x155a, 0x5b06, 0x1fe8, 0x5429, 0xfffd, 0x1029, 0x5e0d, 0x150c, 0x545a, 
0x0040, 0x105a, 0x541b, 0xff00, 0x4705, 0x101b, 0x0fe8, 0x5e8e, 0x0002, 0x1ff0, 
0x708e, 0x5ed3, 0x153e, 0x5b03, 0x5e18, 0x0508, 0x0029, 0x1019, 0x1f1a, 0x1fe8, 
0x5429, 0xfff3, 0x1029, 0x5e0d, 0x150c, 0x541b, 0xf7f0, 0x101b, 0x0fe8, 0x5e0b, 
0x8000, 0x5e8e, 0x0012, 0x1ff0, 0x708e, 0x000d, 0x54ea, 0x00ff, 0x1012, 0x540d, 
0x00ff, 0x00cb, 0x1013, 0x450c, 0x6ae2, 0x0442, 0x6aea, 0x0442, 0x1013, 0x5edd, 
0x1500, 0x0012, 0x10de, 0x5b06, 0x1612, 0x5e0d, 0x150c, 0x1fe8, 0x541b, 0xff00, 
0x4702, 0x101b, 0x0fe8, 0x5e8e, 0x0002, 0x1ff0, 0x708e, 0x5513, 0x000c, 0x6ae2, 
0x0442, 0x6aea, 0x0442, 0x1013, 0x5edd, 0x1500, 0x0012, 0x10de, 0x5b06, 0x1612, 
0x1fe8, 0x541b, 0xff00, 0x4702, 0x101b, 0x0fe8, 0x5e8e, 0x0002, 0x1ff0, 0x708e, 
0x5edd, 0x1500, 0x0012, 0x10de, 0x0013, 0x19cf, 0x5b00, 0x5713, 0x1500, 0x100d, 
0x1fe8, 0x541b, 0xff00, 0x4700, 0x101b, 0x0fe8, 0x5e0b, 0x8000, 0x5e8e, 0x0012, 
0x1ff0, 0x708e, 0x001b, 0x440f, 0x52d1, 0x045d, 0x6fc0, 0x709c, 0x7390, 0x742b, 
0x73ba, 0x73de, 0x73f3, 0x0011, 0x6de2, 0x0079, 0x540d, 0x000f, 0x52d1, 0x046b, 
0x6fc0, 0x7472, 0x7340, 0x7314, 0x7661, 0x733c, 0x7690, 0x76e7, 0x1fe8, 0x6dc0, 
0x047c, 0x6dc0, 0x0025, 0x0fe8, 0x5e8e, 0x0010, 0x1ff0, 0x708e, 0x5e0b, 0x8002, 
0x5ed1, 0xffff, 0x4201, 0x67e2, 0x5ed1, 0x8000, 0x52d1, 0xffff, 0x66e2, 0x53d1, 
0x7fff, 0x67e2, 0x5ed1, 0x5aa5, 0x53ea, 0xa55a, 0x67e2, 0x5ed1, 0xa55a, 0x53cd, 
0x52ad, 0x67e2, 0x5ed1, 0x52ad, 0x53cb, 0xa55a, 0x67e2, 0x5ec7, 0x00a5, 0x5e65, 
0x093a, 0x5e88, 0x012e, 0x5edc, 0x33cc, 0x5ed5, 0x6699, 0x5ed4, 0x1234, 0x5ea0, 
0x5a22, 0x5ea1, 0x0039, 0x5eb0, 0x5a5a, 0x5eb1, 0x3399, 0x5eb2, 0x478f, 0x5eb3, 
0x1234, 0x5eb4, 0x9fa3, 0x5eb5, 0x78a3, 0x5eb6, 0x98a4, 0x5eb7, 0x6320, 0x5eb8, 
0x5641, 0x5eb9, 0xabcd, 0x5eba, 0xf7c8, 0x5ebb, 0x13ef, 0x5ebc, 0xdccf, 0x66d4, 
0x66e4, 0x53c7, 0x00a5, 0x67e2, 0x5365, 0x093a, 0x67e2, 0x5388, 0x012e, 0x67e2, 
0x53dc, 0x33cc, 0x67e2, 0x53d5, 0x6699, 0x67e2, 0x53d4, 0x1234, 0x67e2, 0x53a0, 
0x5a22, 0x67e2, 0x53a1, 0x0039, 0x67e2, 0x53b0, 0x5a5a, 0x67e2, 0x53b1, 0x3399, 
0x67e2, 0x53b2, 0x478f, 0x67e2, 0x53b3, 0x1234, 0x67e2, 0x53b4, 0x9fa3, 0x67e2, 
0x53b5, 0x78a3, 0x67e2, 0x53b6, 0x98a4, 0x67e2, 0x53b7, 0x6320, 0x67e2, 0x53b8, 
0x5641, 0x67e2, 0x53b9, 0xabcd, 0x67e2, 0x53ba, 0xf7c8, 0x67e2, 0x53bb, 0x13ef, 
0x67e2, 0x53bc, 0xdccf, 0x67e2, 0x5ec7, 0x005a, 0x5e65, 0x06c5, 0x5e88, 0x00d1, 
0x5edc, 0xcc33, 0x5ed5, 0x9966, 0x5ed4, 0x0000, 0x5ea0, 0xa5bb, 0x5ea1, 0x00c6, 
0x5eb0, 0xa5a5, 0x5eb1, 0xcc66, 0x5eb2, 0xb870, 0x5eb3, 0xedcb, 0x5eb4, 0x605c, 
0x5eb5, 0x875c, 0x5eb6, 0x675b, 0x5eb7, 0x9cdf, 0x5eb8, 0xa9be, 0x5eb9, 0x5b32, 
0x5eba, 0x0837, 0x5ebb, 0xec10, 0x5ebc, 0x2330, 0x67d4, 0x66e4, 0x53c7, 0x005a, 
0x67e2, 0x5365, 0x06c5, 0x67e2, 0x5388, 0x00d1, 0x67e2, 0x53dc, 0xcc33, 0x67e2, 
0x53d5, 0x9966, 0x67e2, 0x00d4, 0x67e2, 0x5ed5, 0x0000, 0x5ed4, 0xedcb, 0x66d4, 
0x67e4, 0x00d5, 0x67e2, 0x53d4, 0xedcb, 0x67e2, 0x53a0, 0xa5bb, 0x67e2, 0x53a1, 
0x00c6, 0x67e2, 0x53b0, 0xa5a5, 0x67e2, 0x53b1, 0xcc66, 0x67e2, 0x53b2, 0xb870, 
0x67e2, 0x53b3, 0xedcb, 0x67e2, 0x53b4, 0x605c, 0x67e2, 0x53b5, 0x875c, 0x67e2, 
0x53b6, 0x675b, 0x67e2, 0x53b7, 0x9cdf, 0x67e2, 0x53b8, 0xa9be, 0x67e2, 0x53b9, 
0x5b32, 0x67e2, 0x53ba, 0x0837, 0x67e2, 0x53bb, 0xec10, 0x67e2, 0x53bc, 0x2330, 
0x67e2, 0x5e31, 0x8000, 0x6dc0, 0x05db, 0x67e2, 0x5e31, 0x4000, 0x6dc0, 0x05db, 
0x67e2, 0x5e31, 0x2000, 0x6dc0, 0x05db, 0x67e2, 0x5e31, 0x1000, 0x6dc0, 0x05db, 
0x67e2, 0x5e31, 0x0800, 0x6dc0, 0x05db, 0x67e2, 0x5e31, 0x0800, 0x6dc0, 0x05db, 
0x67e2, 0x5e31, 0x0400, 0x6dc0, 0x05db, 0x67e2, 0x5e31, 0x0200, 0x6dc0, 0x05db, 
0x67e2, 0x5e31, 0x8100, 0x6dc0, 0x05db, 0x67e2, 0x5e31, 0x0080, 0x6dc0, 0x0615, 
0x67e2, 0x5e31, 0x0040, 0x6dc0, 0x0615, 0x67e2, 0x5e31, 0x0020, 0x6dc0, 0x0615, 
0x67e2, 0x5e31, 0x0010, 0x6dc0, 0x0615, 0x67e2, 0x5e31, 0x0008, 0x6dc0, 0x0615, 
0x67e2, 0x67a4, 0x67a3, 0x5e31, 0x0004, 0x6dc0, 0x0615, 0x67e2, 0x67ab, 0x67a9, 
0x67a5, 0x66a4, 0x66a3, 0x67a7, 0x67a6, 0x5e31, 0x0002, 0x6dc0, 0x0615, 0x67e2, 
0x66ab, 0x66a9, 0x66a5, 0x1f67, 0x66a7, 0x66a6, 0x5e0b, 0x8000, 0x67c0, 0x0031, 
0x54d1, 0xff00, 0x4fea, 0x1033, 0x5233, 0xfeff, 0x1034, 0x4201, 0x1032, 0x5234, 
0x0100, 0x52d1, 0xffff, 0x1036, 0x0034, 0x1080, 0x1081, 0x1082, 0x1083, 0x1084, 
0x1085, 0x0032, 0x10c3, 0x0533, 0x0634, 0x1f67, 0x0034, 0x100b, 0x00d2, 0x4b34, 
0x67e2, 0x00c3, 0x100b, 0x4b36, 0x67e2, 0x00d3, 0x100b, 0x4b33, 0x67e2, 0x0080, 
0x4b33, 0x67e2, 0x0081, 0x4b33, 0x67e2, 0x0082, 0x4b31, 0x67e2, 0x0083, 0x4b33, 
0x67e2, 0x0084, 0x4b33, 0x67e2, 0x0085, 0x4b31, 0x67e2, 0x0031, 0x10a2, 0x10a3, 
0x10a4, 0x10a5, 0x10a6, 0x10a7, 0x10a8, 0x10a9, 0x10aa, 0x10ab, 0x10ac, 0x10ad, 
0x10ae, 0x52d1, 0xffff, 0x1030, 0x54d1, 0xfffe, 0x1035, 0x0031, 0x0030, 0x1076, 
0x5e61, 0xfff5, 0x1f64, 0x1f67, 0x00ae, 0x4b35, 0x67e2, 0x00ad, 0x4b35, 0x67e2, 
0x00ac, 0x4b35, 0x67e2, 0x00a4, 0x4b35, 0x67e2, 0x00a3, 0x4b35, 0x67e2, 0x00ab, 
0x4b30, 0x67e2, 0x00a9, 0x4b30, 0x67e2, 0x00a7, 0x4b30, 0x67e2, 0x00a6, 0x4b30, 
0x67e2, 0x00a5, 0x4b30, 0x67e2, 0x00a2, 0x4b30, 0x67e2, 0x0076, 0x4b31, 0x67e2, 
0x5e61, 0x000a, 0x1f67, 0x00a8, 0x4b30, 0x67e2, 0x00aa, 0x4b30, 0x67e2, 0x67c0, 
0x5e0b, 0x8003, 0x7688, 0x0ff4, 0x1ff5, 0x1fcc, 0x00cc, 0x53e7, 0x0001, 0x6be2, 
0x065e, 0x00cc, 0x53e7, 0x0002, 0x6be2, 0x065e, 0x00cc, 0x53e7, 0x0003, 0x6be2, 
0x065e, 0x1fcc, 0x00cc, 0x5eab, 0x0480, 0x5e61, 0x0040, 0x4bd1, 0x1f95, 0x00cb, 
0x4acc, 0x6aab, 0x067a, 0x53d1, 0x1111, 0x6ae2, 0x0686, 0x5e0b, 0x8001, 0x7688, 
0x5e0b, 0x8000, 0x1fcc, 0x00cc, 0x1ff1, 0x5e8e, 0x0010, 0x1ff0, 0x1ff4, 0x708e, 
0x1fe8, 0x6dc0, 0x004f, 0x5e74, 0x0099, 0x1f75, 0x5e61, 0x0010, 0x000e, 0x1030, 
0x000f, 0x1031, 0x5e0b, 0x8000, 0x5e8e, 0x0010, 0x1ff0, 0x5e9b, 0xffff, 0x1f1b, 
0x00b0, 0x1035, 0x00b1, 0x1036, 0x00b2, 0x1037, 0x5e30, 0x0020, 0x5e31, 0x1004, 
0x1f32, 0x1f33, 0x1f34, 0x6ac4, 0x06b9, 0x6bd8, 0x06b8, 0x6bca, 0x06db, 0x76b1, 
0x0088, 0x6aa5, 0x06b9, 0x5e9b, 0xffff, 0x5e87, 0x8000, 0x5e69, 0x000c, 0x5ec5, 
0x3022, 0x6ac3, 0x06c3, 0x6bca, 0x06d6, 0x6bd8, 0x06d4, 0x5ed9, 0x30fe, 0x6ac3, 
0x06cb, 0x6bca, 0x06d6, 0x6bd8, 0x06d4, 0x5ed9, 0x31fe, 0x76c3, 0x0088, 0x76b9, 
0x5ed8, 0x30fe, 0x6ad8, 0x06d8, 0x0088, 0x530c, 0x0001, 0x6be2, 0x06e3, 0x530d, 
0x0006, 0x6ae2, 0x06f2, 0x6dc0, 0x004f, 0x0fe8, 0x708e, 0x1fe8, 0x6dc0, 0x004f, 
0x5e74, 0x00b9, 0x1f75, 0x5e61, 0x0010, 0x60ff, 0x6aa5, 0x06f0, 0x1fe8, 0x0ff0, 
0x5e0b, 0x8000, 0x5e8e, 0x0010, 0x1ff0, 0x5e9b, 0xffff, 0x6bca, 0x06db, 0x60ff, 
0x60ff, 0x549b, 0x0400, 0xc505, 0x5e74, 0x00b9, 0x76fb, 0x5e74, 0x0099, 0x5e9b, 
0xffff, 0x76fb, 0x60ff, 0x60ff, 0x10d4, 0x60ff, 0x60ff, 0x60ff, 0x60ff, 0x6bd7, 
0x0715, 0x60ff, 0x60ff, 0xc718, 0x60ff, 0x60ff, 0xd11b, 0x60ff, 0x60ff, 0x991e, 
0x60ff, 0x60ff, 0x6b66, 0x0722, 0x60ff, 0x60ff, 0xbd25, 0x60ff, 0x60ff, 0xa728, 
0x60ff, 0x60ff, 0xbb2b, 0x60ff, 0x60ff, 0xa52e, 0x60ff, 0x60ff, 0xad31, 0x60ff, 
0x60ff, 0xa134, 0x60ff, 0x60ff, 0xa337, 0x60ff, 0x60ff, 0xc33a, 0x60ff, 0x60ff, 
0x833d, 0x60ff, 0x60ff, 0x8540, 0x60ff, 0x60ff, 0x9343, 0x60ff, 0x60ff, 0x7743, 
0x1f6c, 0x5edb, 0x0018, 0x7958, 0x5e8c, 0x0700, 0x5edb, 0x8000, 0x7b6b, 0x006e, 
0x52e2, 0x0751, 0x6fc0, 0x7760, 0x7760, 0x7760, 0x7760, 0x7763, 0x778d, 0x7760, 
0x7760, 0x7760, 0x7760, 0x7760, 0x7760, 0x77a0, 0x7760, 0x7760, 0x5e9b, 0xffaf, 
0x7958, 0x5e9b, 0x0010, 0x5484, 0x00ff, 0x55d1, 0x0002, 0xd56b, 0x7958, 0x5474, 
0xfff7, 0x1074, 0x5ea3, 0x07d0, 0x5e61, 0x0100, 0x5e9b, 0x0010, 0x1f84, 0x6aa3, 
0x0775, 0x5484, 0x00ff, 0x55d1, 0x0064, 0xd56b, 0x5774, 0x0008, 0x1074, 0x60ff, 
0x60ff, 0x5e9b, 0x0010, 0x007e, 0x544d, 0x0001, 0x6ae2, 0x0958, 0x5ea3, 0xea60, 
0x5e61, 0x0100, 0x7958, 0x5e9b, 0x0020, 0x544d, 0x0001, 0x6be2, 0x0958, 0x574d, 
0x0001, 0x104d, 0x544f, 0xff80, 0x104f, 0x5ea3, 0xea60, 0x5e61, 0x0100, 0x5e63, 
0x0002, 0x7958, 0x5e9a, 0xffff, 0x5e9b, 0xffff, 0x7b6b, 0x00c8, 0x69c0, 0x07b0, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x77ba, 0x77b8, 
0x7958, 0x77c8, 0x7958, 0x7958, 0x7958, 0x7958, 0x1f77, 0x7958, 0x0ff6, 0x7958, 
0x5edb, 0x0020, 0x0030, 0x4202, 0x57d1, 0x3000, 0x5e69, 0x0004, 0x10c4, 0x5ee5, 
0x0008, 0x7958, 0x0088, 0x001c, 0x6ae2, 0x0958, 0x69c0, 0x07d0, 0x0000, 0x0000, 
0x77e0, 0x77ed, 0x77e0, 0x77e0, 0x77e0, 0x77e0, 0x77e0, 0x77e0, 0x77e0, 0x77e0, 
0x77e0, 0x77e0, 0x77e0, 0x77fb, 0x77fb, 0x77fb, 0x54c8, 0x0002, 0x6ae2, 0x07e8, 
0x1f77, 0x1f1c, 0x1f87, 0x7958, 0x001c, 0x10e6, 0x1f1c, 0x1f87, 0x7958, 0x54c8, 
0x0002, 0xc5e4, 0x544b, 0x0040, 0xc5f5, 0x001c, 0x10e6, 0x544b, 0xffbf, 0x104b, 
0x1f1c, 0x1f87, 0x7958, 0x541c, 0xe000, 0x6be2, 0x0802, 0x1f1c, 0x1f87, 0x7958, 
0x541c, 0x8000, 0x6ae2, 0x0958, 0x5e61, 0x0400, 0x5e63, 0x0800, 0x7958, 0x0062, 
0x69c0, 0x0810, 0x0000, 0x0000, 0x7958, 0x7844, 0x78a3, 0x7885, 0x785b, 0x786a, 
0x7958, 0x7888, 0x7958, 0x781c, 0x7958, 0x78be, 0x60ff, 0x7822, 0x60ff, 0x60ff, 
0x5e9b, 0x0040, 0x1ff2, 0x5ebe, 0x0001, 0x5ec8, 0x0001, 0x5edb, 0x0080, 0x5e86, 
0x0200, 0x5ea5, 0x03e8, 0x5e61, 0x0010, 0x5e65, 0xffef, 0x544d, 0x0001, 0xc436, 
0x5e63, 0x0002, 0x5e11, 0x0008, 0x5e89, 0x0700, 0x5ee6, 0xfeff, 0x5ee4, 0x8000, 
0x1f1c, 0x1f87, 0x6abf, 0x0840, 0x1f77, 0x7958, 0x544d, 0x0001, 0xc458, 0x544d, 
0x0002, 0xc558, 0x544f, 0x007f, 0x53d1, 0x0005, 0xc455, 0x524f, 0x0001, 0x104f, 
0x5e61, 0x0100, 0x7958, 0x574d, 0x000e, 0x104d, 0x5e62, 0x0002, 0x7958, 0x5211, 
0x085e, 0x6fc0, 0x7958, 0x7958, 0x7958, 0x7bb9, 0x7b75, 0x7958, 0x7958, 0x7958, 
0x7aa1, 0x5e8c, 0x0600, 0x7b6b, 0x5311, 0x0005, 0x6ae2, 0x0867, 0x0080, 0x6be2, 
0x0880, 0x0081, 0x6be2, 0x0880, 0x0082, 0x6be2, 0x0880, 0x0083, 0x6be2, 0x0880, 
0x0084, 0x6be2, 0x0880, 0x0085, 0x6ae2, 0x0882, 0x5ee4, 0x0020, 0x5e61, 0x0008, 
0x7958, 0x5e8c, 0x0500, 0x7b6b, 0x5211, 0x088b, 0x6fc0, 0x7958, 0x7b75, 0x7adb, 
0x7958, 0x7b75, 0x7958, 0x7894, 0x7958, 0x7958, 0x5e61, 0x0002, 0x541b, 0x0100, 
0x6ae2, 0x08a0, 0x541b, 0xfeff, 0x101b, 0x5ee4, 0x0080, 0x7958, 0x5ee4, 0x0088, 
0x7958, 0x5311, 0x0004, 0xc5ab, 0x5e62, 0x0004, 0x5ee4, 0x0040, 0x7958, 0x5311, 
0x0003, 0xc5b4, 0x544b, 0x0020, 0xc5b4, 0x524b, 0x0010, 0x104b, 0x5e61, 0x0080, 
0x5e69, 0x000c, 0x0030, 0x4202, 0x57d1, 0x3000, 0x10c4, 0x7958, 0x5e62, 0x0800, 
0x5311, 0x0008, 0xc5cf, 0x541c, 0x8000, 0x6ae2, 0x0958, 0x5e69, 0x000c, 0x0030, 
0x4202, 0x57d1, 0x3000, 0x10c4, 0x7958, 0x5311, 0x0007, 0x6be2, 0x0958, 0x57b8, 
0x0001, 0x10b8, 0x5ebe, 0x0004, 0x5ee4, 0x0080, 0x544d, 0x0010, 0xc5e4, 0x574d, 
0x0010, 0x104d, 0x5e9b, 0x0020, 0x5e99, 0x0020, 0x5e61, 0x0802, 0x5e65, 0xfd5f, 
0x544d, 0x0001, 0xc4ed, 0x5e63, 0x0002, 0x5ee9, 0x0080, 0x5e11, 0x0006, 0x5e89, 
0x0600, 0x5ee5, 0x0001, 0x6abf, 0x08f5, 0x1f87, 0x7958, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0fff, 0x0000, 0x544d, 0xfffc, 0x104d, 0x211d, 0x231e, 0x231e, 
0x9c56, 0x001e, 0x4d3e, 0xc50f, 0x001f, 0x4d3f, 0xc50f, 0x0020, 0x4d40, 0x1fe2, 
0x17d4, 0x21d1, 0x2122, 0x4504, 0xc440, 0xd453, 0x1021, 0x6ae5, 0x0917, 0x8f56, 
0x21d1, 0x05d1, 0x53cf, 0x0001, 0xc432, 0x53cf, 0x0002, 0xc439, 0x00ce, 0x00ea, 
0x1023, 0x0021, 0x4d23, 0xc440, 0xd453, 0x1021, 0x0023, 0x55cd, 0x0001, 0x9753, 
0x21ff, 0x4501, 0xc52e, 0x7917, 0x2127, 0x0021, 0x4504, 0xc440, 0xd453, 0x1021, 
0x7917, 0x2324, 0x0021, 0x4508, 0xc440, 0xd453, 0x1021, 0x7917, 0x5ede, 0x00ff, 
0x61e5, 0x1be3, 0x00ca, 0xaf58, 0x541d, 0x000f, 0x52d1, 0x094b, 0x6fc0, 0x7a9c, 
0x7960, 0x7a62, 0x7ae6, 0x799a, 0x79b7, 0x7a20, 0x7958, 0x5ede, 0x00ff, 0x61e5, 
0x00ca, 0x00ca, 0x00db, 0x69e2, 0x0010, 0x052f, 0x062e, 0x072d, 0x002c, 0x65c0, 
0x5e62, 0x0008, 0x0022, 0x5322, 0x0007, 0xc594, 0x5311, 0x0002, 0xc558, 0xa058, 
0xad58, 0xa558, 0x8579, 0x544b, 0x0001, 0xc585, 0x574b, 0x0001, 0x104b, 0x54d5, 
0x0002, 0xc581, 0x5ee4, 0x0002, 0x7958, 0x544b, 0x0002, 0x6be2, 0x0ae3, 0x574b, 
0x0002, 0x104b, 0x7973, 0x574b, 0x0040, 0x104b, 0x7958, 0x5ea5, 0x0a28, 0x5e61, 
0x0019, 0x5e65, 0xff4f, 0x5ee5, 0x0001, 0x5e11, 0x0004, 0x5e89, 0x0400, 0x5ee4, 
0x0040, 0x7958, 0x0022, 0x5322, 0x040b, 0x6ae2, 0x0b5c, 0x7958, 0x5211, 0x099d, 
0x6fc0, 0x7958, 0x7ba1, 0x7958, 0x7958, 0x7bd0, 0x7958, 0x79b4, 0x7958, 0x6ae1, 
0x0958, 0x5e61, 0x0400, 0x5e65, 0xf7ff, 0x544d, 0x0001, 0xc4b0, 0x5e63, 0x0002, 
0x1f1c, 0x5e11, 0x0007, 0x7958, 0x5e5c, 0x0002, 0x79d1, 0x001e, 0x105d, 0x001f, 
0x105e, 0x0020, 0x105f, 0x5211, 0x09c0, 0x6fc0, 0x7958, 0x7ba1, 0x7958, 0x7958, 
0x79f8, 0x7958, 0x79c9, 0x7958, 0x7958, 0xc2b4, 0x6bc1, 0x0a18, 0x6bc2, 0x0a18, 
0x6be8, 0x0a40, 0x7a38, 0x5ed5, 0x0010, 0x1f87, 0x1f1c, 0x54b8, 0xfffe, 0x10b8, 
0x5ebe, 0x0003, 0x60ff, 0x6abf, 0x09db, 0x1f77, 0x5ee5, 0x0001, 0x5474, 0xfffd, 
0x4704, 0x1074, 0x5edb, 0x0080, 0x5ea5, 0x0a28, 0x5e61, 0x0019, 0x5e65, 0xff4f, 
0x544d, 0x0001, 0xc4f1, 0x5e63, 0x0002, 0x5e11, 0x0004, 0x5e89, 0x0400, 0x7958, 
0x1f87, 0x79d3, 0x5e62, 0x0008, 0x6ac1, 0x0a04, 0x6ac2, 0x0a18, 0x5e61, 0x0001, 
0x541b, 0xfdff, 0x101b, 0x7958, 0x6bc2, 0x0a18, 0x6be8, 0x0a10, 0x5ee4, 0x0001, 
0x001e, 0x103e, 0x001f, 0x103f, 0x0020, 0x1040, 0x5e61, 0x0081, 0x5e63, 0x0084, 
0x571b, 0x0200, 0x101b, 0x7958, 0x00db, 0x69e2, 0x0010, 0x052f, 0x062e, 0x072d, 
0x002c, 0x65c0, 0x001e, 0x105d, 0x001f, 0x105e, 0x0020, 0x105f, 0x5211, 0x0a29, 
0x6fc0, 0x7958, 0x7ba1, 0x7958, 0x7958, 0x7a44, 0x7958, 0x7a32, 0x7958, 0x7958, 
0x6bc1, 0x0a18, 0x6bc2, 0x0a18, 0x6be8, 0x0a40, 0x5ee4, 0x0001, 0x001e, 0x103e, 
0x001f, 0x103f, 0x0020, 0x1040, 0x571b, 0x0100, 0x101b, 0x7958, 0x5e62, 0x0008, 
0x6bc1, 0x0a18, 0x6bc2, 0x0a18, 0x5e61, 0x0080, 0x5e63, 0x0004, 0x541b, 0x0200, 
0xc454, 0x5e86, 0x0008, 0x7a57, 0x571b, 0x0200, 0x101b, 0x6be8, 0x0a18, 0x5ee4, 
0x0001, 0x001e, 0x103e, 0x001f, 0x103f, 0x0020, 0x1040, 0x7958, 0x575a, 0x4000, 
0x105a, 0x5729, 0x0002, 0x1029, 0x5211, 0x0a6b, 0x6fc0, 0x7958, 0x7b9e, 0x7ade, 
0x7a74, 0x7a77, 0x7ac2, 0x79f6, 0x7958, 0x79d1, 0x6ae1, 0x0b0b, 0x7958, 0x5ebe, 
0x0002, 0x5ee6, 0x0040, 0x5ea5, 0x0a28, 0x5e61, 0x0011, 0x00b0, 0x4d24, 0x6be2, 
0x0a9c, 0x00b1, 0x4d25, 0x6be2, 0x0a9c, 0x00b2, 0x4d26, 0x6be2, 0x0a9c, 0x5327, 
0x0001, 0x6ae2, 0x0a9c, 0x6abf, 0x0a8f, 0x5465, 0x0008, 0x6ae2, 0x0a18, 0x5e61, 
0x0020, 0x5e63, 0x0008, 0x6abf, 0x0a99, 0x7958, 0x5e62, 0x0008, 0x6abf, 0x0a9e, 
0x7958, 0x5ea5, 0x03e8, 0x5e61, 0x0090, 0x5e65, 0xffeb, 0x544d, 0x0001, 0xc4ac, 
0x5e63, 0x0002, 0x544b, 0xffc3, 0x104b, 0x5e5c, 0x0001, 0x5ed5, 0x2010, 0x1f87, 
0x1f1c, 0x5e5b, 0x0003, 0x5e11, 0x0003, 0x5e89, 0x0300, 0x54b8, 0xfffe, 0x10b8, 
0x5474, 0xfffd, 0x1074, 0x7958, 0x6ae1, 0x0b0e, 0x544d, 0xfffc, 0x104d, 0x5ea5, 
0x03e8, 0x5e61, 0x0090, 0x5e65, 0xffeb, 0x544b, 0xffc3, 0x104b, 0x5ed5, 0x2000, 
0x1f1c, 0x1f87, 0x5e5b, 0x0003, 0x5e11, 0x0003, 0x5e89, 0x0300, 0x7958, 0x5e8c, 
0x0300, 0x7b6b, 0x5ee6, 0x0040, 0x5e8c, 0x0100, 0x7b6b, 0x5e8c, 0x0200, 0x7b6b, 
0x575a, 0x0020, 0x105a, 0x5729, 0x0002, 0x1029, 0x5211, 0x0aef, 0x6fc0, 0x7958, 
0x7958, 0x7bdd, 0x7afe, 0x7bd0, 0x7958, 0x7afb, 0x7958, 0x7af8, 0x6ae1, 0x09d1, 
0x7958, 0x5e5c, 0x0001, 0x79d1, 0x6be1, 0x0b28, 0x00b0, 0x4d1e, 0x6be2, 0x0b09, 
0x00b1, 0x4d1f, 0xc509, 0x00b2, 0x4d20, 0x6bea, 0x0b25, 0x5774, 0x0004, 0x1074, 
0x544d, 0xfffc, 0x104d, 0x5ebe, 0x0003, 0x5ea5, 0x0a28, 0x5e61, 0x0019, 0x5e65, 
0xff4f, 0x1f1c, 0x1f87, 0x1fd5, 0x5e11, 0x0004, 0x5e89, 0x0400, 0x5ee5, 0x0001, 
0x6abf, 0x0b22, 0x7958, 0x5e5b, 0x0004, 0x7958, 0x003e, 0x4d24, 0x6be2, 0x0b54, 
0x003f, 0x4d25, 0x6be2, 0x0b54, 0x0040, 0x4d26, 0x6be2, 0x0b54, 0x544b, 0x0020, 
0x6ae2, 0x0b50, 0x544b, 0x0008, 0x6ae2, 0x0b50, 0x544d, 0xfffc, 0x104d, 0x5774, 
0x0002, 0x1074, 0x5ea5, 0x03e8, 0x5e61, 0x0018, 0x5e65, 0xffef, 0x1f1c, 0x5ed5, 
0x8100, 0x5e11, 0x0008, 0x5e89, 0x0700, 0x7958, 0x524b, 0x0004, 0x104b, 0x7958, 
0x5e5c, 0x0003, 0x5ee4, 0x0010, 0x1f1c, 0x5e87, 0x0800, 0x7b0b, 0x5e8c, 0x0400, 
0x54a0, 0x8000, 0x6be2, 0x0958, 0x5474, 0xffbf, 0x1074, 0x575a, 0x0100, 0x105a, 
0x5729, 0x0002, 0x1029, 0x1fe8, 0x60ff, 0x5edc, 0xffff, 0x5e89, 0x0000, 0x571b, 
0x8000, 0x101b, 0x7958, 0x6dc0, 0x0b78, 0x7958, 0x5474, 0xfffb, 0x57d1, 0x0080, 
0x1074, 0x1ff2, 0x5ebe, 0x0001, 0x5ec8, 0x0001, 0x5ed5, 0x2000, 0x5e5b, 0x0003, 
0x5e11, 0x0003, 0x5e89, 0x0300, 0x5ea5, 0x03e8, 0x5e61, 0x0090, 0x5e65, 0xffeb, 
0x544d, 0x0001, 0xc495, 0x5e63, 0x0002, 0x544b, 0xffc3, 0x104b, 0x6abf, 0x0b98, 
0x1f1c, 0x1f87, 0x1f77, 0x67c0, 0x5e8c, 0x0000, 0x7b6b, 0x5774, 0x0084, 0x1074, 
0x544b, 0xfffc, 0x104b, 0x574d, 0x0010, 0x104d, 0x5e9b, 0xffff, 0x5e99, 0x0020, 
0x5e61, 0x0001, 0x5e65, 0xff7f, 0x5ee4, 0x0002, 0x5e11, 0x0002, 0x5e89, 0x0200, 
0x7958, 0x5e61, 0x0084, 0x5e65, 0xffdb, 0x544d, 0x0001, 0xc4c2, 0x5e63, 0x0002, 
0x5ed5, 0x4000, 0x1f1c, 0x1f87, 0x544d, 0x0002, 0xc4cb, 0x5e5b, 0x0002, 0x5e11, 
0x0005, 0x5e89, 0x0500, 0x7958, 0x5ebe, 0x0002, 0x5ee6, 0x0040, 0x5e62, 0x0008, 
0x5ea5, 0x0a28, 0x5e61, 0x0010, 0x6abf, 0x0bda, 0x7958, 0x5ebe, 0x0002, 0x5ee6, 
0x0040, 0x6abf, 0x0be1, 0x7958, 0x544d, 0xfff7, 0x104d, 0x5211, 0x0bea, 0x6fc0, 
0x709c, 0x709c, 0x7079, 0x709c, 0x7bf3, 0x7c10, 0x7bf6, 0x709c, 0x7bf6, 0x6dc0, 
0x0b78, 0x709c, 0x5ea5, 0x03e8, 0x5e61, 0x0090, 0x5e65, 0xffeb, 0x544b, 0xffc3, 
0x104b, 0x5ed5, 0x2000, 0x1f87, 0x1f1c, 0x5e5b, 0x0002, 0x5e11, 0x0003, 0x5e89, 
0x0300, 0x54b8, 0xfffe, 0x10b8, 0x5474, 0xfffd, 0x1074, 0x709c, 0x5e5b, 0x0002, 
0x709c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x5800, 0x3b45, 
};