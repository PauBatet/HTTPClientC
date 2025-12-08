#include"HTMLTemplating.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>
#include<math.h>

// Example converter functions
char* convert_string(const void* value) {
	return strdup((const char*)value);
}

char* convert_int(const void* value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d", *(const int*)value);
    return strdup(buffer);
}

char* convert_float(const void* value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f", *(const float*)value);
    return strdup(buffer);
}

char* convert_bool(const void* value) {
    return strdup(*(const bool*)value ? "true" : "false");
}

// Helper function to check if a value looks like a string
bool is_string(const void* value) {
    const char* str = (const char*)value;
    // Check if it points to valid memory and looks like a string
    if (!str) return false;
    
    // Check first few characters to see if they're printable
    for (int i = 0; i < 8 && str[i] != '\0'; i++) {
        if (!isprint(str[i])) return false;
    }
    return true;
}

// Helper function to check if a value looks like a number
bool is_number(const void* value, size_t size) {
    
    // Check for common float patterns in memory
    if (size == sizeof(float)) {
        float f = *(const float*)value;
        return !isnan(f) && !isinf(f);
    }
    
    // For integers, check if the value looks reasonable
    if (size == sizeof(int)) {
        int i = *(const int*)value;
        return i > -1000000000 && i < 1000000000; // Reasonable range
    }
    
    return false;
}

// Helper function to find and replace template parameters
char* replace_template_params(const char* template, TemplateParam* params, int param_count) {
    char* result = strdup(template);
    
    for (int i = 0; i < param_count; i++) {
        const char *patterns[2];
        char buf1[128], buf2[128];

        snprintf(buf1, sizeof(buf1), "{{%s}}", params[i].key);
        snprintf(buf2, sizeof(buf2), "{{ %s }}", params[i].key);

        patterns[0] = buf1;
        patterns[1] = buf2;

        ValueConverter converter = params[i].converter ? params[i].converter : convert_string;
        
        // Convert the value using the converter function
        char* value_str = converter(params[i].value);
        if (!value_str) {
            free(result);
            return NULL;
        }
 
        for (int p = 0; p < 2; p++) {
            const char *pattern = patterns[p];
            char *pos;

            while ((pos = strstr(result, pattern)) != NULL) {
                int value_len = strlen(value_str);
                int pattern_len = strlen(pattern);
                int new_size = strlen(result) - pattern_len + value_len + 1;

                char *new_result = malloc(new_size);

                strncpy(new_result, result, pos - result);
                strcpy(new_result + (pos - result), value_str);
                strcpy(new_result + (pos - result) + value_len, pos + pattern_len);

                free(result);
                result = new_result;
            }
        }
        free(value_str);
    }
    
    return result;
}

void render_html(HTTPRequest *request, const char *file_path, TemplateParam* params, int param_count) {
    size_t len = snprintf(NULL, 0, "%s/%s", TEMPLATE_DIR, file_path) + 1;
    char *fullpath = malloc(len);
    snprintf(fullpath, len, "%s/%s", TEMPLATE_DIR, file_path);
    FILE *file = fopen(fullpath, "r");
    if (!file) {
        const char *body = "<h1>404 Not Found</h1>";
        HTTPServer_send_response(request, body, "", 404, "");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    if (!file_content) {
        fclose(file);
        const char *body = "<h1>500 Internal Server Error</h1>";
        HTTPServer_send_response(request, body, "", 500, "");
        return;
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    char* processed_content = replace_template_params(file_content, params, param_count);
    free(file_content);
    if (!processed_content) {
        const char *body = "<h1>500 Internal Server Error</h1>";
        HTTPServer_send_response(request, body, "", 500, "");
        return;
    }

    HTTPServer_send_response(request, processed_content, "", 0, "");

    free(processed_content);
}

char *process_html(const char *file_path, TemplateParam* params, int param_count) {
    size_t len = snprintf(NULL, 0, "%s/%s", TEMPLATE_DIR, file_path) + 1;
    char *fullpath = malloc(len);
    snprintf(fullpath, len, "%s/%s", TEMPLATE_DIR, file_path);
    FILE *file = fopen(fullpath, "r");
    if (!file) {
        return strdup("");
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    if (!file_content) {
        fclose(file);
        return strdup("");
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    char* processed_content = replace_template_params(file_content, params, param_count);
    free(file_content);
    if (!processed_content) {
        return strdup("");
    }

    return processed_content;
}
