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

void hexdump(void *addr, size_t size)
{
	for (int i = 0 ; i < size ; i++) {
		fprintf(stdout, " %02x,", *(((char *)addr) + i) & 0xff);
		if (i > 0 && (i + 1) % 8 == 0) {
			fprintf(stdout, "\n");
		}
	}
	fprintf(stdout, "\n");
}

// hardcoded for x86_64 right now
int read_remote_mem(pid_t pid, void *addr, void *buf, size_t size)
{
	//fprintf(stdout, "attempting to read %d bytes from %d at %p\n", size, pid, addr);
	//fprintf(stdout, "writing to %p\n", buf);
	for (size_t s = 0 ; s < size ; s+=8) {
		errno = 0;
		*((long *)buf + s/8) = ptrace(PTRACE_PEEKDATA, pid, addr + s, NULL);
		if (errno != 0) {
			//fprintf(stderr, "ptrace PEEK_DATA failed %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

/**
 * The CallInfo contains a pointer to the current PC - savedpc
 * The Function Proto contains an array of Instructions
 * So we figure out what index the savedpc is in that array,
 * and then look up that index is in lineinfo
 *
 * See ldebug.c/.h
 */
int getLineNumber(pid_t pid, CallInfo *ci, struct Proto *p, int *line)
{
	void *scratch = NULL; // use a pointer b/c we can't read only an int w/ ptrace - need to read a word
	// not sure what the -1 is for, but pcRel does this, and
	// it seems to work
	int index = ci->u.l.savedpc - p->code - 1;
	// read the appropriate line info
	int *target = p->lineinfo + index;
	if (read_remote_mem(pid, target, &scratch, sizeof(scratch))) {
		return -1;
	}
	*line = (int) scratch;
	return 0;
}


/*
 * reimplementation of lua_getstack using ptrace on a remote process
 */
int my_lua_getstack(lua_State *L, pid_t pid) {
	int status = 0;
	CallInfo ci;
	char *funcname = NULL;
	TValue tval;
	struct LClosure closure;
	struct Proto proto;
	TString sourceStr;
	char sourceBuf[128];
	int line;
	 
	read_remote_mem(pid, L->ci, &ci, sizeof(ci));
	for ( ; ci.previous != NULL ; read_remote_mem(pid, ci.previous, &ci, sizeof(ci))) {

		// get the TValue
		read_remote_mem(pid, ci.func, &tval, sizeof(tval));
		memset(&closure, 0xff, sizeof(closure));
		read_remote_mem(pid, tval.value_.gc, &closure, sizeof(closure));

		memset(&proto, 0xff, sizeof(proto));
		if (read_remote_mem(pid, closure.p, &proto, sizeof(proto))) {
			continue;
		}

		memset(sourceBuf, 0xff, sizeof(sourceBuf));
		if (read_remote_mem(pid, ((char *)proto.source) + sizeof(TString), &sourceBuf, sizeof(sourceBuf))) {
			continue;
		}

		if (getLineNumber(pid, &ci, &proto, &line)) {
			line = -1;
		}
		printf("%s:%d\n", sourceBuf, line);
	}
	return status;
}

int main(int argc, char **argv)
{
	pid_t pid = atoi(argv[1]);
	//attach
	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
		fprintf(stderr, "ptrace attach failed\n");
		return -1;
	}

	// wait for the proc to actually stop
	waitpid(pid, NULL, __WALL);

	long addr = strtol(argv[2], NULL, 16);
	lua_State state;
	memset(&state, 5, sizeof(state));
	if (read_remote_mem(pid, (void *)addr, &state, sizeof(state)) != 0) {
		fprintf(stderr, "read_remote mem failed :( (%s)\n", strerror(errno));
		return -1;
	}
	my_lua_getstack(&state, pid);
	ptrace(PTRACE_DETACH, pid);
	return 0;
}
