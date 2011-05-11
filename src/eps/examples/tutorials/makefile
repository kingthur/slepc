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
LOCDIR     = src/eps/examples/tutorials/
EXAMPLESC  = ex1.c ex2.c ex3.c ex4.c ex5.c ex7.c ex9.c ex11.c ex12.c ex13.c ex18.c
EXAMPLESF  = ex1f.F ex1f90.F90 ex6f.F
MANSEC     = EPS

TESTEXAMPLES_C       = ex1.PETSc runex1_1 ex1.rm \
                       ex2.PETSc runex2_1 ex2.rm \
                       ex3.PETSc runex3_1 ex3.rm \
                       ex4.PETSc runex4_1 ex4.rm \
                       ex5.PETSc runex5_1 ex5.rm \
                       ex7.PETSc runex7_1 ex7.rm \
                       ex9.PETSc runex9_1 ex9.rm \
                       ex11.PETSc runex11_1 ex11.rm \
                       ex12.PETSc runex12_1 ex12.rm \
                       ex13.PETSc runex13_1 ex13.rm \
                       ex18.PETSc runex18_1 ex18.rm
TESTEXAMPLES_FORTRAN = ex1f.PETSc runex1f_1 ex1f.rm \
                       ex6f.PETSc runex6f_1 ex6f.rm
TESTEXAMPLES_F90     = ex1f90.PETSc runex1f90_1 ex1f90.rm

include ${SLEPC_DIR}/conf/slepc_common

ex1: ex1.o chkopts
	-${CLINKER} -o ex1 ex1.o ${SLEPC_LIB}
	${RM} ex1.o

ex1f: ex1f.o chkopts
	-${FLINKER} -o ex1f ex1f.o ${SLEPC_LIB}
	${RM} ex1f.o

ex1f90: ex1f90.o chkopts
	-${FLINKER} -o ex1f90 ex1f90.o ${SLEPC_LIB}
	${RM} ex1f90.o

ex2: ex2.o chkopts
	-${CLINKER} -o ex2 ex2.o ${SLEPC_LIB}
	${RM} ex2.o

ex3: ex3.o chkopts
	-${CLINKER} -o ex3 ex3.o ${SLEPC_LIB}
	${RM} ex3.o

ex4: ex4.o chkopts
	-${CLINKER} -o ex4 ex4.o ${SLEPC_LIB}
	${RM} ex4.o

ex5: ex5.o chkopts
	-${CLINKER} -o ex5 ex5.o ${SLEPC_LIB}
	${RM} ex5.o

ex6f: ex6f.o chkopts
	-${FLINKER} -o ex6f ex6f.o ${SLEPC_LIB}
	${RM} ex6f.o

ex7: ex7.o chkopts
	-${CLINKER} -o ex7 ex7.o ${SLEPC_LIB}
	${RM} ex7.o

ex9: ex9.o chkopts
	-${CLINKER} -o ex9 ex9.o ${SLEPC_LIB}
	${RM} ex9.o

ex11: ex11.o chkopts
	-${CLINKER} -o ex11 ex11.o ${SLEPC_LIB}
	${RM} ex11.o

ex12: ex12.o chkopts
	-${CLINKER} -o ex12 ex12.o ${SLEPC_LIB}
	${RM} ex12.o

ex13: ex13.o chkopts
	-${CLINKER} -o ex13 ex13.o ${SLEPC_LIB}
	${RM} ex13.o

ex18: ex18.o chkopts
	-${CLINKER} -o ex18 ex18.o ${SLEPC_LIB}
	${RM} ex18.o

#------------------------------------------------------------------------------------

runex1_1:
	-@${MPIEXEC} -np 1 ./ex1 > ex1_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex1_1.out ex1_1.tmp) then true; \
	   else echo "Possible problem with ex1_1, diffs above"; fi; \
	   ${RM} -f ex1_1.tmp

testex1: ex1.PETSc
	@if [ "${PETSC_WITH_BATCH}" != "" ]; then \
           echo "Running with batch filesystem; to test run src/examples/ex1 with" ; \
           echo "your systems batch system"; \
	else \
           ${MPIEXEC} -np 1 ./ex1 > ex1_1.tmp 2>&1; \
	   if (${DIFF} output/ex1_1.out ex1_1.tmp > /dev/null 2>&1) then \
           echo "C/C++ example src/examples/ex1 run successfully with 1 MPI process"; \
	   else echo "Possible error running C/C++ src/examples/ex1 with 1 MPI process"; \
           cat ex1_1.tmp; fi; \
           ${RM} -f ex1 ex1_1.tmp; fi

runex1f_1:
	-@${MPIEXEC} -np 1 ./ex1f > ex1f_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex1f_1.out ex1f_1.tmp) then true; \
	   else echo "Possible problem with ex1f_1, diffs above"; fi; \
	   ${RM} -f ex1f_1.tmp

testex1f: ex1f.PETSc
	@if [ "${PETSC_WITH_BATCH}" != "" ]; then \
	   echo "Running with batch filesystem; to test run src/examples/ex1f with" ; \
	   echo "your systems batch system"; \
	else \
           ${MPIEXEC} -np 1 ./ex1f > ex1f_1.tmp 2>&1; \
	   if (${DIFF} output/ex1f_1.out ex1f_1.tmp > /dev/null 2>&1) then \
           echo "Fortran example src/examples/ex1f run successfully with 1 MPI process"; \
	   else echo "Possible error running Fortran src/examples/ex1f with 1 MPI process"; \
           cat ex1f_1.tmp; fi; \
           ${RM} -f ex1f ex1f_1.tmp; fi

runex1f90_1:
	-@${MPIEXEC} -np 1 ./ex1f90 > ex1f90_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex1f90_1.out ex1f90_1.tmp) then true; \
	   else echo "Possible problem with ex1f90_1, diffs above"; fi; \
	   ${RM} -f ex1f90_1.tmp

runex2_1:
	-@${MPIEXEC} -np 1 ./ex2 > ex2_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex2_1.out ex2_1.tmp) then true; \
	   else echo "Possible problem with ex2_1, diffs above"; fi; \
	   ${RM} -f ex2_1.tmp

runex3_1:
	-@${MPIEXEC} -np 1 ./ex3 > ex3_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex3_1.out ex3_1.tmp) then true; \
	   else echo "Possible problem with ex3_1, diffs above"; fi; \
	   ${RM} -f ex3_1.tmp

runex4_1:
	-@${MPIEXEC} -np 1 ./ex4 -file ${SLEPC_DIR}/src/mat/examples/rdb200.petsc > ex4_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex4_1.out ex4_1.tmp) then true; \
	   else echo "Possible problem with ex4_1, diffs above"; fi; \
	   ${RM} -f ex4_1.tmp

runex5_1:
	-@${MPIEXEC} -np 1 ./ex5 -st_shift 1 > ex5_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex5_1.out ex5_1.tmp) then true; \
	   else echo "Possible problem with ex5_1, diffs above"; fi; \
	   ${RM} -f ex5_1.tmp

runex6f_1:
	-@${MPIEXEC} -np 1 ./ex6f -st_shift 1 -eps_max_it 1000 -eps_ncv 12 -eps_tol 1e-5 > ex6f_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex6f_1.out ex6f_1.tmp) then true; \
	   else echo "Possible problem with ex6f_1, diffs above"; fi; \
	   ${RM} -f ex6f_1.tmp

runex7_1:
	-@${MPIEXEC} -np 1 ./ex7 -f1 ${SLEPC_DIR}/src/mat/examples/bfw62a.petsc -f2 ${SLEPC_DIR}/src/mat/examples/bfw62b.petsc > ex7_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex7_1.out ex7_1.tmp) then true; \
	   else echo "Possible problem with ex7_1, diffs above"; fi; \
	   ${RM} -f ex7_1.tmp

runex9_1:
	-@${MPIEXEC} -np 1 ./ex9 > ex9_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex9_1.out ex9_1.tmp) then true; \
	   else echo "Possible problem with ex9_1, diffs above"; fi; \
	   ${RM} -f ex9_1.tmp

runex11_1:
	-@${MPIEXEC} -np 1 ./ex11 > ex11_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex11_1.out ex11_1.tmp) then true; \
	   else echo "Possible problem with ex11_1, diffs above"; fi; \
	   ${RM} -f ex11_1.tmp

runex12_1:
	-@${MPIEXEC} -np 1 ./ex12 -eps_type power -st_shift 1 -eps_nev 2 > ex12_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex12_1.out ex12_1.tmp) then true; \
	   else echo "Possible problem with ex12_1, diffs above"; fi; \
	   ${RM} -f ex12_1.tmp

runex13_1:
	-@${MPIEXEC} -np 1 ./ex13 > ex13_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex13_1.out ex13_1.tmp) then true; \
	   else echo "Possible problem with ex13_1, diffs above"; fi; \
	   ${RM} -f ex13_1.tmp

runex18_1:
	-@${MPIEXEC} -np 1 ./ex18 > ex18_1.tmp 2>&1;	  \
	   if (${DIFF} output/ex18_1.out ex18_1.tmp) then true; \
	   else echo "Possible problem with ex18_1, diffs above"; fi; \
	   ${RM} -f ex18_1.tmp
