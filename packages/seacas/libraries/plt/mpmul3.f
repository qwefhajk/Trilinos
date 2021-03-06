C Copyright(C) 1999-2020 National Technology & Engineering Solutions
C of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C NTESS, the U.S. Government retains certain rights in this software.
C 
C See packages/seacas/LICENSE for details

C $Id: mpmul3.f,v 1.2 1993/07/16 22:11:17 gdsjaar Exp $
C $Log: mpmul3.f,v $
C Revision 1.2  1993/07/16 22:11:17  gdsjaar
C Unrolled do loops to speed up execution.
C
c Revision 1.1  1993/07/16  16:47:19  gdsjaar
c Changed plt to library rather than single source file.
c
C=======================================================================
      SUBROUTINE MPMUL3(N,X0,Y0,Z0,MAT,RES1,RES2,RES3,RES4)
      DIMENSION X0(*),Y0(*),Z0(*),MAT(4,4),RES1(*),RES2(*),RES3(*),
     *          RES4(*)
      REAL MAT

      DO 3100 I = 1,N
        RES1(I) = MAT(1,1)*X0(I) + MAT(2,1)*Y0(I) + MAT(3,1)*Z0(I) +
     *    MAT(4,1)
        RES2(I) = MAT(1,2)*X0(I) + MAT(2,2)*Y0(I) + MAT(3,2)*Z0(I) +
     *    MAT(4,2)
        RES3(I) = MAT(1,3)*X0(I) + MAT(2,3)*Y0(I) + MAT(3,3)*Z0(I) +
     *    MAT(4,3)
        RES4(I) = MAT(1,4)*X0(I) + MAT(2,4)*Y0(I) + MAT(3,4)*Z0(I) +
     *    MAT(4,4)
 3100 CONTINUE
      RETURN

      END
