#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum { END=256, ARRAY, OF, INT, RETURN, IF, THEN, ELSE, WHILE, DO, VAR,
 NOT, OR, ASSIGNOP };

char* yytext = NULL; // last matched string
int yyleng = 1; // length of last matched string or 1
int yylen = 0;

char *whitespaces = "\n\t ";

/** Idee: Finite State Automata programmieren für substring-suche
 * DIGIT + ID + INTEGER + HEXDIGIT
 * Effiziente Speicherdarstellung?
 * Effiziente Navigation?
 */



//                                      1-----------------------2---------------3-------4----5--6
const unsigned char *machine_states = " eaoirtwdvn:;(),<#[]-+*{}nlrrffnehhoao=|~dsrtteereaunryren";
//                                                            2425            1641     849  5  31

const int transfer[58][25] = {
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 0 },
	
	{ 25, 26, 40, 39, 0 }, //1
	{ 27, 40, 39, 0 },
	{ 28, 29, 40, 39, 0 },
	{ 30, 31, 40, 39, 0 },
	{ 32, 40, 39, 0},
	{ 33, 40, 39, 0},
	{ 34, 40, 39, 0},
	{ 35, 40, 39, 0},
	{ 36, 40, 39, 0},
	{ 37, 40, 39, 0},
	{ 38, 0 },
	{ 0}, 
	{ 0},
	{ 0},
	{ 0},
	{ 0},
	{ 0}, 
	{ 0}, 
	{ 0},
	{ 0},
	{ 0}, 
	{ 0}, 	
	{ 40, 39, 0 },
	{ 24, 0 }, // 24
	
	{ 41, 40, 39, 0 }, //25
	{ 42, 40, 39, 0 },
	{ 43, 40, 39, 0 },
	{ 40, 39, 0},
	{ 40, 39, 0},
	{ 40, 39, 0},
	{ 44, 40, 39, 0},
	{ 45, 40, 39, 0},
	{ 46, 40, 39, 0},
	{ 47, 40, 39, 0},
	{ 40, 39, 0},
	{ 48, 40, 39, 0},
	{ 40, 39, 0 },
	{ 40, 39, 0 },
	{ 39, 0 },
	{ 40, 39, 0 }, //40
	
	{ 39, 40, 0}, //41
	{ 49, 39, 40, 0},
	{ 50, 39, 40, 0},
	{ 39, 40, 0},
	{ 51, 39, 40, 0},
	{ 52, 39, 40, 0},
	{ 53, 39, 40, 0},
	{ 39, 40, 0}, // 48
	
	{ 39, 40, 0}, //49
	{ 54, 39, 40, 0},
	{ 55, 39, 40, 0},
	{ 39, 40, 0},
	{ 56, 39, 40, 0}, // 53
	
	{ 39, 40, 0 }, // 54
	{ 57, 39, 40, 0},
	{ 39, 40, 0}, // 56
	
	{ 39, 40, 0} // 57
};

int state_machine_state = 0;

// Check all possible next states for matches
int next_state(int current_state, char *current_char) {
	
	int next_class = -1;
	
	for(int i=0;;i++) {
		int check_class = transfer[current_state][i];
		char char_class = machine_states[check_class];
	
		if(char_class > 122) {
			if(char_class == '{' &&
				(
					(*current_char >= 97 && *current_char <= 122) ||
					(*current_char >= 65 && *current_char <=90)
				)
			) {
				next_class = check_class;
				break;
			} else if(char_class == '}' &&
				(
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				next_class = check_class;
				break;
			} else if(char_class == '|' &&
				(
					(*current_char >= 97 && *current_char <= 122) ||
					(*current_char >= 65 && *current_char <=90) || 
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				next_class = check_class;
				break;
			} else if(char_class == '~' &&
				(
					(*current_char >= 97 && *current_char <= 102) ||
					(*current_char >= 65 && *current_char <=70) || 
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				next_class = check_class;
				break;
			}
			
		} else if(char_class == ' ') {
			if(*current_char == ' ' || *current_char == '\n' || *current_char == '\r') {
				next_class = check_class;
			}
			break;
		} else {
			if(*current_char == char_class) {
				next_class = check_class;
				break;
			}
		}
	}
	
	return next_class;	
	
}

unsigned long next_state_machine(char *current_char) {
	
	int next_class = next_state(state_machine_state, current_char);
	
	if(next_class == -1) {
		
		if(yylen > 0) {
			printf("%s\n", yytext);
			free(yytext);
			yytext = NULL;
			yylen = 0;		
		}
		
		next_class = next_state(0, current_char);
		
		if(next_class == -1) {
		
			printf("\nUnknown Character found: %c %d %c\n", *current_char, state_machine_state, machine_states[state_machine_state]);
			exit(1);
		} else if (next_class > 0) {
			char *tmp = realloc(yytext, yylen + 2);
			yytext = tmp;
			yytext[yylen] = *current_char;
			yytext[yylen+1] = '\0';
			yylen += 1;
		}
	} else {
		
		if(next_class == 0) {
			if(yylen > 0) {
				printf("%s\n", yytext);
				free(yytext);
				yytext = NULL;
				yylen = 0;
			}
		} else {
			char *tmp = realloc(yytext, yylen + 2);
			yytext = tmp;
			yytext[yylen] = *current_char;
			yytext[yylen+1] = '\0';
			yylen += 1;
		}
	}
	// printf("%c ", machine_states[state_machine_state]);
	state_machine_state = next_class;
	// if next_class == -1 :: FEHLER
	// if next_class == 0 :: NEUES WORT -> HASH !
	// else continue	
}

// */


/** Idee: Harte Substring Suche für definierte Worte
 * --*, end, array, of, int, return, if, then, else, while, do, var, not, or
 */

const char * keywords[] = {
	"end",
	"array",
	"of",
	"int",
	"return",
	"if",
	"then",
	"else",
	"while",
	"do",
	"var",
	"not",
	"or"
};

signed char cur_key_index[] = {
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

void reset_yytext() {
	yytext = malloc(1);
	yytext[0] = '\0';
}

void set_yytext(const char *string, size_t n) {
	yytext = malloc(n+1);
	memcpy(yytext, string, n+1);
	yytext[n+1] = '\0';
}

void reset_match_keyword() {
	//printf("\n# Reset\n");
	for(int i=0; i<13; i++) {
		cur_key_index[i] = -1;
	}
}

unsigned long match_keyword(char *current_char) {
	// BENEDIKT'S SOLUTION
	//
	// for(int i=0; i<13; i++) {
	//
	// 	if(keywords[i][cur_key_index[i] + 1] == *current_char) {
	//
	// 		cur_key_index[i] += 1;
	// 		if(cur_key_index[i] + 1 == strlen(keywords[i])) {
	// 			reset_match_keyword();
	// 			return i + 256;
	// 		}
	// 	} else {
	// 		cur_key_index[i] = -1;
	// 	} // */
	// }
	// return 0;
	int keywords_array_length = sizeof(keywords)/sizeof(keywords[0]);
	int matched_chars = 0;
	for (int i = 0; i < keywords_array_length; i++) {
		int keyword_length = strlen(keywords[i]);
		matched_chars = 0;
		for (int j = 0; j < keyword_length; j++) {
			if (current_char[j] == keywords[i][j]) {
				matched_chars++;
			}
		}
		if (matched_chars == strlen(keywords[i])) {
			set_yytext(keywords[i], strlen(keywords[i]));
			yyleng = strlen(yytext);
			return i + 256;
		}
	}
	reset_yytext();
	yyleng = 1;
	return 0;
}


/** Einfacher Zeichenabgleich für lexem char
 * ;(),:<#[]-+*
 */

const char * lexem_char = ";(),:<#[]-+*";

unsigned long match_lexem_char(char *current_char) {
	for(int i = 0; i < 12; i++) {
		if(*current_char == lexem_char[i]) {
			return *current_char;
			//unsigned long value = (unsigned long) *current_char - '0';
			//printf("# %c %ld\n", *current_char, value);
			//return value;
		}
	}
	return 0;
}

/**
 * Lexem char matcht allein mit : ?
 */

int assign_op_pos = 0;

unsigned long match_assign_op(char *current_char) {
	if(*current_char == ':') {
		assign_op_pos = 1;
	} else if(assign_op_pos && *current_char == '=') {
		assign_op_pos = 0;
		return 269;
	} else {
		assign_op_pos = 0;
	}
	return 0;
}

/**
 * Probleme: Match für jedes Zeichen eines Kommentars und andere Zeichen des Kommentars trigger uU. 
 * z.B. match_lexem_char
 */

int comment_pos = 0;

unsigned long match_comment(char *current_char) {
	if(*current_char == '-') {
		comment_pos += 1;
	} else if(comment_pos == 2 && *current_char != '\n') {
		return 1;
	} else {
		comment_pos = 0;
	}
	return 0;
}

int is_comment(char *remaining_string) {
	int comment_length = 0;
	if (*remaining_string == '-' && *(remaining_string+1) == '-') {
		char curr_char = *(remaining_string+2);
		comment_length = 2;
		while (curr_char != '\n' && curr_char != '\0') {
			curr_char = *(remaining_string + ++comment_length);
		}
		if (curr_char == '\n') {
			comment_length++;
		}
		yyleng = comment_length;
		set_yytext(remaining_string, comment_length);
		return 1;
	} else {
		reset_yytext();
		yyleng = 1;
		return 0;
	}
}

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
	  fclose(yyin);
	}
	buffer[length] = '\0';
	return buffer;
}

unsigned long apply_rule(char *remaining_string) {
	// NOTE: the parameter is the address of the current character (see for loop in main)
	
	// TODO: check if remaining_string starts with any keyword (see global variable 'keywords')
	
	// TODO: if so, then apply rule (see scanner.l)
	// TODO: set yytext to matched string and yyleng to length of yytext
	
	// TODO: else return and let the caller iterate to the next char
	// TODO: set yytext to anything recognizable (e.g. NULL) and yylengh to 1 (tells the for loop in the main function to continue with the next character)
	
	// TODO: return applied rule (see enum values) or -1 if no match
	
	 
	// printf("\n%c", *remaining_string);
	
	next_state_machine(remaining_string);
	
	/* 
=======
// unsigned long value = 0;
//
// 	value = match_keyword(remaining_string);
// 	if(value > 0) {
// 		printf("MK: %ld\n", value);
//
// 		match_lexem_char(remaining_string);
// 		match_assign_op(remaining_string);
// 		match_comment(remaining_string);
//
// 		return value;
// 	}
//
// 	value = match_lexem_char(remaining_string);
// 	if(value > 0) {
// 		printf("LC: %ld\n", value);
//
// 		match_assign_op(remaining_string);
// 		match_comment(remaining_string);
//
// 		return value;
// 	}
//
// 	value = match_assign_op(remaining_string);
// 	if(value > 0) {
// 		printf("AO: %ld\n", value);
//
// 		match_comment(remaining_string);
//
// 		return value;
// 	}
//
// 	value = match_comment(remaining_string);
// 	if(value > 0) {
// 		printf("CO: %c\n", *remaining_string);
// 		return 0;
// 	}
//
// 	return 0; // */

	/* 
	unsigned long value = 0;
	value = match_keyword(remaining_string);
	if (value == 0) {
		// TODO: call next matching function until one returns value > 0 or all rules where handled
	}
	
	value = match_assign_op(remaining_string);
	if(value > 0) {
		printf("AO: %ld\n", value);
		
		match_comment(remaining_string);
		
		return value;
	}
	
	value = match_comment(remaining_string);
	if(value > 0) {
		printf("CO: %c\n", *remaining_string);
		return 0;
	} 
	
	return value;// */
	
	return 0;
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
	int count = 0;

	for (int i = 0; i < strlen(file_content); i+=1) {
		
		/* 
		// rules should be checked in right order!
		
		// TODO: if apply_rule() works, use this line of code
		unsigned long result = apply_rule(&file_content[i]);
		
		// TODO: until apply_rule() works as accepted, result is 1 for every character
		//unsigned long result = 1;
		
		if (!result) {
			// skip comments
			if (is_comment(&file_content[i])) {
				// yytext and yyleng are set in is_comment(char*)
				continue;
			}
			// skip whitespaces
			
			if (strchr(whitespaces, file_content[i])) {
				yyleng = 1;
				set_yytext(&file_content[i], 1);
				continue;
			}
			printf("Lexical error. Unrecognised input \"%s\"\n", &file_content[i]);
			exit(1);
		}
		hash = (hash+result)*hashmult;
		// */
		
		apply_rule(&file_content[i]);
		
		
	}
	printf("%lx\n", hash);
	
	return 0;
}
