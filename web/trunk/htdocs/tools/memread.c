/* memread - read physical memory in a running Linux system
 *
 * Copyright (C) 2006 Stefan Weil
 *
 * This code is licensed under the terms of the GNU Public License (GPL).
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	// lseek

#if !defined(O_BINARY)
# define O_BINARY 0
#endif

static void memread(unsigned long offset, unsigned long length)
{
	const char *filename = "/dev/mem";
	int f = open(filename, O_RDONLY | O_BINARY);
	if (f >= 0) {
		off_t pos = lseek(f, offset, SEEK_SET);
		if (pos == offset) {
			while (length-- > 0) {
				char c;
				if (read(f, &c, 1) != 1) break;
				write(1, &c, 1);
			}
		}
		close(f);
	}
}

int main(int argc, char *argv[])
{
	if (argc == 3) {
		unsigned long offset = strtoul(argv[1], 0, 0);
		unsigned long length = strtoul(argv[2], 0, 0);
		memread(offset, length);
	}
	return 0;
}

// eof
