/*                       

   SLEPc nonlinear eigensolver: "slp"

   Method: Succesive linear problems

   Algorithm:

       Newton-type iteration based on first order Taylor approximation.

   References:

       [1] A. Ruhe, "Algorithms for the nonlinear eigenvalue problem", SIAM J.
           Numer. Anal. 10(4):674-689, 1973.

   Last update: Feb 2013

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

#include <slepc-private/nepimpl.h>         /*I "slepcnep.h" I*/
#include <slepc-private/epsimpl.h>         /*I "slepceps.h" I*/

typedef struct {
  EPS       eps;             /* linear eigensolver for T*z = mu*Tp*z */
  PetscBool setfromoptionscalled;
} NEP_SLP;

#undef __FUNCT__
#define __FUNCT__ "NEPSetUp_SLP"
PetscErrorCode NEPSetUp_SLP(NEP nep)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx = (NEP_SLP*)nep->data;
  
  PetscFunctionBegin;
  if (nep->ncv) { /* ncv set */
    if (nep->ncv<nep->nev) SETERRQ(PetscObjectComm((PetscObject)nep),1,"The value of ncv must be at least nev"); 
  } else if (nep->mpd) { /* mpd set */
    nep->ncv = PetscMin(nep->n,nep->nev+nep->mpd);
  } else { /* neither set: defaults depend on nev being small or large */
    if (nep->nev<500) nep->ncv = PetscMin(nep->n,PetscMax(2*nep->nev,nep->nev+15));
    else {
      nep->mpd = 500;
      nep->ncv = PetscMin(nep->n,nep->nev+nep->mpd);
    }
  }
  if (!nep->mpd) nep->mpd = nep->ncv;
  if (nep->ncv>nep->nev+nep->mpd) SETERRQ(PetscObjectComm((PetscObject)nep),1,"The value of ncv must not be larger than nev+mpd"); 
  if (nep->nev>1) { ierr = PetscInfo(nep,"Warning: requested more than one eigenpair but SLP can only compute one\n");CHKERRQ(ierr); }
  if (!nep->max_it) nep->max_it = PetscMax(5000,2*nep->n/nep->ncv);
  if (!nep->max_funcs) nep->max_funcs = nep->max_it;

  ierr = EPSSetWhichEigenpairs(ctx->eps,EPS_TARGET_MAGNITUDE);CHKERRQ(ierr);
  ierr = EPSSetTarget(ctx->eps,0.0);CHKERRQ(ierr);
  ierr = EPSSetDimensions(ctx->eps,1,nep->ncv,nep->mpd);CHKERRQ(ierr);
  ierr = EPSSetTolerances(ctx->eps,nep->rtol==PETSC_DEFAULT?SLEPC_DEFAULT_TOL/10.0:nep->rtol/10.0,nep->max_it);CHKERRQ(ierr);
  if (ctx->setfromoptionscalled) {
    ierr = EPSSetFromOptions(ctx->eps);CHKERRQ(ierr);
    ctx->setfromoptionscalled = PETSC_FALSE;
  }

  ierr = NEPAllocateSolution(nep);CHKERRQ(ierr);
  ierr = NEPDefaultGetWork(nep,1);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPSolve_SLP"
PetscErrorCode NEPSolve_SLP(NEP nep)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx = (NEP_SLP*)nep->data;
  Mat            T=nep->function,Tp=nep->jacobian;
  Vec            u=nep->V[0],r=nep->work[0];
  PetscScalar    lambda,mu,im;
  PetscReal      relerr;
  PetscInt       nconv;
  MatStructure   mats;

  PetscFunctionBegin;
  /* get initial approximation of eigenvalue and eigenvector */
  ierr = NEPGetDefaultShift(nep,&lambda);CHKERRQ(ierr);
  if (!nep->nini) {
    ierr = SlepcVecSetRandom(u,nep->rand);CHKERRQ(ierr);
  }
  
  /* Restart loop */
  while (nep->reason == NEP_CONVERGED_ITERATING) {
    nep->its++;

    /* evaluate T(lambda) and T'(lambda) */
    ierr = NEPComputeFunction(nep,lambda,0,&T,&T,&mats);CHKERRQ(ierr);
    ierr = NEPComputeJacobian(nep,lambda,0,&Tp,&mats);CHKERRQ(ierr);

    /* form residual,  r = T(lambda)*u (used in convergence test only) */
    ierr = MatMult(T,u,r);CHKERRQ(ierr);

    /* convergence test */
    ierr = VecNorm(r,NORM_2,&relerr);CHKERRQ(ierr);
    nep->errest[nep->nconv] = relerr;
    nep->eigr[nep->nconv] = lambda;
    if (relerr<=nep->rtol) {
      nep->nconv = nep->nconv + 1;
      nep->reason = NEP_CONVERGED_FNORM_RELATIVE;
    } 
    ierr = NEPMonitor(nep,nep->its,nep->nconv,nep->eigr,nep->eigi,nep->errest,1);CHKERRQ(ierr);

    if (!nep->nconv) {
      /* compute eigenvalue correction mu and eigenvector approximation u */
      ierr = EPSSetOperators(ctx->eps,T,Tp);CHKERRQ(ierr);
      ierr = EPSSetInitialSpace(ctx->eps,1,&u);CHKERRQ(ierr);
      ierr = EPSSolve(ctx->eps);CHKERRQ(ierr);
      ierr = EPSGetConverged(ctx->eps,&nconv);CHKERRQ(ierr);
      if (!nconv) {
        ierr = PetscInfo1(nep,"iter=%D, inner iteration failed, stopping solve\n",nep->its);CHKERRQ(ierr);
        nep->reason = NEP_DIVERGED_LINEAR_SOLVE;
        break;
      }
      ierr = EPSGetEigenpair(ctx->eps,0,&mu,&im,u,NULL);CHKERRQ(ierr);
      if (PetscAbsScalar(im)>PETSC_MACHINE_EPSILON) SETERRQ(PetscObjectComm((PetscObject)nep),1,"Complex eigenvalue approximation - not implemented in real scalars");

      /* correct eigenvalue */
      lambda = lambda - mu;
    }
    if (nep->its >= nep->max_it) nep->reason = NEP_DIVERGED_MAX_IT;
  } 
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPSetFromOptions_SLP"
PetscErrorCode NEPSetFromOptions_SLP(NEP nep)
{
  NEP_SLP *ctx = (NEP_SLP*)nep->data;

  PetscFunctionBegin;
  ctx->setfromoptionscalled = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPSLPSetEPS_SLP"
static PetscErrorCode NEPSLPSetEPS_SLP(NEP nep,EPS eps)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx = (NEP_SLP*)nep->data;

  PetscFunctionBegin;
  ierr = PetscObjectReference((PetscObject)eps);CHKERRQ(ierr);
  ierr = EPSDestroy(&ctx->eps);CHKERRQ(ierr);  
  ctx->eps = eps;
  ierr = PetscLogObjectParent(nep,ctx->eps);CHKERRQ(ierr);
  nep->setupcalled = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPSLPSetEPS"
/*@
   NEPSLPSetEPS - Associate a linear eigensolver object (EPS) to the
   nonlinear eigenvalue solver. 

   Collective on NEP

   Input Parameters:
+  nep - nonlinear eigenvalue solver
-  eps - the eigensolver object

   Level: advanced

.seealso: NEPSLPGetEPS()
@*/
PetscErrorCode NEPSLPSetEPS(NEP nep,EPS eps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(nep,NEP_CLASSID,1);
  PetscValidHeaderSpecific(eps,EPS_CLASSID,2);
  PetscCheckSameComm(nep,1,eps,2);
  ierr = PetscTryMethod(nep,"NEPSLPSetEPS_C",(NEP,EPS),(nep,eps));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPSLPGetEPS_SLP"
static PetscErrorCode NEPSLPGetEPS_SLP(NEP nep,EPS *eps)
{
  NEP_SLP *ctx = (NEP_SLP*)nep->data;

  PetscFunctionBegin;
  *eps = ctx->eps;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPSLPGetEPS"
/*@
   NEPSLPGetEPS - Retrieve the linear eigensolver object (EPS) associated
   to the nonlinear eigenvalue solver.

   Not Collective

   Input Parameter:
.  nep - nonlinear eigenvalue solver

   Output Parameter:
.  eps - the eigensolver object

   Level: advanced

.seealso: NEPSLPSetEPS()
@*/
PetscErrorCode NEPSLPGetEPS(NEP nep,EPS *eps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(nep,NEP_CLASSID,1);
  PetscValidPointer(eps,2);
  ierr = PetscTryMethod(nep,"NEPSLPGetEPS_C",(NEP,EPS*),(nep,eps));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPView_SLP"
PetscErrorCode NEPView_SLP(NEP nep,PetscViewer viewer)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx = (NEP_SLP*)nep->data;

  PetscFunctionBegin;
  ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
  ierr = EPSView(ctx->eps,viewer);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPReset_SLP"
PetscErrorCode NEPReset_SLP(NEP nep)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx = (NEP_SLP*)nep->data;

  PetscFunctionBegin;
  ierr = EPSReset(ctx->eps);CHKERRQ(ierr);
  ierr = NEPDefaultFreeWork(nep);CHKERRQ(ierr);
  ierr = NEPFreeSolution(nep);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "NEPDestroy_SLP"
PetscErrorCode NEPDestroy_SLP(NEP nep)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx = (NEP_SLP*)nep->data;

  PetscFunctionBegin;
  ierr = EPSDestroy(&ctx->eps);CHKERRQ(ierr);
  ierr = PetscFree(nep->data);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)nep,"NEPSLPSetEPS_C","",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)nep,"NEPSLPGetEPS_C","",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "NEPCreate_SLP"
PetscErrorCode NEPCreate_SLP(NEP nep)
{
  PetscErrorCode ierr;
  NEP_SLP        *ctx;

  PetscFunctionBegin;
  ierr = PetscNewLog(nep,NEP_SLP,&ctx);CHKERRQ(ierr);
  nep->data                = (void*)ctx;
  nep->ops->solve          = NEPSolve_SLP;
  nep->ops->setup          = NEPSetUp_SLP;
  nep->ops->setfromoptions = NEPSetFromOptions_SLP;
  nep->ops->reset          = NEPReset_SLP;
  nep->ops->destroy        = NEPDestroy_SLP;
  nep->ops->view           = NEPView_SLP;
  ierr = PetscObjectComposeFunction((PetscObject)nep,"NEPSLPSetEPS_C","NEPSLPSetEPS_SLP",NEPSLPSetEPS_SLP);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)nep,"NEPSLPGetEPS_C","NEPSLPGetEPS_SLP",NEPSLPGetEPS_SLP);CHKERRQ(ierr);

  ierr = EPSCreate(PetscObjectComm((PetscObject)nep),&ctx->eps);CHKERRQ(ierr);
  ierr = EPSSetOptionsPrefix(ctx->eps,((PetscObject)nep)->prefix);CHKERRQ(ierr);
  ierr = EPSAppendOptionsPrefix(ctx->eps,"nep_");CHKERRQ(ierr);
  ierr = STSetOptionsPrefix(ctx->eps->st,((PetscObject)ctx->eps)->prefix);CHKERRQ(ierr);
  ierr = PetscObjectIncrementTabLevel((PetscObject)ctx->eps,(PetscObject)nep,1);CHKERRQ(ierr);  
  ierr = PetscLogObjectParent(nep,ctx->eps);CHKERRQ(ierr);
  if (!nep->ip) { ierr = NEPGetIP(nep,&nep->ip);CHKERRQ(ierr); }
  ierr = EPSSetIP(ctx->eps,nep->ip);CHKERRQ(ierr);
  ctx->setfromoptionscalled = PETSC_FALSE;
  PetscFunctionReturn(0);
}
EXTERN_C_END
