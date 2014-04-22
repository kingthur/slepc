/*
   BV private kernels that use the BLAS.

   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2013, Universitat Politecnica de Valencia, Spain

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

#include <slepc-private/bvimpl.h>      /*I "slepcbv.h" I*/
#include <slepcblaslapack.h>

#define BLOCKSIZE 64

#undef __FUNCT__
#define __FUNCT__ "BVMult_BLAS_Private"
PetscErrorCode BVMult_BLAS_Private(PetscInt m_,PetscInt n_,PetscInt k_,PetscScalar alpha,PetscScalar *A,PetscScalar *B,PetscScalar beta,PetscScalar *C)
{
  PetscErrorCode ierr;
  PetscBLASInt   m,n,k,l,bs=BLOCKSIZE;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(m_,&m);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(n_,&n);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(k_,&k);CHKERRQ(ierr);
  l = k % bs;
  if (l) PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&m,&alpha,A,&k,B,&m,&beta,C,&k));
  for (;l<k;l+=bs) {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&bs,&n,&m,&alpha,A+l,&k,B,&m,&beta,C,&k));
  }
  ierr = PetscLogFlops(2.0*m*n*k);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMultVec_BLAS_Private"
PetscErrorCode BVMultVec_BLAS_Private(PetscInt n_,PetscInt k_,PetscScalar alpha,PetscScalar *A,PetscScalar *B,PetscScalar beta,PetscScalar *C)
{
  PetscErrorCode ierr;
  PetscBLASInt   n,k,one=1;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(n_,&n);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(k_,&k);CHKERRQ(ierr);
  PetscStackCallBLAS("BLASgemv",BLASgemv_("N",&n,&k,&alpha,A,&n,B,&one,&beta,C,&one));
  ierr = PetscLogFlops(2.0*n*k);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMultInPlace_BLAS_Private"
PetscErrorCode BVMultInPlace_BLAS_Private(PetscInt m_,PetscInt s,PetscInt e,PetscInt k_,PetscScalar *A,PetscScalar *B)
{
  PetscErrorCode ierr;
  PetscScalar    zero=0.0,one=1.0,awork[128],*work = awork;
  PetscBLASInt   m,n,k,l,bs=BLOCKSIZE;
  PetscInt       j,n_=e-s;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(m_,&m);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(n_,&n);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(k_,&k);CHKERRQ(ierr);
  if (BLOCKSIZE*n_ > 128) {
    ierr = PetscMalloc1(BLOCKSIZE*n_,&work);CHKERRQ(ierr);
  }
  l = k % bs;
  if (l) {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&m,&one,A,&k,B+s*m,&m,&zero,work,&l));
    for (j=0;j<n;j++) {
      ierr = PetscMemcpy(A+(s+j)*k,work+j*l,l*sizeof(PetscScalar));CHKERRQ(ierr);
    }
  }
  for (;l<k;l+=bs) {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&bs,&n,&m,&one,A+l,&k,B+s*m,&m,&zero,work,&bs));
    for (j=0;j<n;j++) {
      ierr = PetscMemcpy(A+(s+j)*k+l,work+j*bs,bs*sizeof(PetscScalar));CHKERRQ(ierr);
    }
  }
  if (BLOCKSIZE*n_ > 128) {
    ierr = PetscFree(work);CHKERRQ(ierr);
  }
  ierr = PetscLogFlops(2.0*m*n*k);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMultInPlace_Vecs_Private"
PetscErrorCode BVMultInPlace_Vecs_Private(PetscInt m_,PetscInt n_,PetscInt k_,Vec *V,PetscScalar *B)
{
  PetscErrorCode ierr;
  PetscScalar    zero=0.0,one=1.0,awork[128],*work = awork,*out,*pv;
  PetscBLASInt   m,n,k,l,bs=BLOCKSIZE;
  PetscInt       j;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(m_,&m);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(n_,&n);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(k_,&k);CHKERRQ(ierr);
  if (2*BLOCKSIZE*n_ > 128) {
    ierr = PetscMalloc1(2*BLOCKSIZE*n_,&work);CHKERRQ(ierr);
  }
  out = work+BLOCKSIZE*n_;
  l = k % bs;
  if (l) {
    for (j=0;j<n;j++) {
      ierr = VecGetArray(V[j],&pv);CHKERRQ(ierr);
      ierr = PetscMemcpy(work+j*l,pv,l*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArray(V[j],&pv);CHKERRQ(ierr);
    }
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&n,&one,work,&l,B,&m,&zero,out,&l));
    for (j=0;j<n;j++) {
      ierr = VecGetArray(V[j],&pv);CHKERRQ(ierr);
      ierr = PetscMemcpy(pv,out+j*l,l*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArray(V[j],&pv);CHKERRQ(ierr);
    }
  }
  for (;l<k;l+=bs) {
    for (j=0;j<n;j++) {
      ierr = VecGetArray(V[j],&pv);CHKERRQ(ierr);
      ierr = PetscMemcpy(work+j*bs,pv+l,bs*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArray(V[j],&pv);CHKERRQ(ierr);
    }
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&bs,&n,&n,&one,work,&bs,B,&m,&zero,out,&bs));
    for (j=0;j<n;j++) {
      ierr = VecGetArray(V[j],&pv);CHKERRQ(ierr);
      ierr = PetscMemcpy(pv+l,out+j*bs,bs*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArray(V[j],&pv);CHKERRQ(ierr);
    }
  }
  if (2*BLOCKSIZE*n_ > 128) {
    ierr = PetscFree(work);CHKERRQ(ierr);
  }
  ierr = PetscLogFlops(2.0*n*n*k);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVDot_BLAS_Private"
PetscErrorCode BVDot_BLAS_Private(PetscInt m_,PetscInt n_,PetscInt k_,PetscScalar *A,PetscScalar *B,PetscScalar *C,PetscBool mpi,MPI_Comm comm)
{
  PetscErrorCode ierr;
  PetscScalar    zero=0.0,one=1.0,awork[128],*work = awork;
  PetscBLASInt   m,n,k;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(m_,&m);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(n_,&n);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(k_,&k);CHKERRQ(ierr);
  if (mpi) {
    if (m*n > 128) {
      ierr = PetscMalloc1(m*n,&work);CHKERRQ(ierr);
    }
    PetscStackCallBLAS("BLASgemm",BLASgemm_("C","N",&m,&n,&k,&one,A,&k,B,&k,&zero,work,&m));
    ierr = MPI_Allreduce(work,C,m*n,MPIU_SCALAR,MPIU_SUM,comm);CHKERRQ(ierr);
    if (m*n > 128) {
      ierr = PetscFree(work);CHKERRQ(ierr);
    }
  } else {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("C","N",&m,&n,&k,&one,A,&k,B,&k,&zero,C,&m));
  }
  ierr = PetscLogFlops(2.0*m*n*k);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVDotVec_BLAS_Private"
PetscErrorCode BVDotVec_BLAS_Private(PetscInt n_,PetscInt k_,PetscScalar *A,PetscScalar *B,PetscScalar *C,PetscBool mpi,MPI_Comm comm)
{
  PetscErrorCode ierr;
  PetscScalar    zero=0.0,done=1.0,awork[128],*work = awork;
  PetscBLASInt   n,k,one=1;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(n_,&n);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(k_,&k);CHKERRQ(ierr);
  if (mpi) {
    if (k > 128) {
      ierr = PetscMalloc1(k,&work);CHKERRQ(ierr);
    }
    PetscStackCallBLAS("BLASgemv",BLASgemv_("C",&n,&k,&done,A,&n,B,&one,&zero,work,&one));
    ierr = MPI_Allreduce(work,C,k,MPIU_SCALAR,MPIU_SUM,comm);CHKERRQ(ierr);
    if (k > 128) {
      ierr = PetscFree(work);CHKERRQ(ierr);
    }
  } else {
    PetscStackCallBLAS("BLASgemv",BLASgemv_("C",&n,&k,&done,A,&n,B,&one,&zero,C,&one));
  }
  ierr = PetscLogFlops(2.0*n*k);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

