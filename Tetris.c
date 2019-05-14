/* ---------------------------------------------------------------------- */
/* TETRIS                                                                 */
/*                                                                        */
/* Compiler   MicroSoft C 5.0                                             */
/*            Quick     C 1.0                                             */
/*                                                                        */
/* (c) 1988   R.A. van Wier                                               */
/*            Nwe Prinsengr. 60 II                                        */
/*            1018 VT  Amsterdam                                          */
/*                                                                        */
/*	      This program is a clone of the tetris program		  */
/*	      by A. Pajitnov & V. Gerasimov.				  */
/*                                                                        */
/*	      This software may be used, copied and altered		  */
/*	      in terms that the source will always be supplied		  */
/*	      with the program and that the CopyRight remarks		  */
/*	      are not removed or altered.				  */
/*                                                                        */
/*	      Keep FreeWare free of viruses, add sources		  */
/* ---------------------------------------------------------------------- */
/* further changes by Markus Noller 01-91				  */
/*	    - added automatic multi-line capability			  */
/*	    - added selectable Topscore placement with "-m" parameter	  */
/*									  */
/*				    12-91:				  */
/*	    -included MAXCOMM.DLL, now COMM.DLL *OR* MAXCOMM.DLL	  */
/*	     can be used by TETRIS/2 in DOOR mode !			  */
/*									  */
/*				    08-91 :				  */
/*                                                                        */
/*	    -removed a smaller bug in semaphore handling, which did	  */
/*	     arise only in OS/2 1.0 systems, fixed now for ALL systems	  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
/* changes by Markus Noller 11-90 :					  */
/*									  */
/*	    - textes and comments altered to english			  */
/*									  */
/*	    - interface altered for ANSI compatibility			  */
/*									  */
/*	    - program changed to DOOR-Program (for BBS use)		  */
/*									  */
/*	    - program use changed to pure OS/2				  */
/*									  */
/*	    - mouse interface removed					  */
/*									  */
/* compile:    cl -Gs -G2 tetris.c    (for OS/2)			  */
/*									  */
/*------------------------------------------------------------------------*/
/* Aanpassingen door de Wizard of Frobozz : 08-88                         */
/*                                                                        */
/*          - Model teruggebracht tot Small. alleen scherm array hoeft    */
/*            far te zijn. Dit zorgt voor kleinere, en snellere code      */
/*                                                                        */
/*          - OS/2 geimplementeerd. Opmerkingen hierbij:                  */
/*                                                                        */
/*                  * OS/2 versie schrijft ook direct naar scherm. Via    */
/*                    het OS het scherm benaderen is te traag voor de     */
/*                    gewenste snelheid. DUS NIET SWITCHEN !              */
/*                  * Timing problemen maken OS/2 versie sneller dan DOS  */
/*                                                                        */
/*          - Deze source compileren met :                                */
/*                  cl -Ox -DOS2 -G2 -Lp tetris.c    (voor OS/2)          */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSDATETIME
#define INCL_DOSINFOSEG
#define INCL_DOSMODULEMGR
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  DOOR

#ifdef	 DOOR
//#include "comm.h"

typedef SHANDLE  HCOMM;
#define COMMAPI  pascal far

static	 HCOMM	  hcomm;
static	 BOOL	  bDoor = 0;
static	 SHORT	  state = 0;
static	 SHORT	  remain = -1;
static	 SHORT	  start_time;
#endif

static long    reaction_time;
static int     start_level;
static int     level;
static char    name[255];
static char    ThreadStack[2000];
static TID     tid;

#define KEY_END     0x0045
#define KEY_RIGHT   0x4D00
#define KEY_LEFT    0x4B00
#define KEY_DROP    0x0020
#define KEY_ROTATER 0x4800
#define KEY_ROTATEL 0x5000
#define KEY_RANDOM  0x0052
#define KEY_SOUND   0x0042

#define INIT_SHAPE  0
#define DOWN_SHAPE  1
#define RIGHT_SHAPE 2
#define LEFT_SHAPE  3
#define ROT_SHAPEL  4
#define ROT_SHAPER  5


#define BLANK_SCREEN	  0x0700 | ' '

#define CHAR_FULL	   (unsigned char)219
#define COLOUR_BLACK	   0
#define COLOUR_BROWN	   6  // 6
#define COLOUR_GREY	   7
#define COLOUR_GREEN	   10 // 10
#define COLOUR_CYAN	   14 // 11
#define COLOUR_RED	   9  // 12
#define COLOUR_MAGENTA	   13 // 13
#define COLOUR_YELLOW	   11 // 14
#define COLOUR_WHITE	   15
#define COLOUR_SCORE	   75
#define COLOUR_TOPSCORE    30
#define COLOUR_LOGO	   31
#define COLOUR_NEXT	   16

#define COLOUR_BORDER	   3

#define LOG_SIZE_ROW	   20
#define LOG_SIZE_COL	   10

#define MIN_ROW 	   2
#define MAX_ROW 	   (LOG_SIZE_ROW + MIN_ROW)
#define MIN_COL 	   28
#define MAX_COL 	   (MIN_COL + (LOG_SIZE_COL * 2))

#define NUMBER_SHAPES	   7
#define SIZE_OF_SHAPE	   3

#define SCORE_FILE	   "TETRIS.TOP"
#define NUMBER_SCORES	   20

#define INFINITE	   -1L
#define TIMEOUT 	   10000L /* millisewconds */

typedef int		   SHAPE[SIZE_OF_SHAPE][SIZE_OF_SHAPE];
static SHAPE		   shapes[NUMBER_SHAPES + 1];
static SHAPE		   actual_shape;

static int		   shape_points[NUMBER_SHAPES + 1];
static int		   next_shape = -1;
static int		   log_screen [LOG_SIZE_ROW][LOG_SIZE_COL];
static int		   stat_screen[LOG_SIZE_ROW][LOG_SIZE_COL];
static int		   col;
static long		   points;
static unsigned 	   keys;
static int		   beep_on = 1;
static int		   random_on = 0;

static int		   old_row = 0;
static int		   old_column = 0;
static int		   old_colour = 0;

/* --- Semaphores --- */

#define SEM_NAME	   "\\SEM\\TETRIS%d"
#define ERROR_SEM_TIMEOUT  121

static long		   rsKeyGet = 0;
static long		   rsKeyGot = 0;
static HSEM		   semTimeOut = 0;

static MUXSEMLIST	   mslSemList;
static USHORT		   usIndexNr;

static HTIMER		   hTimer;

#ifdef DOOR

/* --- Comm Functions --- */

static PFN		   ComHRegister;
static PFN		   ComClose;
static PFN		   ComPutc;
static PFN		   ComRxWait;
static PFN		   ComGetc;
static PFN		   ComInCount;
static PFN		   ComIsOnline;

/* ---------------------------------------------------------------------- */
/* ComResolve								  */
/* Task   : Load COMM.DLL and resolve APIs				  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
void ComResolve()
{
   char    errortext[80];
   HMODULE hmodComm;

   if( DosLoadModule(errortext, 79, "MAXCOMM", &hmodComm))
   {
      printf("Missing File: %s\n",errortext);
      if( DosLoadModule(errortext, 79, "COMM", &hmodComm))
      {
	  printf("Missing File: %s\n",errortext);
	  exit(1);
      }
   }
   DosGetProcAddr(hmodComm, "COMHREGISTER", &ComHRegister);
   DosGetProcAddr(hmodComm, "COMCLOSE",     &ComClose);
   DosGetProcAddr(hmodComm, "COMPUTC",	    &ComPutc);
   DosGetProcAddr(hmodComm, "COMRXWAIT",    &ComRxWait);
   DosGetProcAddr(hmodComm, "COMGETC",	    &ComGetc);
   DosGetProcAddr(hmodComm, "COMINCOUNT",   &ComInCount);
   DosGetProcAddr(hmodComm, "COMISONLINE",  &ComIsOnline);
}

/* ---------------------------------------------------------------------- */
/* ComPuts								  */
/* Task   : Put String to Port						  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
LONG ComPuts(HCOMM hc, CHAR *s)
{
while (*s != '\0')
   {
   (*ComPutc)(hc, *s);
   s++;
   }
}

/* ---------------------------------------------------------------------- */
/* ComREADs								  */
/* Task   : Get String from Port					  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
SHORT ComReads(HCOMM hc, CHAR *buffer, SHORT length)
{
SHORT	index;

    index = 0;
    do
    {
	do
	{
	    while ((*ComRxWait)(hc, TIMEOUT) == ERROR_SEM_TIMEOUT)
	    {
		if (!(*ComIsOnline)(hc))
		    DosExit(1,2);
	    }
	    buffer[index] = (*ComGetc)(hc) ;
	}
	while ((buffer[index] < ' ' || index >= length)
	       && buffer[index] != '\x0d' && buffer[index] != '\x08') ;
	if (buffer[index] == '\x08')
	{
	    if (index > 0)
	    {
		ComPuts(hc,"\x08 \x08");
		index-- ;
	    }
	}
	else
	{
	    (*ComPutc)(hc, buffer[index]) ;
	    index++ ;
	}
    }
    while (buffer[index-1] != '\x0d') ;
    buffer[index-1] = '\n';
    buffer[index]   = '\0';
    (*ComPutc)(hc,'\x0a');

return index;
}

/* ---------------------------------------------------------------------- */
/* Comm_KEYS								  */
/* Task   : Check if keys are pressed and read them if so		  */
/* ---------------------------------------------------------------------- */
void APIENTRY comm_keys()
{
   while (1)
   {
      while ((*ComRxWait)(hcomm, TIMEOUT) == ERROR_SEM_TIMEOUT)
      {
	 if (!(*ComIsOnline)(hcomm))
	    DosExit(1,2);
      }
      keys = (*ComGetc)(hcomm);

      switch (keys)
      {
	 case '8':	      keys = KEY_ROTATER; break;
	 case '6':	      keys = KEY_RIGHT;   break;
	 case '4':	      keys = KEY_LEFT;	  break;
	 case '2':	      keys = KEY_ROTATEL; break;
	 case '\033':
	    while ((*ComRxWait)(hcomm, TIMEOUT) == ERROR_SEM_TIMEOUT)
	    {
	       if (!(*ComIsOnline)(hcomm))
		  DosExit(1,2);
	    }
	    keys = (*ComGetc)(hcomm);
	    if (keys == '[')
	    {
	       while ((*ComRxWait)(hcomm, TIMEOUT) == ERROR_SEM_TIMEOUT)
	       {
		  if (!(*ComIsOnline)(hcomm))
		     DosExit(1,2);
	       }
	       keys = (*ComGetc)(hcomm);
	       switch (keys)
	       {
		  case 'A':   keys = KEY_ROTATER; break;
		  case 'B':   keys = KEY_ROTATEL; break;
		  case 'C':   keys = KEY_RIGHT;   break;
		  case 'D':   keys = KEY_LEFT;	  break;
		  default :   keys = '\0';
	       }
	    }
	    else
			      keys = '\0';
	    break;
	 default:	      keys = toupper(keys) ;
      }

      if (remain == 0)	      keys = KEY_END;

      if (keys != '\0')
      {
	 DosSemSet   ( (HSEM) &rsKeyGet);
	 DosSemClear ( (HSEM) &rsKeyGot);
	 DosSemWait  ( (HSEM) &rsKeyGet, INFINITE);
      }
   }
}

#endif


/* ---------------------------------------------------------------------- */
/* SYSTIME                                                                */
/* Task   : Returns system time in timer ticks				  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
long systime(int res)
{
    static long far *pGlob = (long far *)0;
    if (pGlob == (long far *)0) {
            unsigned selG, selL;
	    DosGetInfoSeg((PSEL)&selG, (PSEL)&selL);
            pGlob = (long far *)((long)selG << 16);
    }
    return(pGlob[res]);
}

/* ---------------------------------------------------------------------- */
/* READ_KEYS								  */
/* Task   : Check if keys are pressed and read them if so		  */
/* ---------------------------------------------------------------------- */
void APIENTRY read_keys()
{
   while (1)
   {
      keys = getch();
      if ( keys == 0 || keys == 0xE0 )
	 keys = ( getch() << 8);
      else
	 keys = toupper(keys) ;
      DosSemSet   ( (HSEM) &rsKeyGet);
      DosSemClear ( (HSEM) &rsKeyGot);
      DosSemWait  ( (HSEM) &rsKeyGet, INFINITE);
   }
}

/* ---------------------------------------------------------------------- */
/* WAIT_KEYS								  */
/* Task   : Wait until a Key is pressed 				  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void wait_keys()
{
      DosSemWait( (HSEM) &rsKeyGot, INFINITE);
}

/* ---------------------------------------------------------------------- */
/* CLEAR_KDB_BUFFER							  */
/* Task   : Clear Keyboard buffer					  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void clear_kbd_buffer()
{
   if (DosSemWait( (HSEM) &rsKeyGot, 10L) == ERROR_SEM_TIMEOUT)
      return ; /* Nothing in buffer */
#ifdef DOOR
   if (bDoor)
      while ( (*ComInCount)(hcomm) > 0)
	 (void) (*ComGetc)(hcomm);
   else
#endif
      while (  kbhit() )
	 (void) getch();
   DosSemSet  ( (HSEM) &rsKeyGot );
   DosSemClear( (HSEM) &rsKeyGet );
}

/* ---------------------------------------------------------------------- */
/* BEEP                                                                   */
/* Task   : Give a BEEP 						  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void beep()
{
   if (beep_on)
#ifdef DOOR
      if (bDoor)
	 (*ComPutc)(hcomm,'\007');
      else
#endif
	 putch('\007');
}

/* ---------------------------------------------------------------------- */
/* PUT_CHARACTER							  */
/* Task   : Put a character on screen at a given position with		  */
/*	    a given colour						  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void put_character(int row, int column, int colour,
			  unsigned int charac)
{

   char scratch[80];

   if (colour != old_colour)
   {
      sprintf(scratch, "\033[%01d;%02dm",(colour & 8) >>3, (colour &7)+30 );
#ifdef DOOR
      if (bDoor)
	ComPuts(hcomm, scratch);
#endif
      printf(scratch);
   }

   if (row != old_row || column != old_column)
      {
      sprintf(scratch, "\033[%02d;%02dH%c", row + 1, column + 1, charac) ;
#ifdef DOOR
      if (bDoor)
	 ComPuts(hcomm, scratch);
#endif
      printf(scratch);
      }
   else
      {
#ifdef DOOR
      if (bDoor)
	 (*ComPutc)(hcomm, charac);
#endif
      putchar(charac) ;
      }

   old_row    = row ;
   old_column = column + 1 ;
   old_colour = colour ;
}
/* ---------------------------------------------------------------------- */
/* PUT_STRING								  */
/* Task   : Place a string on the screen at a given position and in a	  */
/*	    given colour						  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void put_string(int row, int column, int colour,
		       unsigned char *string)
{

   char scratch[150];

   if (colour != old_colour)
   {
      sprintf(scratch, "\033[%01d;%02dm",(colour & 8) >>3, (colour &7)+30 );
#ifdef DOOR
      if (bDoor)
	ComPuts(hcomm, scratch);
#endif
	printf(scratch);
   }

   if (row != old_row || column != old_column)
      {
      sprintf(scratch, "\033[%02d;%02dH%s", row + 1, column + 1, string) ;
#ifdef DOOR
      if (bDoor)
	 ComPuts(hcomm, scratch);
#endif
      printf(scratch);
      }
   else
      {
#ifdef DOOR
      if (bDoor)
	 ComPuts(hcomm, string);
#endif
      printf(string) ;
      }

   old_row    = row ;
   old_column = column + strlen(string) ;
   old_colour = colour ;
}

/* ---------------------------------------------------------------------- */
/* CLS                                                                    */
/* Task   : Clear Screen						  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void cls()
{
#ifdef DOOR
   if (bDoor)
     ComPuts(hcomm,"\033[2J");
#endif
   printf("\033[2J");
}

/* ---------------------------------------------------------------------- */
/* INIT_SCREEN								  */
/* Task   : Fill the screen with the layout				  */
/*                                                                        */
/* ---------------------------------------------------------------------- */

static void init_screen()
{
   register int   i;
   char 	  scratch[100];
   long 	  now;

   /* Start random funktion */

   now = systime(1);
   srand(now % 32113);

   /* Put the border on the screen */

   cls();

   for ( i =  MIN_ROW	  ; i <= MAX_ROW; i++ )
   {
       put_character(i, MIN_COL-1, COLOUR_BORDER, CHAR_FULL);
       put_character(i, MAX_COL,   COLOUR_BORDER, CHAR_FULL);
   }
   for ( i = (MIN_COL -1) ; i <= MAX_COL; i++ )
       put_character(MAX_ROW, i, COLOUR_BORDER, CHAR_FULL);

#ifdef DOOR
   if (remain > 0)
      sprintf(scratch," T E T R I S / 2   %-20.20s  remaining: mmm (c) 1991, Markus Noller ",name);
   else
#endif
      sprintf(scratch," T E T R I S / 2    hh:mm:ss   %-20.20s     (c) 1991, Markus Noller ",name);
   put_string(0, 0, COLOUR_LOGO, scratch);
   put_string(2, 3, COLOUR_MAGENTA, "Version 1.3");

   put_string(24, 0, COLOUR_LOGO, " keys : cursor LEFT RIGHT, UP/DOWN=rotate, E=exit, R=random, B=beep, SPACE=drop");

}

/* ---------------------------------------------------------------------- */
/* SHOW_TIME								  */
/* Task   : Show the time on the screen 				  */
/*                                                                        */
/* ---------------------------------------------------------------------- */

static void show_time()
{
   char        scratch[20];
#ifdef DOOR
   int	       time;

   if (remain > 0)
   {
      time = systime(0);
      if (time - start_time >= 60)
      {
	 remain--;
	 start_time += 60;
      }
      sprintf(scratch,"%3d", remain);
      put_string(0, 52, COLOUR_LOGO, scratch);
   }
   else
#endif
   {
      _strtime(scratch);
      put_string(0, 20, COLOUR_LOGO, scratch);
   }
}

/* ---------------------------------------------------------------------- */
/* SHOW_STATUS                                                            */
/* Task   : Shows the status of RANDOM					  */
/* ---------------------------------------------------------------------- */

static void show_status()
{
   put_string(16, 66, COLOUR_NEXT | COLOUR_WHITE,"            ");
   if ( random_on )
      put_string(17, 66, COLOUR_NEXT | COLOUR_WHITE,"   RANDOM   ");
   else
      put_string(17, 66, COLOUR_NEXT | COLOUR_WHITE,"            ");

   put_string(18, 66, COLOUR_NEXT | COLOUR_WHITE,"            ");
}

/* ---------------------------------------------------------------------- */
/* SHOW_SCORE                                                             */
/* Task   : Shows the score and the level on the screen 		  */
/*	    and displays the next SHAPE 				  */
/* ---------------------------------------------------------------------- */

static void show_score()
{
   char 	 scratch[80];
   register int  i,j,x;

   put_string(10, 0, COLOUR_SCORE, "             ");
   sprintf(scratch," level  %1d / %1d ",start_level,level);
   put_string(11,0,COLOUR_SCORE,scratch);
   put_string(12,0,COLOUR_SCORE,"             ");
   sprintf(scratch," Score %5ld ",points);
   put_string(13,0,COLOUR_SCORE,scratch);
   put_string(14,0,COLOUR_SCORE,"             ");
   put_string(15,0,COLOUR_SCORE,"             ");

   put_string(10,66,COLOUR_NEXT | COLOUR_GREY,"  following  ");
   put_string(11,66,COLOUR_NEXT 	    ,"            ");
   put_string(12,66,COLOUR_NEXT 	    ,"            ");
   put_string(13,66,COLOUR_NEXT 	    ,"            ");
   put_string(14,66,COLOUR_NEXT 	    ,"            ");
   put_string(15,66,COLOUR_NEXT 	    ,"            ");

   /* Plaats de volgende SHAPE op het screen */

   for ( i = 0; i < 3 ; i++)
   {
      for ( j = 0; j < 3; j++ )
      {
	  x = shapes[next_shape][i][j];
	  if ( x != COLOUR_BLACK )
          {
	     put_character(12+i,69+j+j	,COLOUR_NEXT | x,CHAR_FULL);
	     put_character(12+i,69+j+j+1,COLOUR_NEXT | x,CHAR_FULL);
          }
      }
   }
   show_status();
}
/* ---------------------------------------------------------------------- */
/* SHOW_TOPSCORE                                                          */
/* Task   : Shows the Top score (only one per player)			  */
/*                                                                        */
/* ---------------------------------------------------------------------- */

static void show_topscore(char *pszScore_File)
{
   register int   i,j;
   char 	  scratch[80];
   long 	  scratch_score;
   long 	  scratch_average;
   long 	  scratch_played;

   static struct
	  {
	    int      number;
	    long     score   [NUMBER_SCORES];
	    long     average [NUMBER_SCORES];
	    long     played  [NUMBER_SCORES];
	    char     name    [NUMBER_SCORES]  [22];
	  }
	  score_record;

   FILE       *score_file;

   /* Read in the scores */

   score_file = fopen(pszScore_File,"rb");
   if ( score_file == NULL )
      score_record.number = 0;
   else
   {
      if ( fread(&score_record,sizeof(score_record),1,score_file) < 1)
	 score_record.number = 0;
      fclose(score_file);
   }

   /* Look if the current player is already in the score list  */

   name[21] = '\0';
   strupr(name);
   j  = 99;
   for ( i = 0; i < score_record.number; i++ )
   {
       if ( strcmp(name,score_record.name[i]) == 0 )
          j = i;
   }
   if ( j < 99 )
   {
       /* Calculate the scores (top/average) of the current player */
       /* over the last ten games				   */

       if ( points > score_record.score[j] )
	  score_record.score[j] = points;
       score_record.average[j] =
			 ( ( score_record.average[j] * score_record.played[j] )
			     + points ) / ( score_record.played[j] + 1);
       score_record.played[j] = score_record.played[j] + 1;
       if (score_record.played[j] > 10) score_record.played[j] = 10;
   }
   else
   {
      if ( score_record.number < NUMBER_SCORES )
      {
	 /* Add player to the list */

	 score_record.score  [score_record.number] = points;
	 score_record.average[score_record.number] = points;
	 score_record.played [score_record.number] = 1;
	 strcpy(score_record.name[score_record.number],name);
	 j = score_record.number;
	 score_record.number++;
      }
      else
      {
	 if ( points > score_record.score[NUMBER_SCORES - 1] )
         {
	    /* Replace last player with current */

	    score_record.score	[NUMBER_SCORES - 1] = points;
	    score_record.average[NUMBER_SCORES - 1] = points;
	    score_record.played [NUMBER_SCORES - 1] = 1;
	    strcpy(score_record.name[NUMBER_SCORES - 1],name);
	    j = NUMBER_SCORES - 1;
         }
      }
   }

   if ( j < 99 )
   {
      /* raise current player if he improved */

      while (   ( j > 0 )
            &&  ( score_record.score[j] > score_record.score[j-1] ) )
      {
	 /* Raise player by one */

	 strcpy (scratch,score_record.name[j-1]);
	 scratch_score	 = score_record.score	[j-1];
	 scratch_average = score_record.average [j-1];
	 scratch_played  = score_record.played	[j-1];

	 strcpy(score_record.name[j-1],score_record.name[j]);
	 score_record.score   [j-1] = score_record.score   [j];
	 score_record.average [j-1] = score_record.average [j];
	 score_record.played  [j-1] = score_record.played  [j];

	 strcpy(score_record.name[j],scratch);
	 score_record.score   [j] = scratch_score;
	 score_record.average [j] = scratch_average;
	 score_record.played  [j] = scratch_played;

         j--;
      }
   }

   /* Write scores to disk */

   score_file = fopen(pszScore_File,"wb");
   if ( score_file != NULL )
   {
      fwrite(&score_record,sizeof(score_record),1,score_file);
      fclose(score_file);
   }

   /* Show score on screen */

   for (i = 0; i < LOG_SIZE_ROW; i++)
   {
       if ( i < score_record.number )
	  sprintf(scratch," Top %5ld, average %5ld points %-18.18s ",
                  score_record.score[i],
		  score_record.average[i],score_record.name[i]);
       else
	  strcpy(scratch,"                                                   ");
       put_string(MIN_ROW+i,MIN_COL,COLOUR_TOPSCORE,scratch);
   }
}

/* ---------------------------------------------------------------------- */
/* LOG_DISPLAY                                                            */
/* Task   : Show alteren patterns in logical screen (20x10) on physical   */
/*                                                                        */
/* ---------------------------------------------------------------------- */

static void log_display()
{
   register int     i, j;

   for ( i = 0; i < LOG_SIZE_ROW; i++ )
   {
       for ( j = 0; j < LOG_SIZE_COL; j++ )
       {
	 /* 1 logical point is 2 blocks on screen */

	 if (log_screen[i][j] != stat_screen[i][j])
	 {
	   put_character(MIN_ROW+i,MIN_COL+(j*2),log_screen[i][j],CHAR_FULL);
	   put_character(MIN_ROW+i,MIN_COL+1+(j*2),log_screen[i][j],CHAR_FULL);
	   stat_screen[i][j] = log_screen[i][j];
	 }
       }
   }
}

/* ---------------------------------------------------------------------- */
/* LOG_CLS                                                                */
/* Task   : Clear logical screen					  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void log_cls()
{
   register int 	i, j;

   for ( i = 0 ; i < LOG_SIZE_ROW; i++)
   {
       for ( j = 0; j < LOG_SIZE_COL; j++)
       {
	   log_screen [i][j] = COLOUR_BLACK;
	   stat_screen[i][j] = COLOUR_WHITE;
       }
   }
   log_display();
}

/* ---------------------------------------------------------------------- */
/* RM_FULL_ROW								  */
/* Task   : Remove a full row from the logical screen			  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void rm_full_row()
{
   register int    i, j;
   int		   x, action;

   for ( i = LOG_SIZE_ROW - 1 ; i > 0; i--)
   {
       /* Look if there is a hole in this row */

       action = 1;
       for ( j = 0; action && (j < LOG_SIZE_COL); j++)
       {
	   if (log_screen[i][j] == COLOUR_BLACK) action = 0;
       }

       if ( action )
       {
	  /* Copy the rows above the full row one row down */

          for ( x = i ; x > 0; x--)
          {
	     for ( j = 0; j < LOG_SIZE_COL; j++)
             {
		 log_screen[x][j] = log_screen[x-1][j];
             }
          }
	  /* Make the highest Row all black */

	  for ( j = 0; j < LOG_SIZE_COL; j++)
          {
	      log_screen[0][j] = COLOUR_BLACK;
          }
	  /* Beep and look for this row again */

          beep();
          log_display();
          i++;
       }
   }
}

/* ---------------------------------------------------------------------- */
/* PUT_SHAPE								  */
/* Task   : Put the current shape onto the logical screen and manipulate  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static int put_shape(int action)
					     /* 0 = initial put       */
					     /* 1 = move down	      */
					     /* 2 = right	      */
					     /* 3 = left	      */
					     /* 4 = rotate left       */
					     /* 5 = rotate right      */
{
   register int   i, j;

   static   int   reg, col;
   int		  scratch, collision;

   collision = 1;

   if ( action != INIT_SHAPE )
   {
      /* clean the shape from it's old position */

      for ( i = 0; i < 3; i++ )
      {
         for ( j = 0; j < 3; j++ )
         {
	    if ( actual_shape[i][j] != COLOUR_BLACK )
	       log_screen[reg+i][col+j]   = COLOUR_BLACK;
         }
      }
   }

   switch(action)
   {
      case INIT_SHAPE :  /* initial placing */
               reg = 0;
	       col = LOG_SIZE_COL / 2;
               break;
      case DOWN_SHAPE :  /* move down */
               reg++;
               break;
      case RIGHT_SHAPE : /* move to the right */
	       col++;
               break;
      case LEFT_SHAPE :  /* move to the left */
	       col--;
               break;
      case ROT_SHAPEL :   /* rotate shape left */
	       scratch		  = actual_shape[0][0];
	       actual_shape[0][0] = actual_shape[0][2];
	       actual_shape[0][2] = actual_shape[2][2];
	       actual_shape[2][2] = actual_shape[2][0];
	       actual_shape[2][0] = scratch;

	       scratch		  = actual_shape[1][0];
	       actual_shape[1][0] = actual_shape[0][1];
	       actual_shape[0][1] = actual_shape[1][2];
	       actual_shape[1][2] = actual_shape[2][1];
	       actual_shape[2][1] = scratch;
               break;
      case ROT_SHAPER :   /* rotate shape right */
	       scratch		  = actual_shape[0][0];
	       actual_shape[0][0] = actual_shape[2][0];
	       actual_shape[2][0] = actual_shape[2][2];
	       actual_shape[2][2] = actual_shape[0][2];
	       actual_shape[0][2] = scratch;

	       scratch		  = actual_shape[0][1];
	       actual_shape[0][1] = actual_shape[1][0];
	       actual_shape[1][0] = actual_shape[2][1];
	       actual_shape[2][1] = actual_shape[1][2];
	       actual_shape[1][2] = scratch;
               break;
   }

   /* Look for a collision on the new place */

   for ( i = 0; i < 3; i++ )
   {
      for ( j = 0; j < 3; j++ )
      {
	 if ( actual_shape[i][j] != COLOUR_BLACK )
         {
	    if	( log_screen[reg+i][col+j] != COLOUR_BLACK )
		collision = 0;
	    if ( ( reg+i ) >= LOG_SIZE_ROW ) collision = 0;
	    if ( ( reg+i ) < 0 ) collision = 0;
	    if ( ( col+j ) >= LOG_SIZE_COL ) collision = 0;
	    if ( ( col+j ) < 0 ) collision = 0;
         }
      }
   }

   if ( collision == 0 )
   {
      /* Restore old status */
      switch(action)
      {
	 case DOWN_SHAPE :
                  reg--;
                  break;
	 case RIGHT_SHAPE :
		  col--;
                  break;
	 case LEFT_SHAPE :
		  col++;
                  break;
	 case ROT_SHAPEL :
		  scratch	     = actual_shape[0][0];
		  actual_shape[0][0] = actual_shape[2][0];
		  actual_shape[2][0] = actual_shape[2][2];
		  actual_shape[2][2] = actual_shape[0][2];
		  actual_shape[0][2] = scratch;

		  scratch	     = actual_shape[1][0];
		  actual_shape[1][0] = actual_shape[2][1];
		  actual_shape[2][1] = actual_shape[1][2];
		  actual_shape[1][2] = actual_shape[0][1];
		  actual_shape[0][1] = scratch;
                  break;
	 case ROT_SHAPER :
		  scratch	     = actual_shape[0][0];
		  actual_shape[0][0] = actual_shape[0][2];
		  actual_shape[0][2] = actual_shape[2][2];
		  actual_shape[2][2] = actual_shape[2][0];
		  actual_shape[2][0] = scratch;

		  scratch	     = actual_shape[0][1];
		  actual_shape[0][1] = actual_shape[1][2];
		  actual_shape[1][2] = actual_shape[2][1];
		  actual_shape[2][1] = actual_shape[1][0];
		  actual_shape[1][0] = scratch;
                  break;
      }
   }

   /* Put the shape on it's new position */

   for ( i = 0; i < 3; i++ )
   {
      for ( j = 0; j < 3; j++ )
      {
	 if ( actual_shape[i][j] != COLOUR_BLACK )
	    log_screen[reg+i][col+j]   = actual_shape[i][j];
      }
   }

   if ( collision )
      log_display();

   return(collision);
}

/* ---------------------------------------------------------------------- */
/* INIT_SHAPES								  */
/* Task   : Draw the forms to the form table				  */
/*                                                                        */
/* ---------------------------------------------------------------------- */
static void init_shapes()
{
   register int   i, j;
   int		  colour;

   shape_points[0] = 10;
   colour	   = COLOUR_BROWN;
   shapes[0][0][0] = COLOUR_BLACK;     /*   ..#      */
   shapes[0][0][1] = COLOUR_BLACK;     /*   .##      */
   shapes[0][0][2] = colour;	       /*   .#.      */

   shapes[0][1][0] = COLOUR_BLACK;
   shapes[0][1][1] = colour;
   shapes[0][1][2] = colour;

   shapes[0][2][0] = COLOUR_BLACK;
   shapes[0][2][1] = colour;
   shapes[0][2][2] = COLOUR_BLACK;

   shape_points[1] = 10;
   colour	   = COLOUR_CYAN;
   shapes[1][0][0] = colour ;	       /*   #..      */
   shapes[1][0][1] = COLOUR_BLACK;     /*   ##.      */
   shapes[1][0][2] = COLOUR_BLACK;     /*   .#.      */

   shapes[1][1][0] = colour ;
   shapes[1][1][1] = colour ;
   shapes[1][1][2] = COLOUR_BLACK;

   shapes[1][2][0] = COLOUR_BLACK;
   shapes[1][2][1] = colour ;
   shapes[1][2][2] = COLOUR_BLACK;

   shape_points[2] = 1;
   colour	   = COLOUR_RED;
   shapes[2][0][0] = COLOUR_BLACK;     /*   ...      */
   shapes[2][0][1] = COLOUR_BLACK;     /*   ###      */
   shapes[2][0][2] = COLOUR_BLACK;     /*   ...      */

   shapes[2][1][0] = colour ;
   shapes[2][1][1] = colour ;
   shapes[2][1][2] = colour ;

   shapes[2][2][0] = COLOUR_BLACK;
   shapes[2][2][1] = COLOUR_BLACK;
   shapes[2][2][2] = COLOUR_BLACK;

   shape_points[3] = 7;
   colour	   = COLOUR_MAGENTA;
   shapes[3][0][0] = COLOUR_BLACK;     /*   ...      */
   shapes[3][0][1] = COLOUR_BLACK;     /*   ###      */
   shapes[3][0][2] = COLOUR_BLACK;     /*   ..#      */

   shapes[3][1][0] = colour ;
   shapes[3][1][1] = colour ;
   shapes[3][1][2] = colour ;

   shapes[3][2][0] = COLOUR_BLACK;
   shapes[3][2][1] = COLOUR_BLACK;
   shapes[3][2][2] = colour ;

   shape_points[4] = 7;
   colour	   = COLOUR_YELLOW;
   shapes[4][0][0] = COLOUR_BLACK;     /*   ...      */
   shapes[4][0][1] = COLOUR_BLACK;     /*   ###      */
   shapes[4][0][2] = COLOUR_BLACK;     /*   #..      */

   shapes[4][1][0] = colour ;
   shapes[4][1][1] = colour ;
   shapes[4][1][2] = colour ;

   shapes[4][2][0] = colour ;
   shapes[4][2][1] = COLOUR_BLACK;
   shapes[4][2][2] = COLOUR_BLACK;

   shape_points[5] = 4;
   colour	   = COLOUR_GREY;
   shapes[5][0][0] = COLOUR_BLACK;     /*   ...      */
   shapes[5][0][1] = COLOUR_BLACK;     /*   ##.      */
   shapes[5][0][2] = COLOUR_BLACK;     /*   ##.      */

   shapes[5][1][0] = colour ;
   shapes[5][1][1] = colour ;
   shapes[5][1][2] = COLOUR_BLACK;

   shapes[5][2][0] = colour ;
   shapes[5][2][1] = colour ;
   shapes[5][2][2] = COLOUR_BLACK;

   shape_points[6] = 3;
   colour	   = COLOUR_GREEN;
   shapes[6][0][0] = COLOUR_BLACK;     /*   ...      */
   shapes[6][0][1] = COLOUR_BLACK;     /*   .#.      */
   shapes[6][0][2] = COLOUR_BLACK;     /*   ###      */

   shapes[6][1][0] = COLOUR_BLACK;
   shapes[6][1][1] = colour ;
   shapes[6][1][2] = COLOUR_BLACK;

   shapes[6][2][0] = colour ;
   shapes[6][2][1] = colour ;
   shapes[6][2][2] = colour;

}

/* ---------------------------------------------------------------------- */
/* RANDOM_SHAPE 							  */
/* Task   : Select a Random Shape and store points of it		  */
/*                                                                        */
/* ---------------------------------------------------------------------- */

static void random_shape()
{
   register int   i, j;
   static   int   tel = 0;
   static   long  vpoints = 0;

   if (  ( vpoints > points )
      || ( next_shape < 0 ) )
   {
      vpoints = 0;
      tel = 0;
      next_shape = rand() % NUMBER_SHAPES;
   }

   /* Place the chosen shape in actual_shape */

   for ( i = 0; i < 3 ; i++)
   {
      for ( j = 0; j < 3; j++ )
      {
	  actual_shape[i][j] = shapes[next_shape][i][j];
      }
   }
   points = points + shape_points[next_shape] + start_level;

   tel++;

   if (  ( ( tel % 25 ) == 0 )
      && ( level < 9 ) )
   {
      level++;
      reaction_time = (10 - level) * 60;
      DosSemClear ( semTimeOut) ;
      DosTimerStop (hTimer) ;
      DosSemSet ( semTimeOut) ;
      DosTimerStart (reaction_time, semTimeOut, &hTimer) ;
      beep();
   }

   vpoints = points;
   if (  ( random_on )
      && ( rand() % 2 ) )
   {
      /* Generate a random shape */

      next_shape = NUMBER_SHAPES;
      for ( i = 0; i < 3 ; i++)
      {
         for ( j = 0; j < 3; j++ )
         {
             if ( rand() % 2 )
		shapes[next_shape][i][j] = COLOUR_WHITE;
             else
		shapes[next_shape][i][j] = COLOUR_BLACK;
         }
      }
      shapes[next_shape][1][1] = COLOUR_WHITE;
      shape_points[next_shape] = 50;
   }
   else
      next_shape = rand() % NUMBER_SHAPES;
   show_score();
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/*		     M A I N   P R O G R A M				    */
/*                                                                          */
/* ------------------------------------------------------------------------ */

void main(argc, argv)
int  argc;
char *argv[];
{
   register   int	  i;
   register   int	  drop;
   char 		  scratch[80];
   char 		  pszSemName[15];
   char 		  pszScoreFile[128];

   level  = 99;
   name[0] = '\0';
   strcpy(pszScoreFile,SCORE_FILE);

#ifdef DOOR

   for (i=1; i<argc; i++)
   {
      if ((argv[i][0] == '-') || (argv[i][0] == '/'))
      {
	 switch (argv[i][1])
	 {
	    case 'p':
	       ComResolve();
	       (* ComHRegister) (atoi(&argv[i][2]), (HCOMM far *) &hcomm, 0, 0);
	       bDoor = TRUE;
	       break;

	    case 'n':
	       strcpy(name,&argv[i][2]);
	       while ((i+1 < argc) && (argv[i+1][0] != '-'))
	       {
		  i++;
		  if (name[0] != '\0')
		     strcat(name," ");
		  strcat(name, argv[i]);
	       }
	       break;

	    case 'l':
	       level = atoi(&argv[i][2]);
	       break;

	    case 'r':
	       remain = atoi(&argv[i][2]);
	       start_time = systime(0);
	       break;

	    case 'm':
	       strcpy(pszScoreFile,(&argv[i][2]));
	       break;
	 }
      }
   }
#endif

   /* Ask for players name and skill level */

   while ( name[0] == '\0' )
   {
#ifdef DOOR
      if (bDoor)
      {
	 ComPuts(hcomm, "\nPlease give your name: ");
	 ComReads(hcomm, name, 22);
      }
      else
#endif
      {
	 printf("\nPlease give your name: ");
	 gets(name);
      }
   }
   i = strlen(name) - 1;
   while ( (name[i] == ' ' || name[i] == '\n') && (i > 0) )  i--;
   i++;
   name[i] = '\0';

   while (  ( level < 0 )
	 || ( level > 9 ) )
   {
#ifdef DOOR
      if (bDoor)
      {
	 sprintf(scratch, "\nWhat skill level, %s (0..9)? ",name) ;
	 ComPuts(hcomm, scratch) ;
	 ComReads(hcomm, scratch, 2) ;
	 level = atoi(scratch) ;
      }
      else
#endif
      {
	 printf("\nWhat skill level, %s (0..9)? ",name);
	 scanf("%d",&level);
      }
   }

   start_level = level;
   points = 0;

   /* Initialize the screen */

   init_screen();
   log_cls();
   init_shapes();

   put_string(MIN_ROW,MIN_COL+3,COLOUR_WHITE,"Press a key");

#ifdef DOOR
   if (bDoor)
      DosCreateThread( (PFNTHREAD) comm_keys, &tid, &ThreadStack[1999]);
   else
#endif
      DosCreateThread( (PFNTHREAD) read_keys, &tid, &ThreadStack[1999]);
   clear_kbd_buffer();
   wait_keys();

   put_string(MIN_ROW,MIN_COL+3,COLOUR_WHITE,"           ");

   i=0;
   do
   {
      if (i>99)
	 exit (100);
      sprintf(pszSemName,SEM_NAME, i);
      i++;
   }
   while (DosCreateSem(CSEM_PUBLIC, &semTimeOut, pszSemName) != 0);

   mslSemList.cmxs = 2L;
   mslSemList.amxs[0].zero = 0L;
   mslSemList.amxs[1].zero = 0L;
   mslSemList.amxs[0].hsem = (HSEM) &rsKeyGot;
   mslSemList.amxs[1].hsem = semTimeOut;

   reaction_time = (10 - level) * 60;
   DosSemSet ( (HSEM) semTimeOut) ;
   DosTimerStart (reaction_time, semTimeOut, &hTimer);

   random_shape();

   while (( keys != KEY_END )
	 && ( put_shape(0) ) )	     /* set new shape on screen */
   {
      drop = 0;
      keys = 0;
      DosSemSet  ( (HSEM) &rsKeyGot );
      DosSemClear( (HSEM) &rsKeyGet );

      while (( keys != KEY_END )
	    && ( put_shape(1) ) )
      {
	  /* Process the keys pressed and move the shape down one row */

      show_time();

      while (( !drop )
	 && ( keys != KEY_END ))
	 {

	 DosMuxSemWait(&usIndexNr, &mslSemList, INFINITE);

	 /* Process the keys */

	 if (usIndexNr == 0)
	    {
	    switch(keys)
	       {
	       case KEY_RIGHT	 :
				   put_shape(RIGHT_SHAPE);
				   break;
	       case KEY_LEFT	 :
				   put_shape(LEFT_SHAPE);
				   break;
	       case KEY_DROP	 :
				   i = 0;
				   while( put_shape(DOWN_SHAPE)) i++;
				   points = points + start_level +
				      ((i / LOG_SIZE_ROW) * start_level);
				   drop = 1;
				   break;
	       case KEY_SOUND	 :
				   beep_on = !beep_on;
				   break;
	       case KEY_RANDOM	 :
				   random_on = !random_on;
				   show_status();
				   break;
	       case KEY_ROTATEL  :
				   put_shape(ROT_SHAPEL);
				   break;
	       case KEY_ROTATER  :
				   put_shape(ROT_SHAPER);
				   break;
	       }
	    DosSemSet  ( (HSEM) &rsKeyGot );
	    DosSemClear( (HSEM) &rsKeyGet );
	    }
	 else
	    {
	    DosSemSet ( semTimeOut) ;
	    if (!put_shape(DOWN_SHAPE))
	       break;
	    }
	 }

      }
      /* Remove full rows, clear kbd buffer and get a new shape */

      rm_full_row();
      clear_kbd_buffer();
      random_shape();
   }

   if ( keys != KEY_END )
   {
      /* Show the score until a key is pressed */

      show_topscore(pszScoreFile);
      DosSleep(100L);
      clear_kbd_buffer();
      wait_keys();
   }
   put_string(0,0,COLOUR_GREY," ");
   cls();

#ifdef DOOR
   if (bDoor)
   {
      if (remain == 0)
	 ComPuts(hcomm, "\nTimelimit exceeded, Program ended. Sorry\n");
      (* ComClose) (hcomm);
   }
#endif
   exit(0);
}

