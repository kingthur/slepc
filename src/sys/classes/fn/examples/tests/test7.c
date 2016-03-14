/*
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2015, Universitat Politecnica de Valencia, Spain

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

static char help[] = "Test matrix square root.\n\n";

#include <slepcfn.h>

#undef __FUNCT__
#define __FUNCT__ "TestMatSqrt"
/*
   Compute matrix square root B = sqrtm(A)
   Check result as norm(B*B-A)
 */
PetscErrorCode TestMatSqrt(FN fn,Mat A,PetscViewer viewer,PetscBool verbose)
{
  PetscErrorCode ierr;
  PetscScalar    tau,eta;
  PetscReal      nrm;
  PetscInt       n;
  Mat            S,R;

  PetscFunctionBeginUser;
  ierr = MatGetSize(A,&n,NULL);CHKERRQ(ierr);
  ierr = MatCreateSeqDense(PETSC_COMM_SELF,n,n,NULL,&S);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)S,"S");CHKERRQ(ierr);
  ierr = MatCreateSeqDense(PETSC_COMM_SELF,n,n,NULL,&R);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)R,"R");CHKERRQ(ierr);
  ierr = FNGetScale(fn,&tau,&eta);CHKERRQ(ierr);
  /* compute square root */
  ierr = FNEvaluateFunctionMat(fn,A,S);CHKERRQ(ierr);
  if (verbose) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Matrix A - - - - - - - -\n");CHKERRQ(ierr);
    ierr = MatView(A,viewer);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Computed sqrtm(A) - - - - - - -\n");CHKERRQ(ierr);
    ierr = MatView(S,viewer);CHKERRQ(ierr);
  }
  /* check error ||S*S-A||_F */
  ierr = MatMatMult(S,S,MAT_REUSE_MATRIX,PETSC_DEFAULT,&R);CHKERRQ(ierr);
  if (eta!=1.0) {
    ierr = MatScale(R,1.0/(eta*eta));CHKERRQ(ierr);
  }
  ierr = MatAXPY(R,-tau,A,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = MatNorm(R,NORM_FROBENIUS,&nrm);CHKERRQ(ierr);
  if (nrm<100*PETSC_MACHINE_EPSILON) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"||S*S-A||_F < 100*eps\n");CHKERRQ(ierr);
  } else {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"||S*S-A||_F = %g\n",(double)nrm);CHKERRQ(ierr);
  }
  ierr = MatDestroy(&S);CHKERRQ(ierr);
  ierr = MatDestroy(&R);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  FN             fn;
  Mat            A;
  PetscInt       i,j,n=10;
  PetscScalar    *As,tau=1.0,eta=1.0;
  PetscViewer    viewer;
  PetscBool      verbose;
  PetscRandom    myrand;
  PetscReal      v;

  SlepcInitialize(&argc,&argv,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetScalar(NULL,NULL,"-tau",&tau,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetScalar(NULL,NULL,"-eta",&eta,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,NULL,"-verbose",&verbose);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Matrix square root, n=%D.\n",n);CHKERRQ(ierr);

  /* Create exponential function eta*sqrt(tau*x) */
  ierr = FNCreate(PETSC_COMM_WORLD,&fn);CHKERRQ(ierr);
  ierr = FNSetType(fn,FNSQRT);CHKERRQ(ierr);
  ierr = FNSetScale(fn,tau,eta);CHKERRQ(ierr);

  /* Set up viewer */
  ierr = PetscViewerASCIIGetStdout(PETSC_COMM_WORLD,&viewer);CHKERRQ(ierr);
  ierr = FNView(fn,viewer);CHKERRQ(ierr);
  if (verbose) {
    ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
  }

  /* Create matrix */
  ierr = MatCreateSeqDense(PETSC_COMM_SELF,n,n,NULL,&A);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)A,"A");CHKERRQ(ierr);

  /* Compute square root of a symmetric matrix A */
  ierr = MatDenseGetArray(A,&As);CHKERRQ(ierr);
  for (i=0;i<n;i++) As[i+i*n]=2.5;
  for (j=1;j<3;j++) {
    for (i=0;i<n-j;i++) { As[i+(i+j)*n]=1.0; As[(i+j)+i*n]=1.0; }
  }
  ierr = MatDenseRestoreArray(A,&As);CHKERRQ(ierr);
  ierr = MatSetOption(A,MAT_HERMITIAN,PETSC_TRUE);CHKERRQ(ierr);
  ierr = TestMatSqrt(fn,A,viewer,verbose);CHKERRQ(ierr);

  /* Repeat with upper triangular A */
  ierr = MatDenseGetArray(A,&As);CHKERRQ(ierr);
  for (j=1;j<3;j++) {
    for (i=0;i<n-j;i++) As[(i+j)+i*n]=0.0;
  }
  ierr = MatDenseRestoreArray(A,&As);CHKERRQ(ierr);
  ierr = MatSetOption(A,MAT_HERMITIAN,PETSC_FALSE);CHKERRQ(ierr);
  ierr = TestMatSqrt(fn,A,viewer,verbose);CHKERRQ(ierr);

  /* Repeat with non-symmetic A */
  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&myrand);CHKERRQ(ierr);
  ierr = PetscRandomSetFromOptions(myrand);CHKERRQ(ierr);
  ierr = PetscRandomSetInterval(myrand,0.0,1.0);CHKERRQ(ierr);
  ierr = MatDenseGetArray(A,&As);CHKERRQ(ierr);
  for (j=1;j<3;j++) {
    for (i=0;i<n-j;i++) { 
      ierr = PetscRandomGetValueReal(myrand,&v);CHKERRQ(ierr);
      As[(i+j)+i*n]=v;
    }
  }
  ierr = MatDenseRestoreArray(A,&As);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&myrand);CHKERRQ(ierr);
  ierr = MatSetOption(A,MAT_HERMITIAN,PETSC_FALSE);CHKERRQ(ierr);
  ierr = TestMatSqrt(fn,A,viewer,verbose);CHKERRQ(ierr);

  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = FNDestroy(&fn);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}