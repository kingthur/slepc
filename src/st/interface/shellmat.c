/*
      This file contains the subroutines which implement various operations
      of the matrix associated to the shift-and-invert technique for eigenvalue
      problems, and also a subroutine to create it.

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


#include <private/stimpl.h>

#undef __FUNCT__
#define __FUNCT__ "STMatShellMult"
static PetscErrorCode STMatShellMult(Mat A,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST             ctx;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  ierr = MatMult(ctx->A,x,y);CHKERRQ(ierr);
  if (ctx->sigma != 0.0) { 
    if (ctx->B) {  /* y = (A - sB) x */
      ierr = MatMult(ctx->B,x,ctx->w);CHKERRQ(ierr);
      ierr = VecAXPY(y,-ctx->sigma,ctx->w);CHKERRQ(ierr); 
    } else {    /* y = (A - sI) x */
      ierr = VecAXPY(y,-ctx->sigma,x);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "STMatShellMultTranspose"
static PetscErrorCode STMatShellMultTranspose(Mat A,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST             ctx;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  ierr = MatMultTranspose(ctx->A,x,y);CHKERRQ(ierr);
  if (ctx->sigma != 0.0) { 
    if (ctx->B) {  /* y = (A - sB) x */
      ierr = MatMultTranspose(ctx->B,x,ctx->w);CHKERRQ(ierr);
      ierr = VecAXPY(y,-ctx->sigma,ctx->w);CHKERRQ(ierr); 
    } else {    /* y = (A - sI) x */
      ierr = VecAXPY(y,-ctx->sigma,x);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "STMatShellGetDiagonal"
static PetscErrorCode STMatShellGetDiagonal(Mat A,Vec diag)
{
  PetscErrorCode ierr;
  ST             ctx;
  Vec            diagb;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  ierr = MatGetDiagonal(ctx->A,diag);CHKERRQ(ierr);
  if (ctx->sigma != 0.0) { 
    if (ctx->B) {
      ierr = VecDuplicate(diag,&diagb);CHKERRQ(ierr);
      ierr = MatGetDiagonal(ctx->B,diagb);CHKERRQ(ierr);
      ierr = VecAXPY(diag,-ctx->sigma,diagb);CHKERRQ(ierr);
      ierr = VecDestroy(&diagb);CHKERRQ(ierr);
    } else {
      ierr = VecShift(diag,-ctx->sigma);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STMatShellCreate"
PetscErrorCode STMatShellCreate(ST st,Mat *mat)
{
  PetscErrorCode ierr;
  PetscInt       n,m,N,M;
  PetscBool      hasA,hasB;

  PetscFunctionBegin;
  ierr = MatGetSize(st->A,&M,&N);CHKERRQ(ierr);  
  ierr = MatGetLocalSize(st->A,&m,&n);CHKERRQ(ierr);  
  ierr = MatCreateShell(((PetscObject)st)->comm,m,n,M,N,(void*)st,mat);CHKERRQ(ierr);
  ierr = MatShellSetOperation(*mat,MATOP_MULT,(void(*)(void))STMatShellMult);CHKERRQ(ierr);
  ierr = MatShellSetOperation(*mat,MATOP_MULT_TRANSPOSE,(void(*)(void))STMatShellMultTranspose);CHKERRQ(ierr);

  ierr = MatHasOperation(st->A,MATOP_GET_DIAGONAL,&hasA);CHKERRQ(ierr);
  if (st->B) { ierr = MatHasOperation(st->B,MATOP_GET_DIAGONAL,&hasB);CHKERRQ(ierr); }
  if ((hasA && !st->B) || (hasA && hasB)) {
    ierr = MatShellSetOperation(*mat,MATOP_GET_DIAGONAL,(void(*)(void))STMatShellGetDiagonal);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

