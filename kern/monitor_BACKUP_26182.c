// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
<<<<<<< HEAD
#include <kern/trap.h>

=======
#include <kern/pmap.h>
>>>>>>> lab2
#define CMDBUF_SIZE	80	// enough for one VGA text line

static int
runcmd(char *buf, struct Trapframe *tf);

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display backtrace information(function name line parameter) ", mon_backtrace },
	{ "time", "Report time consumed by pipeline's execution.", mon_time },
	{ "memdump", "Dump the contents of a range of memory", mon_memdump },
  { "showmappings", "Display hte physical page mappings and corresponding permission bits", mon_showmappings },
  { "chmapping", "Change the permission bits of a mapping", mon_chmapping }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr() {
    uint32_t pretaddr;
    __asm __volatile("leal 4(%%ebp), %0" : "=r" (pretaddr)); 
    return pretaddr;
}

void
do_overflow(void)
{
    cprintf("Overflow success\n");
}

void
start_overflow(void)
{
    char * pret_addr = (char *) read_pretaddr();
    uint32_t overflow_addr = (uint32_t) do_overflow;
    for (int i = 0; i < 4; i++)
      cprintf("%*s%n\n", pret_addr[i] & 0xFF, "", pret_addr + 4 + i);
    for (int i = 0; i < 4; i++)
      cprintf("%*s%n\n", (overflow_addr >> (8*i)) & 0xFF, "", pret_addr + i);
}

void
overflow_me(void)
{
        start_overflow();
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    overflow_me();
    uint32_t * ebp = (uint32_t *) read_ebp();
    while (ebp != NULL) {
      uint32_t eip = *(ebp+1);
      struct Eipdebuginfo info;
      cprintf("  eip %08x  ebp %08x  args %08x %08x %08x %08x %08x\n",
          eip, ebp, ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);      
      debuginfo_eip((uintptr_t)eip, &info);
      cprintf("         %s:%u %.*s+%u\n",
          info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - (uint32_t)info.eip_fn_addr);
      ebp = (uint32_t *) (*ebp);
    }
    cprintf("Backtrace success\n");
	return 0;
}

// time implement
int
mon_time(int argc, char **argv, struct Trapframe *tf){
  
  char buf[1024];
  int bufi=0;
  for(int i=1;i<argc;i++){
    char * argi =argv[i];
    int j,ch;
    for(j=0,ch=*argi;ch!='\0';ch=argi[++j]){
      buf[bufi++]=ch;
    }
    if(i == argc-1){
      buf[bufi++]='\n';
      buf[bufi++]='\0';
      break;
    }else{
      buf[bufi++]=' ';
    }
  }
  unsigned long eax, edx;
  __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
  unsigned long long timestart  = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

  runcmd(buf, NULL);

  __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
  unsigned long long timeend  = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

  cprintf("kerninfo cycles: %08d\n",timeend-timestart);
  return 0;
}

int
mon_memdump(int argc, char **argv, struct Trapframe *tf)
{
        int i, start, end, op;
        if (argc != 4) {
                cprintf("Usage: < -v | -p > <begin> <end>\n");
                return 0;
        }
        if (strcmp(argv[1], "-v") == 0) {
                op = 0;
        }
        else if (strcmp(argv[1], "-p") == 0) {
                op = 1;
        }
        else {
                cprintf("Usage: < -v | -p > <begin> <end>\n");
                return 0;
        }
        start = strtol(argv[2],NULL,0);
        end =strtol(argv[3],NULL,0);

        cprintf("0x%x:", start);
        int cnt = 0;
        for (i = start; i <= end; ++i) {
                if (cnt == 8) {
                        cnt = 0;
                        cprintf("\n0x%x:", i);
                }
                if (op) {
                        cprintf(" %02x", ((unsigned int)*(char *)(i + KERNBASE)) & 0xff);
                }
                else {
                        cprintf(" %02x", ((unsigned int)*(char *)i) & 0xff);
                }
                cnt++;
        }
        cprintf("\n");
        return 0;
}

int mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
        int i, start, end, cur;
        if (argc != 3) {
                cprintf("Usage: <begin> <end>\n");
                return 0;
        }
        start = strtol(argv[1],NULL,0);
        end = strtol(argv[2],NULL,0);
        i = start;
        pte_t *pte;
        for(;i<=end;i+=PTSIZE){
                pte = pgdir_walk(kern_pgdir, (void *)i, 0);
                if (pte && (*pte & PTE_P)) {
                        if (*pte & PTE_PS) {
                                cur = PDX(i) << PDXSHIFT;
                        }
                        else {
                                cur = PGNUM(i) << PTXSHIFT;
                        }
                        cprintf("0x%08x => 0x%08x", cur, PGNUM(*pte) << PTXSHIFT);
                        if (*pte & PTE_W) {
                                cprintf(" W");
                        }
                        if (*pte & PTE_U) {
                                cprintf(" U");
                        }
                        if (*pte & PTE_PWT) {
                                cprintf(" PWT");
                        }
                        if (*pte & PTE_PCD) {
                                cprintf(" PCD");
                        }
                        if (*pte & PTE_A) {
                                cprintf(" A");
                        }
                        if (*pte & PTE_D) {
                                cprintf(" D");
                        }
                        if (*pte & PTE_PS) {
                                cprintf(" PS");
                        }
                        if (*pte & PTE_G) {
                                cprintf(" G");
                        }
                        cprintf("\n");
                }
                else {
                        cur = PGNUM(i) << PTXSHIFT;
                        cprintf("0x%08x => NOT EXIST\n", cur);
                }
        }
        return 0;
}

int mon_chmapping(int argc, char **argv, struct Trapframe *tf)
{
        int perm, op, addr;
        if (argc != 3) {
                cprintf("Usage: <option> <addr>\n");
                return 0;
        }
        if (strchr(argv[1], '-')) {
                op = 0;
        }
        else if (strchr(argv[1], '+')) {
                op = 1;
        }
        else {
                cprintf("Usage: <option> <addr>\n");
                return 0;
        }
        if (strchr(argv[2], 'W') == 0) {
                perm = PTE_W;
        } else if (strchr(argv[2], 'U') == 0) {
                perm = PTE_U;
        } else if (strcmp(argv[2], "PWT") == 0) {
                perm = PTE_PWT;
        } else if (strcmp(argv[2], "PCD") == 0) {
                perm = PTE_PCD;
        } else if (strchr(argv[2], 'A') == 0) {
                perm = PTE_A;
        } else if (strchr(argv[2], 'D') == 0) {
                perm = PTE_D;
        } else if (strchr(argv[2], 'G') == 0) {
                perm = PTE_G;
        } else {
                cprintf("Usage: <option> <addr>\n");
                return 0;
        }

        addr = strtol(argv[2],NULL,0);
        pte_t *pte = pgdir_walk(kern_pgdir, (void *)addr, 0);
        if (pte) {
                if (op) {
                        *pte = *pte | perm;
                }
                else {
                        *pte = *pte & ~perm;
                }
        }
        return 0;
}
/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
