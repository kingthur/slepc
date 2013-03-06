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

static char help[] = "Test exponential function.\n\n";

#include <slepcfn.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  FN             fn;
  PetscScalar    x,y,yp,tau=-0.2,eta=1.3;

  SlepcInitialize(&argc,&argv,(char*)0,help);
  ierr = FNCreate(PETSC_COMM_WORLD,&fn);CHKERRQ(ierr);

  /* plain exponential exp(x) */
  ierr = FNSetType(fn,FNEXP);CHKERRQ(ierr);
  ierr = FNView(fn,NULL);CHKERRQ(ierr);
  x = 2.2;
  ierr = FNEvaluateFunction(fn,x,&y);CHKERRQ(ierr);
  ierr = FNEvaluateDerivative(fn,x,&yp);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f(%3.1F)=%G\n",x,y);CHKERRQ(ierr); 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f'(%3.1F)=%G\n",x,yp);CHKERRQ(ierr); 

  /* exponential with scaling factors eta*exp(tau*x) */
  ierr = FNSetType(fn,FNEXP);CHKERRQ(ierr);
  ierr = FNSetParameters(fn,1,&tau,1,&eta);CHKERRQ(ierr);
  ierr = FNView(fn,NULL);CHKERRQ(ierr);
  x = 2.2;
  ierr = FNEvaluateFunction(fn,x,&y);CHKERRQ(ierr);
  ierr = FNEvaluateDerivative(fn,x,&yp);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f(%3.1F)=%G\n",x,y);CHKERRQ(ierr); 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f'(%3.1F)=%G\n",x,yp);CHKERRQ(ierr); 

  ierr = FNDestroy(&fn);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}