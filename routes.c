#include"HTTPFramework.h"
#include"views.h"
#include<stddef.h>

Route routes[] = {
	{"/", home},
	{"/about", about},
	{"/ABT", abt},
	{"/delay", Delay30},
	{NULL, NULL},
};
