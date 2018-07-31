#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "rproc.h"

struct remote_proc *rproc_create(pid_t pid)
{
	struct remote_proc *p;
	p = calloc(1, sizeof(*p));
	if (!p) {
		perror("alloc failed\n");		
	}
	p->pid = pid;
	return p;
}

int rproc_pause(struct remote_proc *p)
{
	if (ptrace(PTRACE_ATTACH, p->pid, NULL, NULL) == -1) {
		perror("attach failed\n");
		return -1;
	}

	// wait for the proc to actually stop
	waitpid(p->pid, NULL, __WALL);
	return 0;
}

int rproc_resume(struct remote_proc *p)
{
	if (ptrace(PTRACE_DETACH, p->pid)) {
		perror("detach failed\n");
		return -1;
	}
	return 0;
}

/*
 * hardcoded for x86_64 right now
 */
int rproc_read_mem(struct remote_proc *p,
				   void *va,
				   void *buf,
				   size_t size)
{
	for (size_t s = 0 ; s < size ; s+=8) {
		errno = 0;
		*((long *)buf + s/8) = ptrace(PTRACE_PEEKDATA, p->pid, va + s, NULL);
		if (errno != 0) {
			return -1;
		}
	}
	return 0;
}
