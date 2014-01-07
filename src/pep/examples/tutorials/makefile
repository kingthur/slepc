#
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  SLEPc - Scalable Library for Eigenvalue Problem Computations
#  Copyright (c) 2002-2013, Universitat Politecnica de Valencia, Spain
#
#  This file is part of SLEPc.
#
#  SLEPc is free software: you can redistribute it and/or modify it under  the
#  terms of version 3 of the GNU Lesser General Public License as published by
#  the Free Software Foundation.
#
#  SLEPc  is  distributed in the hope that it will be useful, but WITHOUT  ANY
#  WARRANTY;  without even the implied warranty of MERCHANTABILITY or  FITNESS
#  FOR  A  PARTICULAR PURPOSE. See the GNU Lesser General Public  License  for
#  more details.
#
#  You  should have received a copy of the GNU Lesser General  Public  License
#  along with SLEPc. If not, see <http://www.gnu.org/licenses/>.
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#

CFLAGS     =
FFLAGS     =
CPPFLAGS   =
FPPFLAGS   =
LOCDIR     = src/pep/examples/tutorials/
EXAMPLESC  = ex23.c
EXAMPLESF  = 
MANSEC     = PEP

TESTEXAMPLES_C           =
TESTEXAMPLES_C_NOCOMPLEX = ex23.PETSc runex23_1 ex23.rm
TESTEXAMPLES_F90         =

include ${SLEPC_DIR}/conf/slepc_common

ex23: ex23.o chkopts
	-${CLINKER} -o ex23 ex23.o ${SLEPC_LIB}
	${RM} ex23.o

#------------------------------------------------------------------------------------
DATAPATH = ${SLEPC_DIR}/share/slepc/datafiles/matrices

runex23_1:
	-@${MPIEXEC} -np 1 ./ex23 -A ${DATAPATH}/speaker107k.petsc,${DATAPATH}/speaker107c.petsc,${DATAPATH}/speaker107m.petsc -pep_type toar -pep_nev 4 -pep_ncv 20 -pep_tol 1e-9 -pep_terse > ex23_1.tmp 2>&1; \
	   if (${DIFF} output/ex23_1.out ex23_1.tmp) then true; \
	   else echo "Possible problem with ex23_1, diffs above"; fi; \
	   ${RM} -f ex23_1.tmp
