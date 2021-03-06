C Copyright(C) 1999-2020 National Technology & Engineering Solutions
C of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C NTESS, the U.S. Government retains certain rights in this software.
C 
C See packages/seacas/LICENSE for details

C=======================================================================
      SUBROUTINE FFONOF (IFLD, INTYP, CFIELD, ISON, *)
C=======================================================================
C$Id: ffonof.f,v 1.2 2009/03/25 12:46:02 gdsjaar Exp $
C$Log: ffonof.f,v $
CRevision 1.2  2009/03/25 12:46:02  gdsjaar
CAdd copyright and license notice to all files.
C
CRevision 1.1.1.1  1990/08/14 16:14:36  gdsjaar
CTesting
C
c Revision 1.1  90/08/14  16:14:35  gdsjaar
c Initial revision
c
c Revision 1.1  90/08/09  13:39:26  gdsjaar
c Initial revision
c

C   --*** FFONOF *** (FFLIB) Parse free-field ON/OFF
C   --   Written by Amy Gilkey - revised 02/24/86
C   --
C   --FFONOF parses an on/off option from an input field.  No field is
C   --assumed 'ON'.
C   --
C   --Parameters:
C   --   IFLD - IN/OUT - the index of the current field number, incremented
C   --   INTYP - IN - the input type from the free-field reader
C   --   CFIELD - IN - the input option string
C   --   ISON - OUT - true iff the option is ON, set only if no error
C   --   * - return statement if the field is invalid; message is printed

      INTEGER IFLD
      INTEGER INTYP(*)
      CHARACTER*(*) CFIELD(*)
      LOGICAL ISON

      CHARACTER*4 OPT

      IF (INTYP(IFLD) .EQ. 0) THEN
         OPT = CFIELD(IFLD)
      ELSE IF (INTYP(IFLD) .LE. -1) THEN
         OPT = 'ON'
      ELSE
         OPT = ' '
      END IF
      IF ((OPT(:2) .NE. 'ON') .AND. (OPT(:3) .NE. 'OFF')) THEN
         CALL PRTERR ('CMDERR', 'Expected "ON" or "OFF"')
         GOTO 100
      END IF

      ISON = (OPT(:2) .EQ. 'ON')

      IF (INTYP(IFLD) .GE. -1) IFLD = IFLD + 1
      RETURN

  100 CONTINUE
      IF (INTYP(IFLD) .GE. -1) IFLD = IFLD + 1
      RETURN 1
      END
