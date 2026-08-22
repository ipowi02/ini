#ifndef INI_H_
#define INI_H_


#ifdef _WIN32
#warning "Windows-style CRLF file endings not supported yet"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "./thirdparty/da.h"

#ifndef INI_BUFLEN
#define INI_BUFLEN 256
#endif

#ifndef INI_SECTNAMELEN
#define INI_SECTNAMELEN 32
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

struct ini_line {
     char key[INI_BUFLEN/2];
     char val[INI_BUFLEN/2];
     // use strlen on them to get their actual length, INI_BUFLEN is a maximum to avoid heap allocations

     unsigned isempty : 1;
};

struct section {
     da_member(struct ini_line);

     char name[INI_SECTNAMELEN];
     unsigned isempty : 1;
};

struct ini_parser {
     da_member(struct section);
};

enum ini_result line_nonzeroed(struct ini_line *out, FILE *f);
enum ini_result sectname_nonzeroed(char out[static INI_SECTNAMELEN], FILE *f);
enum ini_result section_nonzeroed(struct section *s, int named, FILE *f);

enum ini_result line(struct ini_line *out, FILE *f);
enum ini_result sectname(char out[static INI_SECTNAMELEN], FILE *f);
enum ini_result section(struct section *s, int named, FILE *f);

enum ini_result ini_parse_nonzeroed(struct ini_parser *i, FILE *f);
enum ini_result ini_parse(struct ini_parser *i, FILE *f);

#ifdef INI_IMPLEMENTATION

enum ini_result line_nonzeroed(struct ini_line *out, FILE *f) {
     // Assumes *out is null-initialized
     unsigned char keycurr = 0;
     unsigned char valcurr = 0;
     int foundeq = 0;
     
     int c, d;
     int atstart = 1;

     c = fgetc(f);
     if (c == EOF) {
	  if (ferror(f))
	       return INI_EIO;
	  else
	       return INI_EEOF;
     }
     
     if (c == '\n') {
	  out->isempty = 1; 
	  goto end;
     }
     if (c == '[') {
	  return INI_EKEY; // unexpected section name start
     }
     
     do {
	  switch (c) {
	  case '=':
	       if (foundeq) {
		    goto kvchar;
	       }
	       
	       foundeq = 1;
	       goto outer;

	  case ';':
	  case '#':
	       
	       if (atstart)
		    out->isempty = 1;
	       
	       d = fgetc(f);
	       
	       
	       if (d == '\n' || d == EOF) {
		    goto end;
	       } else if (isspace(d)) {
		    
		    while ((d = fgetc(f)) != '\n' && d != EOF)
			 ; // skip rest of the line
		    
		    goto end;
		    
	       } else {	 
		    ungetc(d, f);
		    ungetc(c, f);
		    out->isempty = 0;
	       }

	  kvchar:
	  default:
	       if (valcurr + 1 >= INI_BUFLEN/2 || keycurr + 1 >= INI_BUFLEN/2) {
		    return INI_ELONG;
	       }
	       if (!foundeq)
		    out->key[keycurr++] = c;
	       else
		    out->val[valcurr++] = c;
	  }
	  
     outer:	 
	  c = fgetc(f);
	  atstart = 0;
     } while (c != '\n' && c != EOF);
     
end:
     if (!foundeq && !out->isempty) {
	  return INI_EPARSE; // no key-val separator found on line
     }
     return 0;
}

enum ini_result sectname_nonzeroed(char out[static INI_SECTNAMELEN], FILE *f) {
     int c, d;
     unsigned char sectcurr = 0;
     
     c = fgetc(f);

     if (c != '[') {
	  return INI_ESECT;
     }
     
     while ((c = fgetc(f)) != EOF && c != ']') {
	  if (c == '\n') {
	       return INI_ESECT;
	  }
	  
	  if (isspace(c)) {
	       if (sectcurr + 1 >= INI_SECTNAMELEN) {
		    return INI_ELONG;
	       }
	       
	       out[sectcurr++] = c;
	       
	       while ((d = fgetc(f)) != EOF) {
		    if (!isspace(d)) {
			 ungetc(d, f);
			 break;
		    }
	       }
	       continue;
	  }
	  
	  if (sectcurr + 1 >= INI_SECTNAMELEN) {
	       return INI_ELONG;
	  }
	  
	  out[sectcurr++] = c;
     }
     
     c = fgetc(f);
     
     while (c != '\n' && c != EOF && isspace(c)) // skip multiple whitespaces
	  c = fgetc(f);
     
     if (c != '\n' && c != EOF) {
	  return INI_ESECT;
     }
     return 0;
}

enum ini_result section_nonzeroed(struct section *s, int named, FILE *f) {
     if (named) {
	  int result = sectname(s->name, f);
	  if (result != INI_OK) {
	       da_free(s);
	       return result;
	  }
     }
     
     if (!named) {
	  s->name[0] = '\0';
	  s->isempty = 1;
     }

     struct ini_line ln;
     fpos_t pos;
     int c;
     
     if (fgetpos(f, &pos) != 0) {
	  da_free(s);
	  return INI_EIO;
     } 
     
     for (;;) {
	  c = fgetc(f);

	  if (c == EOF || c == '[') {
	       if (ferror(f)) {
		    da_free(s);
		    return INI_EIO;
	       }
	       
	       if (fsetpos(f, &pos) != 0) {
		    da_free(s);
		    return INI_EIO;
	       }
	       
	       break;
	  }

	  ungetc(c, f);

	  int result = line(&ln, f);
	  if (result != INI_OK) {
	       da_free(s);
	       return result;     
	  }
	  

	  da_push(s, ln);
	  if (fgetpos(f, &pos) != 0) {
	       da_free(s);
	       return INI_EIO;
	  }
     }
     return 0;
}

enum ini_result line(struct ini_line *out, FILE *f) {
     memset(out, 0, INI_BUFLEN);
     return line_nonzeroed(out, f);
}

enum ini_result sectname(char out[static INI_SECTNAMELEN], FILE *f) {
     memset(out, 0, INI_SECTNAMELEN);
     return sectname_nonzeroed(out, f);
}

enum ini_result section(struct section *s, int named, FILE *f) {
     memset(s, 0, sizeof *s);
     return section_nonzeroed(s, named, f);
}

enum ini_result ini_parse_nonzeroed(struct ini_parser *i, FILE *f) {
     int c = fgetc(f);
     if (c == EOF && ferror(f)) {
	  da_free(i);
	  return INI_EIO;
     }
     struct section sect = {0};
     
     while (c != EOF) {
	  ungetc(c, f);

	  int result = section(&sect, c == '[', f);
	  if (result != INI_OK) {
	       da_free(i);
	       
	       return result;
	  }
	  
	  da_push(i, sect);
	  
	  da_free(&sect);
	  c = fgetc(f);
     }

     return 0;
}

enum ini_result ini_parse(struct ini_parser *i, FILE *f) {
     memset(i, 0, sizeof *i);
     return ini_parse_nonzeroed(i, f);
}




#endif // INI_IMPLEMENTATION

#endif // INI_H_
