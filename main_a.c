#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* yytext = NULL; // last matched string
int yylen = 0;
int maxbuf = 0;

/*****************************************/
/***** COPY+PASTE FROM LEX PROGRAM *******/
/*****************************************/
#define hashmult 13493690561280548289ULL

unsigned long total_hash = 0;

unsigned long hash(char *s)
{
	unsigned long r=0;
	char *p;
	for (p=s; *p; p++) {
		
		r = (r+*p)*hashmult;
		// printf("%c %lu\n", *p, r);
	}
	
	//printf("ID %s!\n", s);
	return r;
}

/** Idee: Finite State Automata programmieren für substring-suche
 * DIGIT + ID + INTEGER + HEXDIGIT
 * Effiziente Speicherdarstellung?
 * Effiziente Navigation?
 */

// Jeder char is ein State und der char selbst symbolisiert den moeglichen uebergang in diesen state
// { = a-zA-Z
// } = 0-9
// | = a-zA-Z0-9
// ~ = afAf0-9
//   = Whitespace (\n\t ) bzw. Start-Zustand
// ! = . (ausgenommen \n)
//                                      1------------------------2----------------3---------4----5--6
const unsigned char *machine_states = " eaoirtwdvn:;(),<#[]-+*${}nlrrffnehhoao=-|~dsrtteirt!eaunlyren";
//                                                             2526             1742      1052  5  31

// Welche Hash-Funktion ausgeführt werden soll pro state der Maschine (index des arrays = index von machine_states)
// -1 = Ungueltig
// 0 = kein Hash
// 1 = hash()
// 2 = strtoul(yytext, NULL, 10) ^ 0x8000;
// 3 = strtoul(yytext+1, NULL, 16) ^ 0x4000;
// >3 = genau jener wert
const int hash_function[62] = {
	0,
	
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	':',
	';',
	'(',
	')',
	',',
	'<',
	'#',
	'[',
	']',
	'-',
	'+',
	'*',
	-1,
	1,
	2,
	
	1,
	1,
	1,
	256 + 12, //or
	256 + 2, //of
	256 + 5, //if
	1,
	1,
	1,
	1,
	256 + 9, //do
	1,
	1,
	256 + 13, //ASSIGNOP :=
	0, //COMMENT --.*
	1,
	3,
	
	256 + 0, //end
	1,
	1,
	256 + 3, //int
	1,
	1,
	1,
	256+10, //var
	256+11, //not
	0, //COMMENT --.*
	
	256+7, //else
	1,
	1,
	256 + 6, //then
	1,
	
	256 + 1, //array
	1,
	256 + 8, //while
	
	256 + 4 //return
};			

// Von welchen Zustaenden man in welche Zustaende darf (alle zahlen und indizes sind indizes von machine_states)
// Jeder Zustand wird angenommen und 0 ist immer Start / Ende des Wortes etc. und wird daher als letztes ueberprueft
const int transfer[62][26] = {
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0 },
	
	{ 26, 27, 41, 0 }, //1
	{ 28, 41, 0 },
	{ 29, 30, 41, 0 },
	{ 31, 32, 41, 0 },
	{ 33, 41, 0},
	{ 34, 41, 0},
	{ 35, 41, 0},
	{ 36, 41, 0},
	{ 37, 41, 0},
	{ 38, 41, 0},
	{ 39, 0 },
	{ 0}, 
	{ 0},
	{ 0},
	{ 0},
	{ 0},
	{ 0}, 
	{ 0}, 
	{ 0},
	{ 40, 0},
	{ 0}, 
	{ 0},
	{ 42, 0 },
	{ 41, 0 },
	{ 25, 0 }, // 25
	
	{ 43, 41, 0 }, //26
	{ 44, 41, 0 },
	{ 45, 41, 0 },
	{ 41, 0},
	{ 41, 0},
	{ 41, 0},
	{ 46, 41, 0},
	{ 47, 41, 0},
	{ 48, 41, 0},
	{ 49, 41, 0},
	{ 41, 0},
	{ 50, 41, 0},
	{ 51, 41, 0 },
	{ 0 },
	{ 52, 0 },
	{ 41, 0 },
	{ 42, 0 }, //42
	
	{ 41, 0}, //43
	{ 53, 41, 0},
	{ 54, 41, 0},
	{ 41, 0},
	{ 55, 41, 0},
	{ 56, 41, 0},
	{ 57, 41, 0},
	{ 41, 0}, 
	{ 41, 0},
	{ 52, 0}, // 52
	
	{ 41, 0}, //53
	{ 58, 41, 0},
	{ 59, 41, 0},
	{ 41, 0},
	{ 60, 41, 0}, // 57
	
	{ 41, 0 }, // 58
	{ 61, 41, 0},
	{ 41, 0}, // 60
	
	{ 41, 0} // 61
};

int state_machine_state = 0;

// Suche anhand des derzeitigen Characters und des gegebenen zustands den naechsten zustand
// -1 wenn kein passender zustand gefunden (bzw. wort zu ende)

int next_state(int current_state, char *current_char) {
	
	int next_class = -1;
	
			
	for(int i=0;;i++) {
		int check_class = transfer[current_state][i];
		char char_class = machine_states[check_class];
	
		
		if(char_class > 122) {
			if(char_class == '|' &&
				(
					(*current_char >= 97 && *current_char <= 122) ||
					(*current_char >= 65 && *current_char <=90) || 
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				next_class = check_class;
				break;
			} else if(char_class == '{' &&
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
			if(*current_char == ' ' || *current_char == '\n' || *current_char == '\t') {
				next_class = check_class;
			}
			break;
		} else if(char_class == '!') {
			if(*current_char != '\n') {
				next_class = check_class;
				break;
			}
		} else {
			if(*current_char == char_class) {
				next_class = check_class;
				break;
			}
		}
	}
	
	return next_class;	
	
}

// Berechne den Hash wenn ein wort gefunden wurde
void new_word() {
	
	int hashfunc = hash_function[state_machine_state];
	if(hashfunc > 0) {
		unsigned long r = 0;
		
		if(hashfunc == 1) {
			// printf("ID\n");
			r = (int)hash(yytext);
		} else if(hashfunc == 2) {
			// printf("INT\n");
			r = (int)strtoul(yytext, NULL, 10) ^ 0x8000;
		} else if(hashfunc == 3) {
			// printf("HEX\n");
			r = (int)strtoul(yytext+1, NULL, 16) ^ 0x4000;
		} else if(hashfunc > 0) {
			r = hashfunc;
		} else {
			printf("Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
			exit(1);
		}
		// printf(">%s< \t %lu\n", yytext, r);
		
		// TOTAL
		total_hash = (total_hash + r) * hashmult;
		
		
	
	}
	
	// free(yytext);
	// yytext = NULL;
	yytext[0] = '\0';
	yylen = 0;
}

void append_char(char *c) {
	// printf("%d %d %c\n", yylen+2, maxbuf, *c);
	
	if(yylen+2 > maxbuf) {
		// printf("Realloc\n");
		char *tmp = realloc(yytext, yylen + 2);
		yytext = tmp;
		maxbuf = yylen + 2;
	}
	
	// printf("%d %d %c\n", yylen+2, maxbuf, *c);
	
	yytext[yylen] = *c;
	yylen += 1;	
	yytext[yylen] = '\0';
}
// Wechsle in den naechsten Status - oder brich bei ungueltigem character ab
unsigned long next_state_machine(char *current_char) {
	
	int next_class = next_state(state_machine_state, current_char);
	
	if(next_class == -1) { // Kein naechster status gefunden - wahrscheinlich neues wort

		if(yylen > 0) {
			new_word();
		}
		
		next_class = next_state(0, current_char); // Probiere es vom start-zustand nochmals
		
		// Verlaengere Wort (bzw. fange neu an)		
		append_char(current_char);
		
		if(next_class == -1) { // Ungueltiger character

			printf("Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
			exit(1);
		}
	} else {

		if(next_class == 0) { // Neues Wort wurde angefangen
			if(yylen > 0) {
				new_word();
			}
		} else { // Wort wird verlaengert
			append_char(current_char);
		}
	}
	state_machine_state = next_class;
}

// */

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

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Usage: %s <INPUT-FILE>\n", argv[0]);
		exit(1);
	}
	
	char *file_content = read_file(argv[1]);

	int filelen = strlen(file_content);

	for (int i = 0; i < filelen; i+=1) {
		next_state_machine(&file_content[i]);
	}
	
	if(yylen > 0) {
		new_word();
	}
	
	printf("%lx\n", total_hash);
	
	return 0;
}
