#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum { END=256, ARRAY, OF, INT, RETURN, IF, THEN, ELSE, WHILE, DO, VAR,
 NOT, OR, ASSIGNOP };

int eof = 0; // indicates end of file

char* yytext; // last matched string
int yyleng = 1; // length of last matched string or 1

/*
	Array of array of strings. Every array entry contains
	an array of strings with same character length
*/
// TODO: instantiate the correct way!
// char *keywords[] = {
// 	{"or", "do", "if", "of"},
// 	{"end", "int", "var", "not"},
// 	{"then", "else"},
// 	{"array", "while"},
// 	{"return"}
// };

/*
	Calculates hash of string
*/
unsigned long hash(char *s);

/*****************************************/
/***** COPY+PASTE FROM LEX PROGRAM *******/
/*****************************************/
#define hashmult 13493690561280548289ULL

unsigned long hash(char *s)
{
  unsigned long r=0;
  char *p;
  for (p=s; *p; p++)
    r = (r+*p)*hashmult;
  return r;
}
/*****************************************/
/*****************************************/
/*****************************************/

char *read_file(char *filename) {
	char *buffer = 0;
	long length;
	FILE *yyin = fopen(filename, "r");

	if (yyin) {
	  fseek(yyin, 0, SEEK_END);
	  length = ftell(yyin);
	  fseek(yyin, 0, SEEK_SET);
	  buffer = malloc(length + 1);
	  if (buffer) {
	    fread(buffer, 1, length, yyin);
	  }
	  // do not close yet ...
	  // fclose(yyin);
	}

	return buffer;
}

int apply_rule(char *remaining_string) {
	// NOTE: the parameter is the address of the current character (see for loop in main)
	
	// TODO: check if remaining_string starts with any keyword (see global variable 'keywords')
	
	// TODO: if so, then apply rule (see scanner.l)
	// TODO: set yytext to matched string and yyleng to length of yytext
	
	// TODO: else return and let the caller iterate to the next char
	// TODO: set yytext to anything recognizable (e.g. NULL) and yylengh to 1 (tells the for loop in the main function to continue with the next character)
	
	// TODO: return applied rule (see enum values) or -1 if no match
	return -1;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Usage: %s <INPUT-FILE>\n", argv[0]);
		exit(1);
	}
	
	char *file_content = read_file(argv[1]);
	
	unsigned long hash = 0;
	
	// iterate over the whole file content.
	// yyleng is modified in apply_rules(). yyleng is the length of the string
	// matched in the last iteration or 1 if no string matched in the last
	// iteration
	for (int i = 0; i < strlen(file_content); i+=yyleng) {
		int result = apply_rule(&file_content[i]/* TODO: should provide substring from index i to end of file_content */);
		hash = (hash+result)*hashmult;
	}
	printf("%lx\n", hash);
	
	return 0;
}