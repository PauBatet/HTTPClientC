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

// Helper function to infer the type and return appropriate converter
ValueConverter infer_converter(const void* value) {
    // Try to infer string first
    if (is_string(value)) {
        return convert_string;
    }
    
    // Try to infer bool (check both 0/1 and true/false patterns)
    const bool* bool_val = (const bool*)value;
    if (sizeof(bool) == 1 && (*bool_val == 0 || *bool_val == 1)) {
        return convert_bool;
    }
    
    // Try to infer float
    if (is_number(value, sizeof(float))) {
        const float f = *(const float*)value;
        if (f != (int)f) {  // Has decimal part
            return convert_float;
        }
    }
    
    // Try to infer int
    if (is_number(value, sizeof(int))) {
        return convert_int;
    }
    
    // Default to string if we can't determine the type
    return convert_string;
}

// Helper function to find and replace template parameters
char* replace_template_params(const char* template, TemplateParam* params, int param_count) {
    char* result = strdup(template);
    
    for (int i = 0; i < param_count; i++) {
        char search_pattern[256];
        snprintf(search_pattern, sizeof(search_pattern), "{{%s}}", params[i].key);
        
        // Remove spaces from search pattern
        char* clean_pattern = strdup(search_pattern);
        char* src = clean_pattern;
        char* dst = clean_pattern;
        while (*src) {
            if (*src != ' ') {
                *dst = *src;
                dst++;
            }
            src++;
        }
        *dst = '\0';
        
        // If no converter is provided, try to infer one
        ValueConverter converter = params[i].converter ? 
                                 params[i].converter : 
                                 infer_converter(params[i].value);
        
        // Convert the value using the converter function
        char* value_str = converter(params[i].value);
        if (!value_str) {
            free(clean_pattern);
            free(result);
            return NULL;
        }
        
        char* pos;
        while ((pos = strstr(result, clean_pattern)) != NULL) {
            int value_len = strlen(value_str);
            int pattern_len = strlen(clean_pattern);
            int new_size = strlen(result) - pattern_len + value_len + 1;
            
            char* new_result = (char*)malloc(new_size);
            if (!new_result) {
                free(clean_pattern);
                free(value_str);
                free(result);
                return NULL;
            }
            
            strncpy(new_result, result, pos - result);
            strcpy(new_result + (pos - result), value_str);
            strcpy(new_result + (pos - result) + value_len, pos + pattern_len);
            
            free(result);
            result = new_result;
        }
        
        free(clean_pattern);
        free(value_str);
    }
    
    return result;
}

void render_html(HTTPRequest *request, const char *file_path, TemplateParam* params, int param_count) {
    FILE *file = fopen(file_path, "r");
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
        const char *body = "<h1>500 Internal Server Error</h1>";
        HTTPServer_send_response(request, body, "", 500, "");
        fclose(file);
        return;
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    char* processed_content = replace_template_params(file_content, params, param_count);
    if (!processed_content) {
        const char *body = "<h1>500 Internal Server Error</h1>";
        HTTPServer_send_response(request, body, "", 500, "");
        free(file_content);
        return;
    }

    HTTPServer_send_response(request, processed_content, "", 0, "");

    free(processed_content);
    free(file_content);
}

char *process_html(const char *file_path, TemplateParam* params, int param_count) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        return "";
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    if (!file_content) {
        fclose(file);
        return "";
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    char* processed_content = replace_template_params(file_content, params, param_count);
    if (!processed_content) {
        free(file_content);
        return "";
    }

    free(file_content);
    return processed_content;
}