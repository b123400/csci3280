/*

	CSCI 3280, Introduction to Multimedia Systems
	Spring 2015
	
	Assignment 01 Skeleton

	ascii.cpp
	
*/

#include "stdio.h"
#include "malloc.h"
#include "memory.h"
#include "math.h"
#include "bmp.h"		//	Simple .bmp library

#define MAX_SHADES 8


//
//	***** PUT ALL YOUR CODE INSIDE main() *****
//
int main( int argc, char** argv)
{

	char shades[MAX_SHADES] = {' ', '.', '+', '*', 'X', '%', '#', '@'};
	int		w, h;

	//	Read in image data ( command line: argv[1] )
	//
	Bitmap myBmp( argv[1] );
	
	w = myBmp.getWidth();
	h = myBmp.getHeight();

	//	Prepare output TXT file ( command line: argv[2] )
	//
	FILE *fout = fopen( argv[2], "w" );
	fprintf( fout, "%d %d", w, h );
	
	//
	//	Your code goes here ....
	//	
	//	Advice:
	//	Use Bitmap.getColor(x,y,R,G,B) for getting color of pixel at (x,y)
	//	Use fputc() to read character and "\n" to end your line.
	//
	
	for (int index = 0; index < w*h; index++) {
		int x = index % w;
		int y = index / w;
		unsigned char r, g, b;
		myBmp.getColor(x, y, r, g, b);
		int grey = 0.299 * r + 0.587 * g + 0.114 * b;
		int color = grey / 32;
		char toWrite = shades[color];
		if (x == 0) {
			fputc('\n', fout);
		}
		fputc(toWrite, fout);
	}
	
	//	close ASCII file
	fclose(fout);

	return 0;

} 
