C Copyright(C) 1999-2020 National Technology & Engineering Solutions
C of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C NTESS, the U.S. Government retains certain rights in this software.
C 
C See packages/seacas/LICENSE for details

C $Log: grcaln.f,v $
C Revision 1.2  2009/03/25 12:36:44  gdsjaar
C Add copyright and license notice to all files.
C Permission to assert copyright has been granted; blot is now open source, BSD
C
C Revision 1.1  1994/04/07 20:02:09  gdsjaar
C Initial checkin of ACCESS/graphics/blotII2
C
c Revision 1.2  1990/12/14  08:51:28  gdsjaar
c Added RCS Id and Log to all files
c
C=======================================================================
      SUBROUTINE GRCALN (STRING, LSTR)
C=======================================================================

C   --*** GRCALN *** (GRPLIB) Align software characters
C   --   Written by Amy Gilkey - revised 06/09/87
C   --
C   --GRCALN pads the text string with blanks to make numbers line up
C   --in software characters.  It is known that all numbers, minus and
C   --period have the same character size.  A space is half that size
C   --and is expanded to two spaces.
C   --
C   --Parameters:
C   --   STRING - IN/OUT - the string to be aligned; maximum of 80 characters
C   --      with a maximum of 40 blanks before a number
C   --   LSTR - OUT - the output length of the string
C   --
C   --Common Variables:
C   --   Uses ICURDV, SOFTCH of /GRPCOM/

C   --Routines Called:
C   --   LENSTR - (STRLIB) Find string length

      COMMON /GRPCOC/ DEVNAM(2), DEVCOD(2)
      CHARACTER*3 DEVNAM
      CHARACTER*8 DEVCOD
      COMMON /GRPCOM/ ICURDV, ISHARD, DEVOK(2), TALKOK(2),
     &   NSNAP(2), IFONT(2), SOFTCH(2), AUTOPL(2),
     &   MAXCOL(2), NUMCOL(0:1,2), MAPALT(2), MAPUSE(2)
      LOGICAL ISHARD, DEVOK, TALKOK, SOFTCH, AUTOPL

      CHARACTER*(*) STRING

      CHARACTER*80 TMPSTR
      CHARACTER*40 BLANKS
      CHARACTER CH

      LSTR = LENSTR(STRING)

      IF (.NOT. SOFTCH(ICURDV)) RETURN

      INOBLK = 0
      IBLK = INDEX (STRING, ' ')
  100 CONTINUE
      IF ((IBLK .GT. INOBLK) .AND. (IBLK .LE. LSTR)) THEN

         INOBLK = IBLK
  110    CONTINUE
         INOBLK = INOBLK + 1
         IF (STRING(INOBLK:INOBLK) .EQ. ' ') GOTO 110
         NBLK = INOBLK - IBLK

         CH = STRING(INOBLK:INOBLK)
         IF ((CH .EQ. '-') .OR. (CH .EQ. '.')
     &      .OR. ((CH .GE. '0') .AND. (CH .LE. '9'))) THEN
            BLANKS = ' '
            TMPSTR = STRING(INOBLK:LSTR)
            LSTR = LSTR + NBLK
            STRING(INOBLK:LSTR) = BLANKS(1:NBLK) // TMPSTR
            INOBLK = INOBLK + NBLK
         END IF

         IBLK = INDEX (STRING(INOBLK+1:), ' ') + INOBLK
         GOTO 100
      END IF

      RETURN
      END
