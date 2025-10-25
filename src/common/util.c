#include "util.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* normalized_strcmp: compares normalized versions of a and b.
 * returns same semantics as strcmp.
 * handles NULL pointers (treat as empty).
 */
int normalized_strcmp(const char *a, const char *b) {
    char *na = normalize_string(a ? a : "");
    char *nb = normalize_string(b ? b : "");
    if (!na || !nb) {
        if (na) free(na);
        if (nb) free(nb);
        return strcmp(a ? a : "", b ? b : "");
    }
    int r = strcmp(na, nb);
    free(na);
    free(nb);
    return r;
}

char *normalize_string(const char *s) {
    if (s == NULL) {
        char *empty = malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t in_len = strlen(s);
    char *out = malloc(in_len + 1); // worst-case buffer: same length + 1
    if (!out) return NULL;

    size_t i = 0;
    size_t out_idx = 0;
    
    while (s[i] != '\0' && i <= KEY_PREFIX_LEN) {
        unsigned char c1 = s[i];

        // Manejo de caracteres ASCII de 1 byte (los más comunes)
        if (c1 < 128) {
            char lower = tolower(c1);
            if ((lower >= 'a' && lower <= 'z') || (lower >= '0' && lower <= '9')) {
                out[out_idx++] = lower;
            }
            i++;
            continue;
        }

        // Manejo de caracteres UTF-8 de 2 bytes para español
        if (c1 == 0xc3) {
            unsigned char c2 = s[i+1];
            switch (c2) {
                case 0x81: // Á
                case 0xa1: // á
                    out[out_idx++] = 'a';
                    break;
                case 0x89: // É
                case 0xa9: // é
                    out[out_idx++] = 'e';
                    break;
                case 0x8d: // Í
                case 0xad: // í
                    out[out_idx++] = 'i';
                    break;
                case 0x93: // Ó
                case 0xb3: // ó
                    out[out_idx++] = 'o';
                    break;
                case 0x9a: // Ú
                case 0xba: // ú
                    out[out_idx++] = 'u';
                    break;
                case 0x91: // Ñ
                case 0xb1: // ñ
                    out[out_idx++] = 'n';
                    break;
                default:
                    // Si es un carácter UTF-8 que no manejamos, lo ignoramos
                    break;
            }
            i += 2;
        } else {
            i++;
        }
    }
    
    out[out_idx] = '\0';
    return out;
}
