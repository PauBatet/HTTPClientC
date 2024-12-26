#ifndef HTTPFRAMEWORK_H
#define HTTPFRAMEWORK_H

#include"HTTPServer.h"

typedef struct {
	const char *path;
	void (*handler)(HTTPRequest *request);
}Route;

extern Route routes[];

#endif
