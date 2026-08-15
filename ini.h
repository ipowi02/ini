#ifndef INI_H_
#define INI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "./thirdparty/da.h"

#define INI_BUFLEN 256
#define INI_SECTNAMELEN 32

#define ini_isempty(s) ((s)[1] == '\1')
#define ini_lnempty(ln) ini_isempty((ln).key)


struct line {
     char key[INI_BUFLEN/2];
     char val[INI_BUFLEN/2];
     // use strlen on them to get their actual length, INI_BUFLEN is a maximum to avoid heap allocations
};

struct section {
     da_member(struct line);
     char name[INI_SECTNAMELEN];
     // TODO: rewrite as char*, it segfaults that way for some reason?
};

struct ini {
     da_member(struct section);
};

int line_nonzeroed(struct line *out, FILE *f);
int sectname_nonzeroed(char out[static INI_SECTNAMELEN], FILE *f);
int section_nonzeroed(struct section *s, int named, FILE *f);

int line(struct line *out, FILE *f);
int sectname(char out[static INI_SECTNAMELEN], FILE *f);
int section(struct section *s, int named, FILE *f);

int ini(struct ini *i, FILE *f);

#ifdef INI_IMPLEMENTATION

int line_nonzeroed(struct line *out, FILE *f) {
     // Assumes *out is null-initialized
     unsigned char keycurr = 0;
     unsigned char valcurr = 0;
     int foundeq = 0;
     
     int c, d;
     int prev = 0;

     c = fgetc(f);
     if (c == EOF)
	  return -2;
     if (c == '\n') {
	  out->key[1] = '\1'; // to indicate an empty line, but still make it compatible with string functions
	  goto end;
     }
     if (c == '[') {
	  fprintf(stderr, "Unexpected section name start\n");
	  return -1;
     }
     
     do {
	  switch (c) {
	  case '=':
	       foundeq = 1;
	       goto outer;

	  case ';':
	  case '#':
	       
	       fseek(f, -2, SEEK_CUR);
	       prev = fgetc(f);
	       fseek(f, 1, SEEK_CUR);
	       
	       d = fgetc(f);
	       
	       if (d != EOF) {
		    if (isspace(d)) {

			 while ((d = fgetc(f)) != '\n' && d != EOF)
			      ; // skip rest of the line

			 if (prev == '\n') {
			      out->key[1] = '\1'; // empty line
			 } 
			 goto end;
		    } else {	 
			 ungetc(d, f);
		    }
	       }
	       break;

	  default:
	       if (valcurr + 1 >= INI_BUFLEN/2 || keycurr + 1 >= INI_BUFLEN/2) {
		    fprintf(stderr, "parsing failed, line too long\n");
		    return -1;
	       }
	       if (!foundeq)
		    out->key[keycurr++] = c;
	       else
		    out->val[valcurr++] = c;
	  }
	  
     outer:	 
	  c = fgetc(f);
     } while (c != '\n' && c != EOF);
     
end:
     if (!foundeq && !ini_lnempty(*out)) {
	  fprintf(stderr, "No key-val separator found on line\n");
	  return -1;
     }
     return 0;
}

int sectname_nonzeroed(char out[static INI_SECTNAMELEN], FILE *f) {
     int c, d;
     unsigned char sectcurr = 0;
     
     c = fgetc(f);

     if (c != '[') {
	  fprintf(stderr, "Parsing section name failed, [ was expected, got %c\n", c);
	  return -1;
     }
     
     while ((c = fgetc(f)) && c != ']') {
	  if (c == '\n') {
	       fprintf(stderr, "Parsing section name failed, line ended prematurely\n");
	       return -1;
	  }
	  if (isspace(c)) {
	       if (sectcurr + 1 >= INI_SECTNAMELEN) {
		    fprintf(stderr, "Parsing failed, section name too long\n");
		    return -1;
	       }
	       
	       out[sectcurr++] = c;
	       while ((d = fgetc(f))) {
		    if (!isspace(d)) {
			 ungetc(d, f);
			 break;
		    }
	       }
	       continue;
	  }
	  
	  if (sectcurr + 1 >= INI_SECTNAMELEN) {
	       fprintf(stderr, "Parsing failed, section name too long\n");
	       return -1;
	  }
	  
	  out[sectcurr++] = c;
     }
     
     c = fgetc(f);
     
     while (c != '\n' && c != EOF && isspace(c))
	  c = fgetc(f);
     
     if (c != '\n' && c != EOF) {
	  fprintf(stderr, "Parsing failed, newline after section name was expected, got %c\n", c);

	  return -1;
     }
     return 0;
}

int section_nonzeroed(struct section *s, int named, FILE *f) {
     if (named && sectname(s->name, f))
	  return -1;

     if (!named) {
	  s->name[0] = '\0';
	  s->name[1] = '\1'; // sentinel to indicate empty name
     }

     struct line ln;
     fpos_t pos;
     char c;
     
     for (fgetpos(f, &pos); ; fgetpos(f, &pos)) {
	  c = fgetc(f);

	  if (c == EOF || c == '[') {
	       fsetpos(f, &pos);
	       break;
	  }

	  ungetc(c, f);

	  if (line(&ln, f))
	       return -1;

	  da_push(s, ln);
     }
     return 0;
}

int line(struct line *out, FILE *f) {
     memset(out, 0, INI_BUFLEN);
     return line_nonzeroed(out, f);
}

int sectname(char out[static INI_SECTNAMELEN], FILE *f) {
     memset(out, 0, INI_SECTNAMELEN);
     return sectname_nonzeroed(out, f);
}

int section(struct section *s, int named, FILE *f) {
     memset(s, 0, sizeof *s);
     return section_nonzeroed(s, named, f);
}

int ini(struct ini *i, FILE *f) {
     char c = fgetc(f);
     struct section sect = {0};
     
     while (c != EOF) {
	  ungetc(c, f);
	  if (section(&sect, c == '[', f))
	       return -1;

	  da_push(i, sect);
	  c = fgetc(f);
     }

     return 0;
}




#endif // INI_IMPLEMENTATION

#endif // INI_H_
