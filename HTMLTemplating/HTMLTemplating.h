#ifndef HTML_TEMPLATING_H
#define HTML_TEMPLATING_H

#include"HTTPServer.h"

void render_html(HTTPRequest *request, const char *file_path);

#endif
