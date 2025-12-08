#include "../HTTPServer/HTTPServer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

bool route_match(const char *templ, const char *path, HTTPRequest *req);

