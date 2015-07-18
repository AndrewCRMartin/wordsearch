/*************************************************************************

   Program:    WordSearch
   File:       WordSearch.c
   
   Version:    V1.2
   Date:       12.07.01
   Function:   Build a WordSearch creating PostScript, LaTeX or ASCII
               output
   
   Copyright:  (c) SciTech Software 1994-2001
   Author:     Dr. Andrew C. R. Martin
   Address:    SciTech Software
               23, Stag Leys,
               Ashtead,
               Surrey,
               KT21 2TD.
   Phone:      +44 (0)1372 275775
   EMail:      andrew@andrew-martin.org
               
**************************************************************************

   This program is not in the public domain, but it may be freely copied
   and distributed for no charge providing this header is included.
   The code may be modified as required, but any modifications must be
   documented so that the person responsible can be identified. If someone
   else breaks this code, I don't want to be blamed for code that does not
   work! The code may not be sold commercially without prior permission 
   from the author, although it may be given away free with commercial 
   products, providing it is made clear that this program is free and that 
   the source code is provided with the program.

**************************************************************************

   Description:
   ============

**************************************************************************

   Usage:
   ======

**************************************************************************

   Revision History:
   =================
   V1.0  14.01.94 Original
                  N.B. SortByLength() is currently a dummy routine. 
   V1.1  11.07.01 Outputs a proper PostScript header
   V1.2  12.07.01 Fixed bug in selecting random start for diagonals

*************************************************************************/
/* Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/************************************************************************/
/* Defines and macros
*/
typedef int BOOL;

#define TERMINATE(x) {  int j;               \
                        for(j=0; x[j]; j++)  \
                        {  if(x[j] == '\n')  \
                           {  x[j] = '\0';   \
                              break;         \
                     }  }  }

#define UPPER(x) {  int i; \
                    for(i=0; i<(int)strlen(x) && x[i]; i++) \
                       x[i] = (char)toupper(x[i]);          \
                 }


#ifndef TRUE
#define TRUE          1
#define FALSE         0
#endif

#define MAXBUFF     160

#define STYLE_PS      1    /* Output styles                             */
#define STYLE_LATEX   2
#define STYLE_ASCII   3

#define MAXWORDS     30    /* These defaults may be modified            */
#define MAXWORDLEN   15
#define GRIDSIZE     20
#define MAXTRY      100

/************************************************************************/
/* Globals
*/
FILE  *gIn        = NULL, 
      *gOut       = NULL;
BOOL  gSolution   = FALSE,
      gWordList   = TRUE;
int   gMaxWords   = MAXWORDS,
      gMaxWordLen = MAXWORDLEN,
      gGridSize   = GRIDSIZE,
      gOutput     = STYLE_PS,
      gFontSize   = 18;
char  **gGrid     = NULL,
      **gWords    = NULL;

/************************************************************************/
/* Prototypes
*/
int  main(int argc, char **argv);
BOOL Initialise(char *infile, char *outfile);
BOOL ReadCmdLine(int argc, char **argv, char *infile, char *outfile);
BOOL BuildArrays(void);
BOOL OpenFiles(char *infile, char *outfile);
int  ReadInputData(void);
BOOL FitWords(int NWords);
void PrintSolution(void);
void FillSpaces(void);
void PrintPuzzle(int NWords);
void Usage(void);
void SortByLength(char **gWords, int NWords);
int  RandomNum(int maxran);
BOOL PlaceWord(char *word);
void InitOutput(int style);
void EndOutput(int style);
void DoPSOutput(char **grid, int gridsize, BOOL WordList, int NWords, 
                BOOL solution);
void DoLaTeXOutput(char **grid, int gridsize, BOOL WordList, int NWords, 
                   BOOL solution);
void DoASCIIOutput(char **grid, int gridsize, BOOL WordList, 
                   int NWords, BOOL solution);

/************************************************************************/
/*>int main(int argc, char **argv)
   -------------------------------
   Main program for WordSearch
   
   13.01.94 Original    By: ACRM
   14.01.94 Added calls to InitOutput() and EndOutput()
*/
int main(int argc, char **argv)
{
   char  infile[MAXBUFF],
         outfile[MAXBUFF];
   int   NWords = 0;
         
   if(Initialise(infile,outfile))
   {
      if(ReadCmdLine(argc, argv, infile, outfile))
      {
         if(BuildArrays())
         {
            if(OpenFiles(infile,outfile))
            {
               if((NWords=ReadInputData()) != 0)
               {
                  if(FitWords(NWords))
                  {
                     InitOutput(gOutput);
                     if(gSolution) PrintSolution();
                     FillSpaces();
                     PrintPuzzle(NWords);
                     EndOutput(gOutput);
                  }
                  else
                  {
                     fprintf(stderr,"Unable to build puzzle.\n");
                     return(1);
                  }
               }
            }
         }
         else
         {
            fprintf(stderr,"Unable to allocate memory.\n");
            return(1);
         }
      }
      else
      {
         Usage();
      }
   }

   return(0);
}

/************************************************************************/
/*>BOOL Initialise(char *infile, char *outfile)
   --------------------------------------------
   Initialise variables and seed the random number generator
   
   13.01.94 Original    By: ACRM
*/
BOOL Initialise(char *infile, char *outfile)
{
   time_t seed;
   
   infile[0]   = '\0';
   outfile[0]  = '\0';
   
   gMaxWords   = MAXWORDS;
   gMaxWordLen = MAXWORDLEN;
   gGridSize   = GRIDSIZE;

   /* Seed random number generator                                      */
   time(&seed);
   srand((unsigned int)seed);
   
   return(TRUE);
}

/************************************************************************/
/*>BOOL ReadCmdLine(int argc, char **argv, char *infile, char *outfile)
   --------------------------------------------------------------------
   Read the command line. Get flags from switches and record filenames
   if specified. Returns FALSE if there is an error.
   
   13.01.94 Original    By: ACRM
   14.01.94 Added p,l,a,f and n switches
            Added return after reading filenames
*/
BOOL ReadCmdLine(int argc, char **argv, char *infile, char *outfile)
{
   argc--; argv++;
   
   while(argc)
   {
      if(argv[0][0] == '-')   /* Handle switches                        */
      {
         switch(argv[0][1])
         {
         case 'w': case 'W':
            argc--; argv++;
            sscanf(argv[0],"%d",&gMaxWords);
            break;
         case 'm': case 'M':
            argc--; argv++;
            sscanf(argv[0],"%d",&gMaxWordLen);
            break;
         case 'g': case 'G':
            argc--; argv++;
            sscanf(argv[0],"%d",&gGridSize);
            break;
         case 's': case 'S':
            gSolution = TRUE;
            break;
         case '?': case 'h': case 'H':
            Usage();
            exit(0);
         case 'n': case 'N':
            gWordList = FALSE;
            break;
         case 'p': case 'P':
            gOutput = STYLE_PS;
            break;
         case 'l': case 'L':
            gOutput = STYLE_LATEX;
            break;
         case 'a': case 'A':
            gOutput = STYLE_ASCII;
            break;
         case 'f': case 'F':
            argc--; argv++;
            sscanf(argv[0],"%d",&gFontSize);
            if(gFontSize < 1 || gFontSize > 48) gFontSize = 18;
            break;
         default:
            fprintf(stderr,"Unknown switch: %s (ignored)\n",argv[0]);
            break;
         }
      }
      else                    /* Handle file specifications             */
      {
         switch(argc)
         {
         case 2:
            strcpy(outfile,argv[1]);
            /* Fall through                                             */
         case 1:
            strcpy(infile,argv[0]);
            break;
         default:
            return(FALSE);
         }
         
         return(TRUE);
      }
      argc--; argv++;
   }
   return(TRUE);
}

/************************************************************************/
/*>BOOL BuildArrays(void)
   ----------------------
   Assign memory for grid and words array. Returns FALSE if memory
   allocation failed.
   
   13.01.94 Original    By: ACRM
*/
BOOL BuildArrays(void)
{
   int i,
       j;
   
   /* Build the grid and fill with spaces                               */
   if((gGrid = (char **)malloc(gGridSize * sizeof(char *)))==NULL)
      return(FALSE);

   for(i=0; i<gGridSize; i++)
   {
      gGrid[i] = (char *)malloc((gGridSize+1) * sizeof(char));
      if(gGrid[i] == NULL) return(FALSE);
      
      for(j=0; j<gGridSize; j++)
         gGrid[i][j] = ' ';
      gGrid[i][gGridSize] = '\0';
   }
   
   /* Build the words array                                             */
   if((gWords = (char **)malloc(gMaxWords * sizeof(char *)))==NULL)
      return(FALSE);

   for(i=0; i<gMaxWords; i++)
   {
      gWords[i] = (char *)malloc((gMaxWordLen+1) * sizeof(char));
      if(gWords[i] == NULL) return(FALSE);
      
      gWords[i][0] = '\0';
   }
   
   return(TRUE);
}

/************************************************************************/
/*>BOOL OpenFiles(char *infile, char *outfile)
   -------------------------------------------
   Open the files of specified. Otherwise assume stdin/stdout.
   Returns FALSE if unable to open a file.
   
   13.01.94 Original    By: ACRM
*/
BOOL OpenFiles(char *infile, char *outfile)
{
   gIn   = stdin;
   gOut  = stdout;
   
   if(infile[0])
   {
      if((gIn = fopen(infile,"r"))==NULL)
      {
         fprintf(stderr,"Unable to open input file: %s\n",infile);
         return(FALSE);
      }
   }
   
   if(outfile[0])
   {
      if((gOut = fopen(outfile,"w+"))==NULL)
      {
         fprintf(stderr,"Unable to open output file: %s\n",outfile);
         return(FALSE);
      }
   }
   
   return(TRUE);
}

/************************************************************************/
/*>int ReadInputData(void)
   -----------------------
   Read words from the input file. Returns the number of words given.
   
   13.01.94 Original    By: ACRM
   14.01.94 Terminates word in word list
*/
int ReadInputData(void)
{
   char  buffer[MAXBUFF],
         *ptr;
   int   NWords = 0;
   
   while(fgets(buffer,MAXBUFF-1,gIn))
   {
      TERMINATE(buffer);
      UPPER(buffer);
      
      /* Remove any leading spaces                                      */
      for(ptr=buffer; (*ptr==' ' || *ptr=='\t'); ptr++);
      
      /* Return if it was a blank line                                  */
      if(!strlen(ptr)) return(NWords);
      
      /* Copy the string into the words array                           */
      strncpy(gWords[NWords], ptr, gMaxWordLen);
      gWords[NWords][gMaxWordLen] = '\0';
            
      if(++NWords >= gMaxWords) return(NWords);
   }
   
   return(NWords);
}

/************************************************************************/
/*>void FillSpaces(void)
   ---------------------
   Fill in spaces in the grid with random letters
   
   13.01.94 Original    By: ACRM
*/
void FillSpaces(void)
{
   int i, j;
   
   for(i=0; i<gGridSize; i++)
   {
      for(j=0; j<gGridSize; j++)
      {
         if(gGrid[i][j] == ' ')
         {
            /* If it's a space, put in a random letter                  */
            gGrid[i][j] = (char)(65 + RandomNum(26));
         }
      }
   }
}

/************************************************************************/
/*>int RandomNum(int maxran)
   -------------------------
   Return a random integer between 0 and maxran-1
   
   13.01.94 Original    By: ACRM
*/
int RandomNum(int maxran)
{
   float frac;
   int   rnum;
   
   frac = (float)rand() / RAND_MAX;
   rnum = (int)(maxran * frac);
   
   return(rnum);
}

/************************************************************************/
/*>BOOL FitWords(int NWords)
   -------------------------
   Place words in the grid
   
   13.01.94 Original    By: ACRM
*/
BOOL FitWords(int NWords)
{
   int i;
   
   SortByLength(gWords, NWords);
   
   for(i=0; i<NWords; i++)
      if(!PlaceWord(gWords[i])) return(FALSE);
   
   return(TRUE);
}

/************************************************************************/
/*>void Usage(void)
   ----------------
   Print a usage message.
   
   13.01.94 Original    By: ACRM
   14.01.94 Added n,p,l,a and f switches
*/
void Usage(void)
{
   fprintf(stderr,"WordSearch V1.2 (c) 1994-2001, Dr. Andrew C. R. \
Martin, SciTech Software\n");
   fprintf(stderr,"Usage: wordsearch [-w maxwords] [-m maxwordlen] \
[-g gridsize]\n");
   fprintf(stderr,"                  [-s] [-h] [-n] [-p] [-l] [-a] \
[infile] [outfile]\n\n");
   fprintf(stderr,"       -w      Max words (Default: %d)\n",MAXWORDS);
   fprintf(stderr,"       -m      Max word length (Default: %d)\n",
           MAXWORDLEN);
   fprintf(stderr,"       -g      Grid size (Default: %d)\n",GRIDSIZE);
   fprintf(stderr,"       -s      Output solution\n");
   fprintf(stderr,"       -n      Do not output word list\n");
   fprintf(stderr,"       -p      Postscript output (default)\n");
   fprintf(stderr,"       -l      LaTeX output\n");
   fprintf(stderr,"       -a      ASCII output\n");
   fprintf(stderr,"       -f      PostScript font size (Default: 18)\n");

   fprintf(stderr,"       -h      This help message\n");
   fprintf(stderr,"       infile  Optional input file\n");
   fprintf(stderr,"       outfile Optional output file\n\n");
   
   fprintf(stderr,"WordSearch takes a list of words from standard \
input or\n");
   fprintf(stderr,"infile if specified and creates a word search \
grid.\n");
   fprintf(stderr,"Output which is in PostScript format by default, \
goes\n");
   fprintf(stderr,"to standard output or to outfile if specified.\n");
}

/************************************************************************/
/*>BOOL PlaceWord(char *word)
   --------------------------
   Place a word in the grid. Returns FALSE if unable to do so.

   13.01.94 Original    By: ACRM
   12.07.01 Fixed bug in selecting random numbers on diagonals.
*/
BOOL PlaceWord(char *word)
{
   int   try, 
         i, 
         x, y, 
         direction, 
         len, 
         xstep, ystep;
   BOOL  ok;
   
   len = strlen(word);
   
   
   for(try=0; try<MAXTRY; try++)
   {
      /* Select the direction (0: horiz, 1: vert, 2: diagonal)          */
      direction = RandomNum(3);
      
      /* Select the start point                                         */
      switch(direction)
      {
      case 0:
         x = RandomNum(gGridSize-len);
         y = RandomNum(gGridSize);
         xstep = 1; ystep = 0;
         break;
      case 1:
         x = RandomNum(gGridSize);
         y = RandomNum(gGridSize-len);
         xstep = 0; ystep = 1;
         break;
      case 2:
         x = RandomNum(gGridSize-len);
         y = RandomNum(gGridSize-len);
         xstep = 1; ystep = 1;
         break;
      }
      
      /* See if the word fits                                           */
      ok = TRUE;
      for(i=0; i<strlen(word); i++)
      {
         char ch;
         
         ch = gGrid[y + i*ystep][x + i*xstep];
         
         if(ch != ' ' && ch != word[i])
         {
            ok = FALSE;
            break;
         }
      }
      
      if(ok)
      {
         /* Put in the word                                             */
         for(i=0; i<strlen(word); i++)
            gGrid[y + i*ystep][x + i*xstep] = word[i];
         
         return(TRUE);
      }
   }
   
   return(FALSE);
}

/************************************************************************/
/*>void PrintSolution(void)
   ------------------------
   Print the grid with only the specified words (no random letters)
   
   13.01.94 Original    By: ACRM
   14.01.94 Changed to call DoASCIIOutput()
*/
void PrintSolution(void)
{
   switch(gOutput)
   {
   case STYLE_ASCII:
      DoASCIIOutput(gGrid, gGridSize, gWordList, 0, TRUE);
      break;
   case STYLE_PS:
      DoPSOutput(gGrid, gGridSize, gWordList, 0, TRUE);
      break;
   case STYLE_LATEX:
      DoLaTeXOutput(gGrid, gGridSize, gWordList, 0, TRUE);
      break;
   }
}

/************************************************************************/
/*>void PrintPuzzle(int NWords)
   ----------------------------
   Print the complete grid with the words for which to search.
   
   13.01.94 Original    By: ACRM
   14.01.94 Changed to call DoASCIIOutput()
*/
void PrintPuzzle(int NWords)
{
   switch(gOutput)
   {
   case STYLE_ASCII:
      DoASCIIOutput(gGrid, gGridSize, gWordList, NWords, FALSE);
      break;
   case STYLE_PS:
      DoPSOutput(gGrid, gGridSize, gWordList, NWords, FALSE);
      break;
   case STYLE_LATEX:
      DoLaTeXOutput(gGrid, gGridSize, gWordList, NWords, FALSE);
      break;
   }
}

/************************************************************************/
/*>void InitOutput(int style)
   --------------------------
   Initialise an output file (ASCII, PostScript or LaTeX)

   14.01.94 Original    By: ACRM
   11.07.01 Outputs PostScript header
*/
void InitOutput(int style)
{
   switch(style)
   {
   case STYLE_ASCII:
      break;
   case STYLE_PS:
      /* Print the PostScript header                                    */
      fprintf(gOut,"%%!PS_Adobe-2.0\n");
      fprintf(gOut,"%%%%Creator: WordSearch 1.2 (c) 1994-2001 \
Andrew C.R. Martin\n");
      fprintf(gOut,"%%%%EndComments\n");
   

      fprintf(gOut,"/max\n");
      fprintf(gOut,"%% n1 n2...max...n\n");
      fprintf(gOut,"{  2 copy\n");
      fprintf(gOut,"   lt { exch } if\n");
      fprintf(gOut,"   pop\n");
      fprintf(gOut,"}  def\n\n");

      fprintf(gOut,"/Helvetica-Bold findfont %d scalefont setfont\n\n",
              gFontSize);

      fprintf(gOut,"/size (W)  stringwidth pop\n");
      fprintf(gOut,"      (\\() stringwidth exch pop\n");
      fprintf(gOut,"      max 4 add def\n\n");
      
      fprintf(gOut,"/xstart  72 def\n");
      fprintf(gOut,"/ystart 720 def\n");
      
      fprintf(gOut,"/xpos xstart def\n");
      fprintf(gOut,"/ypos ystart def\n");

      fprintf(gOut,"%%%%EndProlog\n\n");
      fprintf(gOut,"%%%%Page: 1 1\n");
      break;
   case STYLE_LATEX:
      fprintf(gOut,"\\documentstyle[12pt,a4]{article}\n");
         
      fprintf(gOut,"\\oddsidemargin -0.3 in\n");
      fprintf(gOut,"\\evensidemargin -0.3 in\n");
      fprintf(gOut,"\\marginparwidth 0.75 in\n");
      fprintf(gOut,"\\textwidth 7.0 true in\n");
      
      fprintf(gOut,"\\pagestyle{empty}\n");
      fprintf(gOut,"\\newcommand{\\s}[1]{\\makebox[1.5em]{#1}}\n");
      fprintf(gOut,"\\begin{document}\n");
      fprintf(gOut,"\\Large\n");
      break;
   }
}

/************************************************************************/
/*>void EndOutput(int style)
   -------------------------
   Do any tidying up to end a file

   14.01.94 Original    By: ACRM
*/
void EndOutput(int style)
{
   switch(style)
   {
   case STYLE_ASCII:
      break;
   case STYLE_PS:
      fprintf(gOut,"showpage\n");
      break;
   case STYLE_LATEX:
      fprintf(gOut,"\\end{document}\n");
      break;
   }
}

/************************************************************************/
/*>void DoPSOutput(char **grid, int gridsize, BOOL WordList, 
                   int NWords, BOOL solution)
   ---------------------------------------------------------
   Create PostScript output.
   Input:   char  **grid      The character grid
            int   gridsize    The size of the grid
            BOOL  WordList    Should be display the word list if this
                              isn't the solution display
            int   NWords      Number of words in the word list
            BOOL  solution    Is this a solution display

   14.01.94 Original    By: ACRM
   11.07.01 Outputs PostScript page count
*/
void DoPSOutput(char **grid, int gridsize, BOOL WordList, 
                int NWords, BOOL solution)
{
   int   i, j,
         FontSize;
   

   if(solution)
   {
      fprintf(gOut,"xpos ypos moveto (Solution:) show\n");
      fprintf(gOut,"/ypos ypos size 2 mul sub def\n");
   }
   
   for(i=0; i<gridsize; i++)
   {
      for(j=0; j<gridsize; j++)
      {
         fprintf(gOut,"xpos ypos moveto (%c) show \
/xpos xpos size add def\n",grid[i][j]);
      }
      fprintf(gOut,"/ypos ypos size sub def\n");
      fprintf(gOut,"/xpos xstart def\n");
   }

   if(solution)            /* Finish this page and start another        */
   {
      fprintf(gOut,"showpage\n\n");
      fprintf(gOut,"%%%%Page: 2 2\n");
      fprintf(gOut,"/xpos xstart def\n");
      fprintf(gOut,"/ypos ystart def\n");
   }
   else                    /* Print the word list                       */
   {
      /* Leave a blank line                                             */
      fprintf(gOut,"/ypos ypos size sub def\n");

      /* Reset the font size                                            */
      FontSize = (gFontSize >= 12) ? gFontSize - 2 : gFontSize;
      fprintf(gOut,"/Helvetica-Bold findfont %d scalefont setfont\n\n",
              FontSize);
       
      if(WordList)
      {       
         /* Display the word list, 3 to a line                          */
         for(i=0; i<NWords; i+=3)
         {
            fprintf(gOut,"/xpos xstart def\n");
            for(j=0; j<3; j++)
            {
               if(i+j < NWords)
               {
                  fprintf(gOut,"xpos ypos moveto (%s) show\n",
                          gWords[i+j]);
                  fprintf(gOut,"/xpos xpos 175 add def\n");
               }
            }
            fprintf(gOut,"/ypos ypos size sub def\n");
         }
      }
   }
}

/************************************************************************/
/*>void DoASCIIOutput(char **grid, int gridsize, BOOL WordList, 
                      int NWords, BOOL solution)
   ------------------------------------------------------------
   Create ASCII output.
   Input:   char  **grid      The character grid
            int   gridsize    The size of the grid
            BOOL  WordList    Should be display the word list if this
                              isn't the solution display
            int   NWords      Number of words in the word list
            BOOL  solution    Is this a solution display

   14.01.94 Original    By: ACRM
*/
void DoASCIIOutput(char **grid, int gridsize, BOOL WordList, 
                   int NWords, BOOL solution)
{
   int   i, j, k;
   
   if(solution)
      fprintf(gOut,"Solution:\n");

   for(i=0; i<gridsize; i++)
      fprintf(gOut,"%s\n",grid[i]);
   fprintf(gOut,"\n");

   if(!solution)           /* Print the word list                       */
   {
      if(WordList)
      {
         char buffer[88];
         
         /* Display the word list, 3 to a line                          */
         for(i=0; i<NWords; i+=3)
         {
            for(j=0; j<80; j++) buffer[j] = ' ';
            buffer[80] = '\0';
         
            for(j=0; j<3; j++)
            {
               if(i+j < NWords)
               {
                  for(k=0; k<strlen(gWords[i+j]); k++)
                     buffer[j*26 + k] = gWords[i+j][k];
               }
            }
            fprintf(gOut,"%s\n",buffer);
         }
      }
   }
}

/************************************************************************/
/*>void DoLaTeXOutput(char **grid, int gridsize, BOOL WordList, 
                      int NWords, BOOL solution)
   ------------------------------------------------------------
   Create LaTeX output.
   Input:   char  **grid      The character grid
            int   gridsize    The size of the grid
            BOOL  WordList    Should be display the word list if this
                              isn't the solution display
            int   NWords      Number of words in the word list
            BOOL  solution    Is this a solution display

   14.01.94 Original    By: ACRM
*/
void DoLaTeXOutput(char **grid, int gridsize, BOOL WordList, 
                   int NWords, BOOL solution)
{
   int   i, j;
   
   if(solution)
   {
      fprintf(gOut,"\\noindent Solution:\n\n");
      fprintf(gOut,"\\vspace{2em}\n\n");
   }

   fprintf(gOut,"\\begin{center}\n");
   for(i=0; i<gridsize; i++)
   {
      for(j=0; j<gridsize; j++)
         fprintf(gOut,"\\s{%c}",grid[i][j]);
      fprintf(gOut,"\n\n");
   }
   fprintf(gOut,"\\end{center}\n");

   if(solution)
   {
      fprintf(gOut,"\\newpage\n");
   }
   else                    /* Print the word list                       */
   {
      if(WordList)
      {
         /* Display the word list, 3 to a line                          */
         fprintf(gOut,"\\vspace{2em}\n");
         fprintf(gOut,"\\begin{center}\n");
         fprintf(gOut,"\\begin{tabular}{lll}\n");
            
         for(i=0; i<NWords; i+=3)
         {
            for(j=0; j<3; j++)
            {
               if(i+j < NWords)
                  fprintf(gOut,"%s ",gWords[i+j]);
               else
                  fprintf(gOut," ");
                  
               fprintf(gOut,"%s",((j<2) ? "&" : "\\\\\n"));
            }
         }

         fprintf(gOut,"\\end{tabular}\n");
         fprintf(gOut,"\\end{center}\n");
      }
   }
}

/************************************************************************/
/*>void SortByLength(char **Words, int NWords)
   -------------------------------------------
   Sort strings in an array by length (longest first).
   
   13.01.94 Framework
*/
void SortByLength(char **Words, int NWords)
{
}

