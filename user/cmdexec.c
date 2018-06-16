#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int fd, n, r;
	char buf[512+1];

	binaryname = "cmdexec";

	cprintf("cmdexec startup\n");

	cprintf("cmdexec: open /motd\n");
	if ((fd = open("/motd", O_RDONLY)) < 0)
		panic("cmdexec: open /motd: %e", fd);

	cprintf("cmdexec: read /motd\n");
	while ((n = read(fd, buf, sizeof buf-1)) > 0)
		sys_cputs(buf, n);

	cprintf("cmdexec: close /motd\n");
	close(fd);

	cprintf("cmdexec: exec /init\n");
	if ((r = execl("/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
		panic("cmdexec: exec /init: %e", r);

	cprintf("cmdexec: exiting\n"); // should never see it
}
