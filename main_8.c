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

const signed char machine_states[61] = {
	0,
	'-', // Comment
	'-', 
	'$',
	-4, //afAF09
	-3, //0-9
	':',
	'=',
	'n', //8 not
	'o',
	't',
	'v', //11 var
	'a',
	'r',
	'd', //14 do
	'o',
	'w', //16 while
	'h',
	'i',
	'l',
	'e',
	't', //21 then
	'h',
	'e',
	'n',
	'o', //25 or / of
	'r',
	'f',
	'a', //28 array
	'r',
	'r',
	'a',
	'y',
	'r', //33 return
	'e',
	't',
	'u',
	'r',
	'n',
	'i', //39 if / int
	'f',
	'n',
	't',
	'e', //43 else / end
	'l',
	's',
	'e', 
	'n', //47
	'd',
	-1, // 49 [ID] azAZ 49
	-2, // 50 [ID] azAZ09
	';', // 51
	'(',
	')',
	',',
	'<',
	'#',
	'[',
	']',
	'+',
	'*'
};

//int *transfers = calloc(63 * 63, sizeof *transfers);
// unsigned long transfers[61*61];

// Welche Hash-Funktion ausgeführt werden soll pro state der Maschine (index des arrays = index von machine_states)
// -1 = Ungueltig
// 0 = kein Hash
// 1 = hash()
// 2 = strtoul(yytext, NULL, 10) ^ 0x8000;
// 3 = strtoul(yytext+1, NULL, 16) ^ 0x4000;
// >3 = genau jener wert

const int hash_function[61] = {
	-1, // START
	0, //COMMENT --.* (ehemals 41)
	'-',
	-1, // $
	3,
	2,
	':',
	256 + 13, // ASSIGNOP
	1,
	1,
	256 + 11, //not
	1,
	1,
	256 + 10, //var
	1,
	256 + 9, //do
	1,
	1,
	1,
	1,
	256 + 8, // while
	1,
	1,
	1,
	256 + 6, //then 24
	1,
	256 + 12, //or 26
	256 + 2, //of 27
	1,
	1,
	1,
	1,
	256 + 1, //array 32
	1,
	1,
	1,
	1,
	1,
	256 + 4,//return 38
	1,
	256 + 5, //if 40
	1, 
	256 + 3, //int 42
	1,
	1,
	1,
	256 + 7, //else 45
	1,
	256 + 0, //end 48
	1, // [ID]
	1, // [ID]
	';',
	'(',
	')',
	',',
	'<',
	'#',
	'[',
	']',
	'+',
	'*', // 60
};			

// Von welchen Zustaenden man in welche Zustaende darf (alle zahlen und indizes sind indizes von machine_states)
// Jeder Zustand wird angenommen und 0 ist immer Start / Ende des Wortes etc. und wird daher als letztes ueberprueft

//Changed from int to char (needs less space in memory: 1/4 -> should be kept more easily in cache)
const char transfer[61][26] = {
	{ 8, 11, 14, 16, 21, 25, 28, 33, 39, 43, 
		2, 6, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
		5, 49, 3, -1 },
		
	{ }, //1 Kommentar
	{ 1, -1 }, // 2 -
	{ 4, -1 }, // 3 $
	{ 4, -1 }, // 4 Hash
	{ 5, -1 }, // 5 [INT]
	{ 7, -1 }, // 6 :
	{ -1}, // 7 = ASSIGNOP
	{ 9, 50, -1}, // 
	{ 10, 50, -1}, // 
	{ 50, -1}, // 10 not 
	{ 12, 50, -1}, //
	{ 13, 50, -1}, //
	{ 50, -1 }, // 13 var
	{ 15, 50, -1 }, //
	{ 50, -1 }, // 15 do
	{ 17, 50, -1}, //
	{ 18, 50, -1}, // 
	{ 19, 50, -1}, //
	{ 20, 50, -1}, //
	{ 50, -1}, // 20 while
	{ 22, 50, -1}, //
	{ 23, 50, -1 }, // 
	{ 24, 50, -1}, // 
	{ 50, -1 }, // 24 then
	{ 26, 27, 50, -1 }, //
	{ 50, -1 }, // 26 or
	{ 50, -1 }, // 27 of
	{ 29, 50, -1 }, //
	{ 30, 50, -1 }, //
	{ 31, 50, -1}, //
	{ 32, 50, -1}, //
	{ 50, -1}, // 32 array
	{ 34, 50, -1}, //
	{ 35, 50, -1}, //
	{ 36, 50, -1}, // 
	{ 37, 50, -1}, //
	{ 38, 50, -1}, //
	{ 50, -1}, // 38 return
	{ 40, 41, 50, -1}, //
	{ 50, -1 }, // 40 if
	{ 42, 50, -1 }, //
	{ 50, -1 }, // 42 int
	{ 44, 47, 50, -1 }, //
	{ 45, 50, -1}, //
	{ 46, 50, -1}, //
	{ 50, -1}, // 46 else
	{ 48, 50, -1}, //
	{ 50, -1}, // 48 end
	{ 50, -1}, // 49 [azAz]
	{ 50, -1}, // 50 [azAZ09]
	{ -1 }, 
	{ -1 },
	{ -1 },
	{ -1 },
	{ -1 },
	{ -1 },
	{ -1 },
	{ -1 },
	{ -1 },
	{ -1 },
};

char state_machine_state = 0;

// Berechne den Hash wenn ein wort gefunden wurde
// Case instead of if
void new_word() {
	
	if(state_machine_state > 1) {
		int hashfunc = hash_function[state_machine_state];
		
		if(hashfunc > 0) {
			unsigned long r = 0;
			
			if(hashfunc == 1) {
				//methodone++;
				//6208419
				// printf("ID\n");
				r = (int)hash(yytext);
			} else if(hashfunc == 2) {
				//methodtwo++;
				//1350000
				// printf("INT\n");
				r = (int)strtoul(yytext, NULL, 10) ^ 0x8000;
			} else if(hashfunc == 3) {
				//methodthree++;
				//30000
				// printf("HEX\n");
				r = (int)strtoul(yytext+1, NULL, 16) ^ 0x4000;
			} else {
				r = hashfunc;
				// r = hashfunc + 14;
			}
			 
			// printf(">%s< \t %lu \t (%d) %d\n", yytext, r, state_machine_state, hashfunc);
			// printf(">%s< \t %lu\n", yytext, r);
			// TOTAL
			total_hash = (total_hash + r) * hashmult;
	
		} else if (hashfunc == -1) {
			printf("H Lexical error. Unrecognised input \"%s\"\n", yytext);
			exit(1);
		}
	
		yytext[0] = '\0';
		yylen = 0;
	}
}

// Suche anhand des derzeitigen Characters und des gegebenen zustands den naechsten zustand
// -1 wenn kein passender zustand gefunden (bzw. wort zu ende)
int next_state(int current_state, unsigned char *current_char) {
	
	if(current_state != 1 && (*current_char == ' ' || *current_char == '\n' || *current_char == '\t')) { // Not comment mode + whitespace -> deal
		new_word();
		return 0;
	} else if(current_state == 1) { // Kommentar
		if(*current_char != '\n') { // Ignore the char
			return 1;
		} else {
			// new_word(); // Reset and start from scratch
			yytext[0] = '\0';
			yylen = 0;
			return 0;
		}
	} else if(current_state > 50) { // ; ( ) ...
		return -1; // Cannot close the old word and return 0 because this char needs to be dealt with
	} else { 
	
		int i=0;
		int check_class = transfer[current_state][i];
		while(check_class > 0) {
			signed char char_class = machine_states[check_class];
			if(char_class < 0) {
				//100m cycles changed class ordering
				if(char_class == -2 &&
					(
						(*current_char >= 97 && *current_char <= 122) ||
						(*current_char >= 65 && *current_char <=90) || 
						(*current_char >= 48 && *current_char <= 57)
					)
				) {
					return check_class;
				} else if(char_class == -1 &&
					(
						(*current_char >= 97 && *current_char <= 122) ||
						(*current_char >= 65 && *current_char <=90)
					)
				) {
					return check_class;
				} else if(char_class == -3 &&
					(
						(*current_char >= 48 && *current_char <= 57)
					)
				) {
					return check_class;
				} else if(char_class == -4 &&
					(
						(*current_char >= 97 && *current_char <= 102) ||
						(*current_char >= 65 && *current_char <=70) || 
						(*current_char >= 48 && *current_char <= 57)
					)
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
	}
	
	return -1;
}

void append_char(unsigned char *c) {
	//220m cycles let yylen start with 2 and yylen >0 be yylen >2
	if(yylen+2 > maxbuf) {
		unsigned char *tmp = realloc(yytext, yylen+2);
		yytext = tmp;
		maxbuf = yylen+2;
	}
	
	yytext[yylen] = *c;
	yylen += 1;	
	yytext[yylen] = '\0';
}

// Wechsle in den naechsten Status - oder brich bei ungueltigem character ab
unsigned long next_state_machine(unsigned char *current_char) {
	
	int next_class = next_state(state_machine_state, current_char);
	/* if(next_class > -1) {
		transfers[state_machine_state * 61 + next_class]++;
	} // */
	
	if(next_class == -1) { // Kein naechster Status
		new_word(); // Berechne Hash fuer derzeitiges wort
		
		
		
		// printf("R %d -> %d %c %c\n", state_machine_state, next_class, machine_states[next_class], *current_char);
		next_class = next_state(0, current_char); //zurueck zum start
		append_char(current_char);
		
		if(next_class == -1) {
			printf("N Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
			exit(1);
		}
		// transfers[state_machine_state * 61 + next_class]++;
	} else if (next_class > 1 && next_class < 51) {
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
	//6 m cycles 
	//int filelen = strlen(file_content);

	unsigned char *c;
	
	for(c=file_content; *c; c++) {
	// for (int i = 0; i < filelen; i+=1) {
		next_state_machine(c); // &file_content[i]
	}
	
	new_word();
	
	printf("%lx\n", total_hash);
	/* 
	for(int i=0; i < 61; i++) {
		for(int j=0; j < 61; j++) {
			if(transfers[i * 61 + j] >= 1000000) {
				printf("%d -> %d %fu\n", i, j, (float)transfers[i * 61 + j] / 1000000);
			}
		}
	} // */
	
	return 0;
}
