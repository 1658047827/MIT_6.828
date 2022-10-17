// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Now in umain\n");
	asm volatile("int $3");
}

