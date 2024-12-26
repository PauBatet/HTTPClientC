#include"HTTPFramework.h"
#include"views.c"
#include<stddef.h>

Route routes[] = {
	{"/", home},
	{"/about", about},
	{NULL, NULL},
};
