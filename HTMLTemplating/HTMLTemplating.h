#ifndef HTML_TEMPLATING_H
#define HTML_TEMPLATING_H

#include"HTTPServer.h"

// Function pointer type for value conversion
typedef char* (*ValueConverter)(const void* value);

typedef struct {
    const char* key;
    const void* value;
    ValueConverter converter;
} TemplateParam;

void render_html(HTTPRequest *request, const char *file_path, TemplateParam* params, int param_count);

#endif
