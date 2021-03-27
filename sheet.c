/***********************************************

 * PROJEKT 1 - PRACE S TEXTEM
 * PREDMET IZP 2020/21
 * AUTOR: Lucie Svobodova, VUT login: xsvobo1x

 ***********************************************/

/***
 * Ocekavane spousteni programu:
 * - pro upravu tabulky : ./sheet [-d DELIM] [Prikazy pro upravu tabulky]
 * - pro zpracovani dat: ./sheet [-d DELIM] [Prikaz pro selekci radku] [Prikaz pro zpracovani dat]
 *   poznamky - prikazu pro upravu dat muze byt zadano vice
 *            - prikaz pro selekci radku muze byt zadan maximalne jeden
 *            - prikaz pro zpracovani dat muze byt zadan maximalne jeden
 * - program se spusti i pri zadani vice nepovolenych prikazu nebo jejich
 *   kombinaci, v tom pripade ale nemusi byt provedeny vsechny operace spravne. 
 *   Je proto nutne spoustet pouze povolene kombinace prikazu.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define MAX_CHAR_LINE 10242     // maximalni povolena delka radku (10 KiB + '\n' a '\0')
#define MAX_CHAR_CELL 101       // maximalni povolena delka bunky nebo argumentu (100 znaku + '\0')

enum return_codes {LAST_ROW = -3, E_FAILURE = -2, NOT_SPECIFIED = -1, NOT_IN_INTERVAL = -1, E_SUCCESS = 0, E_FILE_ERR = 1};

// Funkce do retezce delim ulozi vstupni oddelovace - vystupni oddelovac je pak delim[0].
bool get_delim(int argc, char **argv, char **delim)
{
	for(int i = 1; i < argc; i++)
    {
		if(strcmp(argv[i], "-d") == 0)
        {
			if (i+1 < argc) {
                *delim = argv[i+1];
                if (strlen(*delim) >= MAX_CHAR_CELL)
                    return false;
			    else
                    return true;
            } else
                return false;
		}
	}
    // pokud neni zadan argument [-d DELIM], oddelovacem je " "
	*delim = " ";
    return true;
}

// Zjisti, zda je znak oddelovacem (true) nebo neni (false);Funkce vraci true, pokud je znak oddelovacem, false pokud neni.
bool is_delim(char c, char *delim)
{
	for (int i = 0; delim[i] != '\0'; i++)
    {
		if (c == delim[i])
			return true;
	}
	return false;
}

// Funkce vraci delku retezce str.
int get_length(char *str)
{
	int length = 0;
	for (int i = 0; str[i] != '\0'; i++)
    {
		length++;
	}
	return length;
}

// Fce odstrani z retezce row_str znak '\n'. Vrati E_FILE_ERR, pokud v row_str znak '\n' nenalezne -
// - radek je prilis dlouhy, nebo E_SUCCESS pri uspechu.
int delete_n(char *row_str, int row_len)
{
    int return_value = E_FILE_ERR;
	for(int i = 0; i < row_len; i++)
    {
        if ((row_str[i] == '\n') || (row_str[i] == '\r'))
        {
            if (row_str[i] == '\n')
                return_value = E_SUCCESS;
            row_str[i] = '\0';
        }
    }
    return return_value;
}

// Fce vraci true, pokud je zadany retezec prirozene ('n') / cele ('z') cislo, jinak vraci false.
bool is_number(char *str, char option)      // option: 'z' - cele, 'n' - prirozene cislo
{
	long int num;
    char *ptr;
    num = strtol(str, &ptr, 10);
    
    // pokud num neni cislo
    if ((ptr == str) || (*ptr != '\0'))
        return false;

    if (option == 'z')
    {
        if (num <= INT_MAX)
            return true;
    }
    if (option == 'n')
    {
        if ((num <= INT_MAX) && (num > 0))
            return true;
    }
    return false;
}

// Fce vraci true, pokud je zadany retezec desetinne cislo, jinak vraci false.
bool is_real_number(char *str)
{
    if (!is_number(str, 'z'))
    {
	    int dots = 0;   // pocet desetinnych oddelovacu
	    int i = 0;
	    if (str[0] == '-' || str[0] == '+')
		    i++;
	    while(str[i] != '\0')
        {
		    if ((str[i] == '.') || (str[i] == ','))
            {
			    dots++;
			    i++;
			    continue;
		    }
		    if (!isdigit(str[i]))
			    return false;
		    i++;
	    }
	    if (dots == 1)
		    return true;
	    return false;
    } else
        return true;
}

// Fce vraci retezec prevedeny na prirozene / cele cislo, v pripade nevalidniho cisla E_FAILURE.
int get_num(char *str, char option)     // option: 'z' - cele cislo, 'n' - prirozene cislo
{
    if (option == 'n')		// prirozene cislo > 0
    {
        if (is_number(str, 'n'))
            return (int)strtol(str, NULL, 10);
    }
    if (option == 'z')		// cele cislo
        return (int)strtol(str, NULL, 10);
    return E_FAILURE;
}



// Fce vraci prvni (option == 1) nebo druhe (option == 2) cislo u argumentu argument z argv.
// Pokud neni cislo validni (validni - prirozene cislo > 0), vraci E_FAILURE.
int get_num_option(int argc, char **argv, char *argument, char option)
{
    for(int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            continue;
        }        
        if (strcmp(argv[i], argument) == 0)
        {
            if (option == 1 && (i+1 < argc))
                return get_num(argv[i+1], 'n');
            if (option == 2 && (i+2 < argc))
                return get_num(argv[i+2], 'n');
        }
    }
    return E_FAILURE;
}

// Funkce vraci pocet sloupcu v prvnim radku vstupni tabulky
// - podle nej jsou pozdeji kontrolovany dalsi sloupce (konzistentnost vstupni tabulky).
int cols_in_first_line(char *row, char *delim)
{
	int num_of_delim = 0;
	for (int i = 0; row[i] != '\0'; i++)
    {
		for (int d = 0; delim[d] != 0; d++)
        {
			if (row[i] == delim[d])
            {
				num_of_delim++;
				break;
			}
		}
	}
	return num_of_delim + 1;     // pocet sloupcu = pocet oddelovacu + 1
}

// Fce vraci predpokladany pocet sloupcu ve vystupni tabulce. V pripade neplatneho argumentu dcols vraci E_FAILURE.
int col_output(int argc, char *argv[], int col_in)
{
    int col_out = col_in;
    for(int i = 1; i < argc; i++)
    {
        if((strcmp(argv[i], "icol") == 0) || (strcmp(argv[i], "acol") == 0))
        {
            col_out++;
            continue;
        }
        if(strcmp(argv[i], "dcol") == 0)
        {
            col_out--;
            continue;
        }
        if(strcmp(argv[i], "dcols") == 0)
        {
            if(i+2 < argc)
            {
                int a = get_num(argv[i+1], 'n');
                int b = get_num(argv[i+2], 'n');
                if ((a != E_FAILURE) && (b != E_FAILURE) && (a <= b))
                {
                    int col_delete = b - a + 1;
                    col_out -= col_delete;
                } else
                    return E_FAILURE;
            } else
                return E_FAILURE;
        }
    }
    // pokud se smazou vsechny sloupce, program bude prazdny radek brat jako 1 sloupec
    if (col_out == 0)
        col_out = 1;
    return col_out;
}

// Funkce nahradi oddelovace v row_str znakem '\0' a ulozi jednotlive bunky do pole cells.
// Pokud je radek delsi je povolene, fce vraci E_FILE_ERR, jinak vraci pocet bunek na radku. 
int sep_cells(char *row_str, char *cells[], char *delim)
{
	char *ptr = row_str;
	int col = 0;
	while(1)
    {
		if(is_delim(*ptr, delim))
        {
			cells[col++] = "";
            if (is_delim(*ptr, delim) && (*(ptr + 1) == '\0'))
            {
	    		*ptr = '\0';
		    	cells[col++] = "";
			    return col;
		    }	
            ptr++;
			continue;
		}
		if (*ptr == '\0' && *(ptr + 1) == '\0')
			return col;
        // nacitani bunky
		cells[col++] = ptr;
        int length = 0;     // delka bunky
        while(!is_delim(*ptr, delim) && (*ptr != '\0'))
        {
			length++;
            ptr++;
		}
        // kontrola maximalniho poctu znaku bunky
        if (length >= MAX_CHAR_CELL)
            return E_FILE_ERR;

		if (is_delim(*ptr, delim) && (*(ptr + 1) == '\0'))
        {
			*ptr = '\0';
			cells[col++] = "";
			return col;
		}
		if(*ptr == '\0')
			return col;
        // prepsani oddelovace v retezci na '\0'
		*ptr++ = '\0';
    }
}

// Fce vraci true, pokud je argument nalezen v argv.
bool is_argument(int argc, char **argv, char *argument)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            continue;
        }        
        if (strcmp(argument, argv[i]) == 0)
            return true;
    }
    return false;
}

// Fce vraci pocet vyskytu kombinace "argument num" (pri num != NOT_SPECIFIED) v argv.
// Pri num == NOT_SPECIFIED vraci pocet vyskytu argumentu v argv.
int get_amount_of_arg(int argc, char **argv, char *argument, int num)
{
    int amount = 0;
	for(int i = 1; i < argc; i++)
    {   
        if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            continue;
        }        
        if((strcmp(argv[i], argument) == 0))
        {       
			if (num == NOT_SPECIFIED)            // prikaz u sebe nema mit zadne cislo (napr. arow, acol)
            {
                amount++;
                continue;
            }
            if (num == get_num(argv[i+1], 'n'))  // kontrola zadani cisla u prikazu s pozadovanym cislem (napr. irow N, dcol N)
            {
                amount++;
                i++;
            }
        }
	}
    return amount;
}

// Fce vraci prvni validni cislo sloupce a retezec (char *str_used), ktere se maji pouzit s
// argumentem (napr. cset N str). Pokud neni zadny argument zadan spravne, vraci E_FAILURE.
int get_str_in_arg(int argc, char **argv, char *argument, char *str_used)
{
    int num = E_FAILURE;
	for(int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            continue;
        }        
		if((strcmp(argv[i], argument) == 0))
        {
		    if ((i+2) < argc)
            {
                num = get_num(argv[i + 1], 'n');
                if (num != E_FAILURE)
                {
                    strcpy(str_used, argv[i + 2]);
                    if (get_length(str_used) >= MAX_CHAR_CELL)
                        num = E_FAILURE;
                    else 
                        return num;
                }
            }
        }
	}
	return num;
}

// Fce vraci E_SUCCESS, pokud je num v intervalu zjistenem z argv, NOT_IN_INTERVAL pokud v intervalu neni,
// LAST_ROW, pokud argumentem je row - - a E_FAILURE, pokud je interval neplatny.
int check_interval(int argc, char **argv, char *argument, int num)
{
    int a, b;       // hranice intervalu
    for(int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            continue;
        }        
        if (strcmp(argv[i], argument) == 0)
        {
            if (i+2 < argc)
            {
                a = get_num(argv[i + 1], 'n');
                b = get_num(argv[i + 2], 'n');
                // pokud je hledanym argumentem rows, mohou nastat i pripady rows N - a rows - -
                if (b == E_FAILURE && strcmp(argv[i], "rows") == 0)
                {
                    if (a != E_FAILURE && strcmp(argv[i + 2], "-") == 0)
                    {
                        if (num >= a)
                            return E_SUCCESS;       // rows N - znamena selekce radku od a do posledniho radku
                        else
                            return NOT_IN_INTERVAL;
                    }
                    if (a == E_FAILURE && strcmp(argv[i + 1], "-") == 0)                   
                        return LAST_ROW;              // rows - - znamena posledni radek
                }
                // pro ostatni argumenty jsou pripustne pouze cisla a, b, kde a < b 
                if (a == E_FAILURE || b == E_FAILURE || a > b)
                    return E_FAILURE;
                // pokud je num v intervalu:
                else if (a <= num && num <= b)
                {
                    return E_SUCCESS;
                } else
                    continue;
            } else
                return E_FAILURE;
        }
    }
    return NOT_IN_INTERVAL;         // num neni v zadanem intervalu
}

// Funkce vypise urcity pocet - amount - radku pouze s oddelovaci.
void insert_rows(char delim, int cols, int amount)
{
    for(int i = 0; i < amount; i++)
    {
        for(int i = 0; i < (cols - 1); i++)
                {
                    fputc(delim, stdout);   
                }
        fputc('\n', stdout);
    }
}

// Funkce vypise urcity pocet - amount - sloupcu (vystupnich oddelovacu).
void insert_cols(char delim, int amount)
{
    for(int i = 0; i < amount; i++)
    {
        fputc(delim, stdout);
    }
}

// Funkce overi, zda je retezec str desetinne cislo, pokud ano, prevede jej na cislo, 
// zaokrouhli (round) nebo vrati jeho celou cast (int) a ulozi jej zpet do char *str. 
// Pokud retezec neni realnym cislem, funkce ho necha v puvodni stavu.
void str_math(char *str, char *option)
{
    // jiny retezec nez desetinne cislo se nezpracovava
    if (!is_real_number(str))
        return;
    
    int i = 0;
    int num = get_num(str, 'z');
    char next;
    // nacteni str k desetinnemu oddelovaci
    while (str[i] != '.' && str[i] != ',')
    {
        // pokud ve str nebyl oddelovac nalezen, je to cele cislo - vracime jej
        if (str[i] == '\0') {
            sprintf(str, "%d", num); 
            return;
        }
        i++;
    }
	next = str[i+1];    // prvni znak po desetinnem oddelovaci
    str[i] = '\0';      // desetinny oddelovat je nahrazen '\0'
    // do num se nacte cislo pred desetinnym oddelovacem
    num = get_num(str, 'z');    
    // zaokrouhleni cisla pri funkci round
    if ((next >= '5' && next <= '9') && (strcmp(option, "round") == 0))
    {
	    if(num < 0)
	   		num--;
    	else
		    num++;
	}
    // prevod zpet na retezec str
    sprintf(str, "%d", num);
}

// Funkce prevede velka pismena v retezci str na mala.
void to_lower(char *str)
{
	int position;
	for (int i = 0; str[i] != '\0'; i++)
    {
		if (str[i] >= 'A' && str[i] <= 'Z')
        {
			position = str[i] - 'A';
			str[i] = 'a' + position;
		}
	}
}

// Funkce prevede mala pismena v retezci str na velka.
void to_upper(char *str)
{
	int position;
	for (int i = 0; str[i] != '\0'; i++)
    {
		if (str[i] >= 'a' && str[i] <= 'z')
        {
			position = str[i] - 'a';
			str[i] = 'A' + position;
		}
	}
}

// Funkce vraci true, pokud bunka cell zacina retezcem str.
bool beginning(char *cell, char *str)
{
    if(str[0] == '\0' && cell[0] != '\0')
        return false;
    for(int i = 0; str[i] != '\0'; i++)
    {
        if (cell[i] != str[i])
            return false;
        if ((cell[i + 1] == '\0') && (str[i + 1] != '\0'))
            return false;
    }        
    return true;
}

// Funkce vraci true, pokud bunka cell obsahuje retezec str.
bool containing(char *cell, char *str)
{
    if (str[0] == '\0' && cell[0] == '\0')
        return true;
    for(int i = 0; cell[i] != '\0'; i++)
    {
        if (cell[i] == str[0])
        {
            int j = i;      // zvysovani indexu cells pouze ve vnorenem cyklu
            for(int k = 0; str[k] != '\0'; k++)
            {
                if (cell[j] != str[k])
                    break;
                if (cell[j + 1] == '\0' && str[k + 1] != '\0')
                    return false;
                j++;
                if (str[k + 1] == '\0')
                    return true;
            }
        }
    }
    return false;
}

// Funkce vraci true a vypise chybove hlaseni, pokud num neni validni cislo, jinak vraci false.
bool catch_error(char *argument, int num)
{
    if (num == E_FAILURE)
    {
        fprintf(stderr, "Argument %s je neplatny.\n", argument);
        return true;
    }
    return false;
}

// Fce vraci E_SUCCESS, pokud je argument zadan spravne (platna cisla), nebo E_FAILURE, pokud v poradku neni.
// Kontroluje argumenty, ktere mohou byt zadany vickrat (irow N, icol N atd.)
int check_arg_num(int argc, char **argv, char *argument)
{
    int num;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)     // preskoci argumenty specifikace oddelovacu
        {
            i++;
            continue;
        }        
        if (strcmp(argument, argv[i]) == 0)
        {
            if (i+1 < argc)
            {    
                num = get_num(argv[i + 1], 'n');
                    if (num == E_FAILURE)
                        return E_FAILURE;
            } else
                return E_FAILURE;
        }
    }
    return E_SUCCESS;
}

// Funkce zkontroluje, zda jsou jednotlive prikazy zadany spravne (s pripadnymi
// spravnymi cisly / retezcem). Vraci true, pokud jsou vsechny argumenty v poradku, 
// false, pokud je alespon jeden argument chybny - v tom pripade je vypsano chybove hlaseni.
bool check_arg_validity(int argc, char **argv, int row, char *str)
{
    // indexy:      0       1       2       3       4           5          6       7
    char *arg[] = {"irow", "drow", "icol", "dcol", "tolower", "toupper", "round", "int",
    // indexy:      8       9        10      11      12      13      14            15
                   "drows", "rows", "copy", "swap", "move", "cset", "beginswith", "contains"};

    // prochazeni pole argumentu a zjistovani spravnosti zadanych prikazu, pokud je arg[i] argumentem
    for(int i = 0; i <= 15; i++)
    {
        if (is_argument(argc, argv, arg[i]))
        {
            // vypocet cisla num podle jednotlivych skupin argumentu
            int num;
            if (i >= 0 && i <=3)            // prikazy irow R, drow R, icol C, dcol C
                num = check_arg_num(argc, argv, arg[i]);
            else if (i >= 4 && i <= 7)      // prikazy tolower C, toupper C, round C, int C
                num = get_num_option(argc, argv, arg[i], 1);
            else if (i == 8 || i == 9)      // prikazy drows N M, rows N M
                num = check_interval(argc, argv, arg[i], row);
            else if (i >= 10 && i <= 12) {  // prikazy copy N M, swap N M, move N M
                num = get_num_option(argc, argv, arg[i], 1);
                if (catch_error(arg[i], num))
                    return false;
                num = get_num_option(argc, argv, arg[i], 2);
            } else if (i >= 13)             // prikazy cset C STR, beginswith C STR, contains C STR
                num = get_str_in_arg(argc, argv, arg[i], str);
            // pokud je cislo num E_FAILURE, je vraceno false - chyba
            if (catch_error(arg[i], num))
                return false;
        }
    }
    return true;
}

// Funkce vypise bunku na vystup a prida oddelovac, pokud je aktualni cislo radku > 1.
void printout(char *str, int col, char delim)
{
    if(col != 1)
        fputc(delim, stdout);
    fputs(str, stdout);
}

/**************************************************************************************************************************************/

int main(int argc, char **argv) 
{
	char row_str[MAX_CHAR_LINE];                     	    // hodnota aktualne nacteneho radku
	int col_out;                                            // pocet sloupcu ve vystupni tabulce
	char *delim;                        				    // vstupni oddelovace (vystupni: delim[0])
	int num_sel;                                            // cislo pouzivane selekci
	char str_used[MAX_CHAR_CELL], str_sel[MAX_CHAR_CELL];   // retezce pouzivane funkcemi / selekci
    
    bool last_row = false;              // detekce posledniho radku
    bool selection_rows = false;        // detekce selekce radku pomoci argumentu rows
    bool selection_in[] = {0, 0};       // detekce selekce: selection_in[1]... beginswith, selection_in[2]... contains
    bool selection_now[] = {0, 0};      // detekce selekce: slouzi pro uchovani soucasneho stavu, stejne poradi jako selection_in
    
    // Detekce zadani selekce radku
    if (is_argument(argc, argv, "rows"))
        selection_rows = true;
    else if (is_argument(argc, argv, "beginswith")) {
        num_sel = get_str_in_arg(argc, argv, "beginswith", str_sel);
        selection_in[0] = true;
    } else if (is_argument(argc, argv, "contains")) {
        num_sel = get_str_in_arg(argc, argv, "contains", str_sel);
        selection_in[1] = true;
    }
	// Ziskani oddelovacu
	if (!get_delim(argc, argv, &delim))
    {
        fprintf(stderr, "Oddelovace jsou zadany chybne.\n");
        return E_FAILURE;
    }

	// NACITANI RADKU
	int row = 0;            // nastaveni pocitani radku 
    while (fgets(row_str, MAX_CHAR_LINE, stdin))
    {
        row++;
        
        // Kontrola, zda je aktualne nacteny radek posledni - jestli po soucasnem radku nasleduje EOF
        char last_row_check;
        if ((last_row_check = fgetc(stdin)) == EOF)
            last_row = true;
        ungetc(last_row_check, stdin);
        
		// Ostraneni znaku konce radku z nacteneho radku, pripadne vypis chyby 
        int row_len = get_length(row_str);
        if (delete_n(row_str, row_len) == E_FILE_ERR)
        {
            fprintf(stderr, "Tabulka ma moc dlouhe radky.\n");
            return E_FILE_ERR;
        }
        
        // Prikazy provadene pouze jednou - na prvnim radku 
	    int col_in;            // pocet sloupcu na prvnim radku vstupni tabulky
        if(row == 1)
        {
            // Zjisteni poctu vstupnich sloupcu z prvniho radku
			char first_row[MAX_CHAR_LINE];
			strcpy(first_row, row_str);
			col_in = cols_in_first_line(first_row, delim);
	
            // Zjisteni poctu sloupcu ve vystupni tabulce
            col_out = col_output(argc, argv, col_in);
            // kontrola dcols pomoci col_out
            if (catch_error("dcols", col_out))
                return E_FAILURE;
            
            // Kontrola spravnosti zadani prikazu
            if (!check_arg_validity(argc, argv, row, str_used))
                return E_FAILURE;
        }

        // PRIKAZY PRO UPRAVU TABULKY
        // drow R - odstrani radek cislo R
		if (is_argument(argc, argv, "drow"))
        {
            if (get_amount_of_arg(argc, argv, "drow", row) > 0)
                continue;
        }
        
        // drows N M - odstrani radky N az M
        if (is_argument(argc, argv, "drows"))
        { 
            if (check_interval(argc, argv, "drows", row) == E_SUCCESS)
                continue;
        }
		
        // irow R - vlozi radek pred radek R
		if (is_argument(argc, argv, "irow"))
            insert_rows(delim[0], col_out, get_amount_of_arg(argc, argv, "irow", row));
        
        // NACITANI SLOUPCU a porovnani poctu sloupcu s col_in
        char *cells[col_in];
        int number_of_cols;
        if ((number_of_cols = sep_cells(row_str, cells, delim)) != col_in)
        {
            // Pri zadani vetsich dat nez je pripustne je vypsano chybove hlaseni a ukoncen program
            if (number_of_cols == E_FILE_ERR)
            {
                fprintf(stderr, "Tabulka ma prilis dlouhe bunky.\n");
                return E_FILE_ERR;
            }
            fprintf(stderr, "Tabulka nema pravidelny pocet radku a sloupcu!\n");
		    return E_FILE_ERR;
		}
        
        // Nastaveni hodnot selekce radku podle obsahu bunky v zadanem sloupci
        if (num_sel > col_in)       // pri zadani vyssiho cisla sloupce nez col_in je selekce ignorovana
            selection_now[0] = 0, selection_now[1] = 0;
        // beginswith
        else if (selection_in[0])
            selection_now[0] = beginning(cells[num_sel - 1], str_sel); 
        // contains
        else if (selection_in[1])
            selection_now[1] = containing(cells[num_sel - 1], str_sel);

        int col = 0;            // cislo sloupce ve vstupni tabulce
        int col_act = 0;        // cislo sloupce v upravovane tabulce (slouzi ke kontrole vkladani oddelovacu)
        
        for(int i = 0; i < col_in; i++)
        {
			col++;	
            col_act++;	
			
            // dcol C - odstrani sloupec cislo C
			if (is_argument(argc, argv, "dcol"))
            {
                if (get_amount_of_arg(argc, argv, "dcol", col) > 0)
                {
                    col_act--;
                    continue;
                }
            }
            
            // dcols N M - odstrani sloupce N az M
            if (is_argument(argc, argv, "dcols"))
            {
                if (check_interval(argc, argv, "dcols", col) == E_SUCCESS)
                {
                    col_act--;
                    continue;
                }
            }

            // icol C - vlozi prazdny sloupec pred sloupec C
			if (is_argument(argc, argv, "icol"))
                    insert_cols(delim[0], get_amount_of_arg(argc, argv, "icol", col));

            
            // Zpracovani pouze vybranych radku podle selekce 
            if (selection_rows && (check_interval(argc, argv, "rows", row) == NOT_IN_INTERVAL))
            {
                ;
            } else if (selection_rows && (check_interval(argc, argv, "rows", row) == LAST_ROW) && !last_row ) {
                ;
            // kontrola zadani selekce jako prikazu a platnosti dane selekce pro aktualni radek
            } else if (selection_in[0] && !selection_now[0]) {      // beginswith
                ;
            } else if (selection_in[1] && !selection_now[1]) {      // contains
                ;
            } else {
                
                // PRIKAZY PRO ZPRACOVANI DAT
    			// cset C STR - do bunky ve sloupci C je nastaven retezec STR
	    		if (is_argument(argc, argv, "cset"))
                {
                    if (col == get_str_in_arg(argc, argv, "cset", str_used)) 
                    {
                        printout(str_used, col, delim[0]);
    					continue;
	    			}
                }

                // tolower C - retezec ve sloupci C je preveden na mala pismena
    			else if (is_argument(argc, argv, "tolower"))
                {
		    		if (col == get_num_option(argc, argv, "tolower", 1))
			    		to_lower(cells[i]);
    			}  
	    		
                // toupper C - retezec ve sloupci C je preveden na velka pismena
		    	else if (is_argument(argc, argv, "toupper"))
                {
    				if (col == get_num_option(argc, argv, "toupper", 1))
	    				to_upper(cells[i]);
		    	}
			    
                // round C - desetinne cislo ve sloupci C je zaokrouhleno na cele cislo
    			else if (is_argument(argc, argv, "round"))
                {
		    		if (col == get_num_option(argc, argv, "round", 1))
			        	str_math(cells[i], "round");
    			}

	    		// int C - je odstanena desetinna cast desetinneho cisla ve sloupci C
		    	else if (is_argument(argc, argv, "int"))
                {
    				if (col == get_num_option(argc, argv, "int", 1))
						str_math(cells[i], "int");
	    		}

                // copy N M - prepise obsah bunek ve sloupci M hodnotami ze sloupce N
                else if (is_argument(argc, argv, "copy"))
                {
		    		int num = get_num_option(argc, argv, "copy", 1);
                    int num2 = get_num_option(argc, argv, "copy", 2);
                    if (num <= col_in && num2 <= col_in)
                    {
                        if (col == num2)
                        {
                            printout(cells[num - 1], col_act, delim[0]);
                            continue;
    		    	    }
                    }
                }

    			// swap N M	- zameni hodnoty bunek ve sloupcich N a M
                else if (is_argument(argc, argv, "swap"))
                {
                    int num = get_num_option(argc, argv, "swap", 1);
                    int num2 = get_num_option(argc, argv, "swap", 2);
                    if (num <= col_in && num2 <= col_in)
                    {
                        if (col == num)
                        {
                            printout(cells[num2 - 1], col_act, delim[0]);
		    			    continue;
			    	    } else if (col == num2) {
				    	    printout(cells[num - 1], col_act, delim[0]);
					        continue;
    				    }
                    }
                } 

                // move N M	- presune sloupec N pred sloupec M
	    		else if (is_argument(argc, argv, "move")) {
		    		int num = get_num_option(argc, argv, "move", 1);
                    int num2 = get_num_option(argc, argv, "move", 2);
                    if (num <= col_in && num2 <= col_in)
                    {
                        if (col == num && num != num2)
                        {
                            col_act--;
		    			    continue;
			    	    } else if (col == num2 && num != num2) {
				    	    printout(cells[num - 1], col_act, delim[0]);
                            col_act++;
				    	    printout(cells[i], col_act, delim[0]);
					        continue;
    				    }
                    }
		    	}
            }
			
            // VYPIS BUNKY A ODDELOVACE
            printout(cells[i], col_act, delim[0]);
		}
        // acol - prida prazdny sloupec za posledni sloupec
        if (is_argument(argc, argv, "acol"))
            insert_cols(delim[0], get_amount_of_arg(argc, argv, "acol", NOT_SPECIFIED));
        
        // Pridani znaku konce radku
			fputc('\n', stdout);
        
        // Nastaveni selekce radku na vychozi stav
        selection_now[0] = selection_in[0];
        selection_now[1] = selection_in[1];

    }   

    // Vypsani chyboveho hlaseni, pokud je vstupni tabulka prazdny soubor.
    if (row == 0)
    {
        fprintf(stderr, "Vstupni tabulka je prazdna.\n");
        return E_FILE_ERR;
    }
            
    //arow - prida novy radek tabulky na konec tabulky
    if (is_argument(argc, argv, "arow"))
        insert_rows(delim[0], col_out, get_amount_of_arg(argc, argv, "arow", NOT_SPECIFIED));
	
    return E_SUCCESS;
}
