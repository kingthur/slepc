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

#include <slepc-private/dsimpl.h>      /*I "slepcds.h" I*/
#include <slepcblaslapack.h>

#undef __FUNCT__  
#define __FUNCT__ "DSAllocate_NHEP"
PetscErrorCode DSAllocate_NHEP(DS ds,PetscInt ld)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DSAllocateMat_Private(ds,DS_MAT_A);CHKERRQ(ierr); 
  ierr = DSAllocateMat_Private(ds,DS_MAT_Q);CHKERRQ(ierr); 
  ierr = PetscFree(ds->perm);CHKERRQ(ierr);
  ierr = PetscMalloc(ld*sizeof(PetscInt),&ds->perm);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(ds,ld*sizeof(PetscInt));CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSView_NHEP"
PetscErrorCode DSView_NHEP(DS ds,PetscViewer viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DSViewMat_Private(ds,viewer,DS_MAT_A);CHKERRQ(ierr); 
  if (ds->state>DS_STATE_INTERMEDIATE) {
    ierr = DSViewMat_Private(ds,viewer,DS_MAT_Q);CHKERRQ(ierr); 
  }
  if (ds->mat[DS_MAT_X]) {
    ierr = DSViewMat_Private(ds,viewer,DS_MAT_X);CHKERRQ(ierr); 
  }
  if (ds->mat[DS_MAT_Y]) {
    ierr = DSViewMat_Private(ds,viewer,DS_MAT_Y);CHKERRQ(ierr); 
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSVectors_NHEP_Eigen_Some"
PetscErrorCode DSVectors_NHEP_Eigen_Some(DS ds,PetscInt *k,PetscReal *rnorm,PetscBool left)
{
#if defined(SLEPC_MISSING_LAPACK_TREVC)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"TREVC - Lapack routine is unavailable.");
#else
  PetscErrorCode ierr;
  PetscInt       i;
  PetscBLASInt   mm=1,mout,info,ld,n,inc = 1;
  PetscScalar    tmp,done=1.0,zero=0.0;
  PetscReal      norm;
  PetscBool      iscomplex = PETSC_FALSE;
  PetscBLASInt   *select;
  PetscScalar    *A = ds->mat[DS_MAT_A];
  PetscScalar    *Q = ds->mat[DS_MAT_Q];
  PetscScalar    *X = ds->mat[left?DS_MAT_Y:DS_MAT_X];
  PetscScalar    *Y;

  PetscFunctionBegin;
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
  ierr = DSAllocateWork_Private(ds,0,0,ld);CHKERRQ(ierr); 
  select = ds->iwork;
  for (i=0;i<n;i++) select[i] = (PetscBLASInt)PETSC_FALSE;

  /* Compute k-th eigenvector Y of A */
  Y = X+(*k)*ld;
  select[*k] = (PetscBLASInt)PETSC_TRUE;
#if !defined(PETSC_USE_COMPLEX)
  if ((*k)<n-1 && A[(*k)+1+(*k)*ld]!=0.0) iscomplex = PETSC_TRUE;
  mm = iscomplex? 2: 1;
  if (iscomplex) select[(*k)+1] = (PetscBLASInt)PETSC_TRUE;
  ierr = DSAllocateWork_Private(ds,3*ld,0,0);CHKERRQ(ierr); 
  LAPACKtrevc_(left?"L":"R","S",select,&n,A,&ld,Y,&ld,Y,&ld,&mm,&mout,ds->work,&info);
#else
  ierr = DSAllocateWork_Private(ds,2*ld,ld,0);CHKERRQ(ierr); 
  LAPACKtrevc_(left?"L":"R","S",select,&n,A,&ld,Y,&ld,Y,&ld,&mm,&mout,ds->work,ds->rwork,&info);
#endif
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xTREVC %d",info);
  if (mout != mm) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Inconsistent arguments");

  /* accumulate and normalize eigenvectors */
  if (ds->state>=DS_STATE_CONDENSED) {
    ierr = PetscMemcpy(ds->work,Y,mout*ld*sizeof(PetscScalar));CHKERRQ(ierr);
    BLASgemv_("N",&n,&n,&done,Q,&ld,ds->work,&inc,&zero,Y,&inc);
#if !defined(PETSC_USE_COMPLEX)
    if (iscomplex) BLASgemv_("N",&n,&n,&done,Q,&ld,ds->work+ld,&inc,&zero,Y+ld,&inc);
#endif
    norm = BLASnrm2_(&n,Y,&inc);
#if !defined(PETSC_USE_COMPLEX)
    if (iscomplex) {
      tmp = BLASnrm2_(&n,Y+ld,&inc);
      norm = SlepcAbsEigenvalue(norm,tmp);
    }
#endif
    tmp = 1.0 / norm;
    BLASscal_(&n,&tmp,Y,&inc);
#if !defined(PETSC_USE_COMPLEX)
    if (iscomplex) BLASscal_(&n,&tmp,Y+ld,&inc);
#endif
  }

  /* set output arguments */
  if (iscomplex) (*k)++;
  if (rnorm) {
    if (iscomplex) *rnorm = SlepcAbsEigenvalue(Y[n-1],Y[n-1+ld]);
    else *rnorm = PetscAbsScalar(Y[n-1]);
  }
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__  
#define __FUNCT__ "DSVectors_NHEP_Eigen_All"
PetscErrorCode DSVectors_NHEP_Eigen_All(DS ds,PetscBool left)
{
#if defined(SLEPC_MISSING_LAPACK_TREVC)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"TREVC - Lapack routine is unavailable.");
#else
  PetscErrorCode ierr;
  PetscBLASInt   n,ld,mout,info;
  PetscScalar    *X,*Y,*A = ds->mat[DS_MAT_A];
  const char     *side,*back;

  PetscFunctionBegin;
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
  if (left) {
    X = PETSC_NULL;
    Y = ds->mat[DS_MAT_Y];
    side = "L";
  } else {
    X = ds->mat[DS_MAT_X];
    Y = PETSC_NULL;
    side = "R";
  }
  if (ds->state>=DS_STATE_CONDENSED) {
    /* DSSolve() has been called, backtransform with matrix Q */
    back = "B";
    ierr = PetscMemcpy(left?Y:X,ds->mat[DS_MAT_Q],ld*ld*sizeof(PetscScalar));CHKERRQ(ierr);
  } else back = "A";
#if !defined(PETSC_USE_COMPLEX)
  ierr = DSAllocateWork_Private(ds,3*ld,0,0);CHKERRQ(ierr); 
  LAPACKtrevc_(side,back,PETSC_NULL,&n,A,&ld,Y,&ld,X,&ld,&n,&mout,ds->work,&info);
#else
  ierr = DSAllocateWork_Private(ds,2*ld,ld,0);CHKERRQ(ierr); 
  LAPACKtrevc_(side,back,PETSC_NULL,&n,A,&ld,Y,&ld,X,&ld,&n,&mout,ds->work,ds->rwork,&info);
#endif
  if (info) SETERRQ1(((PetscObject)ds)->comm,PETSC_ERR_LIB,"Error in Lapack xTREVC %i",info);
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__  
#define __FUNCT__ "DSVectors_NHEP"
PetscErrorCode DSVectors_NHEP(DS ds,DSMatType mat,PetscInt *j,PetscReal *rnorm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (mat) {
    case DS_MAT_X:
      if (j) {
        ierr = DSVectors_NHEP_Eigen_Some(ds,j,rnorm,PETSC_FALSE);CHKERRQ(ierr);
      } else {
        ierr = DSVectors_NHEP_Eigen_All(ds,PETSC_FALSE);CHKERRQ(ierr);
      }
      break;
    case DS_MAT_Y:
      if (j) {
        ierr = DSVectors_NHEP_Eigen_Some(ds,j,rnorm,PETSC_TRUE);CHKERRQ(ierr);
      } else {
        ierr = DSVectors_NHEP_Eigen_All(ds,PETSC_TRUE);CHKERRQ(ierr);
      }
      break;
    case DS_MAT_U:
    case DS_MAT_VT:
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not implemented yet");
      break;
    default:
      SETERRQ(((PetscObject)ds)->comm,PETSC_ERR_ARG_OUTOFRANGE,"Invalid mat parameter"); 
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSNormalize_NHEP"
PetscErrorCode DSNormalize_NHEP(DS ds,DSMatType mat,PetscInt col)
{
  PetscErrorCode ierr;
  PetscInt       i,i0,i1;
  PetscBLASInt   ld,n,one = 1;
  PetscScalar    *A = ds->mat[DS_MAT_A],norm,*x;
#if !defined(PETSC_USE_COMPLEX)
  PetscScalar    norm0;
#endif

  PetscFunctionBegin;
  if(ds->state < DS_STATE_INTERMEDIATE) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Unsupported state");
  switch (mat) {
    case DS_MAT_X:
    case DS_MAT_Y:
    case DS_MAT_Q:
      /* Supported matrices */
      break;
    case DS_MAT_U:
    case DS_MAT_VT:
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not implemented yet");
      break;
    default:
      SETERRQ(((PetscObject)ds)->comm,PETSC_ERR_ARG_OUTOFRANGE,"Invalid mat parameter"); 
  }

  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
  ierr = DSGetArray(ds,mat,&x);CHKERRQ(ierr);
  if (col < 0) {
    i0 = 0; i1 = ds->n;
  } else if(col>0 && A[ds->ld*(col-1)+col] != 0.0) {
    i0 = col-1; i1 = col+1;
  } else {
    i0 = col; i1 = col+1;
  }
  for(i=i0; i<i1; i++) {
#if !defined(PETSC_USE_COMPLEX)
    if(i<n-1 && A[ds->ld*i+i+1] != 0.0) {
      norm = BLASnrm2_(&n,&x[ld*i],&one);
      norm0 = BLASnrm2_(&n,&x[ld*(i+1)],&one);
      norm = 1.0/SlepcAbsEigenvalue(norm,norm0);
      BLASscal_(&n,&norm,&x[ld*i],&one);
      BLASscal_(&n,&norm,&x[ld*(i+1)],&one);
      i++;
    } else
#endif
    {
      norm = BLASnrm2_(&n,&x[ld*i],&one);
      norm = 1.0/norm;
      BLASscal_(&n,&norm,&x[ld*i],&one);
     }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSSort_NHEP_Arbitrary"
PetscErrorCode DSSort_NHEP_Arbitrary(DS ds,PetscScalar *wr,PetscScalar *wi,PetscScalar *rr,PetscScalar *ri,PetscInt *k)
{
#if defined(SLEPC_MISSING_LAPACK_TRSEN)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"TRSEN - Lapack routine is unavailable.");
#else
  PetscErrorCode ierr;
  PetscInt       i;
  PetscBLASInt   info,n,ld,mout,lwork,*selection;
  PetscScalar    *T = ds->mat[DS_MAT_A],*Q = ds->mat[DS_MAT_Q],*work;
#if !defined(PETSC_USE_COMPLEX)
  PetscBLASInt   *iwork,liwork;
#endif

  PetscFunctionBegin;
  if (!k) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Must supply argument k");
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
#if !defined(PETSC_USE_COMPLEX)
  lwork = n;
  liwork = 1;
  ierr = DSAllocateWork_Private(ds,lwork,0,liwork+n);CHKERRQ(ierr); 
  work = ds->work;
  lwork = ds->lwork;
  selection = ds->iwork;
  iwork = ds->iwork + n;
  liwork = ds->liwork - n;
#else
  lwork = 1;
  ierr = DSAllocateWork_Private(ds,lwork,0,n);CHKERRQ(ierr); 
  work = ds->work;
  selection = ds->iwork;
#endif
  /* Compute the selected eigenvalue to be in the leading position */
  ierr = DSSortEigenvalues_Private(ds,rr,ri,ds->perm,PETSC_FALSE);CHKERRQ(ierr);
  ierr = PetscMemzero(selection,n*sizeof(PetscBLASInt));CHKERRQ(ierr);
  for (i=0;i<*k;i++) selection[ds->perm[i]] = 1;
#if !defined(PETSC_USE_COMPLEX)
  LAPACKtrsen_("N","V",selection,&n,T,&ld,Q,&ld,wr,wi,&mout,PETSC_NULL,PETSC_NULL,work,&lwork,iwork,&liwork,&info);
#else
  LAPACKtrsen_("N","V",selection,&n,T,&ld,Q,&ld,wr,&mout,PETSC_NULL,PETSC_NULL,work,&lwork,&info);
#endif
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xTRSEN %d",info);
  *k = mout;
  PetscFunctionReturn(0);
#endif 
}

#undef __FUNCT__  
#define __FUNCT__ "DSSort_NHEP_Total"
PetscErrorCode DSSort_NHEP_Total(DS ds,PetscScalar *wr,PetscScalar *wi)
{
#if defined(SLEPC_MISSING_LAPACK_TREXC)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"TREXC - Lapack routine is unavailable.");
#else
  PetscErrorCode ierr;
  PetscScalar    re,im;
  PetscInt       i,j,pos,result;
  PetscBLASInt   ifst,ilst,info,n,ld;
  PetscScalar    *T = ds->mat[DS_MAT_A];
  PetscScalar    *Q = ds->mat[DS_MAT_Q];
#if !defined(PETSC_USE_COMPLEX)
  PetscScalar    *work;
#endif

  PetscFunctionBegin;
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
#if !defined(PETSC_USE_COMPLEX)
  ierr = DSAllocateWork_Private(ds,ld,0,0);CHKERRQ(ierr); 
  work = ds->work;
#endif
  /* selection sort */
  for (i=ds->l;i<n-1;i++) {
    re = wr[i];
    im = wi[i];
    pos = 0;
    j=i+1; /* j points to the next eigenvalue */
#if !defined(PETSC_USE_COMPLEX)
    if (im != 0) j=i+2;
#endif
    /* find minimum eigenvalue */
    for (;j<n;j++) { 
      ierr = (*ds->comp_fun)(re,im,wr[j],wi[j],&result,ds->comp_ctx);CHKERRQ(ierr);
      if (result > 0) {
        re = wr[j];
        im = wi[j];
        pos = j;
      }
#if !defined(PETSC_USE_COMPLEX)
      if (wi[j] != 0) j++;
#endif
    }
    if (pos) {
      /* interchange blocks */
      ifst = PetscBLASIntCast(pos+1);
      ilst = PetscBLASIntCast(i+1);
#if !defined(PETSC_USE_COMPLEX)
      LAPACKtrexc_("V",&n,T,&ld,Q,&ld,&ifst,&ilst,work,&info);
#else
      LAPACKtrexc_("V",&n,T,&ld,Q,&ld,&ifst,&ilst,&info);
#endif
      if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xTREXC %d",info);
      /* recover original eigenvalues from T matrix */
      for (j=i;j<n;j++) {
        wr[j] = T[j+j*ld];
#if !defined(PETSC_USE_COMPLEX)
        if (j<n-1 && T[j+1+j*ld] != 0.0) {
          /* complex conjugate eigenvalue */
          wi[j] = PetscSqrtReal(PetscAbsReal(T[j+1+j*ld])) *
                  PetscSqrtReal(PetscAbsReal(T[j+(j+1)*ld]));
          wr[j+1] = wr[j];
          wi[j+1] = -wi[j];
          j++;
        } else
#endif
        wi[j] = 0.0;
      }
    }
#if !defined(PETSC_USE_COMPLEX)
    if (wi[i] != 0) i++;
#endif
  }
  PetscFunctionReturn(0);
#endif 
}

#undef __FUNCT__  
#define __FUNCT__ "DSSort_NHEP"
PetscErrorCode DSSort_NHEP(DS ds,PetscScalar *wr,PetscScalar *wi,PetscScalar *rr,PetscScalar *ri,PetscInt *k)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!rr || wr == rr) {
    ierr = DSSort_NHEP_Total(ds,wr,wi);CHKERRQ(ierr);
  } else {
    ierr = DSSort_NHEP_Arbitrary(ds,wr,wi,rr,ri,k);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSUpdateExtraRow_NHEP"
PetscErrorCode DSUpdateExtraRow_NHEP(DS ds)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscBLASInt   n,ld,incx=1;
  PetscScalar    *A,*Q,*x,*y,one=1.0,zero=0.0;

  PetscFunctionBegin;
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
  A  = ds->mat[DS_MAT_A];
  Q  = ds->mat[DS_MAT_Q];
  ierr = DSAllocateWork_Private(ds,2*ld,0,0);CHKERRQ(ierr);
  x = ds->work;
  y = ds->work+ld;
  for (i=0;i<n;i++) x[i] = A[n+i*ld];
  BLASgemv_("C",&n,&n,&one,Q,&ld,x,&incx,&zero,y,&incx);
  for (i=0;i<n;i++) A[n+i*ld] = y[i];
  ds->k = n;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSSolve_NHEP"
PetscErrorCode DSSolve_NHEP(DS ds,PetscScalar *wr,PetscScalar *wi)
{
#if defined(SLEPC_MISSING_LAPACK_GEHRD) || defined(SLEPC_MISSING_LAPACK_ORGHR) || defined(PETSC_MISSING_LAPACK_HSEQR)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"GEHRD/ORGHR/HSEQR - Lapack routines are unavailable.");
#else
  PetscErrorCode ierr;
  PetscScalar    *work,*tau;
  PetscInt       i,j;
  PetscBLASInt   ilo,lwork,info,n,ld;
  PetscScalar    *A = ds->mat[DS_MAT_A];
  PetscScalar    *Q = ds->mat[DS_MAT_Q];

  PetscFunctionBegin;
  PetscValidPointer(wi,3);
  n   = PetscBLASIntCast(ds->n);
  ld  = PetscBLASIntCast(ds->ld);
  ilo = PetscBLASIntCast(ds->l+1);
  ierr = DSAllocateWork_Private(ds,ld+ld*ld,0,0);CHKERRQ(ierr); 
  tau  = ds->work;
  work = ds->work+ld;
  lwork = ld*ld;

  /* initialize orthogonal matrix */
  ierr = PetscMemzero(Q,ld*ld*sizeof(PetscScalar));CHKERRQ(ierr);
  for (i=0;i<n;i++) 
    Q[i+i*ld] = 1.0;
  if (n==1) PetscFunctionReturn(0);    

  /* reduce to upper Hessenberg form */
  if (ds->state<DS_STATE_INTERMEDIATE) {
    LAPACKgehrd_(&n,&ilo,&n,A,&ld,tau,work,&lwork,&info);
    if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xGEHRD %d",info);
    for (j=0;j<n-1;j++) {
      for (i=j+2;i<n;i++) {
        Q[i+j*ld] = A[i+j*ld];
        A[i+j*ld] = 0.0;
      }
    }
    LAPACKorghr_(&n,&ilo,&n,Q,&ld,tau,work,&lwork,&info);
    if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xORGHR %d",info);
  }

  /* compute the (real) Schur form */
#if !defined(PETSC_USE_COMPLEX)
  LAPACKhseqr_("S","V",&n,&ilo,&n,A,&ld,wr,wi,Q,&ld,work,&lwork,&info);
  for (j=0;j<ds->l;j++) {
    if (j==n-1 || A[j+1+j*ld] == 0.0) { 
      /* real eigenvalue */
      wr[j] = A[j+j*ld];
      wi[j] = 0.0;
    } else {
      /* complex eigenvalue */
      wr[j] = A[j+j*ld];
      wr[j+1] = A[j+j*ld];
      wi[j] = PetscSqrtReal(PetscAbsReal(A[j+1+j*ld])) *
              PetscSqrtReal(PetscAbsReal(A[j+(j+1)*ld]));
      wi[j+1] = -wi[j];
      j++;
    }
  }
#else
  LAPACKhseqr_("S","V",&n,&ilo,&n,A,&ld,wr,Q,&ld,work,&lwork,&info);
#endif
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xHSEQR %d",info);
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__  
#define __FUNCT__ "DSTruncate_NHEP"
PetscErrorCode DSTruncate_NHEP(DS ds,PetscInt n)
{
  PetscInt       i,newn,ld=ds->ld,l=ds->l;
  PetscScalar    *A;

  PetscFunctionBegin;
  A  = ds->mat[DS_MAT_A];
  /* be careful not to break a diagonal 2x2 block */
  if (A[n+(n-1)*ld]==0.0) newn = n;
  else {
    if (n<ds->n-1) newn = n+1;
    else newn = n-1;
  }
  if (ds->extrarow && ds->k==ds->n) {
    /* copy entries of extra row to the new position, then clean last row */
    for (i=l;i<newn;i++) A[newn+i*ld] = A[ds->n+i*ld];
    for (i=l;i<ds->n;i++) A[ds->n+i*ld] = 0.0;
  }
  ds->k = 0;
  ds->n = newn;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DSCond_NHEP"
PetscErrorCode DSCond_NHEP(DS ds,PetscReal *cond)
{
#if defined(PETSC_MISSING_LAPACK_GETRF) || defined(SLEPC_MISSING_LAPACK_GETRI) || defined(SLEPC_MISSING_LAPACK_LANGE) || defined(SLEPC_MISSING_LAPACK_LANHS)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"GETRF/GETRI/LANGE/LANHS - Lapack routines are unavailable.");
#else
  PetscErrorCode ierr;
  PetscScalar    *work;
  PetscReal      *rwork;
  PetscBLASInt   *ipiv;
  PetscBLASInt   lwork,info,n,ld;
  PetscReal      hn,hin;
  PetscScalar    *A;

  PetscFunctionBegin;
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
  lwork = 8*ld;
  ierr = DSAllocateWork_Private(ds,lwork,ld,ld);CHKERRQ(ierr); 
  work  = ds->work;
  rwork = ds->rwork;
  ipiv  = ds->iwork;

  /* use workspace matrix W to avoid overwriting A */
  ierr = DSAllocateMat_Private(ds,DS_MAT_W);CHKERRQ(ierr);
  A = ds->mat[DS_MAT_W];
  ierr = PetscMemcpy(A,ds->mat[DS_MAT_A],sizeof(PetscScalar)*ds->ld*ds->ld);CHKERRQ(ierr);

  /* norm of A */
  if (ds->state<DS_STATE_INTERMEDIATE) hn = LAPACKlange_("I",&n,&n,A,&ld,rwork);
  else hn = LAPACKlanhs_("I",&n,A,&ld,rwork);

  /* norm of inv(A) */
  LAPACKgetrf_(&n,&n,A,&ld,ipiv,&info);
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xGETRF %d",info);
  LAPACKgetri_(&n,A,&ld,ipiv,work,&lwork,&info);
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xGETRI %d",info);
  hin = LAPACKlange_("I",&n,&n,A,&ld,rwork);

  *cond = hn*hin;
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__  
#define __FUNCT__ "DSTranslateHarmonic_NHEP"
PetscErrorCode DSTranslateHarmonic_NHEP(DS ds,PetscScalar tau,PetscReal beta,PetscBool recover,PetscScalar *gin,PetscReal *gamma)
{
#if defined(PETSC_MISSING_LAPACK_GETRF) || defined(PETSC_MISSING_LAPACK_GETRS) 
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"GETRF/GETRS - Lapack routines are unavailable.");
#else
  PetscErrorCode ierr;
  PetscInt       i,j;
  PetscBLASInt   *ipiv,info,n,ld,one=1,ncol;
  PetscScalar    *A,*B,*Q,*g=gin,*ghat; 
  PetscScalar    done=1.0,dmone=-1.0,dzero=0.0;
  PetscReal      gnorm;

  PetscFunctionBegin;
  n  = PetscBLASIntCast(ds->n);
  ld = PetscBLASIntCast(ds->ld);
  A  = ds->mat[DS_MAT_A];

  if (!recover) {

    ierr = DSAllocateWork_Private(ds,0,0,ld);CHKERRQ(ierr); 
    ipiv = ds->iwork;
    if (!g) {
      ierr = DSAllocateWork_Private(ds,ld,0,0);CHKERRQ(ierr); 
      g = ds->work;
    }
    /* use workspace matrix W to factor A-tau*eye(n) */
    ierr = DSAllocateMat_Private(ds,DS_MAT_W);CHKERRQ(ierr);
    B = ds->mat[DS_MAT_W];
    ierr = PetscMemcpy(B,A,sizeof(PetscScalar)*ld*ld);CHKERRQ(ierr);

    /* Vector g initialy stores b = beta*e_n^T */
    ierr = PetscMemzero(g,n*sizeof(PetscScalar));CHKERRQ(ierr);
    g[n-1] = beta;
 
    /* g = (A-tau*eye(n))'\b */
    for (i=0;i<n;i++) 
      B[i+i*ld] -= tau;
    LAPACKgetrf_(&n,&n,B,&ld,ipiv,&info);
    if (info<0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"Bad argument to LU factorization");
    if (info>0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_MAT_LU_ZRPVT,"Bad LU factorization");
    ierr = PetscLogFlops(2.0*n*n*n/3.0);CHKERRQ(ierr);
    LAPACKgetrs_("C",&n,&one,B,&ld,ipiv,g,&ld,&info);
    if (info) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"GETRS - Bad solve");
    ierr = PetscLogFlops(2.0*n*n-n);CHKERRQ(ierr);

    /* A = A + g*b' */
    for (i=0;i<n;i++) 
      A[i+(n-1)*ld] += g[i]*beta;

  } else { /* recover */

    PetscValidPointer(g,6);
    ierr = DSAllocateWork_Private(ds,ld,0,0);CHKERRQ(ierr); 
    ghat = ds->work;
    Q    = ds->mat[DS_MAT_Q];

    /* g^ = -Q(:,idx)'*g */
    ncol = PetscBLASIntCast(ds->l+ds->k);
    BLASgemv_("C",&n,&ncol,&dmone,Q,&ld,g,&one,&dzero,ghat,&one);

    /* A = A + g^*b' */
    for (i=0;i<ds->l+ds->k;i++)
      for (j=ds->l;j<ds->l+ds->k;j++)
        A[i+j*ld] += ghat[i]*Q[n-1+j*ld]*beta;

    /* g~ = (I-Q(:,idx)*Q(:,idx)')*g = g+Q(:,idx)*g^ */
    BLASgemv_("N",&n,&ncol,&done,Q,&ld,ghat,&one,&done,g,&one);
  }

  /* Compute gamma factor */
  if (gamma) {
    gnorm = 0.0;
    for (i=0;i<n;i++)
      gnorm = gnorm + PetscRealPart(g[i]*PetscConj(g[i]));
    *gamma = PetscSqrtReal(1.0+gnorm);
  }
  PetscFunctionReturn(0);
#endif
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "DSCreate_NHEP"
PetscErrorCode DSCreate_NHEP(DS ds)
{
  PetscFunctionBegin;
  ds->ops->allocate      = DSAllocate_NHEP;
  ds->ops->view          = DSView_NHEP;
  ds->ops->vectors       = DSVectors_NHEP;
  ds->ops->solve[0]      = DSSolve_NHEP;
  ds->ops->sort          = DSSort_NHEP;
  ds->ops->truncate      = DSTruncate_NHEP;
  ds->ops->update        = DSUpdateExtraRow_NHEP;
  ds->ops->cond          = DSCond_NHEP;
  ds->ops->transharm     = DSTranslateHarmonic_NHEP;
  ds->ops->normalize     = DSNormalize_NHEP;
  PetscFunctionReturn(0);
}
EXTERN_C_END
