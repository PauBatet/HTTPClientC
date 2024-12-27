#include"HTTPFramework.h"
#include"views.c"
#include<stddef.h>

Route routes[] = {
	{"/", home},
	{"/about", about},
	{"/ABT", abt},
	{"/delay", Delay30},
	{NULL, NULL},
};
