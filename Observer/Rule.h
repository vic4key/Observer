#ifndef RULE_H
#define RULE_H
#pragma once

#ifdef NTDDI_VERSION
#include <ntddk.h>
#else

#endif //!NTDDI_VERSION

#define RULE_TYPE_REGISTRY			1
#define RULE_TYPE_CREATE_THREAD		2
#define RULE_TYPE_CREATE_PROCESS	3
#define RULE_TYPE_MODULE_LOAD		4
#define RULE_TYPE_DRIVER_LOAD		5

#define ACTION_IGNORE (1 << 0)
#define ACTION_DBGPRINT (1 << 1)
#define ACTION_REPORT (1 << 2)
#define ACTION_EVENTLOG (1 << 3)
#define ACTION_BLOCK (1 << 10)

#define IOCTL_OBSERVER_ADD_RULE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_OBSERVER_REMOVE_RULE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_DATA)

#define REGISTRY_MATCH_IGNORE_CASE (1 << 0)
#define REGISTRY_MATCH_EQUALS (1 << 1)
#define REGISTRY_MATCH_CONTAINS (1 << 2)

#define THREAD_CREATION_CHECK_FROM_PROCESS (1 << 0)
#define THREAD_CREATION_CHECK_TO_PROCESS (1 << 1)
#define THREAD_CREATION_CHECK_FROM_THREAD (1 << 2)
#define THREAD_CREATION_CHECK_NOT_DIFFERENT_PROCESSES (1 << 3)

#define PROCESS_CREATION_CHECK_PARENT (1 << 0)
#define PROCESS_CREATION_CHECK_CREATING_PROCESS (1 << 1)
#define PROCESS_CREATION_CHECK_CREATING_THREAD (1 << 2)

#define DRIVER_LOAD_CHECK_UNSIGNED (1 << 0)
#define DRIVER_LOAD_CHECK_PATH_CONTAINS (1 << 1)
#define DRIVER_LOAD_CHECK_PATH_NOT_CONTAINS (1 << 2)

#define MODULE_LOAD_CHECK_SYSTEM_PROCESS (1 << 0)
#define MODULE_LOAD_CHECK_UNSIGNED (1 << 1)
#define MODULE_LOAD_CHECK_SIGNED (1 << 2)

typedef struct _OBSERVER_REGISTRY_RULE
{
	ULONG Action;
	ULONG MatchFlags;
	ULONG PathLength;
	WCHAR Path[1];
} OBSERVER_REGISTRY_RULE, *POBSERVER_REGISTRY_RULE;

typedef struct _OBSERVER_THREAD_CREATION_RULE
{
	ULONG	Action;
	ULONG	ThreadRuleCheckFlags;
	UINT64  FromProcessID;
	UINT64  ToProcessID;
	UINT64  FromThreadID;
} OBSERVER_THREAD_CREATION_RULE, *POBSERVER_THREAD_CREATION_RULE;

typedef struct _OBSERVER_PROCESS_CREATION_RULE
{
	ULONG	Action;
	ULONG	ProcessRuleCheckFlags;
	UINT64  ParentProcessID;
	UINT64  CreatingProcessID;
	UINT64  CreatingThreadID;
} OBSERVER_PROCESS_CREATION_RULE, *POBSERVER_PROCESS_CREATION_RULE;

typedef struct _OBSERVER_MODULE_LOAD_RULE
{
	ULONG	Action;
	ULONG	ModuleLoadCheckFlags;
	ULONG	PathLength;
	WCHAR	Path[1];
} OBSERVER_MODULE_LOAD_RULE, *POBSERVER_MODULE_LOAD_RULE;

typedef struct _OBSERVER_DRIVER_LOAD_RULE
{
	ULONG	Action;
	ULONG	DriverLoadCheckFlags;
	ULONG	PathLength;
	WCHAR	Path[1];
} OBSERVER_DRIVER_LOAD_RULE, *POBSERVER_DRIVER_LOAD_RULE;

typedef struct _OBSERVER_ADD_RULE 
{
	ULONG RuleType;
	union
	{
		OBSERVER_REGISTRY_RULE			Registry;
		OBSERVER_THREAD_CREATION_RULE	Thread;
		OBSERVER_PROCESS_CREATION_RULE	Process;
		OBSERVER_MODULE_LOAD_RULE		ModuleLoad;
		OBSERVER_DRIVER_LOAD_RULE		DriverLoad;
	} Rule;
} OBSERVER_ADD_RULE, *POBSERVER_ADD_RULE;

typedef struct _OBSERVER_RULE_HANDLE
{
	ULONG	RuleType;
	LONG64	RuleHandle;
} OBSERVER_RULE_HANDLE, *POBSERVER_RULE_HANDLE;

#endif //RULE_H
