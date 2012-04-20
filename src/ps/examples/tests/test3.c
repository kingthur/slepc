/*
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2011, Universitat Politecnica de Valencia, Spain

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

static char help[] = "Test PSHEP with compact storage.\n\n";

#include "slepcps.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main( int argc, char **argv )
{
  PetscErrorCode ierr;
  PS             ps;
  PetscReal      *T;
  PetscScalar    *eig;
  PetscInt       i,n=10,l=2,k=5,ld;
  PetscViewer    viewer;
  PetscBool      verbose;

  SlepcInitialize(&argc,&argv,(char*)0,help);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Solve a Projected System of type PSHEP with compact storage - dimension %D.\n",n);CHKERRQ(ierr); 
  ierr = PetscOptionsGetInt(PETSC_NULL,"-l",&l,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-k",&k,PETSC_NULL);CHKERRQ(ierr);
  if (l>n || k>n || l>k) SETERRQ(PETSC_COMM_WORLD,1,"Wrong value of dimensions");
  ierr = PetscOptionsHasName(PETSC_NULL,"-verbose",&verbose);CHKERRQ(ierr);

  /* Create PS object */
  ierr = PSCreate(PETSC_COMM_WORLD,&ps);CHKERRQ(ierr);
  ierr = PSSetType(ps,PSHEP);CHKERRQ(ierr);
  ierr = PSSetFromOptions(ps);CHKERRQ(ierr);
  ld = n+2;  /* test leading dimension larger than n */
  ierr = PSAllocate(ps,ld);CHKERRQ(ierr);
  ierr = PSSetDimensions(ps,n,l,k);CHKERRQ(ierr);

  /* Set up viewer */
  ierr = PetscViewerASCIIGetStdout(PETSC_COMM_WORLD,&viewer);CHKERRQ(ierr);
  ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_INFO_DETAIL);CHKERRQ(ierr);
  ierr = PSView(ps,viewer);CHKERRQ(ierr);
  ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
  if (verbose) { 
    ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
  }

  /* Fill arrow-tridiagonal matrix */
  ierr = PSSetCompact(ps,PETSC_TRUE);CHKERRQ(ierr);
  ierr = PSGetArrayReal(ps,PS_MAT_T,&T);CHKERRQ(ierr);
  for (i=0;i<n;i++) T[i] = (PetscReal)(i+1);
  for (i=l;i<n-1;i++) T[i+ld] = 1.0;
  ierr = PSRestoreArrayReal(ps,PS_MAT_T,&T);CHKERRQ(ierr);
  if (l==0 && k==0) {
    ierr = PSSetState(ps,PS_STATE_INTERMEDIATE);CHKERRQ(ierr);
  } else {
    ierr = PSSetState(ps,PS_STATE_RAW);CHKERRQ(ierr);
  }
  if (verbose) { 
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Initial - - - - - - - - -\n");CHKERRQ(ierr);
    ierr = PSView(ps,viewer);CHKERRQ(ierr);
  }

  /* Solve */
  ierr = PetscMalloc(n*sizeof(PetscScalar),&eig);CHKERRQ(ierr);
  ierr = PSSolve(ps,eig,PETSC_NULL);CHKERRQ(ierr);
  if (verbose) { 
    ierr = PetscPrintf(PETSC_COMM_WORLD,"After solve - - - - - - - - -\n");CHKERRQ(ierr);
    ierr = PSView(ps,viewer);CHKERRQ(ierr);
  }

  /* Sort */
  ierr = PSSort(ps,eig,PETSC_NULL,SlepcCompareLargestMagnitude,PETSC_NULL);CHKERRQ(ierr);
  if (verbose) { 
    ierr = PetscPrintf(PETSC_COMM_WORLD,"After sort - - - - - - - - -\n");CHKERRQ(ierr);
    ierr = PSView(ps,viewer);CHKERRQ(ierr);
  }

  /* Print eigenvalues */
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Computed eigenvalues =\n",n);CHKERRQ(ierr); 
  for (i=0;i<n;i++) {
    ierr = PetscViewerASCIIPrintf(viewer,"  %.5F\n",PetscRealPart(eig[i]));CHKERRQ(ierr);
  }

  ierr = PetscFree(eig);CHKERRQ(ierr);
  ierr = PSDestroy(&ps);CHKERRQ(ierr);
  ierr = SlepcFinalize();CHKERRQ(ierr);
  return 0;
}
