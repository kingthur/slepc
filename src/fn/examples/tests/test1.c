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

static char help[] = "Test rational function.\n\n";

#include <slepcfn.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  FN             fn;
  PetscInt       na,nb;
  PetscScalar    x,y,yp,p[10],q[10],five=5.0;

  SlepcInitialize(&argc,&argv,(char*)0,help);
  ierr = FNCreate(PETSC_COMM_WORLD,&fn);CHKERRQ(ierr);

  /* polynomial p(x) */
  na = 5;
  p[0] = -3.1; p[1] = 1.1; p[2] = 1.0; p[3] = -2.0; p[4] = 3.5;
  ierr = FNSetType(fn,FNRATIONAL);CHKERRQ(ierr);
  ierr = FNSetParameters(fn,na,p,0,NULL);CHKERRQ(ierr);
  ierr = FNView(fn,NULL);CHKERRQ(ierr);
  x = 2.2;
  ierr = FNEvaluateFunction(fn,x,&y);CHKERRQ(ierr);
  ierr = FNEvaluateDerivative(fn,x,&yp);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f(%3.1F)=%G\n",x,y);CHKERRQ(ierr); 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f'(%3.1F)=%G\n",x,yp);CHKERRQ(ierr); 

  /* inverse of polynomial 1/q(x) */
  nb = 3;
  q[0] = -3.1; q[1] = 1.1; q[2] = 1.0;
  ierr = FNSetType(fn,FNRATIONAL);CHKERRQ(ierr);
  ierr = FNSetParameters(fn,0,NULL,nb,q);CHKERRQ(ierr);
  ierr = FNView(fn,NULL);CHKERRQ(ierr);
  x = 2.2;
  ierr = FNEvaluateFunction(fn,x,&y);CHKERRQ(ierr);
  ierr = FNEvaluateDerivative(fn,x,&yp);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f(%3.1F)=%G\n",x,y);CHKERRQ(ierr); 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f'(%3.1F)=%G\n",x,yp);CHKERRQ(ierr); 

  /* rational p(x)/q(x) */
  na = 2; nb = 3;
  p[0] = -3.1; p[1] = 1.1;
  q[0] = 1.0; q[1] = -2.0; q[2] = 3.5;
  ierr = FNSetType(fn,FNRATIONAL);CHKERRQ(ierr);
  ierr = FNSetParameters(fn,na,p,nb,q);CHKERRQ(ierr);
  ierr = FNView(fn,NULL);CHKERRQ(ierr);
  x = 2.2;
  ierr = FNEvaluateFunction(fn,x,&y);CHKERRQ(ierr);
  ierr = FNEvaluateDerivative(fn,x,&yp);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f(%3.1F)=%G\n",x,y);CHKERRQ(ierr); 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  f'(%3.1F)=%G\n",x,yp);CHKERRQ(ierr); 

  /* constant */
  ierr = FNSetType(fn,FNRATIONAL);CHKERRQ(ierr);
  ierr = FNSetParameters(fn,1,&five,0,NULL);CHKERRQ(ierr);
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