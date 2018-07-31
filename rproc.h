#ifndef RPROC_H
#define RPROC_H

#include <sys/types.h>

struct remote_proc {
	pid_t pid;
};

struct remote_proc *rproc_create(pid_t pid);

int rproc_pause(struct remote_proc *p);

int rproc_resume(struct remote_proc *p);

int rproc_read_mem(struct remote_proc *p,
				   void *va,
				   void *buf,
				   size_t size);

#endif

/* vim: set noai ts=4 sw=4: */
