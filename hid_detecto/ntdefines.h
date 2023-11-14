#pragma once
#include <ntifs.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#ifndef __DISABLE_OUTPUT__
#define	DEBUG_OUTPUT(fmt, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[#] "fmt, ##__VA_ARGS__)
#else
#define	DEBUG_OUTPUT(fmt, ...)
#endif






