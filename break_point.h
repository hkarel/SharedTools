/****************************************************************************
  Author of idea: Fedorin Denis

  В модуле реализована функция имитирующая breakpoint для gdb отладчика.
****************************************************************************/

#pragma once

/***
void sigtrap_handler(int signo)
{}

int debug_init()
{
	return signal(SIGTRAP, sigtrap_handler) == SIG_ERR ? -1 : 0;
}

#define DebugBreak() {asm volatile("int $3");}

int main ()
{
	int a, b, c, max;
	debug_init();
	scanf("%d %d %d", &a, &b, &c);
	if (a > b)
		max = a;
	else
		max = b;
	DebugBreak();
	if (c > max)
		max = c;
	printf("%d\n", max);
	return 0;
}
*/

#ifndef NDEBUG
  #ifdef _MSC_VER
    #define break_point  _CrtDbgBreak();
  #else
    #include <stdio.h>
    #include <signal.h>
    #include <stdlib.h>
    #include <assert.h>

    static void sigtrap_handler(int /*signo*/)
    {}
    struct InitBreakPoint
    {
        InitBreakPoint() {
            assert(signal(SIGTRAP, sigtrap_handler) != SIG_ERR);
        }
    } static init_break_point;
    //#define break_point  {asm volatile("int $3");}
    #define break_point  {asm volatile("int $3" ::: "cc", "memory");}
  #endif // _MSC_VER
#else
    #define break_point
#endif