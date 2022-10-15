/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
/* For challenge, we modified the macro TRAPHANDLER
 * we use a 'if' to distinguish ec and noec
 */
#define TRAPHANDLER(name, num)						\
	.data;	\
		.long name;	\
	.text;	\
		.globl name;			/* define global symbol for 'name' */	\
		.type name, @function;	/* symbol type is function */		\
		.align 2;				/* align function definition */		\
		name:					/* function starts here */		\
		.if !(num==8 || num==17 || (num>=10 && num<=14));	\
		pushl $0;	\
		.endif;	\
		pushl $(num);							\
		jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
# #define TRAPHANDLER_NOEC(name, num)					\
#	.globl name;							\
#	.type name, @function;						\
#	.align 2;							\
#	name:								\
#	pushl $0;							\
#	pushl $(num);							\
#	jmp _alltraps

/* Use TRAPHANDLER_NULL to replace the undefined trap numbers
 * 仅仅用于占位，让syscall下标匹配 
 */
#define TRAPHANDLER_NULL()	\
	.data;	\
		.long 0


/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
 /* To finish challenge, I use 'vectors' instead.
  */
# TRAPHANDLER_NOEC(t_divide, T_DIVIDE)       
# TRAPHANDLER_NOEC(t_debug, T_DEBUG)		   
# TRAPHANDLER_NOEC(t_nmi, T_NMI)			
# TRAPHANDLER_NOEC(t_brkpt, T_BRKPT)
# TRAPHANDLER_NOEC(t_oflow, T_OFLOW)
# TRAPHANDLER_NOEC(t_bound, T_BOUND)
# TRAPHANDLER_NOEC(t_illop, T_ILLOP)
# TRAPHANDLER_NOEC(t_device, T_DEVICE)
# TRAPHANDLER(t_dblflt, T_DBLFLT)

# TRAPHANDLER(t_tss, T_TSS)
# TRAPHANDLER(t_segnp, T_SEGNP)
# TRAPHANDLER(t_stack, T_STACK)
# TRAPHANDLER(t_gpflt, T_GPFLT)
# TRAPHANDLER(t_pgflt, T_PGFLT)

# TRAPHANDLER_NOEC(t_fperr, T_FPERR)
# TRAPHANDLER(t_align, T_ALIGN)
# TRAPHANDLER_NOEC(t_mchk, T_MCHK)
# TRAPHANDLER_NOEC(t_simderr, T_SIMDERR)

# TRAPHANDLER_NOEC(t_syscall, T_SYSCALL)

.data
	.global vectors
vectors:
	TRAPHANDLER(t_divide, T_DIVIDE)       
	TRAPHANDLER(t_debug, T_DEBUG)		   
	TRAPHANDLER(t_nmi, T_NMI)			
	TRAPHANDLER(t_brkpt, T_BRKPT)
	TRAPHANDLER(t_oflow, T_OFLOW)
	TRAPHANDLER(t_bound, T_BOUND)
	TRAPHANDLER(t_illop, T_ILLOP)
	TRAPHANDLER(t_device, T_DEVICE)
	TRAPHANDLER(t_dblflt, T_DBLFLT)
	TRAPHANDLER_NULL()
	TRAPHANDLER(t_tss, T_TSS)
	TRAPHANDLER(t_segnp, T_SEGNP)
	TRAPHANDLER(t_stack, T_STACK)
	TRAPHANDLER(t_gpflt, T_GPFLT)
	TRAPHANDLER(t_pgflt, T_PGFLT)
	TRAPHANDLER_NULL()
	TRAPHANDLER(t_fperr, T_FPERR)
	TRAPHANDLER(t_align, T_ALIGN)
	TRAPHANDLER(t_mchk, T_MCHK)
	TRAPHANDLER(t_simderr, T_SIMDERR)
	TRAPHANDLER_NULL()  # 注意这里要按顺序补充完，IDT中syscall的条目下标才正确
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER_NULL()
	TRAPHANDLER(t_syscall, T_SYSCALL)

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
  	# push_ values to make the stack look like a struct Trapframe
  	pushl %ds
  	pushl %es
  	pushal  # 压入所有的通用寄存器
	# load GD_KD into %ds and %es 
	movw $GD_KD, %ax
	movw %ax, %ds
	movw %ax, %es
	# pushl %esp to pass a pointer to the Trapframe as an argument to trap()
	pushl %esp
	call trap