/* Original version by Petr Novak for SMC7004ABR rev.2
 * Modified to support SMC7004VWBR PN720.x version by BLFC from Openline ISP.
 * Heavily modified to support many different devices by Stefan Weil.
 * $Id: mkfirm.c,v 1.1 2005/09/01 20:49:08 stefan Exp stefan $
 *
 * This software may be distributed under the GNU public license GPL.
 */

#include <stdio.h>
#include <stdlib.h>	// exit
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

typedef unsigned long uint32_t;


/**********************************************************************
 *
 * The following are support routines for inflate.c
 *
 **********************************************************************/

static uint32_t crc_32_tab[256];

/*
 * Code to compute the CRC-32 table. Borrowed from 
 * gzip-1.0.3/makecrc.c.
 */

static void
makecrc(void)
{
/* Not copyrighted 1990 Mark Adler	*/

  unsigned long c;      /* crc shift register */
  unsigned long e;      /* polynomial exclusive-or pattern */
  int i;                /* counter for all possible eight bit values */
  int k;                /* byte being shifted into crc apparatus */

  /* terms of polynomial defining this crc (except x^32): */
  static const int p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  /* Make exclusive-or pattern from polynomial */
  e = 0;
  for (i = 0; i < sizeof(p)/sizeof(int); i++)
    e |= 1L << (31 - p[i]);

  crc_32_tab[0] = 0;

  for (i = 1; i < 256; i++)
  {
    c = 0;
    for (k = i | 256; k != 1; k >>= 1)
    {
      c = c & 1 ? (c >> 1) ^ e : c >> 1;
      if (k & 1)
        c ^= e;
    }
    crc_32_tab[i] = c;
  }
}

unsigned long comp_crc(unsigned char *p, unsigned long len)
{
	unsigned long crc = 0xFFFFFFFFUL;

	while (len--) {
		crc = crc_32_tab[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFFUL;
}

typedef struct {
	const char *magic;
	const char *description;
	size_t pfs_size;
	size_t soho_size;
} device_t;

static const device_t device[] = {
	{"BRNABR", "SMC7004ABR", 0x30000, 0xbb800},
	{"BRNAW", "SMC7004VWBR", 0x30000, 0xbb800},
	{"BRN154BAS", "Sinus 154 DSL Basic SE", 0x30000, 0xbb800},
	{"BRNDTBAS3", "Sinus 154 DSL Basic 3", 0x30000, 0xbb800},
	{"BRN154DSL", "Sinus 154 DSL", 0x30000, 0xbb800},
	{"BRN154KOM", "Sinus 154 DSL Komfort", 0x30000, 0xbb800},
	{0}
};

/* buffer must be large enough to contain pfs + soho + signature */
static unsigned char buffer[0x100000];

static unsigned char signature[10];

static const char *program_name;

static void usage(void)
{
	const device_t *dev;
	fprintf(stderr, "Usage: %s <magic> <pfs> <soho>\n", program_name);
	fprintf(stderr, "<magic>: any of the following\n");
	for (dev = device; ; dev++) {
		const char *magic = dev->magic;
		const char *description = dev->description;
		if (magic == 0) break;
		fprintf(stderr, "\t%-10s%s\n", magic, description);
	}
}

int main(int argc, char *argv[])
{
	unsigned long len;
	unsigned long crc;
	unsigned long *p;
	int fd;

	const char *magic;
	const char *pfs_file;
	const char *soho_file;

	const device_t *dev;
	size_t pfs_size;
	size_t soho_size;
	size_t total_size;

	program_name = *argv++;

	if (argc != 4) {
		usage();
		exit(1);
	}

	magic = *argv++;
	pfs_file = *argv++;
	soho_file = *argv++;

	for (dev = device; ; dev++) {
		if (dev->magic == 0) {
			usage();
			exit(2);
		}
		if (strcmp(magic, dev->magic) == 0) break;
	}

	pfs_size = dev->pfs_size;
	soho_size = dev->soho_size;
	total_size = pfs_size + soho_size + sizeof(signature);
	strncpy(signature, magic, 10);

	if (total_size > sizeof(buffer)) {
		fprintf(stderr, "buffer too small, needs %lu bytes, has %lu bytes\n",
			(unsigned long)total_size, (unsigned long)sizeof(buffer));
		exit(3);
	}

	makecrc();
	memset(buffer, 0xff, total_size);

	fd = open(pfs_file, O_RDONLY);
	if (fd < 0) {
		perror("open PFS (192K) SOHO (576K)");
		exit(1);
	}
	len = read(fd, buffer, pfs_size);
	close(fd);
	fprintf(stderr, "Web (PFS) has %lu (0x%lx) bytes, %lu bytes left\n", len, len, pfs_size - len);
	crc = comp_crc(buffer, len);
	p = (unsigned long *)(buffer + pfs_size - 3 * 4);
	p[0] = len;
	p[1] = 0x12345678;
	p[2] = crc;

	fd = open(soho_file, O_RDONLY);
	if (fd < 0) {
		perror("open SOHO (576K)");
		exit(1);
	}
	len = read(fd, buffer + pfs_size, soho_size);
	close(fd);
	fprintf(stderr, "Firmware (SOHO) has %lu (0x%lx) bytes, %lu bytes left\n", len, len, soho_size - len);
	crc = comp_crc(buffer + pfs_size, len);
	p = (unsigned long *)(buffer + pfs_size + soho_size - 3 * 4);
	p[0] = len;
	p[1] = 0x12345678;
	p[2] = crc;

	memcpy(buffer + pfs_size + soho_size, signature, 10);

	if (!isatty(1)) {
		write(1, buffer, total_size);
	}

	return 0;
}
