#
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  SLEPc - Scalable Library for Eigenvalue Problem Computations
#  Copyright (c) 2002-2010, Universidad Politecnica de Valencia, Spain
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

CFLAGS	   = 
FFLAGS     =
CPPFLAGS   = 
FPPFLAGS   =
LOCDIR     = src/svd/examples/tutorials/
EXAMPLESC  = ex8.c ex14.c ex15.c
EXAMPLESF  = ex15f.F
MANSEC     = SVD

TESTEXAMPLES_C       = ex8.PETSc runex8_1 ex8.rm \
                       ex14.PETSc runex14_1 ex14.rm \
                       ex15.PETSc runex15_1 ex15.rm
TESTEXAMPLES_FORTRAN = ex15f.PETSc runex15f_1 ex15f.rm

include ${SLEPC_DIR}/conf/slepc_common

ex8: ex8.o chkopts
	-${CLINKER} -o ex8 ex8.o ${SLEPC_LIB}
	${RM} ex8.o

ex14: ex14.o chkopts
	-${CLINKER} -o ex14 ex14.o ${SLEPC_LIB}
	${RM} ex14.o

ex15: ex15.o chkopts
	-${CLINKER} -o ex15 ex15.o ${SLEPC_LIB}
	${RM} ex15.o

ex15f: ex15f.o chkopts
	-${FLINKER} -o ex15f ex15f.o ${SLEPC_LIB}
	${RM} ex15f.o

#------------------------------------------------------------------------------------

runex8_1:
	-@${MPIEXEC} -np 1 ./ex8 > ex8_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex8_1.out ex8_1.tmp) then true; \
	   else echo "Possible problem with ex8_1, diffs above"; fi; \
	   ${RM} -f ex8_1.tmp

runex14_1:
	-@${MPIEXEC} -np 1 ./ex14 -file ${SLEPC_DIR}/src/mat/examples/rdb200.petsc > ex14_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex14_1.out ex14_1.tmp) then true; \
	   else echo "Possible problem with ex14_1, diffs above"; fi; \
	   ${RM} -f ex14_1.tmp

runex15_1:
	-@${MPIEXEC} -np 1 ./ex15 > ex15_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex15_1.out ex15_1.tmp) then true; \
	   else echo "Possible problem with ex15_1, diffs above"; fi; \
	   ${RM} -f ex15_1.tmp

runex15f_1:
	-@${MPIEXEC} -np 1 ./ex15f > ex15f_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex15f_1.out ex15f_1.tmp) then true; \
	   else echo "Possible problem with ex15f_1, diffs above"; fi; \
	   ${RM} -f ex15f_1.tmp
