/*
      Implements the Cayley spectral transform.
*/
#include "src/st/stimpl.h"          /*I "slepcst.h" I*/

typedef struct {
  PetscScalar tau;
  PetscTruth  tau_set;
  Vec         w2;
} ST_CAYLEY;

#undef __FUNCT__  
#define __FUNCT__ "STApply_Cayley"
PetscErrorCode STApply_Cayley(ST st,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;
  PetscScalar    tau = ctx->tau;
  
  PetscFunctionBegin;
  if (st->shift_matrix == STMATMODE_INPLACE) { tau = tau + st->sigma; };

  if (st->B) {
    /* generalized eigenproblem: y = (A - sB)^-1 (A + tB)x */
    ierr = MatMult(st->A,x,st->w);CHKERRQ(ierr);
    ierr = MatMult(st->B,x,ctx->w2);CHKERRQ(ierr);
    ierr = VecAXPY(st->w,tau,ctx->w2);CHKERRQ(ierr);    
    ierr = STAssociatedKSPSolve(st,st->w,y);CHKERRQ(ierr);
  }
  else {
    /* standard eigenproblem: y = (A - sI)^-1 (A + tI)x */
    ierr = MatMult(st->A,x,st->w);CHKERRQ(ierr);
    ierr = VecAXPY(st->w,tau,x);CHKERRQ(ierr);
    ierr = STAssociatedKSPSolve(st,st->w,y);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STApplyTranspose_Cayley"
PetscErrorCode STApplyTranspose_Cayley(ST st,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;
  PetscScalar    tau = ctx->tau;
  
  PetscFunctionBegin;
  if (st->shift_matrix == STMATMODE_INPLACE) { tau = tau + st->sigma; };

  if (st->B) {
    /* generalized eigenproblem: y = (A + tB)^T (A - sB)^-T x */
    ierr = STAssociatedKSPSolve(st,x,st->w);CHKERRQ(ierr);
    ierr = MatMult(st->A,st->w,y);CHKERRQ(ierr);
    ierr = MatMult(st->B,st->w,ctx->w2);CHKERRQ(ierr);
    ierr = VecAXPY(y,tau,ctx->w2);CHKERRQ(ierr);    
  }
  else {
    /* standard eigenproblem: y =  (A + tI)^T (A - sI)^-T x */
    ierr = STAssociatedKSPSolve(st,x,st->w);CHKERRQ(ierr);
    ierr = MatMult(st->A,st->w,y);CHKERRQ(ierr);
    ierr = VecAXPY(y,tau,st->w);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STApplyNoB_Cayley"
PetscErrorCode STApplyNoB_Cayley(ST st,Vec x,Vec y)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = STAssociatedKSPSolve(st,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STApplyB_Cayley"
PetscErrorCode STApplyB_Cayley(ST st,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;
  PetscScalar    tau = ctx->tau;
  
  PetscFunctionBegin;
  if (st->shift_matrix == STMATMODE_INPLACE) { tau = tau + st->sigma; };

  if (st->B) {
    /* generalized eigenproblem: y = (A + tB)x */
    ierr = MatMult(st->A,x,y);CHKERRQ(ierr);
    ierr = MatMult(st->B,x,ctx->w2);CHKERRQ(ierr);
    ierr = VecAXPY(y,tau,ctx->w2);CHKERRQ(ierr);    
  }
  else {
    /* standard eigenproblem: y = (A + tI)x */
    ierr = MatMult(st->A,x,y);CHKERRQ(ierr);
    ierr = VecAXPY(y,tau,x);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STBackTransform_Cayley"
PetscErrorCode STBackTransform_Cayley(ST st,PetscScalar *eigr,PetscScalar *eigi)
{
  ST_CAYLEY   *ctx = (ST_CAYLEY *) st->data;
#ifndef PETSC_USE_COMPLEX
  PetscScalar t,i,r;
  PetscFunctionBegin;
  PetscValidPointer(eigr,2);
  PetscValidPointer(eigi,3);
  if (*eigi == 0.0) *eigr = (ctx->tau + *eigr * st->sigma) / (*eigr - 1.0);
  else {
    r = *eigr;
    i = *eigi;
    r = st->sigma * (r * r + i * i - r) + ctx->tau * (r - 1);
    i = - st->sigma * i - ctx->tau * i;
    t = i * i + r * (r - 2.0) + 1.0;    
    *eigr = r / t;
    *eigi = i / t;    
  }
#else
  PetscFunctionBegin;
  PetscValidPointer(eigr,2);
  *eigr = (ctx->tau + *eigr * st->sigma) / (*eigr - 1.0);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STPostSolve_Cayley"
PetscErrorCode STPostSolve_Cayley(ST st)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (st->shift_matrix == STMATMODE_INPLACE) {
    if (st->B) {
      ierr = MatAXPY(st->A,st->sigma,st->B,st->str);CHKERRQ(ierr);
    } else { 
      ierr = MatShift(st->A,st->sigma); CHKERRQ(ierr); 
    }
    st->setupcalled = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STSetUp_Cayley"
PetscErrorCode STSetUp_Cayley(ST st)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;

  PetscFunctionBegin;

  if (st->mat) { ierr = MatDestroy(st->mat);CHKERRQ(ierr); }
  if (!ctx->tau_set) { ctx->tau = st->sigma; }
  if (ctx->tau == 0.0 &&  st->sigma == 0.0) {
    SETERRQ(1,"Values of shift and antishift cannot be zero simultaneously");
  }

  switch (st->shift_matrix) {
  case STMATMODE_INPLACE:
    st->mat = PETSC_NULL;
    if (st->sigma != 0.0) {
      if (st->B) { 
        ierr = MatAXPY(st->A,-st->sigma,st->B,st->str);CHKERRQ(ierr); 
      } else { 
        ierr = MatShift(st->A,-st->sigma);CHKERRQ(ierr); 
      }
    }
    /* In the following line, the SAME_NONZERO_PATTERN flag has been used to
     * improve performance when solving a number of related eigenproblems */
    ierr = KSPSetOperators(st->ksp,st->A,st->A,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
    break;
  case STMATMODE_SHELL:
    ierr = STMatShellCreate(st,&st->mat);CHKERRQ(ierr);
    ierr = KSPSetOperators(st->ksp,st->mat,st->mat,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
    break;
  default:
    ierr = MatDuplicate(st->A,MAT_COPY_VALUES,&st->mat);CHKERRQ(ierr);
    if (st->sigma != 0.0) {
      if (st->B) { 
        ierr = MatAXPY(st->mat,-st->sigma,st->B,st->str);CHKERRQ(ierr); 
      } else { 
        ierr = MatShift(st->mat,-st->sigma);CHKERRQ(ierr); 
      }
    }
    /* In the following line, the SAME_NONZERO_PATTERN flag has been used to
     * improve performance when solving a number of related eigenproblems */
    ierr = KSPSetOperators(st->ksp,st->mat,st->mat,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
  }
  if (st->B) { 
   if (ctx->w2) { ierr = VecDestroy(ctx->w2);CHKERRQ(ierr); }
   ierr = MatGetVecs(st->B,&ctx->w2,PETSC_NULL);CHKERRQ(ierr); 
  }
  ierr = KSPSetUp(st->ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STSetShift_Cayley"
PetscErrorCode STSetShift_Cayley(ST st,PetscScalar newshift)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;

  PetscFunctionBegin;

  /* Nothing to be done if STSetUp has not been called yet */
  if (!st->setupcalled) PetscFunctionReturn(0);

  switch (st->shift_matrix) {
  case STMATMODE_INPLACE:
    /* Undo previous operations */
    if (st->sigma != 0.0) {
      if (st->B) { 
        ierr = MatAXPY(st->A,st->sigma,st->B,st->str);CHKERRQ(ierr);
      } else {
        ierr = MatShift(st->A,st->sigma);CHKERRQ(ierr);
      }
    }
    /* Apply new shift */
    if (newshift != 0.0) {
      if (st->B) { 
        ierr = MatAXPY(st->A,-newshift,st->B,st->str);CHKERRQ(ierr);
      } else {
        ierr = MatShift(st->A,-newshift);CHKERRQ(ierr);
      }
    }
    ierr = KSPSetOperators(st->ksp,st->A,st->A,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
    break;
  case STMATMODE_SHELL:
    ierr = KSPSetOperators(st->ksp,st->mat,st->mat,SAME_NONZERO_PATTERN);CHKERRQ(ierr);    
    break;
  default:
    ierr = MatCopy(st->A, st->mat,SUBSET_NONZERO_PATTERN); CHKERRQ(ierr);
    if (newshift != 0.0) {   
      if (st->B) { ierr = MatAXPY(st->mat,-newshift,st->B,st->str);CHKERRQ(ierr); }
      else { ierr = MatShift(st->mat,-newshift);CHKERRQ(ierr); }
    }
    /* In the following line, the SAME_NONZERO_PATTERN flag has been used to
     * improve performance when solving a number of related eigenproblems */
    ierr = KSPSetOperators(st->ksp,st->mat,st->mat,SAME_NONZERO_PATTERN);CHKERRQ(ierr);    
  }
  st->sigma = newshift;
  if (!ctx->tau_set) { ctx->tau = newshift; }
  ierr = KSPSetUp(st->ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STSetFromOptions_Cayley"
PetscErrorCode STSetFromOptions_Cayley(ST st) 
{
  PetscErrorCode ierr;
  PetscScalar    tau;
  PetscTruth     flg;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;

  PetscFunctionBegin;
  ierr = PetscOptionsHead("ST Cayley Options");CHKERRQ(ierr);
  ierr = PetscOptionsScalar("-st_antishift","Value of the antishift","STSetAntishift",ctx->tau,&tau,&flg); CHKERRQ(ierr);
  if (flg) {
    ierr = STCayleySetAntishift(st,tau);CHKERRQ(ierr);
  }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "STCayleySetAntishift_Cayley"
PetscErrorCode STCayleySetAntishift_Cayley(ST st,PetscScalar newshift)
{
  ST_CAYLEY *ctx = (ST_CAYLEY *) st->data;

  PetscFunctionBegin;
  ctx->tau = newshift;
  ctx->tau_set = PETSC_TRUE;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "STCayleySetAntishift"
/*@
   STCayleySetAntishift - Sets the value of the anti-shift for the Cayley
   spectral transformation.

   Collective on ST

   Input Parameters:
+  st  - the spectral transformation context
-  tau - the anti-shift

   Options Database Key:
.  -st_antishift - Sets the value of the anti-shift

   Level: intermediate

   Note:
   In the generalized Cayley transform, the operator can be expressed as
   OP = inv(A - sigma B)*(A + tau B). This function sets the value of tau.
   Use STSetShift() for setting sigma.

.seealso: STSetShift()
@*/
PetscErrorCode STCayleySetAntishift(ST st,PetscScalar tau)
{
  PetscErrorCode ierr, (*f)(ST,PetscScalar);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(st,ST_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)st,"STCayleySetAntishift_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(st,tau);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STView_Cayley"
PetscErrorCode STView_Cayley(ST st,PetscViewer viewer)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;

  PetscFunctionBegin;
#if !defined(PETSC_USE_COMPLEX)
  ierr = PetscViewerASCIIPrintf(viewer,"  antishift: %g\n",ctx->tau);CHKERRQ(ierr);
#else
  ierr = PetscViewerASCIIPrintf(viewer,"  antishift: %g+%g i\n",PetscRealPart(ctx->tau),PetscImaginaryPart(ctx->tau));CHKERRQ(ierr);
#endif
  ierr = STView_Default(st,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STDestroy_Cayley"
PetscErrorCode STDestroy_Cayley(ST st)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx = (ST_CAYLEY *) st->data;

  PetscFunctionBegin;
  if (ctx->w2) { ierr = VecDestroy(ctx->w2);CHKERRQ(ierr); }
  ierr = PetscFree(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "STCreate_Cayley"
PetscErrorCode STCreate_Cayley(ST st)
{
  PetscErrorCode ierr;
  ST_CAYLEY      *ctx;

  PetscFunctionBegin;
  ierr = PetscNew(ST_CAYLEY,&ctx); CHKERRQ(ierr);
  PetscLogObjectMemory(st,sizeof(ST_CAYLEY));
  st->data                = (void *) ctx;

  st->ops->apply          = STApply_Cayley;
  st->ops->applyB         = STApplyB_Cayley;
  st->ops->applynoB       = STApplyNoB_Cayley;
  st->ops->applytrans     = STApplyTranspose_Cayley;
  st->ops->postsolve      = STPostSolve_Cayley;
  st->ops->backtr         = STBackTransform_Cayley;
  st->ops->setfromoptions = STSetFromOptions_Cayley;
  st->ops->setup          = STSetUp_Cayley;
  st->ops->setshift       = STSetShift_Cayley;
  st->ops->destroy        = STDestroy_Cayley;
  st->ops->view           = STView_Cayley;
  
  st->checknullspace      = STCheckNullSpace_Default;

  ctx->tau                = 0.0;
  ctx->tau_set            = PETSC_FALSE;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STCayleySetAntishift_C","STCayleySetAntishift_Cayley",
                    STCayleySetAntishift_Cayley);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
EXTERN_C_END

