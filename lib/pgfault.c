// User-level page fault handler support.
// Rather than register the C page fault handler directly with the
// kernel as the page fault handler, we register the assembly language
// wrapper in pfentry.S, which in turns calls the registered C
// function.

#include <inc/lib.h>


// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

// Pointer to currently installed C-language pgfault handler.
void (*_pgfault_handler)(struct UTrapframe *utf);

//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _pgfault_upcall routine when a page fault occurs.
//
void
set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
{
	int r;

	if (_pgfault_handler == 0) {
		// First time through!
		// LAB 4: Your code here.

		// envid=0表示当前env
		int result = sys_page_alloc(0, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_P | PTE_W);
		if(result < 0) panic("set_pgfault_handler error: %e", result);
		// 设置处理函数为那段汇编代码，在里面会调用实际上的处理程序
		// 同时这样相当于汇编代码作为中转，还能处理嵌套user page fault的情况
		sys_env_set_pgfault_upcall(0, _pgfault_upcall);
		
		// panic("set_pgfault_handler not implemented");
	}

	// Save handler pointer for assembly to call.
	_pgfault_handler = handler;
}
