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

// fuer 3 characters: r = char1 * hashmult^3 + char2 * hashmult^2 + char3 * hashmult
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
//                                      1------------------------2----------------3---------     4----5--6
const unsigned char *machine_states = " eaoirtwdvn:;(),<#[]-+*${}nlrrffnehhoao=-|~dsrtteirt\x7f""eaunlyren";
//                                                             2526             1742      10     52  5  31

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
	
	{ 26, 27, 41, -1 }, //1
	{ 28, 41, -1 },
	{ 29, 30, 41, -1 },
	{ 31, 32, 41, -1 },
	{ 33, 41, -1},
	{ 34, 41, -1},
	{ 35, 41, -1},
	{ 36, 41, -1},
	{ 37, 41, -1},
	{ 38, 41, -1},
	{ 39, -1 },
	{ -1}, 
	{ -1},
	{ -1},
	{ -1},
	{ -1},
	{ -1}, 
	{ -1}, 
	{ -1},
	{ 40, -1},
	{ -1}, 
	{ -1},
	{ 42, -1 },
	{ 41, -1 },
	{ 25, -1 }, // 25
	
	{ 43, 41, -1 }, //26
	{ 44, 41, -1 },
	{ 45, 41, -1 },
	{ 41, -1},
	{ 41, -1},
	{ 41, -1},
	{ 46, 41, -1},
	{ 47, 41, -1},
	{ 48, 41, -1},
	{ 49, 41, -1},
	{ 41, -1},
	{ 50, 41, -1},
	{ 51, 41, -1},
	{ -1 },
	{ 52, -1 },
	{ 41, -1 },
	{ 42, -1 }, //42
	
	{ 41, -1}, //43
	{ 53, 41, -1},
	{ 54, 41, -1},
	{ 41, -1},
	{ 55, 41, -1},
	{ 56, 41, -1},
	{ 57, 41, -1},
	{ 41, -1}, 
	{ 41, -1},
	{ 52, -1}, // 52
	
	{ 41, -1}, //53
	{ 58, 41, -1},
	{ 59, 41, -1},
	{ 41, -1},
	{ 60, 41, -1}, // 57
	
	{ 41, -1 }, // 58
	{ 61, 41, -1},
	{ 41, -1}, // 60
	
	{ 41, -1} // 61
};

int state_machine_state = 0;

// Suche anhand des derzeitigen Characters und des gegebenen zustands den naechsten zustand
// -1 wenn kein passender zustand gefunden (bzw. wort zu ende)
int next_state(int current_state, char *current_char) {
	
	int next_class = -1;
	
	int i=0;
	int check_class = transfer[current_state][i];
	while(check_class >= 0) {
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
			} else if(char_class == '\x7F' &&
				(*current_char != '\n')
			) {
				next_class = check_class;
				break;
			}
		} else {
			if(char_class == ' ' &&
				(
					*current_char == ' ' || *current_char == '\t' || *current_char == '\n'
				)
			) {
				next_class = check_class;
				break;
			} else if (*current_char == char_class) {
				next_class = check_class;
				break;
			}
		}
		i++;
		check_class = transfer[current_state][i];
	}
	
	return next_class;	
	
}

// Berechne den Hash wenn ein wort gefunden wurde
// Case instead of if
void new_word() {
	
	int hashfunc = hash_function[state_machine_state];
	if(hashfunc > 0) {
		unsigned long r = 0;
		
		switch (hashfunc) {
			case 1:
				r = (int)hash(yytext);
				break;
			case 2:
				r = (int)strtoul(yytext, NULL, 10) ^ 0x8000;
				break;
			case 3:
				r = (int)strtoul(yytext+1, NULL, 16) ^ 0x4000;
				break;
			default:
				r = hashfunc;
		}
		
		// TOTAL
		total_hash = (total_hash + r) * hashmult;

	} else if(hashfunc == -1) {
		printf("Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
		exit(1);
	}

	yytext[0] = '\0';
	yylen = 0;
}

void append_char(char *c) {
	
	if(yylen+2 > maxbuf) {
		char *tmp = realloc(yytext, yylen + 2);
		yytext = tmp;
		maxbuf = yylen + 2;
	}
	
	yytext[yylen] = *c;
	yylen += 1;	
	yytext[yylen] = '\0';
}

// Wechsle in den naechsten Status - oder brich bei ungueltigem character ab
unsigned long next_state_machine(char *current_char) {
	
	int next_class = next_state(state_machine_state, current_char);
	
	if(next_class <= 0) { // Kein naechster status gefunden - wahrscheinlich neues wort
		
		if(yylen > 0) {
			new_word();
		}
		
		if(next_class == -1) {
			next_class = next_state(0, current_char); // Probiere es vom start-zustand nochmals
		
			// Verlaengere Wort (bzw. fange neu an)		
			append_char(current_char);
		
			if(next_class == -1) { // Ungueltiger character

				printf("Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
				exit(1);
			}
		}
	} else { // Verlaengere Wort
		append_char(current_char);
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
