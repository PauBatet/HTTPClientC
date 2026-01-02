#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "payload.h"

void make_path(const char *path) {
    char temp[1024]; char *p = NULL;
    snprintf(temp, sizeof(temp), "%s", path);
    for (p = temp + 1; *p; p++) {
        if (*p == '/') { *p = '\0'; mkdir(temp, 0755); *p = '/'; }
    }
}

// Check if current directory is empty
int is_dir_empty(const char *path) {
    int n = 0;
    struct dirent *d;
    DIR *dir = opendir(path);
    if (dir == NULL) return 1; // Treat non-existent as empty
    while ((d = readdir(dir)) != NULL) {
        if (++n > 2) break; // Skip . and ..
    }
    closedir(dir);
    return n <= 2;
}

// Validate if a template_key exists in the manifest
int template_exists(const char *key) {
    if (strcmp(key, "base") == 0) return 1;
    for (int i = 0; manifest[i].vpath != NULL; i++) {
        if (strcmp(manifest[i].template_key, key) == 0) return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const char *requested = (argc > 1) ? argv[1] : "base";
    
    // 1. Validation Logic
    if (!template_exists(requested)) {
        fprintf(stderr, "Error: Unknown template or option '%s'\n", requested);
        return 1;
    }

    // 2. Determine behavior
    int is_docker_addon = (strstr(requested, "docker") != NULL || strstr(requested, "standalone") != NULL);
    
    // CHANGE: If it's a docker addon, we don't want to use "base" or "core"
    // We only want the files tagged with the addon name.
    const char *base_to_use = is_docker_addon ? "none" : requested;
    const char *addon_to_use = is_docker_addon ? requested : "none";
    const char *core_to_use = is_docker_addon ? "none" : "core";

    // 3. Safety Check
    if (!is_docker_addon && !is_dir_empty(".")) {
        printf("Warning: Current directory is not empty. Overwrite template '%s'? (y/n): ", base_to_use);
        char confirm;
        if (scanf(" %c", &confirm) != 1 || (confirm != 'y' && confirm != 'Y')) {
            printf("Aborted.\n");
            return 0;
        }
    }

    printf("Generating: %s\n", requested);

    for (int i = 0; manifest[i].vpath != NULL; i++) {
        int extract = 0;

        // Only extract if it matches the core (if not addon), the base template, or the addon
        if (strcmp(manifest[i].template_key, core_to_use) == 0) extract = 1;
        else if (strcmp(manifest[i].template_key, base_to_use) == 0) extract = 1;
        else if (strcmp(manifest[i].template_key, addon_to_use) == 0) extract = 1;

        if (extract) {
            make_path(manifest[i].vpath);
            FILE *fp = fopen(manifest[i].vpath, "wb");
            if (fp) {
                fwrite(manifest[i].data, 1, manifest[i].size, fp);
                fclose(fp);
                printf("  [+] %s\n", manifest[i].vpath);
            }
        }
    }

    printf("Done.\n");
    return 0;
}
