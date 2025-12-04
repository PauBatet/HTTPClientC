#ifndef HTML_TEMPLATING_H
#define HTML_TEMPLATING_H

#include"HTTPServer.h"
#include "config.h"

// Function pointer type for value conversion
typedef char* (*ValueConverter)(const void* value);

// Converter declarations
char* convert_string(const void* value);
char* convert_int(const void* value);
char* convert_float(const void* value);
char* convert_bool(const void* value);

typedef struct {
    const char* key;
    const void* value;
    ValueConverter converter;
} TemplateParam;

void render_html(HTTPRequest *request, const char *file_path, TemplateParam* params, int param_count);

char *process_html(const char *file_path, TemplateParam* params, int param_count);

#endif
