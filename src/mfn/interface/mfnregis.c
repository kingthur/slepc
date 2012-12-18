/*
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2012, Universitat Politecnica de Valencia, Spain

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

#include <slepc-private/mfnimpl.h>  /*I "slepcmfn.h" I*/

EXTERN_C_BEGIN
extern PetscErrorCode MFNCreate_Krylov(MFN);
EXTERN_C_END
  
#undef __FUNCT__  
#define __FUNCT__ "MFNRegisterAll"
/*@C
  MFNRegisterAll - Registers all the matrix functions in the MFN package.

  Not Collective

  Level: advanced

.seealso:  MFNRegisterDynamic()
@*/
PetscErrorCode MFNRegisterAll(const char *path)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  MFNRegisterAllCalled = PETSC_TRUE;
  ierr = MFNRegisterDynamic(MFNKRYLOV,path,"MFNCreate_Krylov",MFNCreate_Krylov);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}