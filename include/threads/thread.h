#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
/* 커널 스레드 또는 사용자 프로세스.
 *
 * 각 스레드 구조는 자체 4KB 페이지에 저장됩니다.  스레드 구조 자체는
 * 스레드 구조 자체는 페이지의 맨 아래에 위치합니다.
 * (오프셋 0에서).  페이지의 나머지 부분은 * 스레드의 커널 스택을 위해 예약되어 있습니다.
 * 스레드의 커널 스택을 위해 예약되어 있습니다.
 * 페이지(오프셋 4KB에서)의 상단에서 아래쪽으로 증가합니다.  다음은 그림입니다:
 *
 * 4kB +---------------------------------+
 * | 커널 스택 |
 * | | |
 * | | |
 * | V |
 * | 아래쪽으로 성장합니다 |
 * | |
 * | |
 * | |
 * | |
 * | |
 * | |
 * | |
 * | |
 * +---------------------------------+
 * | magic |
 * | 인트라_프레임 |
 * | :                |
 * | :                |
 * | 이름 |
 * | 상태 |
 * 0 kB +---------------------------------+
 *
 * 결론은 두 가지입니다:
 *
 * 1. 첫째, '구조체 스레드'가 너무 커지지 않도록 해야 합니다.
 * 커지지 않도록 해야 합니다.  만약 그렇게 되면, 커널 스택을 위한 공간이
 * 커널 스택을 위한 공간이 충분하지 않게 됩니다.  우리의 기본 '구조체 스레드'는 단지
 * 몇 바이트 크기입니다.  아마도 1
 * kB.
 *
 * 2. 둘째, 커널 스택이 너무 커지지 않도록 해야 합니다.
 * 커지지 않도록 해야 합니다.  스택이 오버플로되면 스레드 상태가 손상됩니다.
 * 상태가 손상됩니다.  따라서 커널 함수는 큰 구조체나 배열을 비정적 로컬 변수로 할당해서는 안됩니다.
 * 구조체나 배열을 정적이 아닌 로컬 변수로 할당해서는 안 됩니다.  사용
 malloc() 또는 palloc_get_page()를 사용한 * 동적 할당
 * 대신 사용하세요.
 *
 * 이러한 문제의 첫 번째 증상은 아마도 다음과 같습니다.
 * 스레드_current()의 어설션 실패로, 실행 중인 스레드의 멤버가
 * 실행 중인 스레드의 '구조체 스레드'의 `마법' 멤버가
 THREAD_MAGIC으로 설정되어 있는지 확인하는 * 어설션 실패일 수 있습니다.  스택 오버플로는 일반적으로 이
 * 값을 변경하여 어설션을 트리거합니다. */
/* `elem' 멤버는 두 가지 용도로 사용됩니다.  다음의 요소가 될 수 있습니다.
 * 실행 큐(thread.c)의 엘리먼트이거나, 또는
 * 세마포어 대기 목록(synch.c)의 요소일 수도 있습니다.  이 두 가지 방법으로 사용할 수 있습니다.
 상호 배타적이기 때문에 오직 * 준비 상태의 스레드만
 준비 상태의 스레드만 실행 대기열에 있는 반면, * 차단 상태의 스레드만
 * 차단된 상태의 스레드만 세마포어 대기 목록에 있습니다. */

struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
