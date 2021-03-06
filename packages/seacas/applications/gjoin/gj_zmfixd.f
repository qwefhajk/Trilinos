C Copyright(C) 1999-2020 National Technology & Engineering Solutions
C of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C NTESS, the U.S. Government retains certain rights in this software.
C 
C See packages/seacas/LICENSE for details

C=======================================================================
      SUBROUTINE ZMFIXD (NELBLK, NUMELB, NUMLNK, LINK, NUMNP, IXNP)
C=======================================================================
C $Id: zmfixd.f,v 1.1 1999/01/18 19:21:27 gdsjaar Exp $
C $Log: zmfixd.f,v $
C Revision 1.1  1999/01/18 19:21:27  gdsjaar
C ExodusII version of gjoin, needs testing and syncing with exodus 1 version, but is being committed to permit easier testing and modifications.  This was created by Dave Fry at Goodyear
C
c Revision 1.1.1.1  1998/11/05  16:23:28  a294617
c Initial import == gjoin 1.36
c
C Revision 1.1.1.1  1990/11/12 14:36:24  gdsjaar
C GJOIN - X1.00.40 - 7/17/90
C
c Revision 1.1  90/11/12  14:36:23  gdsjaar
c Initial revision
c

C   --*** ZMFIXD *** (GJOIN) Find nodes in element blocks
C   --   Written by Amy Gilkey - revised 01/20/88
C   --
C   --ZMFIXD finds the nodes that are within the existing element blocks.
C   --The new indices of the nodes are stored.
C   --
C   --Parameters:
C   --   NELBLK - IN - the number of element blocks
C   --   NUMELB - IN - the number of elements in each element block
C   --   NUMLNK - IN - the number of nodes per element in each element block
C   --   LINK - IN - the connectivity for all elements
C   --   NUMNP - IN/OUT - the number of nodes; returned compressed
C   --   IXNP - OUT - the index of the compressed node; 0 if deleted

      INTEGER NUMELB(*)
      INTEGER NUMLNK(*)
      INTEGER LINK(*)
      INTEGER IXNP(*)

      DO 100 INP = 1, NUMNP
         IXNP(INP) = 0
  100 CONTINUE

      IXL0 = 0
      DO 130 IELB = 1, NELBLK
         DO 120 NE = 1, NUMELB(IELB)
            DO 110 K = 1, NUMLNK(IELB)
               IXNP(LINK(IXL0+K)) = 1
  110       CONTINUE
            IXL0 = IXL0 + NUMLNK(IELB)
  120    CONTINUE
  130 CONTINUE

C   --Index nodes of selected element block within the zoom mesh

      JNP = 0
      DO 140 INP = 1, NUMNP
         IF (IXNP(INP) .GT. 0) THEN
            JNP = JNP + 1
            IXNP(INP) = JNP
         END IF
  140 CONTINUE

      NUMNP = JNP

      RETURN
      END
