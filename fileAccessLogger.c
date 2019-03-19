#define _32

#include "../jtraceAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h> 
#include <time.h> 
#include <string.h>
 
#define PLUGIN_NAME "File Access Logger"

void sysLog(char* msg)
{
	fprintf(stderr, "[%s] %s.\n", PLUGIN_NAME, msg);
}

struct logger_t {
	unsigned int size;
	void* handle;
	struct event_t* (*addEvent)(const long, const char*);
	void (*flush)();
} logger_t;

static struct logger_t* _logger = NULL;

/**
	Send event to logger
*/
void LOG(char* msg)
{
	if(_logger) {
		struct event_t* event = _logger->addEvent(time(0), msg);
		if(event) {
			_logger->size++;
		}
		if(_logger->size > 100) {
			_logger->flush();
			_logger->size = 0;
		}
	}
}

/**
	Handler for write syscalls 
*/
syscall_handler_ret_t onFileWrite(regs_t* regs, int isExit, int pId)
{
	static int is32 = -1;
	if(is32 < 0) is32 = get_is32();

	if(!isExit) {
		long arg0 = getRegValue(regs, REG_ARG_0, is32);
		long arg1 = getRegValue(regs, REG_ARG_1, is32);
		long arg2 = getRegValue(regs, REG_ARG_2, is32);

		char* fdName = getFDName(arg0);

		char* data = alloca(arg2);
		int result = readProcessMemory(pId, arg1, arg2, data);
		if(result < 0) 
		{ 
			sysLog("Failed to read memory");
		}
		else 
		{
			char* msg = (char*)alloca(strlen(fdName) + arg2 + 16);
			sprintf(msg, "%d|write|%s|%s", pId, fdName, data);
			LOG(msg);
		}
	}

	return DO_NOTHING;
}

/**
	Handler for read syscalls 
*/
syscall_handler_ret_t onFileRead(regs_t* regs, int isExit, int pId)
{
	static int is32 = -1;
	if(is32 < 0) is32 = get_is32();

	if(!isExit) {
		long arg0 = getRegValue(regs, REG_ARG_0, is32);
		long arg1 = getRegValue(regs, REG_ARG_1, is32);
		long arg2 = getRegValue(regs, REG_ARG_2, is32);

		char* fdName = getFDName(arg0);

		char* data = alloca(arg2);
		int result = readProcessMemory(pId, arg1, arg2, data);
		if(result < 0) 
		{ 
			sysLog("Failed to read memory");
		}
		else 
		{
			char* msg = (char*)alloca(strlen(fdName) + arg2 + 106);
			sprintf(msg, "%d|read|%s|%s", pId, fdName, data);
			LOG(msg);
		}
	}

	return DO_NOTHING;
}

/**
	Handler for open syscalls 
*/
syscall_handler_ret_t onFileOpened(regs_t* regs, int isExit, int pId)
{
	static int is32 = -1;
	if (is32 <0) is32 = get_is32();

	fprintf(stderr, "[%s]Handler: PID %d on %s!\n",	PLUGIN_NAME, pId, isExit ? "exit" : "entry");
	
	if(!isExit) 
	{
		long arg0 = getRegValue(regs, REG_ARG_0, is32);
		char *buff = (char*)alloca(512);
		int rc = readProcessMemory (pId, arg0, 512, buff);
		if (rc < 0) 
		{ 
			sysLog("Failed to read memory");
		}
		else 
		{
			char* msg = (char*)alloca(1024);
			sprintf(msg, "%d|%s", pId, buff);
			LOG(msg);
		}
	}

	return DO_NOTHING;
}

/**
	Load and init the logger 
*/
struct logger_t* initLogging()
{
	struct logger_t* logger = (struct logger_t*)calloc(1, sizeof(struct logger_t));
	void* handle = dlopen("lib/liblogging.so", RTLD_NOW|RTLD_GLOBAL);
	if(handle)
	{
		logger->size = 0;
		logger->handle = handle;
		struct event_t* (*addEvent)(const long, const char*);
		void (*flush)();
		addEvent = (struct event_t* (*)(const long, const char*))dlsym(handle, "createEvent");
		flush = (void (*)())dlsym(handle, "flush");
		if(addEvent && flush)
		{
			logger->addEvent = addEvent;
			logger->flush = flush;
			_logger = logger;
			return _logger;
		}
		else
		{
			sysLog("Failed getting library symbols");
		}
	}
	else
	{
		sysLog("Failed opening library");
	}

	return NULL;
}

void closeLogging()
{
	if(_logger) {
		if(_logger->size > 0) {
			_logger->flush();
		}
		dlclose(_logger->handle);
		free(_logger);
	}
}

__attribute__((constructor)) void _init(void) 
{
	sysLog("Loaded");
	struct logger_t* logger = initLogging();
	int result;
	if(logger) {
		result = registerSyscallHandler("open", onFileOpened, 0, 0);
		result = registerSyscallHandler("read", onFileRead, 0, 0);
		result = registerSyscallHandler("write", onFileWrite, 0, 0);
	}
	else {
		sysLog("Failed initiating logger");
	}
}