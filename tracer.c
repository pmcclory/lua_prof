#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <lua.h>
#include <lauxlib.h>
#include <lstate.h>
#include <lualib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "rproc.h"

/**
 * The CallInfo contains a pointer to the current PC - savedpc
 * The Function Proto contains an array of Instructions
 * So we figure out what index the savedpc is in that array,
 * and then look up that index is in lineinfo
 *
 * See ldebug.c/.h
 */
int getLineNumber(struct remote_proc *rproc, CallInfo *ci, struct Proto *p, int *line)
{
	void *scratch = NULL; // use a pointer b/c we can't read only an int w/ ptrace - need to read a word
	// not sure what the -1 is for, but pcRel does this, and
	// it seems to work
	int index = ci->u.l.savedpc - p->code - 1;
	// read the appropriate line info
	int *target = p->lineinfo + index;
	if (rproc_read_mem(rproc, target, &scratch, sizeof(scratch))) {
		return -1;
	}
	*line = (int) scratch;
	return 0;
}


/*
 * reimplementation of lua_getstack using ptrace on a remote process
 */
int my_lua_getstack(lua_State *L, struct remote_proc *rproc) {
	int status = 0;
	CallInfo ci;
	TValue tval;
	struct LClosure closure;
	struct Proto proto;
	char sourceBuf[128];
	int line;
	
	if (rproc_read_mem(rproc, L->ci, &ci, sizeof(ci))) {
		fprintf(stderr, "failed to locate call info\n");
		return -1;		
	}
	for ( ; ci.previous != NULL ;
		rproc_read_mem(rproc, ci.previous, &ci, sizeof(ci))) {
	
		// get the TValue
		if (rproc_read_mem(rproc, ci.func, &tval, sizeof(tval))) {
			printf("???\n");
			continue;
		}
		memset(&closure, 0xff, sizeof(closure));
		if (rproc_read_mem(rproc, tval.value_.gc, &closure, sizeof(closure))) {
			printf("???\n");
			continue;
		}

		memset(&proto, 0xff, sizeof(proto));
		if (rproc_read_mem(rproc, closure.p, &proto, sizeof(proto))) {
			printf("???\n");
			continue;
		}

		memset(sourceBuf, 0xff, sizeof(sourceBuf));
		if (rproc_read_mem(rproc, ((char *)proto.source) + sizeof(TString), &sourceBuf, sizeof(sourceBuf))) {
			printf("???\n");
			continue;
		}

		if (getLineNumber(rproc, &ci, &proto, &line)) {
			line = -1;
		}
		printf("%s:%d\n", sourceBuf, line);
	}
	return status;
}

int main(int argc, char **argv)
{
	pid_t pid = atoi(argv[1]);
	long addr = strtol(argv[2], NULL, 16);
	struct remote_proc *rproc = rproc_create(pid);
	if (!rproc) {
		exit(-1);
	}

	if (rproc_pause(rproc)) {
		exit(-1);
	}

	lua_State state;
	memset(&state, 5, sizeof(state));
	if (rproc_read_mem(rproc, (void *)addr, &state, sizeof(state))) {
		exit(-1);	
	}

	my_lua_getstack(&state, rproc);
	rproc_resume(rproc);
	return 0;
}
