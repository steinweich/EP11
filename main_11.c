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
// const unsigned char *machine_states = " \x80""-;(),<#[]+*aoirtwdvn:-${}nlrrffnehhoao=e|~dsrtteirt\x7f""eaunlyren";
//                                                                   2627             1744      10     54  5  31
const unsigned char machine_states[61] = {
	' ',
	'-', // 1 Comment End state
	';',
	'(',
	')',
	',',
	'[',
	']',
	'<',
	'+',
	'*',
	'#',
	':', // 12 Begin Assignop ?
	'i', // 13 int / if
	'e', // 14 end / else
	'r', // 15 return
	'o', // 16 of / or
	'a', // 17 array
	'v', // 18 var
	'-', // 19 Begin comment
	'$', // 20 Begin Hex
	't', // 21 then
	'w', // 22 while
	'd', // 23 do
	'n', // 24 not
	'{',  // 25 [ID0]
	'}',  // 26 [INT]
	'|',  // 27 [ID1]
	
	'n',
	't', // 29 -int
	'f', // 30 -if
	
	'n',
	'd', // 32 -end
	'l',
	's',
	'e', // 35 -else
	
	'e', 
	't',
	'u',
	'r',
	'n', // 40 -return
	
	'=', // 41 = ASSIGNOP
	
	'f', // 42 -of
	'r', // 43 -or
	
	'r', 
	'r',
	'a',
	'y', // 47 -array
	
	'a', 
	'r', // 49 -var
	
	'h',
	'e',
	'n', // 52 -then
	
	'~', // 53 [HEX]
	
	'h',
	'i',
	'l',
	'e', // 57 -while
	
	'o', // 58 -do
	
	'o',
	't' // 60 -not
};

// unsigned long transfers[61];

// Welche Hash-Funktion ausgeführt werden soll pro state der Maschine (index des arrays = index von machine_states)
// -1 = Ungueltig
// 0 = kein Hash
// 1 = hash() [ID]
// 2 = strtoul(yytext, NULL, 10) ^ 0x8000; [INT]
// 3 = strtoul(yytext+1, NULL, 16) ^ 0x4000; [HEX]
// >3 = genau jener wert
const int hash_function[61] = {
	-1,

	0, //COMMENT --.* (ehemals 41)
	';',
	'(',
	')',
	',',
	'[',
	']',
	'<',
	'+',
	'*',
	'#',
	':',
	1,
	1,
	1,
	1,
	1,
	1,
	'-',
	-1,
	1,
	1,
	1,
	1,
	1, // [ID0]
	2, // [INT] 26
	
	1, // [ID1] 27
	1,
	256 + 3, //int
	256 + 5, //if
	1,
	256 + 0, //end
	1,
	1,
	256+7, //else
	1,
	1,
	1,
	1,	
	256 + 4, //return
	256 + 13, //ASSIGNOP :=
	256 + 2, //of
	256 + 12, //or
	1,
	1,
	1,
	256 + 1, //array
	1,
	256+10, //var	
	1,
	1,
	256 + 6, //then
	3, //[HEX]
	1,
	1,
	1,
	256 + 8, //while
	256 + 9, //do
	1,
	256+11 //not
};			

// Von welchen Zustaenden man in welche Zustaende darf (alle zahlen und indizes sind indizes von machine_states)
// Jeder Zustand wird angenommen und 0 ist immer Start / Ende des Wortes etc. und wird daher als letztes ueberprueft

//Changed from int to char (needs less space in memory: 1/4 -> should be kept more easily in cache)
const char transfer[61][26] = {
	// { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, -1 },
	{},
	
	{ }, // 1 Kommentar
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
	{ 41, -1 }, // : ASSIGN OP
	{ 28, 30, 27, -1 }, // int if
	{ 31, 33, 27, -1 }, // end else
	{ 36, 27, -1 }, // return
	{ 42, 43, 27, -1}, //of or
	{ 44, 27, -1}, // array
	{ 48, 27, -1}, // var
	{ 1, -1}, // - Beginn Kommentar
	{ 53, -1}, // $ Beginn Hex
	{ 50, 27, -1}, // then
	{ 54, 27, -1 }, // while
	{ 58, 27, -1}, // do
	{ 59, 27, -1 }, // not
	{ 27, -1 }, // [ID0]
	{ 26, -1 }, // [INT]
	
	{ 27, -1}, // [ID1]
	{ 29, 27, -1 }, //27
	{ 27, -1 }, // - int
	{ 27, -1 }, // - if
	{ 32, 27, -1}, // 30
	{ 27, -1}, // -end
	{ 34, 27, -1}, // 32
	{ 35, 27, -1}, // 33
	{ 27, -1}, // -else
	{ 37, 27, -1}, // 35
	{ 38, 27, -1}, // 36
	{ 39, 27, -1}, // 37
	{ 40, 27, -1}, // 38
	{ 27, -1}, // -return
	{ -1 }, // ASSIGNOP
	{ 27, -1 }, // of
	{ 27, -1 }, // or
	{ 45, 27, -1 }, //43
	{ 46, 27, -1}, //44
	{ 47, 27, -1}, //45
	{ 27, -1}, // -array
	{ 49, 27, -1}, //47
	{ 27, -1}, // -var
	{ 51, 27, -1}, // 49
	{ 52, 27, -1}, // 50
	{ 27, -1}, // -then
	{ 53, -1}, // [HEX]
	{ 55, 27, -1}, // 53
	{ 56, 27, -1}, //54
	{ 57, 27, -1}, //55
	{ 27, -1}, //-while
	{ 27, -1}, // -do
	{ 60, 27, -1}, // 58
	{ 27, -1 }, // -not
};

char state_machine_state = 0;

// Berechne den Hash wenn ein wort gefunden wurde
// Case instead of if
void new_word() {
	// transfers[state_machine_state]++;
	if(state_machine_state > 1) {
		int hashfunc = hash_function[state_machine_state];
		
		
		if(hashfunc > 0) {
			
			// unsigned long r;
			
			if(hashfunc == 1) {
				//methodone++;
				//6208419
				// printf("ID\n");
				// r = (int)hash(yytext);
				total_hash = (total_hash + (int)hash(yytext)) * hashmult;
			} else if(hashfunc == 2) {
				//methodtwo++;
				//1350000
				// printf("INT\n");
				// r = (int)strtoul(yytext, NULL, 10) ^ 0x8000;
				total_hash = (total_hash + ((int)strtoul(yytext, NULL, 10) ^ 0x8000)) * hashmult;
			} else if(hashfunc == 3) {
				//methodthree++;
				//30000
				// printf("HEX\n");
				// r = (int)strtoul(yytext+1, NULL, 16) ^ 0x4000;
				total_hash = (total_hash + ((int)strtoul(yytext+1, NULL, 16) ^ 0x4000)) * hashmult;
			} else {
				// r = hashfunc;
				total_hash = (total_hash + hashfunc) * hashmult;
				// r = hashfunc + 14;
			}
			 
			// printf(">%s< \t %lu \t (%d) %d\n", yytext, r, state_machine_state, hashfunc);
			// printf(">%s< \t %lu\n", yytext, r);
			// TOTAL
			// 100m cycles: lifting this calculation to the if-else above
			//total_hash = (total_hash + r) * hashmult;
	
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
	} else if(current_state == 1) { // Comment mode
		if(*current_char != '\n') { // Ignore the char
			return 1;
		} else {
			// new_word(); // Reset and start from scratch
			yytext[0] = '\0';
			yylen = 0;
			return 0;
		}
	} else if(current_state > 1 && current_state <= 11) {
		return -1; // Cannot close the old word and return 0 because this char needs to be dealt with
	} else { 
		if(current_state > 0) {
	
			int i=0;
			char check_class = transfer[current_state][i];
			while(check_class >= 0) {
				unsigned char char_class = machine_states[check_class];
				if(char_class > 122) {
					//100m cycles changed class ordering
					if(char_class == '|' &&
						(
							(*current_char >= 97 && *current_char <= 122) ||
							(*current_char >= 65 && *current_char <=90) || 
							(*current_char >= 48 && *current_char <= 57)
						)
					) {
						return check_class;
					}/*  else if(char_class == '{' &&
						(
							(*current_char >= 97 && *current_char <= 122) ||
							(*current_char >= 65 && *current_char <=90)
						)
					) {
						return check_class;
					} */ else if(char_class == '}' &&
						(
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
					}
				} else {
					if (*current_char == char_class) {
						return check_class;
					}
				}
				i++;
				check_class = transfer[current_state][i];
			}
		} else {
			for(int i=2; i < 27; i++) {
				unsigned char char_class = machine_states[i];
				if(char_class > 122) {
					//100m cycles changed class ordering
					if(char_class == '{' &&
						(
							(*current_char >= 97 && *current_char <= 122) ||
							(*current_char >= 65 && *current_char <=90)
						)
					) {
						return i;
					} else if(char_class == '}' &&
						(
							(*current_char >= 48 && *current_char <= 57)
						)
					) {
						return i;
					}
				} else {
					if (*current_char == char_class) {
						return i;
					}
				}
			}
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
	
	if(next_class == -1) { // Kein naechster Status
		new_word(); // Berechne Hash fuer derzeitiges wort
		
		// printf("R %d -> %d %c %c\n", state_machine_state, next_class, machine_states[next_class], *current_char);
		next_class = next_state(0, current_char); //zurueck zum start
		append_char(current_char);
		
		if(next_class == -1) {
			printf("N Lexical error. Unrecognised input \"%s\"\n", yytext); exit(1);
			exit(1);
		}
	} else if (next_class > 11) {
		append_char(current_char);
	}
	
	/* 
	if(next_class == 0) {
		transfers[state_machine_state]++;
	} // */
	
	//printf("A %d -> %d %c %c\n", state_machine_state, next_class, machine_states[next_class], *current_char);
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
	
	/*
	for(int i=0; i < 61; i++) {
		if(transfers[i] > 0) {
			printf("%d %c %d\n", i, machine_states[i], transfers[i]);
		}
	} // */
	
	printf("%lx\n", total_hash);
	
	return 0;
}
