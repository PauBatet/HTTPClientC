#ifndef HTTPFRAMEWORK_H
#define HTTPFRAMEWORK_H

#include"HTMLTemplating.h"
#include"Database/Database.h"

typedef struct {
	const char *path;
	void (*handler)(HTTPRequest *request, Database *db);
}Route;

extern Route routes[];

#endif
