// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display the stack frame and caller info", mon_backtrace},
	{ "showmapping", "Display the physical page mappings that apply to a particular range of virtual/linear addresses", mon_showmapping},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	cprintf("Start backtrace\n");
	struct Eipdebuginfo info;
	uint32_t ebp = read_ebp();
	while(ebp){  // loop until ebp eqs 0
		// print ebp eip and 5 args

		// cast ebp to ptr-type
		uint32_t *base = (uint32_t*)ebp;
		cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", 
				ebp, 
				*(base+1), 
				*(base+2), 
				*(base+3), 
				*(base+4), 
				*(base+5), 
				*(base+6));

		int valid=debuginfo_eip(*(base+1), &info);  // *(base+1) is eip
		if(valid==0){
			cprintf("     %s:%d: %.*s+%d\n", 
					info.eip_file, 
					info.eip_line,
					info.eip_fn_namelen, 
					info.eip_fn_name, 
					*(base+1)-info.eip_fn_addr);
		}
		ebp=*base;
	}
	return 0;
}

int mon_showmapping(int argc, char **argv, struct Trapframe *tf){
	uintptr_t addr_begin=0;
	uintptr_t addr_end=0;

	// 先获取参数，包括判断使用方式是否正确
	if(argc != 3){
		cprintf("Usage: showmapping 0xADDR_BEGIN 0xADDR_END\n");
		return 0;
	}else{
		addr_begin=strtol(argv[1], NULL, 16);
		addr_end=strtol(argv[2], NULL, 16);
		if(addr_begin > addr_end) {
			cprintf("Error: ADDR_BEGIN should be less than ADDR_END\n");
			return 0;
		}
	}

	pte_t *pte = NULL;
	cprintf("-----------------------------------------\n"
			"VirAddress  PhyAddress  PTE_P PTE_W PTE_U\n");
	for(addr_begin=ROUNDDOWN(addr_begin, PGSIZE); addr_begin<=addr_end; addr_begin+=PGSIZE){
		pte=pgdir_walk(kern_pgdir, (void *)addr_begin, 0);  // not create
		if(pte && *pte&PTE_P){
			cprintf("0x%08x  0x%08x    %d     %d     %d\n", 
					addr_begin,
					PTE_ADDR(*pte),
					*pte & PTE_P,
					(*pte & PTE_W)>>1,
					(*pte & PTE_U)>>2);
		}else{
			// 页表页还没分配或者PTE_P=0
			cprintf("0x%08x  ----------    -     -     -\n",
					addr_begin);
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
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
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


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
