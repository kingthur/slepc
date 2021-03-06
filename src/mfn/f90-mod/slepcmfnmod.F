!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!  SLEPc - Scalable Library for Eigenvalue Problem Computations
!  Copyright (c) 2002-2016, Universitat Politecnica de Valencia, Spain
!
!  This file is part of SLEPc.
!
!  SLEPc is free software: you can redistribute it and/or modify it under  the
!  terms of version 3 of the GNU Lesser General Public License as published by
!  the Free Software Foundation.
!
!  SLEPc  is  distributed in the hope that it will be useful, but WITHOUT  ANY
!  WARRANTY;  without even the implied warranty of MERCHANTABILITY or  FITNESS
!  FOR  A  PARTICULAR PURPOSE. See the GNU Lesser General Public  License  for
!  more details.
!
!  You  should have received a copy of the GNU Lesser General  Public  License
!  along with SLEPc. If not, see <http://www.gnu.org/licenses/>.
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        module slepcmfndef
        use slepcbvdef
        use slepcfndef
        use petscvecdef
        use petscmatdef
#include <../src/mfn/f90-mod/slepcmfn.h>
        end module

        module slepcmfn
        use slepcmfndef
        use slepcbv
        use slepcfn
        use petscvec
        use petscmat
#include <../src/mfn/f90-mod/slepcmfn.h90>
        interface
#include <../src/mfn/f90-mod/ftn-auto-interfaces/slepcmfn.h90>
        end interface
        end module

