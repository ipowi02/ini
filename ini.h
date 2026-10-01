#ifndef INI_H_
#define INI_H_

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef INI_KV_DELIM
#define INI_KV_DELIM '='
/* TODO: make it multi-character to support more than one delims
 * in ini_parse_pair(), refactor
 */
#endif

#ifndef INI_BUFLEN
#define INI_BUFLEN 256
#endif

#ifndef INI_SECTNAMELEN
#define INI_SECTNAMELEN 32
#endif

#if INI_BUFLEN < 4 || (INI_BUFLEN % 2) != 0
#error "INI_BUFLEN must be an even number >= 4"
#endif

enum ini_result {
    INI_OK = 0,
    INI_EINVAL,

    INI_EPARSE,
    INI_EKEY,
    INI_EVAL,
    INI_ESECT,

    INI_EIO,
    INI_ELONG,
    INI_EEOF,
};

/*
 * One key-value pair returned by ini_next().
 *
 * Everything is stored by value, so the result remains valid after
 * ini_next() returns and until the caller overwrites it.
 *
 * section is the section containing the pair, or "" for the global section.
 */
struct ini_line {
    char section[INI_SECTNAMELEN];
    char key[INI_BUFLEN / 2];
    char val[INI_BUFLEN / 2];
};

struct ini_parser {
    FILE *f;
    size_t lineno;

    char section[INI_SECTNAMELEN];
};

enum ini_result ini_init(struct ini_parser *p, FILE *f);

/*
 * Read the next key-value pair.
 * out contains the next pair if INI_OK is returned
 */
enum ini_result ini_next(struct ini_parser *p, struct ini_line *out);

#ifdef INI_IMPLEMENTATION

static int ini_isspace(int c)
{
    return c != EOF && isspace(c);
}

static char *ini_ltrim(char *s)
{
    while (ini_isspace(*s))
        ++s;

    return s;
}

static void ini_rtrim(char *s)
{
    size_t n = strlen(s);

    while (n && ini_isspace(s[n - 1]))
        s[--n] = '\0';
}

// The line, including its terminating '\0' must be less than INI_BUFLEN
static enum ini_result ini_getline(struct ini_parser *p, char buf[static INI_BUFLEN])
{
    size_t n = 0;
    int c;

    for (;;) {
        c = fgetc(p->f);

        if (c == EOF) {
            if (ferror(p->f))
                return INI_EIO;

            if (n == 0)
                return INI_EEOF;

            break;
        }

        if (c == '\n')
            break;

        if (n + 1 >= INI_BUFLEN) {
            while ((c = fgetc(p->f)) != '\n' && c != EOF)
                ;

            if (c == EOF && ferror(p->f))
                return INI_EIO;

            ++p->lineno;
            return INI_ELONG;
        }

        buf[n++] = c;
    }

    // for CRLF line endings (imagine using Windows in 2026)
    if (n && buf[n - 1] == '\r')
        --n;

    buf[n] = '\0';
    ++p->lineno;

    return INI_OK;
}

static enum ini_result ini_parse_section(struct ini_parser *p, char *line)
{
    char *name = ini_ltrim(line + 1);
    char *end = strchr(name, ']');
    char *tail;
    size_t len;

    if (!end)
        return INI_ESECT;

    *end = '\0';
    ini_rtrim(name);

    if (*name == '\0')
        return INI_ESECT;

    // After ']' only whitespace or a comment should remain
    tail = ini_ltrim(end + 1);

    if (*tail != '\0' && *tail != ';' && *tail != '#')
        return INI_ESECT;

    len = strlen(name);

    if (len >= sizeof p->section)
        return INI_ELONG;

    memcpy(p->section, name, len + 1);

    return INI_OK;
}

static enum ini_result ini_parse_pair(struct ini_line *out, char *line)
{
    char *eq;
    char *key;
    char *val;
    size_t len;

    eq = strchr(line, INI_KV_DELIM);

    if (!eq)
        return INI_EPARSE;

    *eq = '\0';

    key = ini_ltrim(line);
    ini_rtrim(key);

    val = ini_ltrim(eq + 1);

    // ';' and '#' only form comments when followed by whitespace or end of line
    for (char *s = val; *s; ++s) {
        if ((*s == '#' || *s == ';') &&
            (s[1] == '\0' || ini_isspace(s[1]))) {
            *s = '\0';
            break;
        }
    }

    ini_rtrim(val);

    if (*key == '\0')
        return INI_EKEY;

    len = strlen(key);
    if (len >= sizeof out->key)
        return INI_EKEY;

    memcpy(out->key, key, len + 1);

    len = strlen(val);
    if (len >= sizeof out->val)
        return INI_EVAL;

    memcpy(out->val, val, len + 1);

    return INI_OK;
}

enum ini_result ini_init(struct ini_parser *p, FILE *f)
{
    if (!p || !f)
        return INI_EINVAL;

    memset(p, 0, sizeof *p);

    p->f = f;

    return INI_OK;
}

enum ini_result ini_next(struct ini_parser *p, struct ini_line *out)
{
    char line[INI_BUFLEN];
    enum ini_result result;

    if (!p || !out || !p->f)
        return INI_EINVAL;

    memset(out, 0, sizeof *out);

    for (;;) {
        result = ini_getline(p, line);

        if (result != INI_OK)
            return result;

        char *s = ini_ltrim(line);
        ini_rtrim(s);

        if (*s == '\0' || *s == ';' || *s == '#')
            continue;

        if (*s == '[') {
            result = ini_parse_section(p, s);

            if (result != INI_OK)
                return result;

            continue;
        }

        result = ini_parse_pair(out, s);

        if (result != INI_OK)
            return result;

        memcpy(out->section, p->section, sizeof out->section);
        return INI_OK;
    }
}

#endif // INI_IMPLEMENTATION 

#endif // INI_H_ 

