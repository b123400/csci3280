#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

#define CODE_SIZE  12
#define TRUE 1
#define FALSE 0
#define TABLE_SIZE 4096

/*
* CSCI3280 Introduction to Multimedia Systems *
* --- Declaration --- *
* I declare that the assignment here submitted is original except for source
* material explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/ *
* Assignment 2
* Name: 
* Student ID : 
* Email Addr : 
*/


struct TableEntry
{
	char character; // There can be null in string (for binary file)
	int previous; // If previous == TABLE_SIZE-1, it is the first character
	// An empty entry is an entry with index != 0 && char == '\0' && previous == TABLE_SIZE-1
};
typedef struct TableEntry StringTable[TABLE_SIZE];

/**
 * A reversed linked list for string
 * Easier pushing and handling null-in-string
 * previous == NULL means the first character
 * "abc" is [c]->[b]->[a]->NULL
 */
struct TextNode
{
	char character;
	struct TextNode *previous;
};

/* function prototypes */
unsigned int read_code(FILE*, unsigned int); 
void write_code(FILE*, unsigned int, unsigned int); 
void writefileheader(FILE *,char**,int);
void readfileheader(FILE *,char**,int *);
void compress(FILE*, FILE*, StringTable);
void decompress(FILE*, FILE*, StringTable);

void makeStringTable(StringTable stringTable);

int main(int argc, char **argv)
{
	int printusage = 0;
	int	no_of_file;
	char **input_file_names;    
	char *output_file_names;
	FILE *lzw_file;
	StringTable table;
	makeStringTable(table);

	if (argc >= 3)
	{
		if ( strcmp(argv[1],"-c") == 0)
		{		
			/* compression */
			lzw_file = fopen(argv[2] ,"wb");
		
			/* write the file header */
			input_file_names = argv + 3;
			no_of_file = argc - 3;
			writefileheader(lzw_file,input_file_names,no_of_file);
						
			/* ADD CODES HERE */
			for (int i = 0; i < no_of_file; i++) {
				FILE *file = fopen(input_file_names[i] ,"r");
				compress(file, lzw_file, table);
				fclose(file);
			}

			// Write one more time to make sure it is not in the buffer
			write_code(lzw_file, TABLE_SIZE-1, CODE_SIZE);

			fclose(lzw_file);
		} else
		if ( strcmp(argv[1],"-d") == 0)
		{	
			/* decompress */
			lzw_file = fopen(argv[2] ,"rb");
			
			/* read the file header */
			no_of_file = 0;
			readfileheader(lzw_file,&output_file_names,&no_of_file);

			char *filename;
			filename = strtok(output_file_names, "\n");
			int i = 0;
			while(filename != NULL) {
				if (strlen(filename) != 0) {
					FILE *file = fopen(filename, "w");
					decompress(lzw_file, file, table);
					fclose(file);
				}
				i++;
				if (i >= no_of_file) break;
				filename = strtok(NULL, "\n");
			}
			
			fclose(lzw_file);
			
			free(output_file_names);
		}else
			printusage = 1;
	}else
		printusage = 1;

	if (printusage)
		printf("Usage: %s -<c/d> <lzw filename> <list of files>\n",argv[0]);
	
	return 0;
}

/*****************************************************************
 *
 * writefileheader() -  write the lzw file header to support multiple files
 *
 ****************************************************************/
void writefileheader(FILE *lzw_file,char** input_file_names,int no_of_files)
{
	int i;
	/* write the file header */
	for ( i = 0 ; i < no_of_files; i++) 
	{
		fprintf(lzw_file,"%s\n",input_file_names[i]);
			
	}
	fputc('\n',lzw_file);

}

/*****************************************************************
 *
 * readfileheader() - read the fileheader from the lzw file
 *
 ****************************************************************/
void readfileheader(FILE *lzw_file,char** output_filenames,int * no_of_files)
{
	int noofchar;
	char c,lastc;

	noofchar = 0;
	lastc = 0;
	*no_of_files=0;
	/* find where is the end of double newline */
	while((c = fgetc(lzw_file)) != EOF)
	{
		noofchar++;
		if (c =='\n')
		{
			if (lastc == c )
				/* found double newline */
				break;
			(*no_of_files)++;
		}
		lastc = c;
	}

	if (c == EOF)
	{
		/* problem .... file may have corrupted*/
		*no_of_files = 0;
		return;
	
	}
	/* allocate memeory for the filenames */
	*output_filenames = (char *) malloc(sizeof(char)*noofchar);
	/* roll back to start */
	fseek(lzw_file,0,SEEK_SET);

	fread((*output_filenames),1,(size_t)noofchar,lzw_file);
	
	return;
}

/*****************************************************************
 *
 * read_code() - reads a specific-size code from the code file
 *
 ****************************************************************/
unsigned int read_code(FILE *input, unsigned int code_size)
{
	unsigned int return_value;
	static int input_bit_count = 0;
	static uint32_t input_bit_buffer = 0L;

	/* The code file is treated as an input bit-stream. Each     */
	/*   character read is stored in input_bit_buffer, which     */
	/*   is 32-bit wide.                                         */

	/* input_bit_count stores the no. of bits left in the buffer */

	while (input_bit_count <= 24) {
		unsigned long thing = (unsigned long) getc(input);
		input_bit_buffer |= thing << (24-input_bit_count);
		input_bit_count += 8;
	}
	return_value = input_bit_buffer >> (32 - code_size);
	input_bit_buffer <<= code_size;
	input_bit_count -= code_size;
	
	return(return_value);
}


/*****************************************************************
 *
 * write_code() - write a code (of specific length) to the file 
 *
 ****************************************************************/
void write_code(FILE *output, unsigned int code, unsigned int code_size)
{
	static int output_bit_count = 0;
	static uint32_t output_bit_buffer = 0L;

	/* Each output code is first stored in output_bit_buffer,    */
	/*   which is 32-bit wide. Content in output_bit_buffer is   */
	/*   written to the output file in bytes.                    */

	/* output_bit_count stores the no. of bits left              */    

	output_bit_buffer |= (unsigned long) code << (32-code_size-output_bit_count);
	output_bit_count += code_size;

	while (output_bit_count >= 8) {
		putc(output_bit_buffer >> 24, output);
		output_bit_buffer <<= 8;
		output_bit_count -= 8;
	}


	/* only < 8 bits left in the buffer                          */    

}

int insertIntoTable(struct TextNode *p, char c, StringTable table);
int findCodeWithCharInTable(struct TextNode *p, char c, StringTable table);
int findCodeInTable(struct TextNode *p, StringTable table);
bool isTableIndexEqualToString(StringTable table, unsigned int code, struct TextNode *p);
struct TextNode *appendedNode(struct TextNode *p, char c);

void nodeToString(struct TextNode *p, char out[]) {
	if (p == NULL) {
		out[0] = '\0';
		return;
	}
	nodeToString(p->previous, out);
	unsigned int length = strlen(out);
	out[length] = p->character == '\0' ? '0' : p->character;
	out[length+1] = '\0';
}

void makeStringTable(StringTable stringTable) {
	for (int i = 0; i < 256; i++) {
		stringTable[i].character = i;
		stringTable[i].previous = TABLE_SIZE-1;
	}
	for (int i = 256; i < TABLE_SIZE; i++) {
		stringTable[i].character = '\0';
		stringTable[i].previous = TABLE_SIZE-1;
	}
}

bool isTableIndexEmpty(StringTable table, unsigned int i) {
	return i != 0 && table[i].character == '\0' && table[i].previous == TABLE_SIZE-1;
}

int insertIntoTable(struct TextNode *p, char c, StringTable table) {
	for (unsigned int i = 0; i < TABLE_SIZE-1; i++) {
		bool isEmpty = isTableIndexEmpty(table, i);
		if (isEmpty) {
			table[i].character = c;
			int index = findCodeInTable(p, table);
			if (index == -1) {
				index = insertIntoTable(p->previous, p->character, table);
			}
			table[i].previous = index;
			return i;
		}
	}
	// printf("table is full, reset now\n");
	makeStringTable(table);
	return insertIntoTable(p, c, table);
}

int findCodeWithCharInTable(struct TextNode *p, char c, StringTable table) {
	// printf("10\n");
	struct TextNode *appended = appendedNode(p, c);
	// printf("11\n");
	int index = findCodeInTable(appended, table);
	// printf("12\n");
	free(appended);
	// printf("13\n");
	return index;
}

int findCodeInTable(struct TextNode *p, StringTable table) {
	for (int i = 0; i < TABLE_SIZE-1; i++) {
		if (isTableIndexEmpty(table, i)) return -1;
		// printf("comparing table[%d]\n", i);
		if (isTableIndexEqualToString(table, i, p)) {
			// printf("ok\n");
			// printf("findCodeInTable: found %s, index is %d\n", p, i);
			return i;
		}
		// printf("not ok\n");
	}
	// printf("not found %c in table\n", p->character);
	return -1;
}

bool isTableIndexEqualToString(StringTable table, unsigned int code, struct TextNode *p) {
	struct TableEntry entry = table[code];
	while (true) {
		// printf("14 %d, %c\n", code, entry.character);
		if (entry.character != p->character) return false;
		// printf("15\n");
		if (entry.previous == TABLE_SIZE-1 && p->previous == NULL) return true;
		// printf("16\n");
		if (entry.previous == TABLE_SIZE-1) return false;
		// printf("17\n");
		if (p->previous == NULL) return false;
		// printf("18\n");
		entry = table[entry.previous];
		p = p->previous;
		// printf("19\n");
	}
	printf("wow shouldnt come here\n");
	return false;
}

/**
 * Need free()
 */
struct TextNode *stringFromTableWithIndex(StringTable table, unsigned int code) {
	if (isTableIndexEmpty(table, code)) return NULL;
	struct TextNode *previous = NULL;
	if (table[code].previous != TABLE_SIZE-1) {
		// I love recursion
		previous = stringFromTableWithIndex(table, table[code].previous);
	}
	return appendedNode(previous, table[code].character);
}

/**
 * Need free()!
 */
struct TextNode *appendedNode(struct TextNode *p, char c) {
	struct TextNode *newNode = (struct TextNode*)malloc(sizeof(struct TextNode));
	newNode->character = c;
	newNode->previous = p;
	return newNode;
}

void printToOutout(struct TextNode *p, FILE *output) {
	if (p == NULL) return;
	if (p->previous != NULL) {
		printToOutout(p->previous, output);
	}
	fprintf(output, "%c", p->character);
}

struct TextNode *firstNode(struct TextNode *p) {
	if (p == NULL) return NULL;
	if (p->previous == NULL) return p;
	return firstNode(p->previous);
}

struct TextNode *makeNode(char c) {
	struct TextNode *newNode = (struct TextNode*)malloc(sizeof(struct TextNode));
	newNode->character = c;
	newNode->previous = NULL;
	return newNode;
}

/**
 * free the entire string
 */
void freeNode(struct TextNode *p) {
	if (p->previous != NULL) freeNode(p->previous);
	free(p);
}

/*****************************************************************
 *
 * compress() - compress the source file and output the coded text
 *
 ****************************************************************/
void compress(FILE *input, FILE *output, StringTable table)
{
	char c;
	int _c; // need int for EOF comparison
	struct TextNode *p = NULL;
	unsigned int code = 0;

	while((_c = getc(input))!= EOF) {
		c = _c;
		// printf("trying to find %s + %c\n", p, c);
		unsigned int index = findCodeWithCharInTable(p, c, table);
		if (index != -1) {
			// Found the pattern in the dictionary
			// printf("found %s + %c at %d\n", p, c, index);
			code = index;
			p = appendedNode(p, c);
		} else {
			unsigned int thisCode = findCodeInTable(p, table);
			// printf("not found, p's code is: %d, p = %s\n", thisCode, p);
			// printf("writing code %u, %c\n", thisCode, table[thisCode].character);
			write_code(output, thisCode, CODE_SIZE);

			// In case if the last character is a unseen character
			// we need to put export it as well.
			code = (unsigned int)c;
			insertIntoTable(p, c, table);
			freeNode(p);
			p = makeNode(c);
		}
	}
	freeNode(p);
	if (code != 0) {
		write_code(output, code, CODE_SIZE);
	}
	write_code(output, TABLE_SIZE-1, CODE_SIZE);
}

/*****************************************************************
 *
 * decompress() - decompress a compressed file to the orig. file
 *
 ****************************************************************/
void decompress(FILE *input, FILE *output, StringTable table)
{
	char c;
	struct TextNode *p = NULL;
	unsigned int prevCode = 0;
	unsigned int currentCode = 0;

	prevCode = read_code(input, CODE_SIZE);
	p = stringFromTableWithIndex(table, prevCode);
	// printf("wow %d, %s\n", prevCode, p);
	printToOutout(p, output);

	while ((currentCode = read_code(input, CODE_SIZE)) != TABLE_SIZE-1) {
		freeNode(p);
		p = stringFromTableWithIndex(table, currentCode);
		if (p != NULL) {
			// Found in dictionary
			printToOutout(p, output);

			struct TextNode *first = firstNode(p);
			c = first->character;
			free(p);
			p = stringFromTableWithIndex(table, prevCode);
			int inserted = insertIntoTable(p, c, table);
		} else {
			// Not found in dictionary
			p = stringFromTableWithIndex(table, prevCode);

			struct TextNode *first = firstNode(p);
			c = first->character;

			printToOutout(p, output);
			fprintf(output, "%c", c);
			insertIntoTable(p, c, table);
		}

		prevCode = currentCode;
	}
}
