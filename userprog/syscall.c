#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "include/threads/init.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "include/lib/stdio.h"
#include "include/lib/string.h"
#include "include/lib/user/syscall.h"
#include "devices/input.h"
#include "threads/palloc.h"

void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t fork (const char *thread_name);
int exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void check_address(void *file_addr);
void check_buffer(void *buffer);
int process_add_file(struct file *file);
struct file_descriptor *find_file_descriptor(int fd);

static struct intr_frame *frame;
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
	frame = f;

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
			f->R.rax = fork(f->R.rdi);
			break;
		case SYS_EXEC:
			f->R.rax = exec(f->R.rdi);
			break;
		case SYS_WAIT:
			f->R.rax = wait(f->R.rdi);
			break;
		case SYS_CREATE:
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;	
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;	
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;	
		case SYS_CLOSE:
			close(f->R.rdi);
			break;	

		default:
			break;
	}
	//printf ("system call!\n");
	//thread_exit ();
}

//Pintos 종료
void halt (void) 
{	
	power_off();
}

//현재 유저 프로그램을 종료하여 커널 상태로 리턴
void exit (int status)
{	
	struct thread *cur = thread_current();
	cur->exit_status = status;
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

//열린 버퍼 fd로 buffer에 담긴 length만큼 쓰기 작업을 진행한다.
int write (int fd, const void *buffer, unsigned length)
{
	check_buffer(buffer);
	int byte = 0;
	if(fd == 1)
	{
		putbuf(buffer, length);
		byte = length;
	}else
	{
		struct file_descriptor *curr_fd = find_file_descriptor(fd);
		if(curr_fd == NULL) return NULL;
		byte = file_write(curr_fd->file, buffer, length);
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
	struct file *f = filesys_open(file);

	int fd = -1;
	if(f != NULL)
		fd = process_add_file(f);
	return fd;
}

void close (int fd)
{
	struct file_descriptor *curr_fd = find_file_descriptor(fd);
	if(curr_fd == NULL) return NULL;

	list_remove(&curr_fd->fd_elem);
	file_close(curr_fd->file);
	free(curr_fd);
}

int read (int fd, void *buffer, unsigned length)
{
	check_buffer(buffer);
	
	int byte = 0;
	char *ptr = (char *)buffer;
	if(fd == 0)
	{
		for(int i = 0; i < length; i++)
		{
			*ptr++ = input_getc();
			byte ++;
		}
	}
	else
	{
		struct file_descriptor *curr_fd = find_file_descriptor(fd);
		if(curr_fd == NULL) return -1;
		byte = file_read(curr_fd->file, buffer, length);
	}
	return byte;
}

int filesize (int fd)
{
	struct file_descriptor *curr_fd = find_file_descriptor(fd);
	if(curr_fd == NULL) return -1;
	return file_length(curr_fd->file);
}

pid_t fork (const char *thread_name)
{	
	return process_fork(thread_name,frame);
}

int wait (pid_t pid)
{
	return process_wait(pid);
}

bool remove (const char *file)
{
	check_address(file);
	return filesys_remove(file);
}

int exec (const char *cmd_line)
{
	check_address(cmd_line);

	char *cmd_line_copy;
	cmd_line_copy = palloc_get_page(0);
	if(cmd_line_copy == NULL)
		exit(-1);
	strlcpy(cmd_line_copy, cmd_line, PGSIZE);

	if(process_exec(cmd_line_copy) == -1)
		exit(-1);
}

void seek (int fd, unsigned position)
{
	struct file_descriptor *curr_fd = find_file_descriptor(fd);
	if(curr_fd == NULL)
		return NULL;
	file_seek(curr_fd->file, position);
}
unsigned tell (int fd)
{
	struct file_descriptor *curr_fd = find_file_descriptor(fd);
	if(curr_fd == NULL)
		return NULL;
	return file_tell(curr_fd->file);
}

// 유효한 주소값인지 확인
void check_address(void *file_addr)
{
	struct thread *t = thread_current();
	if(file_addr == "\0" || file_addr == NULL || !is_user_vaddr(file_addr) || pml4_get_page(t->pml4, file_addr) == NULL)
		exit(-1);
}

// 유효한 버퍼 주소 값인지 확인
void check_buffer(void *buffer)
{
	struct thread *t = thread_current();
	if(!is_user_vaddr(buffer) || pml4_get_page(t->pml4, buffer) == NULL)
		exit(-1);
}

//현재 스레드의 파일 디스크립터에 현재 파일을 추가한다.
int process_add_file(struct file *file)
{
	struct thread *curr = thread_current();
	struct file_descriptor *cur_fd = malloc(sizeof(struct file_descriptor));
	struct list *fd_list = &thread_current()->fd_list;
	
	cur_fd->file = file;
	cur_fd->fd_num = (curr->last_create_fd)++;	
	
	list_push_back(fd_list, &cur_fd->fd_elem);

	return cur_fd->fd_num; 
}

struct file_descriptor *find_file_descriptor(int fd)
{
	struct list *fd_list = &thread_current()->fd_list;
	if(list_empty(fd_list)) return NULL;

	struct file_descriptor *file_descriptor;
	struct list_elem *cur_fd_elem = list_begin(fd_list);

	while(cur_fd_elem != list_end(fd_list))
	{
		file_descriptor = list_entry(cur_fd_elem, struct file_descriptor, fd_elem);

		if(file_descriptor->fd_num == fd)
		{
			return file_descriptor;	
		}			
		cur_fd_elem = list_next(cur_fd_elem);
	}
	return NULL;
}
