#include "Routing.h"

bool route_match(const char *templ, const char *path, HTTPRequest *req) {
    const char *tptr = templ;
    const char *pptr = path;

    while (*tptr || *pptr) {
        // Skip leading slashes
        if (*tptr == '/') tptr++;
        if (*pptr == '/') pptr++;

        if (*tptr == '\0' && *pptr == '\0') break;
        if (*tptr == '\0' || *pptr == '\0') return false;

        // Find next segment in template and path
        const char *tnext = strchr(tptr, '/');
        size_t tlen = tnext ? (size_t)(tnext - tptr) : strlen(tptr);

        const char *pnext = strchr(pptr, '/');
        size_t plen = pnext ? (size_t)(pnext - pptr) : strlen(pptr);

        // Copy segments safely
        char tseg[128] = {0};
        char pseg[128] = {0};
        if (tlen >= sizeof(tseg) || plen >= sizeof(pseg)) return false;
        memcpy(tseg, tptr, tlen);
        tseg[tlen] = '\0';
        memcpy(pseg, pptr, plen);
        pseg[plen] = '\0';

        // Param segment?
        if (tseg[0] == '<') {
            char name[64] = {0};
            if (sscanf(tseg, "<%63[^>]>", name) != 1) return false;

            if (!HTTPRequest_add_param(req, name, pseg)) return false;
        } else {
            if (tlen != plen || strncmp(tseg, pseg, tlen) != 0) return false;
        }

        tptr += tlen;
        pptr += plen;
    }

    return (*tptr == '\0' && *pptr == '\0');
}

