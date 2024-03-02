#ifndef DEVICES_TIMER_H // If not defined DEVICES_TIMER_H
#define DEVICES_TIMER_H // Define DEVICES_TIMER_H

#include <round.h>      //반올림 함수
#include <stdint.h>     //정수형

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100  //타이머 인터럽트 횟수

void timer_init (void);      //타이머 초기화
void timer_calibrate (void); //타이머 보정

int64_t timer_ticks (void);       //타이머 틱
int64_t timer_elapsed (int64_t);  //타이머 경과

void timer_sleep (int64_t ticks);         //타이머 슬립
void timer_msleep (int64_t milliseconds); //타이머 밀리초 슬립
void timer_usleep (int64_t microseconds); //타이머 마이크로초 슬립
void timer_nsleep (int64_t nanoseconds);  //타이머 나노초 슬립

void timer_print_stats (void);

#endif /* devices/timer.h */
