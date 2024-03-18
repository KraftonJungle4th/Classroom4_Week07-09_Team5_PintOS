#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/init.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void check_address(void *file_addr);
int process_add_file(struct file *file);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void halt (void) 
{	
	power_off();
}

//현재 유저 프로그램을 종료하여 커널 상태로 리턴
void exit (int status)
{	
	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

//열린 버퍼 fd로 buffer에 담긴 length만큼 쓰기 작업을 진행한다.
int write (int fd, const void *buffer, unsigned length)
{
	int byte = 0;
	if(fd == 1)
	{
		putbuf(buffer, length);
		byte = length;
	}
	return byte;
}

//initial_size만큼 새로운 file 초기화한다.
bool create (const char *file, unsigned initial_size)
{	
	check_address(file);
	return filesys_create (file, initial_size);
}

//file을 연다.
int open (const char *file)
{
	check_address(file);
	struct file *f = filesys_open(file);	

	if(f != NULL)
	{
		int fd = process_add_file(f);
		return fd;
	}
	else
		return -1;
}

int process_add_file(struct file *file)
{
	struct thread *curr = thread_current();
	struct fd *cur_fd = malloc(sizeof(struct fd));

	cur_fd->file = file;
	cur_fd->fd_num = (curr->last_create_fd)++;	
	//list_push_back(&curr->fd_list, &cur_fd->fd_elem);
	
	return cur_fd->fd_num; 
}

// pid_t fork (const char *thread_name);
// int exec (const char *file);
// int read (int fd, void *buffer, unsigned length)
// {
// 	int byte = 0;
// 	if(fd == 0)
// 	{
// 		byte = input_getc();
// 	}
// 	return byte;
// }

// int wait (pid_t);
// bool remove (const char *file);

// int filesize (int fd);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);

// 유효한 주소값인지 확인
void check_address(void *file_addr)
{
	struct thread *t = thread_current();
	if(pml4_get_page(t->pml4, file_addr) == NULL || !is_user_vaddr(file_addr) || file_addr == NULL || file_addr == "\0")
		exit(-1);
}

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
/* 시스템 콜 번호를 받아오고, 어떤 시스템 콜 인자들을 받아오고,
   그에 알맞은 액션을 취해야 한다.*/
void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int sys_num = f->R.rax;	 //시스템 콜 번호
	switch(sys_num)
	{
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:	
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			break;
		case SYS_EXEC:
			break;
		case SYS_WAIT:
			break;
		case SYS_CREATE:
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			break;
		case SYS_READ:
			// f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;	
		case SYS_SEEK:
			break;	
		case SYS_TELL:
			break;	
		case SYS_CLOSE:
			break;	

		default:
			break;
	}
	//printf ("system call!\n");
	//thread_exit ();
}
