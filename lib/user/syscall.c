#include <syscall.h>
#include <stdint.h>
#include "../syscall-nr.h"

__attribute__((always_inline))
static __inline int64_t syscall (uint64_t num_, uint64_t a1_, uint64_t a2_,
		uint64_t a3_, uint64_t a4_, uint64_t a5_, uint64_t a6_) {
	int64_t ret;
	register uint64_t *num asm ("rax") = (uint64_t *) num_;
	register uint64_t *a1 asm ("rdi") = (uint64_t *) a1_;
	register uint64_t *a2 asm ("rsi") = (uint64_t *) a2_;
	register uint64_t *a3 asm ("rdx") = (uint64_t *) a3_;
	register uint64_t *a4 asm ("r10") = (uint64_t *) a4_;
	register uint64_t *a5 asm ("r8") = (uint64_t *) a5_;
	register uint64_t *a6 asm ("r9") = (uint64_t *) a6_;

	__asm __volatile(
			"mov %1, %%rax\n"
			"mov %2, %%rdi\n"
			"mov %3, %%rsi\n"
			"mov %4, %%rdx\n"
			"mov %5, %%r10\n"
			"mov %6, %%r8\n"
			"mov %7, %%r9\n"
			"syscall\n"
			: "=a" (ret)
			: "g" (num), "g" (a1), "g" (a2), "g" (a3), "g" (a4), "g" (a5), "g" (a6)
			: "cc", "memory");
	return ret;
}

/* Invokes syscall NUMBER, passing no arguments, and returns the
   return value as an `int'. */
#define syscall0(NUMBER) ( \
		syscall(((uint64_t) NUMBER), 0, 0, 0, 0, 0, 0))

/* Invokes syscall NUMBER, passing argument ARG0, and returns the
   return value as an `int'. */
#define syscall1(NUMBER, ARG0) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), 0, 0, 0, 0, 0))
/* Invokes syscall NUMBER, passing arguments ARG0 and ARG1, and
   returns the return value as an `int'. */
#define syscall2(NUMBER, ARG0, ARG1) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			0, 0, 0, 0))

#define syscall3(NUMBER, ARG0, ARG1, ARG2) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			((uint64_t) ARG2), 0, 0, 0))

#define syscall4(NUMBER, ARG0, ARG1, ARG2, ARG3) ( \
		syscall(((uint64_t *) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			((uint64_t) ARG2), \
			((uint64_t) ARG3), 0, 0))

#define syscall5(NUMBER, ARG0, ARG1, ARG2, ARG3, ARG4) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			((uint64_t) ARG2), \
			((uint64_t) ARG3), \
			((uint64_t) ARG4), \
			0))

//power_off 호출해서 Pintos 종료
void
halt (void) {
	syscall0 (SYS_HALT);
	NOT_REACHED ();
}

//현재 동작 중인 유저 프로그램 종료, 커널의 상태 리턴(0 성공)
void
exit (int status) {
	syscall1 (SYS_EXIT, status);
	NOT_REACHED ();
}

/*thread_name이라는 이름을 가진 현재 프로세스 복제
  자식 프로세스의 pid 반환해야 한다. 
  자식 프로세스가 리소스를 복제하지 못하면 부모의 fork() 호출이 TID_ERROR 반환할 것이다.
*/
pid_t
fork (const char *thread_name){
	return (pid_t) syscall1 (SYS_FORK, thread_name);
}

/*현재의 프로세스가 cmd_line에서 이름이 주어지는 실행 가능한 프로세스로 변경된다.
  주어진 인자들을 전달, 성공적으로 진행된다면 어떤 것도 반환 x
  파일 디스크립터는 exec 함수 호출 시에 열린 상태로 있다.
*/
int
exec (const char *file) {
	return (pid_t) syscall1 (SYS_EXEC, file);
}

/* 자식 프로세스(pid)를 기다려서 자식의 종료 상태(exit status)를 가져온다.
   만약 pid가 아직 살아있다면, 종료 될 때까지 기다린다.
   종료가 되면 그 프로세스가 exit 함수로 전달해준 상태(exit status) 반환
   만약 pid가 exit()함수를 호출하지 않고 커널에 의해서 종료된다면(exception에 의해서 죽는 경우),
   wait(pid)는 -1 반환해야 한다.
*/
int
wait (pid_t pid) {
	return syscall1 (SYS_WAIT, pid);
}

/*fild(첫 번째 인자)를 이름으로 하고 크기가 inital_size(두 번째 인자)인 새로운 파일 생성
  성공적으로 파일 생성 true, 실패 false */
bool
create (const char *file, unsigned initial_size) {
	return syscall2 (SYS_CREATE, file, initial_size);
}

/* file이라는 이름을 가진 파일 삭제 성공 true, 실패 false*/
bool
remove (const char *file) {
	return syscall1 (SYS_REMOVE, file);
}

/* file이라는 이름을 가진 파일을 연다. 
   성공적으로 열렸다면 파일 식별자(0 또는 양수) 반환, 실패 -1 반환
   각각의 프로세스는 독립적인 파일 식별자들을 갖는다. 파일 식별자는 자식 프로세스들에게 상속
   하나의 프로세스에 의해서든 다른 여러 개의 프로세스에 의해서든, 하나의 파일이 두 번 이상 열리면
   그때마다 open 시스템 콜은 새로운 식별자를 반환한다.
*/
int
open (const char *file) {
	return syscall1 (SYS_OPEN, file);
}

//fd로서 열려있는 파일의 크기가 몇 바이트인지 반환
int
filesize (int fd) {
	return syscall1 (SYS_FILESIZE, fd);
}

/* buffer 안에 fd로 열려있는 파일로부터 size바이트를 읽는다.
   실제로 읽어낸 바이트의 수 반환
   파일 끝에서 시도하면 0, 파일이 읽어질 수 없다면 -1 반환 */
int
read (int fd, void *buffer, unsigned size) {
	return syscall3 (SYS_READ, fd, buffer, size);
}
/* buffer로부터 open file fd로 size 바이트를 적어준다.
   실제로 적힌 바이트의 수 반환해주고, 일부 바이트가 적히지 못했다면
   size보다 더 작은 바이트 수가 반환될 수 있다.
   더 이상 바이트를 적을 수 없다면 0 반환 */
int
write (int fd, const void *buffer, unsigned size) {
	return syscall3 (SYS_WRITE, fd, buffer, size);
}

/* open file fd에서 읽거나 쓸 다음 바이트를 position으로 변경한다. 
   position은 파일 시작부터 바이트 단위로 표시된다. position 0 은 파일의 시작을 의미. */
void
seek (int fd, unsigned position) {
	syscall2 (SYS_SEEK, fd, position);
}

/* 열려진 파일 fd에서 읽히거나 써질 다음 바이트의 위치 반환
   파일의 시작지점부터 몇 바이트인지로 표현*/
unsigned
tell (int fd) {
	return syscall1 (SYS_TELL, fd);
}

/* 파일 식별자 fd를 닫는다.
   프로세스를 나가거나 종료하는 것은 묵시적으로 그 프로세스의 열려있는 파일 식별자들을 닫는다.*/
void
close (int fd) {
	syscall1 (SYS_CLOSE, fd);
}

int
dup2 (int oldfd, int newfd){
	return syscall2 (SYS_DUP2, oldfd, newfd);
}

void *
mmap (void *addr, size_t length, int writable, int fd, off_t offset) {
	return (void *) syscall5 (SYS_MMAP, addr, length, writable, fd, offset);
}

void
munmap (void *addr) {
	syscall1 (SYS_MUNMAP, addr);
}

bool
chdir (const char *dir) {
	return syscall1 (SYS_CHDIR, dir);
}

bool
mkdir (const char *dir) {
	return syscall1 (SYS_MKDIR, dir);
}

bool
readdir (int fd, char name[READDIR_MAX_LEN + 1]) {
	return syscall2 (SYS_READDIR, fd, name);
}

bool
isdir (int fd) {
	return syscall1 (SYS_ISDIR, fd);
}

int
inumber (int fd) {
	return syscall1 (SYS_INUMBER, fd);
}

int
symlink (const char* target, const char* linkpath) {
	return syscall2 (SYS_SYMLINK, target, linkpath);
}

int
mount (const char *path, int chan_no, int dev_no) {
	return syscall3 (SYS_MOUNT, path, chan_no, dev_no);
}

int
umount (const char *path) {
	return syscall1 (SYS_UMOUNT, path);
}
