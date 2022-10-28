// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	// 这里要注意页表页可能不存在
	if(!(uvpd[PDX(addr)]&PTE_P))
		panic("page table does not exist\n");
	pte_t pte = uvpt[PGNUM(addr)];
	if(!(err&FEC_WR))  // 注意不是err != FEC_WR
		panic("FEC_WR check error!\n");
	if(!(pte&PTE_COW))
		panic("PTE_COW check error!\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);  // 注意先向下取整，因为下面的系统调用都要求地址页对齐
	if((r = sys_page_alloc(0, PFTEMP, PTE_U | PTE_P | PTE_W)) < 0) 
		panic("sys_page_alloc error: %e", r);
	memcpy(PFTEMP, addr, PGSIZE);  // 完成真正的信息拷贝
	if((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_W | PTE_U)) < 0)
		panic("sys_page_map error: %e", r);
	if((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("sys_page_unmap error: %e", r);
	

	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void *addr = (void*)(pn << PTXSHIFT);  // pn*PGSIZE
	// 注意判断页表存在
	if(!(uvpd[PDX(addr)]&PTE_P))
		panic("page table does not exist\n");
	if(uvpt[pn]&PTE_W || uvpt[pn]&PTE_COW){  // for write or cow
		// 先映射子进程的
		if((r = sys_page_map(0, addr, envid, addr, PTE_U|PTE_P|PTE_COW)) < 0)
			panic("sys_page_map error: %e", r);
		// 然后映射自己的
		if((r = sys_page_map(0, addr, 0, addr, PTE_U|PTE_P|PTE_COW)) < 0)
			panic("sys_page_map error: %e", r);
	}else{  // for read
		if((r = sys_page_map(0, addr, envid, addr, PTE_U|PTE_P)) < 0)
			panic("sys_page_map error: %e", r);
	}
	
	// panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	size_t pn = 0;
	int r = 0;

	// 设置（父进程自身的）用户页错误处理程序
	set_pgfault_handler(pgfault);
	// 创建子进程，只有这里能模仿dumbfork
	envid_t child = sys_exofork();
	if(child < 0)
		panic("sys_exofork error: %e", child);
	if(child == 0){
		// 子进程，然后fix thisenv
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// 拷贝空间，不要循环到异常栈那里
	for (pn=PGNUM(UTEXT); pn<PGNUM(USTACKTOP); pn++){ 
		// 需要先判断是否映射物理页，以及页表存不存在
        if ((uvpd[pn >> 10] & PTE_P) && (uvpt[pn] & PTE_P))
            if ((r =  duppage(child, pn)) < 0)
                return r;
	}
	// 我将只读页的处理一并放入duppage

	// 为子进程重新分配异常栈
	if((r = sys_page_alloc(child, (void*)UXSTACKTOP-PGSIZE, PTE_U|PTE_P|PTE_W)) < 0)
		panic("sys_page_alloc error: %e", r);

	extern void _pgfault_upcall(void);
	// 设置子进程的页错误处理程序
	sys_env_set_pgfault_upcall(child, _pgfault_upcall);

	// 设置子进程为可run
	if((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status error: %e", r);

	return child;

	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
