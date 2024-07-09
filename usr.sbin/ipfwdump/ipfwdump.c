/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwdump.c,v 1.6 2000/04/18 15:36:08 prb Exp
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/bpf.h>
#define	IPFW
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_circuit.h>
#include <netinet/ipfw_cisco.h>

#include <ipfw.h>
#include <stdio.h>

int pc = 0;

main(int ac, char **av)
{
	char ifname[128];
	struct bpf_insn c;
	ipfw_cf_list_t l;
	ipfw_cf_hdr_t hl;
	cisco_filter_t cf;
	struct ipfw_header h;
	ipfw_filter_t f;
	q128_t q;
	int i, pc;
	unsigned char b[sizeof(int)];
	int fd;

	fd = 0;
	switch (ac) {
	case 0: case 1:
		break;
	case 2:
		if (strcmp(av[1], "-"))
			break;
		if ((fd = open(av[1], 0)) < 0)
			err(1, "%s", av[1]);
		break;
	default:
		fprintf(stderr, "Usage: ipfwdump [file]\n");
		exit (1);
	}

	if (read(fd, &h, sizeof(h)) == sizeof(h)) {
		if (h.magic != IPFW_MAGIC)
			errx(1, "not an IP filter");
	}

	if (h.type != 0) {
		f.type = h.type;
		f.length = 0;
		goto doit;
	}

	while (read(fd, &f, sizeof(f)) == sizeof(f)) {
		printf("Serial #%d\n", f.serial);
		f.length = IPFW_len(&f);
		if (f.length >= sizeof(f))
			f.length -= sizeof(f);
		else
			errx(1, "Invalid filter header");
			
doit:
		switch (f.type) {
		default:
			warnx(1, "unknown type: %d", f.type);
			while (f.length-- > 0)
				read(fd, b, 1);
			break;
		case IPFW_CISCO:
			printf("cisco Filter:\n"); 
			if (f.length == 0 && h.icnt)
				f.length = h.icnt * sizeof(cf);
			while (f.length > 0) {
				f.length -= sizeof(cf);
				if (read(fd, &cf, sizeof(cf)) != sizeof(cf))
					errx(1, "short read");
				if (cf.srcif)
					printf("src-%d ", cf.srcif);
				if (cf.dstif)
					printf("dst-%d ", cf.dstif);
				if (cf.retcode == 0)
					printf("permit");
				else
					printf("deny");
				if (cf.passup)
					printf("[%d]", cf.size);
				switch (cf.proto) {
				case 0:
					break;
				case IPPROTO_TCP:
					printf(" tcp");
					break;
				case IPPROTO_UDP:
					printf(" udp");
					break;
				default:
					printf(" proto%d", cf.proto);
					break;
				}
				if (cf.src_mask.s_addr) {
					printf(" %s", inet_ntoa(cf.src));
					printf("&%s", inet_ntoa(cf.src_mask));
				} else
					printf(" -");
				if (cf.dst_mask.s_addr) {
					printf(" %s", inet_ntoa(cf.dst));
					printf("&%s", inet_ntoa(cf.dst_mask));
				} else
					printf(" -");

				if (cf.oper)
					printf(" %c%d", cf.oper, cf.port);

				if (cf.soper)
					printf(" %c%d", cf.soper, cf.sport);

				if (cf.established)
					printf(" established\n");
				else
					printf("\n");
			}
			break;

		case IPFW_CIRCUIT:
			if (f.length < sizeof(hl))
				errx(1, "short circuit cache filter");
			f.length -= sizeof(hl);
			if (read(fd, &hl, sizeof(hl)) != sizeof(hl))
				errx(1, "short read");
			NTOHL(hl.datamask);

			printf("%sCircuit cache[%d]:",
			    hl.protocol == IPPROTO_TCP ? "TCP " :
			    hl.protocol == IPPROTO_UDP ? "UDP " : "",
			    hl.size); 

			if (hl.protocol != IPPROTO_TCP &&
			    hl.protocol != IPPROTO_UDP)
				printf(" proto %d", hl.protocol);

			if (hl.which == CIRCUIT_SRCONLY)
				printf(" src-only");
			else if (hl.which == CIRCUIT_DSTONLY)
				printf(" dst-only");

			if (hl.datamask == 0)
				printf(" addr-only");
			else if ((hl.datamask & 0xffff0000) == 0)
				printf(" dst-port");
			else if ((hl.datamask & 0x0000ffff) == 0)
				printf(" src-port");
			printf("\n");
			if (hl.index)
				printf("Only filter interface: %d\n", hl.index);

			while (hl.length >  0) {
				hl.length--;
				if (f.length < sizeof(l))
					errx(1, "short filter");
				f.length -= sizeof(l);
				if (read(fd, &l, sizeof(l)) != sizeof(l))
					errx(1, "short read");
				NTOHL(l.cl_ports);
				printf("%15s ", inet_ntoa(l.cl_a1));
				if (hl.which == 0 || hl.which == 2)
					printf("-> %15s ", inet_ntoa(l.cl_a2));
				if (hl.datamask & 0xffff0000)
					printf("%5d", l.cl_ports >> 16);
				if ((hl.datamask & 0xffff0000) &&
				    hl.datamask & 0x0000ffff)
					printf(" -> ");
				if (hl.datamask & 0x0000ffff)
					printf("%5d", l.cl_ports & 0xffff);
				printf(" [%qd]\n", l.cl_when);
			}
			while (f.length-- > 0)
				read(fd, b, 1);
			break;
		case IPFW_BPF:
			printf("BPF Filter:\n"); 
			if (f.length == 0 && h.icnt)
				f.length = h.icnt * sizeof(c);
			while (f.length > 0) {
				f.length -= sizeof(c);
				if (read(fd, &c, sizeof(c)) != sizeof(c))
					errx(1, "short read");
				if (c.code & BPF_TRIPLE) {
					f.length -= sizeof(q);
					if (read(fd, &q, sizeof(q)) != sizeof(q))
						errx(1, "short read");
					dis(&c, &q);
				} else
					dis(&c, NULL);
			}
			while (read(fd, &pc, sizeof(int)) == sizeof(int)) {
			    if (pc >= h.icnt)
				    errx(1, "pc %d out of range(%d)",
					pc, h.icnt);
			    i = 0;
			    do {
				    if (read(fd, ifname + i, sizeof(int)) !=
					sizeof(int))
					    err(1, NULL);
				    i += sizeof(int);
			    } while (i < sizeof(ifname) && ifname[i-1] != '\0');
			    if (i >= sizeof(ifname))
				    errx(1, "interface name too long");
			    printf("L%d: interface(%s)\n", pc, ifname);
			}
			break;
		}
	}
	exit(0);
}

dis(struct bpf_insn *p, q128_t *q)
{
	char N[5*8+1];

	if (q == NULL || inet_ntop(AF_INET6, q, N, sizeof(N)) == NULL)
		strcpy(N, "<INVALID>");

	printf("L%d:\t", pc++);
	if (p->code & BPF_TRIPLE)
		pc += 2;

	switch (p->code) {
	case BPF_RET|BPF_K:
		printf("RET\t#0x%x\n", p->k);
		break;
	case BPF_RET|BPF_A:
		printf("RET\tA\n");
		break;

	case BPF_LD|BPF_128|BPF_ABS:
		printf("LD\t[0x%x : 16]\n", p->k);
		break;
	case BPF_LD|BPF_W|BPF_ABS:
		printf("LD\t[0x%x : 4]\n", p->k);
		break;
	case BPF_LD|BPF_H|BPF_ABS:
		printf("LD\t[0x%x : 2]\n", p->k);
		break;
	case BPF_LD|BPF_B|BPF_ABS:
		printf("LD\t[0x%x : 1]\n", p->k);
		break;
	case BPF_LD|BPF_W|BPF_LEN:
		printf("LD\t#LEN\n");
		break;
	case BPF_LDX|BPF_W|BPF_LEN:
		printf("LDX\t#LEN\n");
		break;

	case BPF_LD|BPF_128|BPF_IND:
		printf("LD\t[X + 0x%x : 16]\n", p->k);
		break;
	case BPF_LD|BPF_W|BPF_IND:
		printf("LD\t[X + 0x%x : 4]\n", p->k);
		break;
	case BPF_LD|BPF_H|BPF_IND:
		printf("LD\t[X + 0x%x : 2]\n", p->k);
		break;
	case BPF_LD|BPF_B|BPF_IND:
		printf("LD\t[X + 0x%x : 1]\n", p->k);
		break;

	case BPF_LDX|BPF_MSH|BPF_B:
		printf("LDX\t4 * ([0x%x] & 0xf)\n", p->k);
		break;
	case BPF_LDX|BPF_MSH|BPF_128:
		printf("LDX\tMSH [0x%x]\n", p->k);
		break;

	case BPF_LD|BPF_IMM|BPF_TRIPLE:
		printf("LD\tQ#%s\n", N);
		break;
	case BPF_LD|BPF_IMM:
		printf("LD\t#0x%x\n", p->k);
		break;
	case BPF_LDX|BPF_IMM:
		printf("LDX\t#0x%x\n", p->k);
		break;

	case BPF_LD|BPF_MEM:
		printf("LD\tM[0x%x]\n", p->k);
		break;
	case BPF_LD|BPF_MEM|BPF_128:
		printf("LD\tM[0x%x : 16]\n", p->k);
		break;
	case BPF_LDX|BPF_MEM:
		printf("LDX\tM[0x%x]\n", p->k);
		break;

	case BPF_LD|BPF_ROM:
		printf("LD\tR[0x%x]\n", p->k);
		break;
	case BPF_LDX|BPF_ROM:
		printf("LDX\tR[0x%x]\n", p->k);
		break;

	case BPF_ST:
	case BPF_ST|BPF_MEM:
		printf("ST\tM[0x%x]\n", p->k);
		break;
	case BPF_ST|BPF_128:
	case BPF_ST|BPF_MEM|BPF_128:
		printf("ST\tM[0x%x : 16]\n", p->k);
		break;
	case BPF_ST|BPF_ROM:
		printf("ST\tR[0x%x]\n", p->k);
		break;
	case BPF_ST|BPF_ABS|BPF_W:
		printf("ST\t[0x%x : 4]\n", p->k);
		break;
	case BPF_ST|BPF_ABS|BPF_H:
		printf("ST\t[0x%x : 2]\n", p->k);
		break;
	case BPF_ST|BPF_ABS|BPF_B:
		printf("ST\t[0x%x : 1]\n", p->k);
		break;
	case BPF_ST|BPF_IND|BPF_W:
		printf("ST\t[X + 0x%x : 4]\n", p->k);
		break;
	case BPF_ST|BPF_IND|BPF_H:
		printf("ST\t[X + 0x%x : 2]\n", p->k);
		break;
	case BPF_ST|BPF_IND|BPF_B:
		printf("ST\t[X + 0x%x : 1]\n", p->k);
		break;

	case BPF_STX:
	case BPF_STX|BPF_MEM:
		printf("STX\tM[0x%x]\n", p->k);
		break;
	case BPF_STX|BPF_ROM:
		printf("STX\tR[0x%x]\n", p->k);
		break;
	case BPF_STX|BPF_ABS|BPF_W:
		printf("STX\t[0x%x : 4]\n", p->k);
		break;
	case BPF_STX|BPF_ABS|BPF_H:
		printf("STX\t[0x%x : 2]\n", p->k);
		break;
	case BPF_STX|BPF_ABS|BPF_B:
		printf("STX\t[0x%x : 1]\n", p->k);
		break;

	case BPF_JMP|BPF_JA:
		printf("JMP\tL%d\n", pc + p->k);
		break;
	case BPF_JMP|BPF_JGT|BPF_K:
		printf("JGT\t#0x%x, L%d, L%d\n", p->k, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JGT|BPF_TRIPLE:
		printf("JGT\tQ#%s, L%d, L%d\n", N, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JGE|BPF_K:
		printf("JGE\t#0x%x, L%d, L%d\n", p->k, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JGE|BPF_TRIPLE:
		printf("JGE\tQ#%s, L%d, L%d\n", N, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JEQ|BPF_K:
		printf("JEQ\t#0x%x, L%d, L%d\n", p->k, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JEQ|BPF_TRIPLE:
		printf("JEQ\tQ#%s, L%d, L%d\n", N, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JSET|BPF_K:
		printf("JSET\t#0x%x, L%d, L%d\n", p->k, pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JGT|BPF_X:
		printf("JGT\tX, L%d, L%d\n", pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JGE|BPF_X:
		printf("JGE\tX, L%d, L%d\n", pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JEQ|BPF_X:
		printf("JEQ\tX, L%d, L%d\n", pc + p->jt, pc + p->jf);
		break;
	case BPF_JMP|BPF_JSET|BPF_X:
		printf("JSET\tX, L%d, L%d\n", pc + p->jt, pc + p->jf);
		break;

	case BPF_JMP|BPF_JNET|BPF_TRIPLE:
		printf("JNET\tQ#%s/%d, L%d, L%d\n", N, p->k, pc + p->jt, pc + p->jf);
		break;

	case BPF_ALU|BPF_ADD|BPF_X:
		printf("ADD\tX\n");
		break;
	case BPF_ALU|BPF_SUB|BPF_X:
		printf("SUB\tX\n");
		break;
	case BPF_ALU|BPF_MUL|BPF_X:
		printf("MUL\tX\n");
		break;
	case BPF_ALU|BPF_DIV|BPF_X:
		printf("DIV\tX\n");
		break;
	case BPF_ALU|BPF_AND|BPF_X:
		printf("AND\tX\n");
		break;
	case BPF_ALU|BPF_OR|BPF_X:
		printf("OR\tX\n");
		break;
	case BPF_ALU|BPF_LSH|BPF_X:
		printf("LSH\tX\n");
		break;
	case BPF_ALU|BPF_RSH|BPF_X:
		printf("RSH\tX\n");
		break;

	case BPF_ALU|BPF_ADD|BPF_K:
		printf("ADD\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_SUB|BPF_K:
		printf("SUB\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_MUL|BPF_K:
		printf("MUL\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_DIV|BPF_K:
		printf("DIV\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_AND|BPF_K:
		printf("AND\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_OR|BPF_K:
		printf("OR\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_LSH|BPF_K:
		printf("LSH\t#0x%x\n", p->k);
		break;
	case BPF_ALU|BPF_RSH|BPF_K:
		printf("RSH\t#0x%x\n", p->k);
		break;

	case BPF_ALU|BPF_NEG:
		printf("NEG\n");
		break;

	case BPF_MISC|BPF_TAX:
		printf("TAX\n");
		break;
	case BPF_MISC|BPF_TXA:
		printf("TXA\n");
		break;
	case BPF_MISC|BPF_CCC:
		printf("CALL\t#0x%x\n", p->k);
		break;
	case BPF_MISC|BPF_ZMEM:
		printf("ZMEM\tM[0x%x]\n", p->k);
		break;

	default:
		if ((p->code & ~(31<<8)) == BPF_JMP|BPF_JNET) {
			printf("JNET\t#0x%x/%d, L%d, L%d\n", p->k, (p->code >> 8) & 31, pc + p->jt, pc + p->jf);
		} else
			printf("04x\t0x%x %d %d\n", p->code, pc + p->jt, pc + p->jf);
		break;
	}
}
