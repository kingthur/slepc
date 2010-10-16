/*
      Spectral transformation module for eigenvalue problems.  

   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2010, Universidad Politecnica de Valencia, Spain

   This file is part of SLEPc.
      
   SLEPc is free software: you can redistribute it and/or modify it under  the
   terms of version 3 of the GNU Lesser General Public License as published by
   the Free Software Foundation.

   SLEPc  is  distributed in the hope that it will be useful, but WITHOUT  ANY 
   WARRANTY;  without even the implied warranty of MERCHANTABILITY or  FITNESS 
   FOR  A  PARTICULAR PURPOSE. See the GNU Lesser General Public  License  for 
   more details.

   You  should have received a copy of the GNU Lesser General  Public  License
   along with SLEPc. If not, see <http://www.gnu.org/licenses/>.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#if !defined(__SLEPCST_H)
#define __SLEPCST_H
#include "petscksp.h"
PETSC_EXTERN_CXX_BEGIN

extern PetscClassId ST_CLASSID;

/*S
     ST - Abstract SLEPc object that manages spectral transformations.
     This object is accessed only in advanced applications.

   Level: beginner

.seealso:  STCreate(), EPS
S*/
typedef struct _p_ST* ST;

/*E
    STType - String with the name of a SLEPc spectral transformation

   Level: beginner

.seealso: STSetType(), ST
E*/
#define STType      char*
#define STSHELL     "shell"
#define STSHIFT     "shift"
#define STSINVERT   "sinvert"
#define STCAYLEY    "cayley"
#define STFOLD      "fold"
#define STPRECOND   "precond"

EXTERN PetscErrorCode STCreate(MPI_Comm,ST*);
EXTERN PetscErrorCode STDestroy(ST);
EXTERN PetscErrorCode STSetType(ST,const STType);
EXTERN PetscErrorCode STGetType(ST,const STType*);
EXTERN PetscErrorCode STSetOperators(ST,Mat,Mat);
EXTERN PetscErrorCode STGetOperators(ST,Mat*,Mat*);
EXTERN PetscErrorCode STSetUp(ST);
EXTERN PetscErrorCode STSetFromOptions(ST);
EXTERN PetscErrorCode STView(ST,PetscViewer);

EXTERN PetscErrorCode STApply(ST,Vec,Vec);
EXTERN PetscErrorCode STGetBilinearForm(ST,Mat*);
EXTERN PetscErrorCode STApplyTranspose(ST,Vec,Vec);
EXTERN PetscErrorCode STComputeExplicitOperator(ST,Mat*);
EXTERN PetscErrorCode STPostSolve(ST);

EXTERN PetscErrorCode STSetKSP(ST,KSP);
EXTERN PetscErrorCode STGetKSP(ST,KSP*);
EXTERN PetscErrorCode STSetShift(ST,PetscScalar);
EXTERN PetscErrorCode STGetShift(ST,PetscScalar*);
EXTERN PetscErrorCode STSetDefaultShift(ST,PetscScalar);
EXTERN PetscErrorCode STSetBalanceMatrix(ST,Vec);
EXTERN PetscErrorCode STGetBalanceMatrix(ST,Vec*);

EXTERN PetscErrorCode STSetOptionsPrefix(ST,const char*);
EXTERN PetscErrorCode STAppendOptionsPrefix(ST,const char*);
EXTERN PetscErrorCode STGetOptionsPrefix(ST,const char*[]);

EXTERN PetscErrorCode STBackTransform(ST,PetscInt,PetscScalar*,PetscScalar*);

EXTERN PetscErrorCode STCheckNullSpace(ST,PetscInt,const Vec[]);

EXTERN PetscErrorCode STGetOperationCounters(ST,PetscInt*,PetscInt*);
EXTERN PetscErrorCode STResetOperationCounters(ST);

/*E
    STMatMode - determines how to handle the coefficient matrix associated
    to the spectral transformation

    Level: intermediate

.seealso: STSetMatMode(), STGetMatMode()
E*/
typedef enum { ST_MATMODE_COPY,
               ST_MATMODE_INPLACE, 
               ST_MATMODE_SHELL } STMatMode;
EXTERN PetscErrorCode STSetMatMode(ST,STMatMode);
EXTERN PetscErrorCode STGetMatMode(ST,STMatMode*);
EXTERN PetscErrorCode STSetMatStructure(ST,MatStructure);
EXTERN PetscErrorCode STGetMatStructure(ST,MatStructure*);

EXTERN PetscErrorCode STRegister(const char*,const char*,const char*,PetscErrorCode(*)(ST));
#if defined(PETSC_USE_DYNAMIC_LIBRARIES)
#define STRegisterDynamic(a,b,c,d) STRegister(a,b,c,0)
#else
#define STRegisterDynamic(a,b,c,d) STRegister(a,b,c,d)
#endif
EXTERN PetscErrorCode STRegisterDestroy(void);

/* --------- options specific to particular spectral transformations-------- */

EXTERN PetscErrorCode STShellGetContext(ST st,void **ctx);
EXTERN PetscErrorCode STShellSetContext(ST st,void *ctx);
EXTERN PetscErrorCode STShellSetApply(ST st,PetscErrorCode (*apply)(ST,Vec,Vec));
EXTERN PetscErrorCode STShellSetApplyTranspose(ST st,PetscErrorCode (*applytrans)(ST,Vec,Vec));
EXTERN PetscErrorCode STShellSetBackTransform(ST st,PetscErrorCode (*backtr)(ST,PetscInt,PetscScalar*,PetscScalar*));
EXTERN PetscErrorCode STShellSetName(ST,const char[]);
EXTERN PetscErrorCode STShellGetName(ST,char*[]);

EXTERN PetscErrorCode STCayleySetAntishift(ST,PetscScalar);

EXTERN PetscErrorCode STPrecondGetMatForPC(ST st,Mat *mat);
EXTERN PetscErrorCode STPrecondSetMatForPC(ST st,Mat mat);
EXTERN PetscErrorCode STPrecondGetKSPHasMat(ST st,PetscTruth *setmat);
EXTERN PetscErrorCode STPrecondSetKSPHasMat(ST st,PetscTruth setmat);

PETSC_EXTERN_CXX_END
#endif

