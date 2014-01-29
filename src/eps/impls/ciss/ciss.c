/*

   SLEPc eigensolver: "ciss"

   Method: Contour Integral Spectral Slicing

   Algorithm:

       Contour integral based on Sakurai-Sugiura method to construct a
       subspace, with various eigenpair extractions (Rayleigh-Ritz,
       explicit moment).

   Based on code contributed by Tetsuya Sakurai.

   References:

       [1] T. Sakurai and H. Sugiura, "A projection method for generalized
           eigenvalue problems", J. Comput. Appl. Math. 159:119-128, 2003.

       [2] T. Sakurai and H. Tadano, "CIRR: a Rayleigh-Ritz type method with
           contour integral for generalized eigenvalue problems", Hokkaido
           Math. J. 36:745-757, 2007.

   Last update: Jun 2013

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

#include <slepc-private/epsimpl.h>                /*I "slepceps.h" I*/
#include <slepcblaslapack.h>

PetscErrorCode EPSSolve_CISS(EPS);

typedef struct {
  /* parameters */
  PetscScalar center;     /* center of the region where to find eigenpairs (default: 0.0) */
  PetscReal   radius;     /* radius of the region (1.0) */
  PetscReal   vscale;     /* vertical scale of the region (1.0; 0.1 if spectrum real) */
  PetscInt    N;          /* number of integration points (32) */
  PetscInt    L;          /* block size (16) */
  PetscInt    M;          /* moment degree (N/4 = 4) */
  PetscReal   delta;      /* threshold of singular value (1e-12) */
  PetscInt    L_max;      /* maximum number of columns of the source matrix V */
  PetscReal   spurious_threshold; /* discard spurious eigenpairs */
  PetscBool   isreal;     /* A and B are real */
  PetscInt    refine_inner;
  PetscInt    refine_outer;
  PetscInt    refine_blocksize;
  /* private data */
  PetscReal    *sigma;     /* threshold for numerical rank */
  PetscInt     num_subcomm;
  PetscInt     subcomm_id;
  PetscInt     num_solve_point;
  PetscScalar  *weight;
  PetscScalar  *omega;
  PetscScalar  *pp;
  Vec          *V;
  Vec          *S;
  Vec          *pV;
  Vec          *Y;
  Vec          xsub;
  Vec          xdup;
  KSP          ksp;
  PetscBool    useconj;
  PetscReal    est_eig;
  VecScatter   scatterin;
  Mat          pA,pB;
  PetscSubcomm subcomm;
} EPS_CISS;

#undef __FUNCT__
#define __FUNCT__ "SetSolverComm"
static PetscErrorCode SetSolverComm(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       N = ctx->N;

  PetscFunctionBegin;
  if (ctx->useconj) N = N/2;
  if (!ctx->subcomm) {
    ierr = PetscSubcommCreate(PetscObjectComm((PetscObject)eps),&ctx->subcomm);CHKERRQ(ierr);
    ierr = PetscSubcommSetNumber(ctx->subcomm,ctx->num_subcomm);CHKERRQ(ierr);CHKERRQ(ierr);
    ierr = PetscSubcommSetType(ctx->subcomm,PETSC_SUBCOMM_INTERLACED);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)eps,sizeof(PetscSubcomm));CHKERRQ(ierr);
  }
  ctx->subcomm_id = ctx->subcomm->color;
  ctx->num_solve_point = N / ctx->num_subcomm;
  if ((N%ctx->num_subcomm) > ctx->subcomm_id) ctx->num_solve_point+=1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CISSRedundantMat"
static PetscErrorCode CISSRedundantMat(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  Mat            A,B;
  PetscInt       nmat;

  PetscFunctionBegin;
  ierr = STGetNumMatrices(eps->st,&nmat);CHKERRQ(ierr);
  if (ctx->subcomm->n != 1) {
    ierr = STGetOperators(eps->st,0,&A);CHKERRQ(ierr);
    ierr = MatGetRedundantMatrix(A,ctx->subcomm->n,ctx->subcomm->comm,MAT_INITIAL_MATRIX,&ctx->pA);CHKERRQ(ierr);
    if (nmat>1) {
      ierr = STGetOperators(eps->st,1,&B);CHKERRQ(ierr);
      ierr = MatGetRedundantMatrix(B,ctx->subcomm->n,ctx->subcomm->comm,MAT_INITIAL_MATRIX,&ctx->pB);CHKERRQ(ierr); 
    } else ctx->pB = NULL;
  } else {
    ctx->pA = NULL;
    ctx->pB = NULL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CISSScatterVec"
static PetscErrorCode CISSScatterVec(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  IS             is1,is2;
  PetscMPIInt    subrank,subsize;
  PetscInt       i,j,k,mstart,mend,mlocal;
  PetscInt       *idx1,*idx2,rstart_sub,rend_sub,mloc_sub;
  const PetscInt *range;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(ctx->subcomm->comm,&subrank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(ctx->subcomm->comm,&subsize);CHKERRQ(ierr);
  ierr = VecGetOwnershipRanges(ctx->V[0],&range);CHKERRQ(ierr); /*end index of mat A*/
  rstart_sub = range[ctx->subcomm->n*subrank]; 
  if (subrank+1 < subsize) rend_sub = range[ctx->subcomm->n*(subrank+1)];
  else rend_sub = eps->n;
  mloc_sub = rend_sub - rstart_sub;
  ierr = MatGetVecs(ctx->pA,&ctx->xsub,NULL);CHKERRQ(ierr);
  ierr = VecCreateMPI(ctx->subcomm->dupparent,mloc_sub,PETSC_DECIDE,&ctx->xdup);CHKERRQ(ierr);
  if (!ctx->scatterin) {
    ierr = VecGetOwnershipRange(ctx->V[0],&mstart,&mend);CHKERRQ(ierr);
    mlocal = mend - mstart;
    ierr = PetscMalloc2(ctx->subcomm->n*mlocal,&idx1,ctx->subcomm->n*mlocal,&idx2);CHKERRQ(ierr);
    j = 0;
    for (k=0;k<ctx->subcomm->n;k++) {
      for (i=mstart;i<mend;i++) {
	idx1[j]   = i;
	idx2[j++] = i + eps->n*k;
      }
    }
    ierr = ISCreateGeneral(PetscObjectComm((PetscObject)eps),ctx->subcomm->n*mlocal,idx1,PETSC_COPY_VALUES,&is1);CHKERRQ(ierr);
    ierr = ISCreateGeneral(PetscObjectComm((PetscObject)eps),ctx->subcomm->n*mlocal,idx2,PETSC_COPY_VALUES,&is2);CHKERRQ(ierr);
    ierr = VecScatterCreate(ctx->V[0],is1,ctx->xdup,is2,&ctx->scatterin);CHKERRQ(ierr);
    ierr = ISDestroy(&is1);CHKERRQ(ierr);
    ierr = ISDestroy(&is2);CHKERRQ(ierr);
    PetscFree2(idx1,idx2);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SetPathParameter"
static PetscErrorCode SetPathParameter(EPS eps)
{
  EPS_CISS  *ctx = (EPS_CISS*)eps->data;
  PetscInt  i;
  PetscReal theta;

  PetscFunctionBegin;
  for (i=0;i<ctx->N;i++) {
    theta = ((2*PETSC_PI)/ctx->N)*(i+0.5);
    ctx->pp[i] = cos(theta) + PETSC_i*ctx->vscale*sin(theta);
    ctx->omega[i] = ctx->center + ctx->radius*ctx->pp[i];
    ctx->weight[i] = (ctx->vscale*cos(theta) + PETSC_i*sin(theta))/(PetscReal)ctx->N;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CISSVecSetRandom"
static PetscErrorCode CISSVecSetRandom(Vec x,PetscRandom rctx)
{
  PetscErrorCode ierr;
  PetscInt       j,nlocal;
  PetscScalar    *vdata;
 
  PetscFunctionBegin;
  ierr = SlepcVecSetRandom(x,rctx);CHKERRQ(ierr);
  ierr = VecGetLocalSize(x,&nlocal);CHKERRQ(ierr);
  ierr = VecGetArray(x,&vdata);CHKERRQ(ierr);
  for (j=0;j<nlocal;j++) {
    vdata[j] = PetscRealPart(vdata[j]);
    if (PetscRealPart(vdata[j]) < 0.5) vdata[j] = -1.0;
    else vdata[j] = 1.0;
  }
  ierr = VecRestoreArray(x,&vdata);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecScatterVecs"
PetscErrorCode VecScatterVecs(EPS eps,Vec *Vin,PetscInt n)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       i;
  PetscScalar    *array;

  PetscFunctionBegin;
  for (i=0;i<n;i++) {
    ierr = VecScatterBegin(ctx->scatterin,Vin[i],ctx->xdup,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(ctx->scatterin,Vin[i],ctx->xdup,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArray(ctx->xdup,&array);CHKERRQ(ierr);
    ierr = VecPlaceArray(ctx->xsub,(const PetscScalar*)array);CHKERRQ(ierr);
    ierr = VecCopy(ctx->xsub,ctx->pV[i]);CHKERRQ(ierr);
    ierr = VecResetArray(ctx->xsub);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SolveLinearSystem"
static PetscErrorCode SolveLinearSystem(EPS eps,Mat A,Mat B,Vec *V,PetscInt L_start,PetscInt L_end,PetscBool initksp)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       i,j,p_id;
  Mat            Fz;
  PC             pc;
  Vec            BV;

  PetscFunctionBegin;
  ierr = MatDuplicate(A,MAT_DO_NOT_COPY_VALUES,&Fz);CHKERRQ(ierr);
  ierr = VecDuplicate(V[0],&BV);CHKERRQ(ierr);

  for (i=0;i<ctx->num_solve_point;i++) {
    if (ctx->num_solve_point !=1 || initksp == PETSC_TRUE) {
      p_id = i*ctx->subcomm->n + ctx->subcomm_id;
      ierr = MatCopy(A,Fz,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
      if (B) {
	ierr = MatAXPY(Fz,-ctx->omega[p_id],B,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
      } else {
	ierr = MatShift(Fz,-ctx->omega[p_id]);CHKERRQ(ierr);
      }
      ierr = KSPSetOperators(ctx->ksp,Fz,Fz,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
      ierr = KSPSetType(ctx->ksp,KSPPREONLY);CHKERRQ(ierr);
      ierr = KSPGetPC(ctx->ksp,&pc);CHKERRQ(ierr);
      ierr = PCSetType(pc,PCREDUNDANT);CHKERRQ(ierr);
      ierr = KSPSetFromOptions(ctx->ksp);CHKERRQ(ierr);
    }
    for (j=L_start;j<L_end;j++) {
      if (B) {
	ierr = MatMult(B,V[j],BV);CHKERRQ(ierr);
	ierr = KSPSolve(ctx->ksp,BV,ctx->Y[i*ctx->L_max+j]);CHKERRQ(ierr);
      } else {
	ierr = KSPSolve(ctx->ksp,V[j],ctx->Y[i*ctx->L_max+j]);CHKERRQ(ierr);
      }
    }
  }
  ierr = MatDestroy(&Fz);CHKERRQ(ierr);
  ierr = VecDestroy(&BV);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EstimateNumberEigs"
static PetscErrorCode EstimateNumberEigs(EPS eps,PetscInt *L_add)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       i,j,p_id;
  PetscScalar    tmp,sum = 0.0;
  PetscReal      eta;
  Vec            v,vtemp;

  PetscFunctionBegin;
  ierr = VecDuplicate(ctx->Y[0],&v);CHKERRQ(ierr);
  ierr = VecDuplicate(ctx->V[0],&vtemp);CHKERRQ(ierr);
  for (j=0;j<ctx->L;j++) {
    ierr = VecSet(v,0);CHKERRQ(ierr);
    for (i=0;i<ctx->num_solve_point; i++) {
      p_id = i*ctx->subcomm->n + ctx->subcomm_id;
      ierr = VecAXPY(v,ctx->weight[p_id],ctx->Y[i*ctx->L_max+j]);CHKERRQ(ierr);
    }
    if (ctx->pA) {
      ierr = VecSet(vtemp,0);CHKERRQ(ierr);
      ierr = VecScatterBegin(ctx->scatterin,v,vtemp,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
      ierr = VecScatterEnd(ctx->scatterin,v,vtemp,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
      ierr = VecDot(ctx->V[j],vtemp,&tmp);CHKERRQ(ierr);
    } else {
      ierr = VecDot(ctx->V[j],v,&tmp);CHKERRQ(ierr);
    }
    if (ctx->useconj) sum += PetscRealPart(tmp)*2;
    else sum += tmp;
  }
  ctx->est_eig = PetscAbsScalar(ctx->radius*sum/(PetscReal)ctx->L);
  eta = PetscPowReal(10,-log10(eps->tol)/ctx->N);
  ierr = PetscInfo1(eps,"Estimation_#Eig %f\n",(double)ctx->est_eig);CHKERRQ(ierr);
  *L_add = (PetscInt)ceil((ctx->est_eig*eta)/ctx->M) - ctx->L;
  if (*L_add < 0) *L_add = 0;
  if (*L_add>ctx->L_max-ctx->L) {
    ierr = PetscInfo(eps,"Number of eigenvalues around the contour path may be too large\n");CHKERRQ(ierr);
    *L_add = ctx->L_max-ctx->L;
  }
  ierr = VecDestroy(&v);CHKERRQ(ierr);
  ierr = VecDestroy(&vtemp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CalcMu"
static PetscErrorCode CalcMu(EPS eps,PetscScalar *Mu)
{
  PetscErrorCode ierr;
  PetscMPIInt    sub_size;
  PetscInt       i,j,k,s;
  PetscScalar    *temp,*temp2,*ppk,alp;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(ctx->subcomm->comm,&sub_size);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->num_solve_point*ctx->L*ctx->L*sizeof(PetscScalar),&temp);CHKERRQ(ierr);
  ierr = PetscMalloc(2*ctx->M*ctx->L*ctx->L*sizeof(PetscScalar),&temp2);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->num_solve_point*sizeof(PetscScalar),&ppk);CHKERRQ(ierr);
  for (i=0;i<2*ctx->M*ctx->L*ctx->L;i++) temp2[i] = 0;
  for (i=0;i<ctx->num_solve_point;i++) {
    for (j=0;j<ctx->L;j++) {
      if (ctx->pA) {
	ierr = VecMDot(ctx->Y[i*ctx->L_max+j],ctx->L,ctx->pV,&temp[(j+i*ctx->L)*ctx->L]);CHKERRQ(ierr);
      } else {
	ierr = VecMDot(ctx->Y[i*ctx->L_max+j],ctx->L,ctx->V,&temp[(j+i*ctx->L)*ctx->L]);CHKERRQ(ierr);
      }
    }
  }
  for (i=0;i<ctx->num_solve_point;i++) ppk[i] = 1;
  for (k=0;k<2*ctx->M;k++) {
    for (j=0;j<ctx->L;j++) {
      for (i=0;i<ctx->num_solve_point;i++) {
	alp = ppk[i]*ctx->weight[i*ctx->subcomm->n + ctx->subcomm_id];
	for (s=0;s<ctx->L;s++) {
	  if (ctx->useconj) temp2[s+(j+k*ctx->L)*ctx->L] += PetscRealPart(alp*temp[s+(j+i*ctx->L)*ctx->L])*2;
	  else temp2[s+(j+k*ctx->L)*ctx->L] += alp*temp[s+(j+i*ctx->L)*ctx->L];
	}
      }
    }
    for (i=0;i<ctx->num_solve_point;i++) 
      ppk[i] *= ctx->pp[i*ctx->subcomm->n + ctx->subcomm_id];
  }
  for (i=0;i<2*ctx->M*ctx->L*ctx->L;i++) temp2[i] /= sub_size;
  ierr = MPI_Allreduce(temp2,Mu,2*ctx->M*ctx->L*ctx->L,MPIU_SCALAR,MPIU_SUM,(PetscObjectComm((PetscObject)eps)));CHKERRQ(ierr);
  ierr = PetscFree(ppk);CHKERRQ(ierr);
  ierr = PetscFree(temp);CHKERRQ(ierr);
  ierr = PetscFree(temp2);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BlockHankel"
static PetscErrorCode BlockHankel(EPS eps,PetscScalar *Mu,PetscInt s,PetscScalar *H)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;
  PetscInt i,j,k,L=ctx->L,M=ctx->M;

  PetscFunctionBegin;
  for (k=0;k<L*M;k++) 
    for (j=0;j<M;j++) 
      for (i=0;i<L;i++)
	H[j*L+i+k*L*M] = Mu[i+k*L+(j+s)*L*L];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVD_H0"
static PetscErrorCode SVD_H0(EPS eps,PetscScalar *S,PetscInt *K)
{
#if defined(SLEPC_MISSING_LAPACK_GESVD)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"GESVD - Lapack routine is unavailable");
#else
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       i,ml=ctx->L*ctx->M;
  PetscBLASInt   m,n,lda,ldu,ldvt,lwork,info;
  PetscReal      *rwork;
  PetscScalar    *work;

  PetscFunctionBegin;
  ierr = PetscMalloc(3*ml*sizeof(PetscScalar),&work);CHKERRQ(ierr);
  ierr = PetscMalloc(5*ml*sizeof(PetscReal),&rwork);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(ml,&m);CHKERRQ(ierr);
  n = m; lda = m; ldu = m; ldvt = m; lwork = 3*m;
  ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
  PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("N","N",&m,&n,S,&lda,ctx->sigma,NULL,&ldu,NULL,&ldvt,work,&lwork,rwork,&info));
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xGESVD %d",info);
  ierr = PetscFPTrapPop();CHKERRQ(ierr);
  (*K) = 0;
  for (i=0;i<ml;i++) {
    if (ctx->sigma[i]/PetscMax(ctx->sigma[0],1)>ctx->delta) (*K)++;
  }
  ierr = PetscFree(work);CHKERRQ(ierr);
  ierr = PetscFree(rwork);CHKERRQ(ierr);
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__
#define __FUNCT__ "ConstructS"
static PetscErrorCode ConstructS(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       i,j,k,vec_local_size,p_id;
  Vec            v;
  PetscScalar    *ppk, *v_data;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(ctx->Y[0],&vec_local_size);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->num_solve_point*sizeof(PetscScalar),&ppk);CHKERRQ(ierr);
  for (i=0;i<ctx->num_solve_point;i++) ppk[i] = 1;
  ierr = VecDuplicate(ctx->Y[0],&v);CHKERRQ(ierr);
  for (k=0;k<ctx->M;k++) {
    for (j=0;j<ctx->L;j++) {
      ierr = VecSet(v,0);CHKERRQ(ierr);
      for (i=0;i<ctx->num_solve_point;i++) {
	p_id = i*ctx->subcomm->n + ctx->subcomm_id;
	ierr = VecAXPY(v,ppk[i]*ctx->weight[p_id],ctx->Y[i*ctx->L_max+j]);CHKERRQ(ierr);
      }
      if (ctx->useconj) {
	ierr = VecGetArray(v,&v_data);CHKERRQ(ierr);
	for (i=0;i<vec_local_size;i++) v_data[i] = PetscRealPart(v_data[i])*2;
	ierr = VecRestoreArray(v,&v_data);CHKERRQ(ierr);
      }
      if (ctx->pA) {
	ierr = VecSet(ctx->S[k*ctx->L+j],0);CHKERRQ(ierr);
	ierr = VecScatterBegin(ctx->scatterin,v,ctx->S[k*ctx->L+j],ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
	ierr = VecScatterEnd(ctx->scatterin,v,ctx->S[k*ctx->L+j],ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
      } else {
	ierr = VecCopy(v,ctx->S[k*ctx->L+j]);CHKERRQ(ierr);
      }
    }
    for (i=0;i<ctx->num_solve_point;i++) {
      p_id = i*ctx->subcomm->n + ctx->subcomm_id;
      ppk[i] *= ctx->pp[p_id];
    }
  }
  ierr = PetscFree(ppk);CHKERRQ(ierr);
  ierr = VecDestroy(&v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVD_S"
static PetscErrorCode SVD_S(Vec *S,PetscInt ml,PetscReal delta,PetscReal *sigma,PetscInt *K)
{
#if defined(SLEPC_MISSING_LAPACK_GESVD)
  PetscFunctionBegin;
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"GESVD - Lapack routine is unavailable");
#else
  PetscErrorCode ierr;
  PetscInt       i,j,k,local_size;
  PetscReal      *rwork;
  PetscScalar    *work,*temp,*B,*tempB,*s_data,*Q1,*Q2,*temp2,alpha=1,beta=0;
  PetscBLASInt   l,m,n,lda,ldu,ldvt,lwork,info,ldb,ldc;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(S[0],&local_size);CHKERRQ(ierr);    
  ierr = VecGetArray(S[0],&s_data);CHKERRQ(ierr);
  ierr = PetscMalloc(ml*ml*sizeof(PetscScalar),&temp);CHKERRQ(ierr);
  ierr = PetscMalloc(ml*ml*sizeof(PetscScalar),&temp2);CHKERRQ(ierr);
  ierr = PetscMalloc(local_size*ml*sizeof(PetscScalar),&Q1);CHKERRQ(ierr);
  ierr = PetscMalloc(local_size*ml*sizeof(PetscScalar),&Q2);CHKERRQ(ierr);
  ierr = PetscMalloc(ml*ml*sizeof(PetscScalar),&B);CHKERRQ(ierr);
  ierr = PetscMemzero(B,ml*ml*sizeof(PetscScalar));CHKERRQ(ierr);
  ierr = PetscMalloc(3*ml*sizeof(PetscScalar),&work);CHKERRQ(ierr);
  ierr = PetscMalloc(5*ml*sizeof(PetscReal),&rwork);CHKERRQ(ierr);
  ierr = PetscMalloc(ml*ml*sizeof(PetscScalar),&tempB);CHKERRQ(ierr);
  ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);

  for (i=0;i<ml;i++) {
    B[i*ml+i]=1;
  }

  for (k=0;k<2;k++) {
    ierr = PetscBLASIntCast(local_size,&m);CHKERRQ(ierr);
    ierr = PetscBLASIntCast(ml,&l);CHKERRQ(ierr);
    n = l; lda = m; ldb = m; ldc = l;
    if (k == 0) {
      PetscStackCallBLAS("BLASgemm",BLASgemm_("C","N",&l,&n,&m,&alpha,s_data,&lda,s_data,&ldb,&beta,temp,&ldc));
    } else if ((k%2)==1) {
      PetscStackCallBLAS("BLASgemm",BLASgemm_("C","N",&l,&n,&m,&alpha,Q1,&lda,Q1,&ldb,&beta,temp,&ldc));
    } else {
      PetscStackCallBLAS("BLASgemm",BLASgemm_("C","N",&l,&n,&m,&alpha,Q2,&lda,Q2,&ldb,&beta,temp,&ldc));
    }
    ierr = PetscMemzero(temp2,ml*ml*sizeof(PetscScalar));CHKERRQ(ierr);
    ierr = MPI_Allreduce(temp,temp2,ml*ml,MPIU_SCALAR,MPIU_SUM,(PetscObjectComm((PetscObject)S[0])));CHKERRQ(ierr);

    ierr = PetscBLASIntCast(ml,&m);CHKERRQ(ierr);
    n = m; lda = m; lwork = 3*m, ldu = 1; ldvt = 1;
    PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("O","N",&m,&n,temp2,&lda,sigma,NULL,&ldu,NULL,&ldvt,work,&lwork,rwork,&info));
    if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xGESVD %d",info);

    ierr = PetscBLASIntCast(local_size,&l);CHKERRQ(ierr);
    ierr = PetscBLASIntCast(ml,&n);CHKERRQ(ierr);
    m = n; lda = l; ldb = m; ldc = l;
    if (k==0) {
      PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&m,&alpha,s_data,&lda,temp2,&ldb,&beta,Q1,&ldc));
    } else if((k%2)==1) {
      PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&m,&alpha,Q1,&lda,temp2,&ldb,&beta,Q2,&ldc));
    } else {
      PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&m,&alpha,Q2,&lda,temp2,&ldb,&beta,Q1,&ldc));
    }

    ierr = PetscBLASIntCast(ml,&l);CHKERRQ(ierr);
    m = l; n = l; lda = l; ldb = m; ldc = l;
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&l,&n,&m,&alpha,B,&lda,temp2,&ldb,&beta,tempB,&ldc));
    for (i=0;i<ml;i++) {
      sigma[i] = sqrt(sigma[i]);
      for (j=0;j<local_size;j++) {
	if ((k%2)==1) Q2[j+i*local_size]/=sigma[i];
	else Q1[j+i*local_size]/=sigma[i];
      }
      for (j=0;j<ml;j++) {
	B[j+i*ml]=tempB[j+i*ml]*sigma[i];
      }
    }
  }

  ierr = PetscBLASIntCast(ml,&m);CHKERRQ(ierr);
  n = m; lda = m; ldu=1; ldvt=1;
  PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("N","O",&m,&n,B,&lda,sigma,NULL,&ldu,NULL,&ldvt,work,&lwork,rwork,&info));
  if (info) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in Lapack xGESVD %d",info);

  ierr = PetscBLASIntCast(local_size,&l);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(ml,&n);CHKERRQ(ierr);
  m = n; lda = l; ldb = m; ldc = l;
  if ((k%2)==1) {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","T",&l,&n,&m,&alpha,Q1,&lda,B,&ldb,&beta,s_data,&ldc));
  } else {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","T",&l,&n,&m,&alpha,Q2,&lda,B,&ldb,&beta,s_data,&ldc));
  }
 
  ierr = PetscFPTrapPop();CHKERRQ(ierr);
  ierr = VecRestoreArray(S[0],&s_data);CHKERRQ(ierr);
  ierr = PetscFree(temp2);CHKERRQ(ierr);
  ierr = PetscFree(Q1);CHKERRQ(ierr);
  ierr = PetscFree(Q2);CHKERRQ(ierr);

  (*K) = 0;
  for (i=0;i<ml;i++) {
    if (sigma[i]/PetscMax(sigma[0],1)>delta) (*K)++;
  }
  ierr = PetscFree(temp);CHKERRQ(ierr);
  ierr = PetscFree(B);CHKERRQ(ierr);
  ierr = PetscFree(tempB);CHKERRQ(ierr);
  ierr = PetscFree(work);CHKERRQ(ierr);
  ierr = PetscFree(rwork);CHKERRQ(ierr);
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__
#define __FUNCT__ "ProjectMatrix"
static PetscErrorCode ProjectMatrix(Mat A,Mat B,PetscInt nv,PetscInt ld,Vec *Q,PetscScalar center,PetscScalar *PA,PetscScalar *PB,Vec w,PetscBool isherm)
{
  PetscErrorCode ierr;
  PetscInt       i,j;

  PetscFunctionBegin;
  if (isherm) {
    for (j=0;j<nv;j++) {
      if (B) {
	ierr = MatMult(B,Q[j],w);CHKERRQ(ierr);
	ierr = VecMDot(w,j+1,Q,PB+j*ld);CHKERRQ(ierr);
      } else {
	ierr = VecMDot(Q[j],j+1,Q,PB+j*ld);CHKERRQ(ierr);
      }
      ierr = MatMult(A,Q[j],w);CHKERRQ(ierr);
      ierr = VecMDot(w,j+1,Q,PA+j*ld);CHKERRQ(ierr);
      for (i=0;i<j+1;i++) PA[i+j*ld]-= center*PB[i+j*ld];
      for (i=0;i<j;i++) {
	PB[j+i*ld] = PetscConj(PB[i+j*ld]);
	PA[j+i*ld] = PetscConj(PA[i+j*ld]);
      }
    }
  } else {
    for (j=0;j<nv;j++) {
      if (B) {
	ierr = MatMult(B,Q[j],w);CHKERRQ(ierr);
	ierr = VecMDot(w,nv,Q,PB+j*ld);CHKERRQ(ierr);
      } else {
	ierr = VecMDot(Q[j],nv,Q,PB+j*ld);CHKERRQ(ierr);
      }
      ierr = MatMult(A,Q[j],w);CHKERRQ(ierr);
      ierr = VecMDot(w,nv,Q,PA+j*ld);CHKERRQ(ierr);
      for (i=0;i<nv;i++) PA[i+j*ld] -= center*PB[i+j*ld];
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "isGhost"
static PetscErrorCode isGhost(EPS eps,PetscInt ld,PetscInt nv,PetscBool *fl)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscInt       i,j;
  PetscScalar    *pX;
  PetscReal      *tau,s1,s2,tau_max=0.0;

  PetscFunctionBegin;
  ierr = PetscMalloc(nv*sizeof(PetscReal),&tau);CHKERRQ(ierr);
  ierr = DSVectors(eps->ds,DS_MAT_X,NULL,NULL);CHKERRQ(ierr);
  ierr = DSGetArray(eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);

  for (i=0;i<nv;i++) {
    s1 = 0;
    s2 = 0;
    for (j=0;j<nv;j++) {
      s1 += PetscAbsScalar(PetscPowScalar(pX[i*ld+j],2));
      s2 += PetscPowReal(PetscAbsScalar(pX[i*ld+j]),2)/ctx->sigma[j];
    }
    tau[i] = s1/s2;
    tau_max = PetscMax(tau_max,tau[i]);
  }
  ierr = DSRestoreArray(eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);
  for (i=0;i<nv;i++) {
    tau[i] /= tau_max;
  }
  for (i=0;i<nv;i++) {
    if (tau[i]>=ctx->spurious_threshold) {
      fl[i] = PETSC_TRUE;
    } else fl[i] = PETSC_FALSE;
  }
  ierr = PetscFree(tau);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "isInsideGamma"
static PetscErrorCode isInsideGamma(EPS eps,PetscInt nv,PetscBool *fl)
{
  EPS_CISS    *ctx = (EPS_CISS*)eps->data;
  PetscInt    i;
  PetscScalar d;
  PetscReal   dx,dy;

  PetscFunctionBegin;
  for (i=0;i<nv;i++) {
    d = (eps->eigr[i]-ctx->center)/ctx->radius;
    dx = PetscRealPart(d);
    dy = PetscImaginaryPart(d);
    if ((dx*dx+(dy*dy)/(ctx->vscale*ctx->vscale))<=1) fl[i] = PETSC_TRUE;
    else fl[i] = PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSetUp_CISS"
PetscErrorCode EPSSetUp_CISS(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  const char     *prefix;

  PetscFunctionBegin;
  eps->ncv = PetscMin(eps->n,ctx->L_max*ctx->M);
  if (!eps->mpd) eps->mpd = eps->ncv;
  if (!eps->which) eps->which = EPS_ALL;
  if (!eps->extraction) { ierr = EPSSetExtraction(eps,EPS_RITZ);CHKERRQ(ierr); } 
  else if (eps->extraction!=EPS_RITZ) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Unsupported extraction type");
  if (eps->arbitrary) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Arbitrary selection of eigenpairs not supported in this solver");

  if (ctx->isreal && PetscImaginaryPart(ctx->center) == 0.0) ctx->useconj = PETSC_TRUE;
  else ctx->useconj = PETSC_FALSE;

  if (!ctx->vscale) {
    if (eps->ishermitian && (eps->ispositive || !eps->isgeneralized) && PetscImaginaryPart(ctx->center) == 0.0) ctx->vscale = 0.1;
    else ctx->vscale = 1.0;
  }

  /* create split comm */
  ierr = SetSolverComm(eps);CHKERRQ(ierr);

  ierr = EPSAllocateSolution(eps,0);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->N*sizeof(PetscScalar),&ctx->weight);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->N*sizeof(PetscScalar),&ctx->omega);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->N*sizeof(PetscScalar),&ctx->pp);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)eps,3*ctx->N*sizeof(PetscScalar));CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->L_max*ctx->M*sizeof(PetscReal),&ctx->sigma);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)eps,ctx->L_max*ctx->N*sizeof(PetscReal));CHKERRQ(ierr);

  /* create a template vector for Vecs on solver communicator */
  ierr = VecDuplicateVecs(eps->t,ctx->L_max*ctx->M,&ctx->S);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(eps,ctx->L_max*ctx->M,ctx->S);CHKERRQ(ierr);
  ierr = VecDuplicateVecs(eps->t,ctx->L_max,&ctx->V);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(eps,ctx->L_max,ctx->V);CHKERRQ(ierr);

  ierr = CISSRedundantMat(eps);CHKERRQ(ierr);
  if (ctx->pA) {
    ierr = CISSScatterVec(eps);CHKERRQ(ierr);
    ierr = VecDuplicateVecs(ctx->xsub,ctx->L_max,&ctx->pV);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)eps,ctx->L_max*sizeof(Vec));CHKERRQ(ierr);
  }
  ierr = KSPCreate(ctx->subcomm->comm,&ctx->ksp);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)eps,sizeof(KSP));CHKERRQ(ierr);
  ierr = PetscObjectIncrementTabLevel((PetscObject)ctx->ksp,(PetscObject)eps,1);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)eps,(PetscObject)ctx->ksp);CHKERRQ(ierr);
  ierr = KSPAppendOptionsPrefix(ctx->ksp,"eps_ciss_");CHKERRQ(ierr);
  ierr = EPSGetOptionsPrefix(eps,&prefix);CHKERRQ(ierr);
  ierr = KSPAppendOptionsPrefix(ctx->ksp,prefix);CHKERRQ(ierr);
  if (ctx->pA) {
    ierr = VecDuplicateVecs(ctx->xsub,ctx->num_solve_point*ctx->L_max,&ctx->Y);CHKERRQ(ierr);
  } else {
    ierr = VecDuplicateVecs(ctx->V[0],ctx->num_solve_point*ctx->L_max,&ctx->Y);CHKERRQ(ierr);
  }
  ierr = PetscLogObjectParents(eps,ctx->num_solve_point*ctx->L_max,ctx->Y);CHKERRQ(ierr);

  if (eps->ishermitian && eps->ispositive) {
    ierr = DSSetType(eps->ds,DSGHEP);CHKERRQ(ierr);
  } else {
    ierr = DSSetType(eps->ds,DSGNHEP);CHKERRQ(ierr);
  }
  ierr = DSAllocate(eps->ds,eps->ncv);CHKERRQ(ierr);
  ierr = EPSSetWorkVecs(eps,2);CHKERRQ(ierr);
  
  /* dispatch solve method */
  if (eps->leftvecs) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Left vectors not supported in this solver");
  eps->ops->solve = EPSSolve_CISS;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSolve_CISS"
PetscErrorCode EPSSolve_CISS(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  Mat            A,B;
  PetscInt       i,ld,nmat,L_add=0,nv,L_base=ctx->L,inner,outer,nlocal;
  PetscScalar    *Mu,*H0,*H1,*rr,*pX,*temp;
  PetscReal      error,max_error;
  PetscBool      *fl1,*fl2;
  Vec            w=eps->work[0];

  PetscFunctionBegin;
  ierr = VecGetLocalSize(w,&nlocal);CHKERRQ(ierr);
  ierr = DSGetLeadingDimension(eps->ds,&ld);CHKERRQ(ierr);
  ierr = STGetNumMatrices(eps->st,&nmat);CHKERRQ(ierr);
  ierr = STGetOperators(eps->st,0,&A);CHKERRQ(ierr);
  if (nmat>1) { ierr = STGetOperators(eps->st,1,&B);CHKERRQ(ierr); }
  else B = NULL;
  ierr = SetPathParameter(eps);CHKERRQ(ierr);
  for (i=0;i<ctx->L;i++) {
    ierr = CISSVecSetRandom(ctx->V[i],eps->rand);CHKERRQ(ierr);
  }

  if (ctx->pA) {
    ierr = VecScatterVecs(eps,ctx->V,ctx->L);CHKERRQ(ierr);
    ierr = SolveLinearSystem(eps,ctx->pA,ctx->pB,ctx->pV,0,ctx->L,PETSC_TRUE);CHKERRQ(ierr);
  } else {
    ierr = SolveLinearSystem(eps,A,B,ctx->V,0,ctx->L,PETSC_TRUE);CHKERRQ(ierr);
  }

  ierr = EstimateNumberEigs(eps,&L_add);CHKERRQ(ierr);
  if (L_add>0) {
    ierr = PetscInfo2(eps,"Changing L %d -> %d by Estimate #Eig\n",ctx->L,ctx->L+L_add);CHKERRQ(ierr);
    for (i=ctx->L;i<ctx->L+L_add;i++) {
      ierr = CISSVecSetRandom(ctx->V[i],eps->rand);CHKERRQ(ierr);
    }
    if (ctx->pA) {
      ierr = VecScatterVecs(eps,ctx->V,ctx->L+L_add);CHKERRQ(ierr);
      ierr = SolveLinearSystem(eps,ctx->pA,ctx->pB,ctx->pV,ctx->L,ctx->L+L_add,PETSC_FALSE);CHKERRQ(ierr);
    } else {
      ierr = SolveLinearSystem(eps,A,B,ctx->V,ctx->L,ctx->L+L_add,PETSC_FALSE);CHKERRQ(ierr);
    }
    ctx->L += L_add;
  }
  ierr = PetscMalloc(ctx->L*ctx->L*ctx->M*2*sizeof(PetscScalar),&Mu);CHKERRQ(ierr);
  ierr = PetscMalloc(ctx->L*ctx->M*ctx->L*ctx->M*sizeof(PetscScalar),&H0);CHKERRQ(ierr);
  for (i=0;i<ctx->refine_blocksize;i++) {
    ierr = CalcMu(eps,Mu);CHKERRQ(ierr);
    ierr = BlockHankel(eps,Mu,0,H0);CHKERRQ(ierr);
    ierr = SVD_H0(eps,H0,&nv);CHKERRQ(ierr);
    if (ctx->sigma[0]<=ctx->delta || nv < ctx->L*ctx->M || ctx->L == ctx->L_max) break;
    L_add = L_base;
    if (ctx->L+L_add>ctx->L_max) L_add = ctx->L_max-ctx->L;
    ierr = PetscInfo2(eps,"Changing L %d -> %d by SVD(H0)\n",ctx->L,ctx->L+L_add);CHKERRQ(ierr);
    for (i=ctx->L;i<ctx->L+L_add;i++) {
      ierr = CISSVecSetRandom(ctx->V[i],eps->rand);CHKERRQ(ierr);
    }
    if (ctx->pA) {
      ierr = VecScatterVecs(eps,ctx->V,ctx->L+L_add);CHKERRQ(ierr);
      ierr = SolveLinearSystem(eps,ctx->pA,ctx->pB,ctx->pV,ctx->L,ctx->L+L_add,PETSC_FALSE);CHKERRQ(ierr);
    } else {
      ierr = SolveLinearSystem(eps,A,B,ctx->V,ctx->L,ctx->L+L_add,PETSC_FALSE);CHKERRQ(ierr);
    }
    ctx->L += L_add;
  }
  ierr = PetscFree(Mu);CHKERRQ(ierr);
  ierr = PetscFree(H0);CHKERRQ(ierr);

  for (outer=0;outer<=ctx->refine_outer;outer++) {
    for (inner=0;inner<=ctx->refine_inner;inner++) {
      ierr = ConstructS(eps);CHKERRQ(ierr);
      for (i=0;i<ctx->L;i++) {
	ierr = VecCopy(ctx->S[i],ctx->V[i]);CHKERRQ(ierr);
      }
      ierr = SVD_S(ctx->S,ctx->L*ctx->M,ctx->delta,ctx->sigma,&nv);CHKERRQ(ierr);
      if (ctx->sigma[0]>ctx->delta && nv==ctx->L*ctx->M && inner!=ctx->refine_inner) {
	if (ctx->pA) {
	  ierr = VecScatterVecs(eps,ctx->V,ctx->L);CHKERRQ(ierr);
	  ierr = SolveLinearSystem(eps,ctx->pA,ctx->pB,ctx->pV,0,ctx->L,PETSC_FALSE);CHKERRQ(ierr);
	} else {
	  ierr = SolveLinearSystem(eps,A,B,ctx->V,0,ctx->L,PETSC_FALSE);CHKERRQ(ierr);
	}
      } else break;
    }

    eps->nconv = 0;
    if (nv == 0) break;
    ierr = DSSetDimensions(eps->ds,nv,0,0,0);CHKERRQ(ierr);
    ierr = DSSetState(eps->ds,DS_STATE_RAW);CHKERRQ(ierr);

    ierr = DSGetArray(eps->ds,DS_MAT_B,&H1);CHKERRQ(ierr);
    ierr = DSGetArray(eps->ds,DS_MAT_A,&H0);CHKERRQ(ierr);
    ierr = ProjectMatrix(A,B,nv,ld,ctx->S,ctx->center,H0,H1,w,eps->ishermitian);CHKERRQ(ierr);
    ierr = DSRestoreArray(eps->ds,DS_MAT_A,&H0);CHKERRQ(ierr);
    ierr = DSRestoreArray(eps->ds,DS_MAT_B,&H1);CHKERRQ(ierr);

    ierr = DSSolve(eps->ds,eps->eigr,NULL);CHKERRQ(ierr);
    ierr = DSVectors(eps->ds,DS_MAT_X,NULL,NULL);CHKERRQ(ierr);
    ierr = DSGetArray(eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);
    ierr = DSRestoreArray(eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);

    for (i=0;i<nv;i++) {
      eps->eigr[i]+=ctx->center;
    }

    ierr = PetscMalloc(nv*sizeof(PetscBool),&fl1);CHKERRQ(ierr);
    ierr = PetscMalloc(nv*sizeof(PetscBool),&fl2);CHKERRQ(ierr);
    ierr = isGhost(eps,ld,nv,fl1);CHKERRQ(ierr);
    ierr = isInsideGamma(eps,nv,fl2);CHKERRQ(ierr);
    ierr = PetscMalloc(nv*sizeof(PetscScalar),&rr);CHKERRQ(ierr);
    for (i=0;i<nv;i++) {
      if (fl1[i] && fl2[i]) {
	rr[i] = 1.0;
	eps->nconv++;
      } else rr[i] = 0.0;
    }
    ierr = PetscFree(fl1);CHKERRQ(ierr);
    ierr = PetscFree(fl2);CHKERRQ(ierr);
    ierr = DSSetEigenvalueComparison(eps->ds,SlepcCompareLargestMagnitude,NULL);CHKERRQ(ierr);
    ierr = DSSort(eps->ds,eps->eigr,NULL,rr,NULL,&eps->nconv);CHKERRQ(ierr);
    for (i=0;i<nv;i++) {
      eps->eigr[i]+=ctx->center;
    }
    ierr = DSSetEigenvalueComparison(eps->ds,eps->comparison,eps->comparisonctx);CHKERRQ(ierr);
    ierr = PetscFree(rr);CHKERRQ(ierr);
    for (i=0;i<nv;i++) {
      ierr = VecCopy(ctx->S[i],eps->V[i]);CHKERRQ(ierr);
    }

    ierr = DSVectors(eps->ds,DS_MAT_X,NULL,NULL);CHKERRQ(ierr);
    ierr = DSGetArray(eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);
    ierr = SlepcUpdateVectors(nv,ctx->S,0,eps->nconv,pX,ld,PETSC_FALSE);CHKERRQ(ierr);
    if (eps->ishermitian) {
      ierr = SlepcUpdateVectors(nv,eps->V,0,eps->nconv,pX,ld,PETSC_FALSE);CHKERRQ(ierr);
    }
    ierr = DSRestoreArray(eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);
    max_error = 0.0;
    for (i=0;i<eps->nconv;i++) {
      ierr = VecNormalize(ctx->S[i],NULL);CHKERRQ(ierr);
      ierr = EPSComputeRelativeError_Private(eps,eps->eigr[i],0,ctx->S[i],NULL,&error);CHKERRQ(ierr);
      max_error = PetscMax(max_error,error);
    }

    if (max_error <= eps->tol || outer == ctx->refine_outer) break;

    if (eps->nconv > ctx->L) nv = eps->nconv;
    else if (ctx->L > nv) nv = ctx->L;
    ierr = PetscMalloc(ctx->L*nv*sizeof(PetscScalar),&temp);CHKERRQ(ierr);
    for (i=0;i<ctx->L*nv;i++) {
      ierr = PetscRandomGetValue(eps->rand,&temp[i]);CHKERRQ(ierr);
      if (PetscRealPart(temp[i]) < 0.5) temp[i] = -1.0;
      else temp[i] = 1.0;
    }
    ierr = SlepcUpdateVectors(nv,ctx->S,0,ctx->L,temp,nv,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscFree(temp);CHKERRQ(ierr);
    for (i=0;i<ctx->L;i++) {
      ierr = VecCopy(ctx->S[i],ctx->V[i]);CHKERRQ(ierr);
    }
    if (ctx->pA) {
      ierr = VecScatterVecs(eps,ctx->V,ctx->L);CHKERRQ(ierr);
      ierr = SolveLinearSystem(eps,ctx->pA,ctx->pB,ctx->pV,0,ctx->L,PETSC_FALSE);CHKERRQ(ierr);
    } else {
      ierr = SolveLinearSystem(eps,A,B,ctx->V,0,ctx->L,PETSC_FALSE);CHKERRQ(ierr);
    }
  }
  eps->reason = EPS_CONVERGED_TOL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetRegion_CISS"
static PetscErrorCode EPSCISSSetRegion_CISS(EPS eps,PetscScalar center,PetscReal radius,PetscReal vscale)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  ctx->center = center;
  if (radius) {
    if (radius == PETSC_DEFAULT) {
      ctx->radius = 1.0;
    } else {
      if (radius<0.0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The radius argument must be > 0.0");
      ctx->radius = radius;
    }
  }
  if (vscale) {
    if (vscale<0.0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The vscale argument must be > 0.0");
    ctx->vscale = vscale;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetRegion"
/*@
   EPSCISSSetRegion - Sets the parameters defining the region where eigenvalues
   must be computed.

   Logically Collective on EPS

   Input Parameters:
+  eps - the eigenproblem solver context
.  center - center of the region
.  radius - radius of the region
-  vscale - vertical scale of the region

   Options Database Keys:
+  -eps_ciss_center - Sets the center
.  -eps_ciss_radius - Sets the radius
-  -eps_ciss_vscale - Sets the vertical scale

   Level: advanced

.seealso: EPSCISSGetRegion()
@*/
PetscErrorCode EPSCISSSetRegion(EPS eps,PetscScalar center,PetscReal radius,PetscReal vscale)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidLogicalCollectiveScalar(eps,center,2);
  PetscValidLogicalCollectiveReal(eps,radius,3);
  PetscValidLogicalCollectiveReal(eps,vscale,4);
  ierr = PetscTryMethod(eps,"EPSCISSSetRegion_C",(EPS,PetscScalar,PetscReal,PetscReal),(eps,center,radius,vscale));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetRegion_CISS"
static PetscErrorCode EPSCISSGetRegion_CISS(EPS eps,PetscScalar *center,PetscReal *radius,PetscReal *vscale)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (center) *center = ctx->center;
  if (radius) *radius = ctx->radius;
  if (vscale) *vscale = ctx->vscale;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetRegion"
/*@
   EPSCISSGetRegion - Gets the parameters that define the region where eigenvalues
   must be computed.

   Not Collective

   Input Parameter:
.  eps - the eigenproblem solver context

   Output Parameters:
+  center - center of the region
.  radius - radius of the region
-  vscale - vertical scale of the region

   Level: advanced

.seealso: EPSCISSSetRegion()
@*/
PetscErrorCode EPSCISSGetRegion(EPS eps,PetscScalar *center,PetscReal *radius,PetscReal *vscale)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  ierr = PetscTryMethod(eps,"EPSCISSGetRegion_C",(EPS,PetscScalar*,PetscReal*,PetscReal*),(eps,center,radius,vscale));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetSizes_CISS"
static PetscErrorCode EPSCISSSetSizes_CISS(EPS eps,PetscInt ip,PetscInt bs,PetscInt ms,PetscInt npart,PetscInt bsmax,PetscBool isreal)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (ip) {
    if (ip == PETSC_DECIDE || ip == PETSC_DEFAULT) {
      if (ctx->N!=32) { ctx->N =32; ctx->M = ctx->N/4; }
    } else {
      if (ip<1) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The ip argument must be > 0");
      if (ip%2) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The ip argument must be an even number");
      if (ctx->N!=ip) { ctx->N = ip; ctx->M = ctx->N/4; }
    }
  }
  if (bs) {
    if (bs == PETSC_DECIDE || bs == PETSC_DEFAULT) {
      ctx->L = 16;
    } else {
      if (bs<1) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The bs argument must be > 0");
      if (bs>ctx->L_max) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The bs argument must be less than or equal to the maximum number of block size");
      ctx->L = bs;
    }
  }
  if (ms) {
    if (ms == PETSC_DECIDE || ms == PETSC_DEFAULT) {
      ctx->M = ctx->N/4;
    } else {
      if (ms<1) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The ms argument must be > 0");
      if (ms>ctx->N) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The ms argument must be less than or equal to the number of integration points");
      ctx->M = ms;
    }
  }
  if (npart) {
    if (npart == PETSC_DECIDE || npart == PETSC_DEFAULT) {
      ctx->num_subcomm = 1;
    } else {
      if (npart<1) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The npart argument must be > 0");
      ctx->num_subcomm = npart;
    }
  }
  if (bsmax) {
    if (bsmax == PETSC_DECIDE || bsmax == PETSC_DEFAULT) {
      ctx->L = 256;
    } else {
      if (bsmax<1) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The bsmax argument must be > 0");
      if (bsmax<ctx->L) ctx->L_max = ctx->L;
      else ctx->L_max = bsmax;
    }
  }
  ctx->isreal = isreal;
  ierr = EPSReset(eps);CHKERRQ(ierr);   /* clean allocated arrays and force new setup */
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetSizes"
/*@
   EPSCISSSetSizes - Sets the values of various size parameters in the CISS solver.

   Logically Collective on EPS

   Input Parameters:
+  eps   - the eigenproblem solver context
.  ip    - number of integration points
.  bs    - block size
.  ms    - moment size
.  npart - number of partitions when splitting the communicator
.  bsmax - max block size
-  isreal - A and B are real

   Options Database Keys:
+  -eps_ciss_integration_points - Sets the number of integration points
.  -eps_ciss_blocksize - Sets the block size
.  -eps_ciss_moments - Sets the moment size
.  -eps_ciss_partitions - Sets the number of partitions
.  -eps_ciss_maxblocksize - Sets the maximum block size
-  -eps_ciss_realmats - A and B are real

   Note:
   The default number of partitions is 1. This means the internal KSP object is shared
   among all processes of the EPS communicator. Otherwise, the communicator is split
   into npart communicators, so that npart KSP solves proceed simultaneously.

   Level: advanced

.seealso: EPSCISSGetSizes()
@*/
PetscErrorCode EPSCISSSetSizes(EPS eps,PetscInt ip,PetscInt bs,PetscInt ms,PetscInt npart,PetscInt bsmax,PetscBool isreal)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidLogicalCollectiveInt(eps,ip,2);
  PetscValidLogicalCollectiveInt(eps,bs,3);
  PetscValidLogicalCollectiveInt(eps,ms,4);
  PetscValidLogicalCollectiveInt(eps,npart,5);
  PetscValidLogicalCollectiveInt(eps,bsmax,6);
  PetscValidLogicalCollectiveBool(eps,isreal,7);
  ierr = PetscTryMethod(eps,"EPSCISSSetSizes_C",(EPS,PetscInt,PetscInt,PetscInt,PetscInt,PetscInt,PetscBool),(eps,ip,bs,ms,npart,bsmax,isreal));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetSizes_CISS"
static PetscErrorCode EPSCISSGetSizes_CISS(EPS eps,PetscInt *ip,PetscInt *bs,PetscInt *ms,PetscInt *npart,PetscInt *bsmax,PetscBool *isreal)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (ip) *ip = ctx->N;
  if (bs) *bs = ctx->L;
  if (ms) *ms = ctx->M;
  if (npart) *npart = ctx->num_subcomm;
  if (bsmax) *bsmax = ctx->L_max;
  if (isreal) *isreal = ctx->isreal;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetSizes"
/*@
   EPSCISSGetSizes - Gets the values of various size parameters in the CISS solver.

   Not Collective

   Input Parameter:
.  eps - the eigenproblem solver context

   Output Parameters:
+  ip    - number of integration points
.  bs    - block size
.  ms    - moment size
.  npart - number of partitions when splitting the communicator
.  bsmax - max block size
-  isreal - A and B are real

   Level: advanced

.seealso: EPSCISSSetSizes()
@*/
PetscErrorCode EPSCISSGetSizes(EPS eps,PetscInt *ip,PetscInt *bs,PetscInt *ms,PetscInt *npart,PetscInt *bsmax,PetscBool *isreal)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  ierr = PetscTryMethod(eps,"EPSCISSGetSizes_C",(EPS,PetscInt*,PetscInt*,PetscInt*,PetscInt*,PetscInt*,PetscBool*),(eps,ip,bs,ms,npart,bsmax,isreal));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetThreshold_CISS"
static PetscErrorCode EPSCISSSetThreshold_CISS(EPS eps,PetscReal delta,PetscReal spur)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (delta) {
    if (delta == PETSC_DEFAULT) {
      ctx->delta = 1e-12;
    } else {
      if (delta<=0.0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The delta argument must be > 0.0");
      ctx->delta = delta;
    }
  }
  if (spur) {
    if (spur == PETSC_DEFAULT) {
      ctx->spurious_threshold = 1e-4;
    } else {
      if (spur<=0.0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The spurious threshold argument must be > 0.0");
      ctx->spurious_threshold = spur;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetThreshold"
/*@
   EPSCISSSetThreshold - Sets the values of various threshold parameters in
   the CISS solver.

   Logically Collective on EPS

   Input Parameters:
+  eps   - the eigenproblem solver context
.  delta - threshold for numerical rank
-  spur  - spurious threshold (to discard spurious eigenpairs)

   Options Database Keys:
+  -eps_ciss_delta - Sets the delta
-  -eps_ciss_spurious_threshold - Sets the spurious threshold

   Level: advanced

.seealso: EPSCISSGetThreshold()
@*/
PetscErrorCode EPSCISSSetThreshold(EPS eps,PetscReal delta,PetscReal spur)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidLogicalCollectiveReal(eps,delta,2);
  PetscValidLogicalCollectiveReal(eps,spur,3);
  ierr = PetscTryMethod(eps,"EPSCISSSetThreshold_C",(EPS,PetscReal,PetscReal),(eps,delta,spur));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetThreshold_CISS"
static PetscErrorCode EPSCISSGetThreshold_CISS(EPS eps,PetscReal *delta,PetscReal *spur)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (delta) *delta = ctx->delta;
  if (spur)  *spur = ctx->spurious_threshold;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetThreshold"
/*@
   EPSCISSGetThreshold - Gets the values of various threshold parameters
   in the CISS solver.

   Not Collective

   Input Parameter:
.  eps - the eigenproblem solver context

   Output Parameters:
+  delta - threshold for numerical rank
-  spur  - spurious threshold (to discard spurious eigenpairs)

   Level: advanced

.seealso: EPSCISSSetThreshold()
@*/
PetscErrorCode EPSCISSGetThreshold(EPS eps,PetscReal *delta,PetscReal *spur)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  ierr = PetscTryMethod(eps,"EPSCISSGetThreshold_C",(EPS,PetscReal*,PetscReal*),(eps,delta,spur));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetRefinement_CISS"
static PetscErrorCode EPSCISSSetRefinement_CISS(EPS eps,PetscInt inner,PetscInt outer,PetscInt blsize)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (inner == PETSC_DEFAULT) {
    ctx->refine_inner = 0;
  } else {
    if (inner<0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The refine inner argument must be >= 0");
    ctx->refine_inner = inner;
  }
  if (outer == PETSC_DEFAULT) {
    ctx->refine_outer = 0;
  } else {
    if (outer<0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The refine outer argument must be >= 0");
    ctx->refine_outer = outer;
  }
  if (blsize == PETSC_DEFAULT) {
    ctx->refine_blocksize = 0;
  } else {
    if (blsize<0) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_ARG_OUTOFRANGE,"The refine blocksize argument must be >= 0");
    ctx->refine_blocksize = blsize;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSSetRefinement"
/*@
   EPSCISSSetRefinement - Sets the values of various refinement parameters
   in the CISS solver.

   Logically Collective on EPS

   Input Parameters:
+  eps    - the eigenproblem solver context
.  inner  - number of iterative refinement iterations (inner loop)
.  outer  - number of iterative refinement iterations (outer loop)
-  blsize - number of iterative refinement iterations (blocksize loop)

   Options Database Keys:
+  -eps_ciss_refine_inner - Sets number of inner iterations
.  -eps_ciss_refine_outer - Sets number of outer iterations
-  -eps_ciss_refine_blocksize - Sets number of blocksize iterations

   Level: advanced

.seealso: EPSCISSGetRefinement()
@*/
PetscErrorCode EPSCISSSetRefinement(EPS eps,PetscInt inner,PetscInt outer,PetscInt blsize)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidLogicalCollectiveInt(eps,inner,2);
  PetscValidLogicalCollectiveInt(eps,outer,3);
  PetscValidLogicalCollectiveInt(eps,blsize,4);
  ierr = PetscTryMethod(eps,"EPSCISSSetRefinement_C",(EPS,PetscInt,PetscInt,PetscInt),(eps,inner,outer,blsize));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetRefinement_CISS"
static PetscErrorCode EPSCISSGetRefinement_CISS(EPS eps,PetscInt *inner,PetscInt *outer,PetscInt *blsize)
{
  EPS_CISS *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  if (inner)  *inner = ctx->refine_inner;
  if (outer)  *outer = ctx->refine_outer;
  if (blsize) *blsize = ctx->refine_blocksize;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCISSGetRefinement"
/*@
   EPSCISSGetRefinement - Gets the values of various refinement parameters
   in the CISS solver.

   Not Collective

   Input Parameter:
.  eps - the eigenproblem solver context

   Output Parameters:
+  inner  - number of iterative refinement iterations (inner loop)
.  outer  - number of iterative refinement iterations (outer loop)
-  blsize - number of iterative refinement iterations (blocksize loop)

   Level: advanced

.seealso: EPSCISSSetRefinement()
@*/
PetscErrorCode EPSCISSGetRefinement(EPS eps, PetscInt *inner, PetscInt *outer,PetscInt *blsize)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  ierr = PetscTryMethod(eps,"EPSCISSGetRefinement_C",(EPS,PetscInt*,PetscInt*,PetscInt*),(eps,inner,outer,blsize));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSReset_CISS"
PetscErrorCode EPSReset_CISS(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  ierr = PetscSubcommDestroy(&ctx->subcomm);CHKERRQ(ierr);
  ierr = PetscFree(ctx->weight);CHKERRQ(ierr);
  ierr = PetscFree(ctx->omega);CHKERRQ(ierr);
  ierr = PetscFree(ctx->pp);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ctx->L_max*ctx->M,&ctx->S);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ctx->L_max,&ctx->V);CHKERRQ(ierr);
  ierr = PetscFree(ctx->sigma);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ctx->L_max*ctx->num_solve_point,&ctx->Y);CHKERRQ(ierr);
  ierr = KSPDestroy(&ctx->ksp);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&ctx->scatterin);CHKERRQ(ierr);
  ierr = VecDestroy(&ctx->xsub);CHKERRQ(ierr);
  ierr = VecDestroy(&ctx->xdup);CHKERRQ(ierr);
  if (ctx->pA) {
    ierr = MatDestroy(&ctx->pA);CHKERRQ(ierr);
    ierr = MatDestroy(&ctx->pB);CHKERRQ(ierr);
    ierr = VecDestroyVecs(ctx->L_max,&ctx->pV);CHKERRQ(ierr);
  }
  ierr = EPSReset_Default(eps);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSetFromOptions_CISS"
PetscErrorCode EPSSetFromOptions_CISS(EPS eps)
{
  PetscErrorCode ierr;
  PetscScalar    s;
  PetscReal      r1,r2,r3,r4;
  PetscInt       i1=0,i2=0,i3=0,i4=0,i5=0,i6=0,i7=0,i8=0;
  PetscBool      b1=PETSC_FALSE;

  PetscFunctionBegin;
  ierr = PetscOptionsHead("EPS CISS Options");CHKERRQ(ierr);
  ierr = EPSCISSGetRegion(eps,&s,&r1,&r2);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-eps_ciss_radius","CISS radius of region","EPSCISSSetRegion",r1,&r1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsScalar("-eps_ciss_center","CISS center of region","EPSCISSSetRegion",s,&s,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-eps_ciss_vscale","CISS vertical scale of region","EPSCISSSetRegion",r2,&r2,NULL);CHKERRQ(ierr);
  ierr = EPSCISSSetRegion(eps,s,r1,r2);CHKERRQ(ierr);

  ierr = PetscOptionsInt("-eps_ciss_integration_points","CISS number of integration points","EPSCISSSetSizes",i1,&i1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_blocksize","CISS block size","EPSCISSSetSizes",i2,&i2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_moments","CISS moment size","EPSCISSSetSizes",i3,&i3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_partitions","CISS number of partitions","EPSCISSSetSizes",i4,&i4,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_maxblocksize","CISS maximum block size","EPSCISSSetSizes",i5,&i5,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-eps_ciss_realmats","CISS A and B are real","EPSCISSSetSizes",b1,&b1,NULL);CHKERRQ(ierr);
  ierr = EPSCISSSetSizes(eps,i1,i2,i3,i4,i5,b1);CHKERRQ(ierr);

  ierr = EPSCISSGetThreshold(eps,&r3,&r4);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-eps_ciss_delta","CISS threshold for numerical rank","EPSCISSSetThreshold",r3,&r3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-eps_ciss_spurious_threshold","CISS threshold for the spurious eigenpairs","EPSCISSSetThreshold",r4,&r4,NULL);CHKERRQ(ierr);
  ierr = EPSCISSSetThreshold(eps,r3,r4);CHKERRQ(ierr);

  ierr = EPSCISSGetRefinement(eps,&i6,&i7,&i8);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_refine_inner","CISS number of inner iterative refinement iterations","EPSCISSSetRefinement",i6,&i6,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_refine_outer","CISS number of outer iterative refinement iterations","EPSCISSSetRefinement",i7,&i7,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_ciss_refine_blocksize","CISS number of blocksize iterative refinement iterations","EPSCISSSetRefinement",i8,&i8,NULL);CHKERRQ(ierr);
  ierr = EPSCISSSetRefinement(eps,i6,i7,i8);CHKERRQ(ierr);

  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSDestroy_CISS"
PetscErrorCode EPSDestroy_CISS(EPS eps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(eps->data);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetRegion_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetRegion_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetSizes_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetSizes_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetThreshold_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetThreshold_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetRefinement_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetRefinement_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSView_CISS"
PetscErrorCode EPSView_CISS(EPS eps,PetscViewer viewer)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;
  PetscBool      isascii;
  char           str[50];

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = SlepcSNPrintfScalar(str,50,ctx->center,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  CISS: region { center: %s, radius: %g, vscale: %g }\n",str,(double)ctx->radius,(double)ctx->vscale);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  CISS: sizes { integration points: %D, block size: %D, moment size: %D, partitions: %D, maximum block size: %D }\n",ctx->N,ctx->L,ctx->M,ctx->num_subcomm,ctx->L_max);CHKERRQ(ierr);
    if (ctx->isreal) {
      ierr = PetscViewerASCIIPrintf(viewer,"  CISS: exploiting symmetry of integration points\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer,"  CISS: threshold { delta: %g, spurious threshold: %g }\n",(double)ctx->delta,(double)ctx->spurious_threshold);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  CISS: iterative refinement  { inner: %D, outer: %D, blocksize: %D }\n",ctx->refine_inner,ctx->refine_outer, ctx->refine_blocksize);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
    /*ierr = KSPView(ctx->ksp,viewer);CHKERRQ(ierr);*/
    ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCreate_CISS"
PETSC_EXTERN PetscErrorCode EPSCreate_CISS(EPS eps)
{
  PetscErrorCode ierr;
  EPS_CISS       *ctx = (EPS_CISS*)eps->data;

  PetscFunctionBegin;
  ierr = PetscNewLog(eps,&ctx);CHKERRQ(ierr);
  eps->data = ctx;
  eps->ops->setup          = EPSSetUp_CISS;
  eps->ops->setfromoptions = EPSSetFromOptions_CISS;
  eps->ops->destroy        = EPSDestroy_CISS;
  eps->ops->reset          = EPSReset_CISS;
  eps->ops->view           = EPSView_CISS;
  eps->ops->backtransform  = PETSC_NULL;
  eps->ops->computevectors = EPSComputeVectors_Schur;
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetRegion_C",EPSCISSSetRegion_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetRegion_C",EPSCISSGetRegion_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetSizes_C",EPSCISSSetSizes_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetSizes_C",EPSCISSGetSizes_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetThreshold_C",EPSCISSSetThreshold_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetThreshold_C",EPSCISSGetThreshold_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSSetRefinement_C",EPSCISSSetRefinement_CISS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSCISSGetRefinement_C",EPSCISSGetRefinement_CISS);CHKERRQ(ierr);
  /* set default values of parameters */
  ctx->center  = 0.0;
  ctx->radius  = 1.0;
  ctx->vscale  = 0.0;
  ctx->N       = 32;
  ctx->L       = 16;
  ctx->M       = ctx->N/4;
  ctx->delta   = 1e-12;
  ctx->L_max   = 64;
  ctx->spurious_threshold = 1e-4;
  ctx->isreal  = PETSC_FALSE;
  ctx->refine_outer = 1;
  ctx->refine_inner = 1;
  ctx->refine_blocksize = 1;
  ctx->num_subcomm = 1;
  PetscFunctionReturn(0);
}

