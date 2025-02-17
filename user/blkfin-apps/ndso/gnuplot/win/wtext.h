/*
 * $Id$
 */

/* GNUPLOT - win/wtext.h */

/*[
 * Copyright 1992 - 1993, 1998   Russell Lang
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

/* redefine functions that can talk to tty devices, to use 
 * implementation in winmain.c/wgnuplot.dll */

#define kbhit()  MyKBHit()
#define getche() MyGetChE()
#define getch()  MyGetCh()
#define putch(ch)  MyPutCh(ch)

#define fgetc(file) MyFGetC(file)
#undef  getchar
#define getchar()   MyFGetC(stdin)
#undef  getc
#define getc(file)  MyFGetC(file)
#define fgets(str,sz,file)  MyFGetS(str,sz,file)
#define gets(str)  	    MyGetS(str)

#define fputc(ch,file) MyFPutC(ch,file)
#undef  putchar
#define putchar(ch)    MyFPutC(ch,stdout)
#undef  putc
#define putc(ch,file)  MyFPutC(ch,file)
#define fputs(str,file)  MyFPutS(str,file)
#define puts(str)        MyPutS(str)

#define fprintf MyFPrintF
#define printf MyPrintF

#define fwrite(ptr, size, n, stream) MyFWrite(ptr, size, n, stream)
#define fread(ptr, size, n, stream) MyFRead(ptr, size, n, stream)

/* now cause errors for some unimplemented functions */

#define vprintf dontuse_vprintf
#define vfprintf dontuse_vfprintf
#define fscanf dontuse_fscanf
#define scanf dontuse_scanf
#define clreol dontuse_clreol
#define clrscr dontuse_clrscr
#define gotoxy dontuse_gotoxy
#define wherex dontuse_wherex
#define wherey dontuse_wherey
#define cgets dontuse_cgets
#define cprintf dontuse_cprintf
#define cputs dontuse_cputs
#define cscanf dontuse_cscanf
#define ungetch dontuse_ungetch

/* now for the prototypes */

int MyPutCh(int ch);
int MyKBHit(void);
int MyGetCh(void);
int MyGetChE(void);
int MyFGetC(FILE *file);
char * MyGetS(char *str);
char * MyFGetS(char *str, unsigned int size, FILE *file);
int MyFPutC(int ch, FILE *file);
int MyFPutS(char *str, FILE *file);
int MyPutS(char *str);
int MyFPrintF(FILE *file, char *fmt, ...);
int MyPrintF(char *fmt, ...);
size_t MyFWrite(const void *ptr, size_t size, size_t n, FILE *stream);
size_t MyFRead(void *ptr, size_t size, size_t n, FILE *stream);

