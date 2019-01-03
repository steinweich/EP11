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
	}
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
// \x80 = Whitespace ( \n\t)
// \x7f = . (ausgenommen \n)
//                                      1     -------------------------2----------------3---------     4----5--6
const unsigned char *machine_states = " \x80""eaoirtwdvn:;(),<#[]-+*${}nlrrffnehhoao=-|~dsrtteirt\x7f""eaunlyren";
//                                                                   2627             1744      10     54  5  31

// Welche Hash-Funktion ausgeführt werden soll pro state der Maschine (index des arrays = index von machine_states)
// -1 = Ungueltig
// 0 = kein Hash
// 1 = hash()
// 2 = strtoul(yytext, NULL, 10) ^ 0x8000;
// 3 = strtoul(yytext+1, NULL, 16) ^ 0x4000;
// >3 = genau jener wert
const int hash_function[63] = {
	-1,
	
	0, // Whitespace
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
const int transfer[63][27] = {
	{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 1, -1 },
	
	{ 1, -1 }, //1
	{ 27, 28, 42, -1 }, 
	{ 29, 42, -1 },
	{ 30, 31, 42, -1 },
	{ 32, 33, 42, -1 },
	{ 34, 42, -1},
	{ 35, 42, -1},
	{ 36, 42, -1},
	{ 37, 42, -1},
	{ 38, 42, -1},
	{ 39, 42, -1},
	{ 40, -1 },
	{ -1}, 
	{ -1},
	{ -1},
	{ -1},
	{ -1},
	{ -1}, 
	{ -1}, 
	{ -1},
	{ 41, -1},
	{ -1}, 
	{ -1},
	{ 43, -1 },
	{ 42, -1 },
	{ 26, -1 }, // 26
	
	{ 44, 42, -1 }, //27
	{ 45, 42, -1 },
	{ 46, 42, -1 },
	{ 42, -1},
	{ 42, -1},
	{ 42, -1},
	{ 47, 42, -1},
	{ 48, 42, -1},
	{ 49, 42, -1},
	{ 50, 42, -1},
	{ 42, -1},
	{ 51, 42, -1},
	{ 52, 42, -1},
	{ -1 },
	{ 53, -1 },
	{ 42, -1 },
	{ 43, -1 }, //43
	
	{ 42, -1}, //44
	{ 54, 42, -1},
	{ 55, 42, -1},
	{ 42, -1},
	{ 56, 42, -1},
	{ 57, 42, -1},
	{ 58, 42, -1},
	{ 42, -1}, 
	{ 42, -1},
	{ 53, -1}, // 53
	
	{ 42, -1}, //54
	{ 59, 42, -1},
	{ 60, 42, -1},
	{ 42, -1},
	{ 61, 42, -1}, // 58
	
	{ 42, -1 }, // 59
	{ 62, 42, -1},
	{ 42, -1}, // 61
	
	{ 42, -1} // 62
};

int state_machine_state = 0;

// Suche anhand des derzeitigen Characters und des gegebenen zustands den naechsten zustand
// -1 wenn kein passender zustand gefunden (bzw. wort zu ende)
int next_state(int current_state, unsigned char *current_char) {
	
	int i=0;
	int check_class = transfer[current_state][i];
	while(check_class >= 0) {
		unsigned char char_class = machine_states[check_class];
		if(char_class > 122) {
			if(char_class == '{' &&
				(
					(*current_char >= 97 && *current_char <= 122) ||
					(*current_char >= 65 && *current_char <=90)
				)
			) {
				return check_class;
			} else if(char_class == '}' &&
				(
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				return check_class;
			} else if(char_class == '|' &&
				(
					(*current_char >= 97 && *current_char <= 122) ||
					(*current_char >= 65 && *current_char <=90) || 
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				return check_class;
			} else if(char_class == '~' &&
				(
					(*current_char >= 97 && *current_char <= 102) ||
					(*current_char >= 65 && *current_char <=70) || 
					(*current_char >= 48 && *current_char <= 57)
				)
			) {
				return check_class;
			} else if(char_class == '\x7F' &&
				(*current_char != '\n')
			) {
				return check_class;
			} else if(char_class == (unsigned char)'\x80' &&
				(*current_char == ' ' || *current_char == '\t' || *current_char == '\n')
			) {
				return check_class;
			}
		} else {
			if (*current_char == char_class) {
				return check_class;
			}
		}
		i++;
		check_class = transfer[current_state][i];
	}
	
	return -1;
}

// Berechne den Hash wenn ein wort gefunden wurde
// Case instead of if
void new_word() {
	
	if(yylen > 0) {
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
			 
			// printf(">%s< \t %lu\n", yytext, r);
			// TOTAL
			total_hash = (total_hash + r) * hashmult;
	
		} else if(hashfunc == -1) {
			printf("H Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
			exit(1);
		}
	
		yytext[0] = '\0';
		yylen = 0;
	}
}

void append_char(unsigned char *c) {
	
	if(yylen+2 > maxbuf) {
		unsigned char *tmp = realloc(yytext, yylen + 2);
		yytext = tmp;
		maxbuf = yylen + 2;
	}
	
	yytext[yylen] = *c;
	yylen += 1;	
	yytext[yylen] = '\0';
}

// Wechsle in den naechsten Status - oder brich bei ungueltigem character ab
unsigned long next_state_machine(unsigned char *current_char) {
	
	int next_class = next_state(state_machine_state, current_char);
	
	if(next_class == -1) { // Kein naechster Status
		new_word(); // Berechne Hash fuer derzeitiges wort
		
		next_class = next_state(0, current_char); //zurueck zum start
		append_char(current_char);
		
		if(next_class == -1) {
			printf("N Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
			exit(1);
		}
	} else {
		append_char(current_char);
	}
	// printf("A %d -> %d %c %c\n", state_machine_state, next_class, machine_states[next_class], *current_char);
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
	
	//for(int i=0; i < strlen(machine_states); i++)
	//	printf("%c %d\n", machine_states[i], machine_states[i]);
	
	unsigned char *file_content = read_file(argv[1]);

	int filelen = strlen(file_content);

	for (int i = 0; i < filelen; i+=1) {
		next_state_machine(&file_content[i]);
	}
	
	new_word();
	
	printf("%lx\n", total_hash);
	
	return 0;
}
