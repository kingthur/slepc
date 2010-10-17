/*
   This provides a simple shell interface for programmers to 
   create their own spectral transformations without writing much 
   interface code.

   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2010, Universidad Politecnica de Valencia, Spain

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

#include "private/stimpl.h"        /*I "slepcst.h" I*/
#include "slepceps.h"

EXTERN_C_BEGIN 
typedef struct {
  void           *ctx;                       /* user provided context */
  PetscErrorCode (*apply)(ST,Vec,Vec);
  PetscErrorCode (*applytrans)(ST,Vec,Vec);
  PetscErrorCode (*backtr)(ST,PetscInt n,PetscScalar*,PetscScalar*);
} ST_Shell;
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "STShellGetContext"
/*@C
    STShellGetContext - Returns the user-provided context associated with a shell ST

    Not Collective

    Input Parameter:
.   st - spectral transformation context

    Output Parameter:
.   ctx - the user provided context

    Level: advanced

    Notes:
    This routine is intended for use within various shell routines
    
.seealso: STShellSetContext()
@*/
PetscErrorCode STShellGetContext(ST st,void **ctx)
{
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(st,ST_CLASSID,1);
  PetscValidPointer(ctx,2); 
  ierr = PetscTypeCompare((PetscObject)st,STSHELL,&flg);CHKERRQ(ierr);
  if (!flg) *ctx = 0; 
  else      *ctx = ((ST_Shell*)(st->data))->ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STShellSetContext"
/*@
    STShellSetContext - sets the context for a shell ST

   Collective on ST

    Input Parameters:
+   st - the shell ST
-   ctx - the context

   Level: advanced

   Fortran Notes: The context can only be an integer or a PetscObject;
      unfortunately it cannot be a Fortran array or derived type.

.seealso: STShellGetContext()
@*/
PetscErrorCode STShellSetContext(ST st,void *ctx)
{
  ST_Shell      *shell = (ST_Shell*)st->data;
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(st,ST_CLASSID,1);
  ierr = PetscTypeCompare((PetscObject)st,STSHELL,&flg);CHKERRQ(ierr);
  if (flg) {
    shell->ctx = ctx;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STApply_Shell"
PetscErrorCode STApply_Shell(ST st,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_Shell       *shell = (ST_Shell*)st->data;

  PetscFunctionBegin;
  if (!shell->apply) SETERRQ(((PetscObject)st)->comm,PETSC_ERR_USER,"No apply() routine provided to Shell ST");
  PetscStackPush("STSHELL apply() user function");
  CHKMEMQ;
  ierr  = (*shell->apply)(st,x,y);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STApplyTranspose_Shell"
PetscErrorCode STApplyTranspose_Shell(ST st,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_Shell       *shell = (ST_Shell*)st->data;

  PetscFunctionBegin;
  if (!shell->applytrans) SETERRQ(((PetscObject)st)->comm,PETSC_ERR_USER,"No applytranspose() routine provided to Shell ST");
  PetscStackPush("STSHELL applytranspose() user function");
  CHKMEMQ;
  ierr  = (*shell->applytrans)(st,x,y);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STBackTransform_Shell"
PetscErrorCode STBackTransform_Shell(ST st,PetscInt n,PetscScalar *eigr,PetscScalar *eigi)
{
  PetscErrorCode ierr;
  ST_Shell       *shell = (ST_Shell*)st->data;

  PetscFunctionBegin;
  if (shell->backtr) {
    PetscStackPush("STSHELL backtransform() user function");
    CHKMEMQ;
    ierr  = (*shell->backtr)(st,n,eigr,eigi);CHKERRQ(ierr);
    CHKMEMQ;
    PetscStackPop;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STDestroy_Shell"
PetscErrorCode STDestroy_Shell(ST st)
{
  PetscErrorCode ierr;
  ST_Shell       *shell = (ST_Shell*)st->data;

  PetscFunctionBegin;
  ierr = PetscFree(shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STShellSetApply_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STShellSetApplyTranspose_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STShellSetBackTransform_C","",PETSC_NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "STShellSetApply_Shell"
PetscErrorCode STShellSetApply_Shell(ST st,PetscErrorCode (*apply)(ST,Vec,Vec))
{
  ST_Shell *shell = (ST_Shell*)st->data;

  PetscFunctionBegin;
  shell->apply = apply;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "STShellSetApplyTranspose_Shell"
PetscErrorCode STShellSetApplyTranspose_Shell(ST st,PetscErrorCode (*applytrans)(ST,Vec,Vec))
{
  ST_Shell *shell = (ST_Shell*)st->data;

  PetscFunctionBegin;
  shell->applytrans = applytrans;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "STShellSetBackTransform_Shell"
PetscErrorCode STShellSetBackTransform_Shell(ST st,PetscErrorCode (*backtr)(ST,PetscInt,PetscScalar*,PetscScalar*))
{
  ST_Shell *shell = (ST_Shell *) st->data;

  PetscFunctionBegin;
  shell->backtr = backtr;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "STShellSetApply"
/*@C
   STShellSetApply - Sets routine to use as the application of the 
   operator to a vector in the user-defined spectral transformation.

   Collective on ST

   Input Parameters:
+  st    - the spectral transformation context
-  apply - the application-provided transformation routine

   Calling sequence of apply:
.vb
   PetscErrorCode apply (ST st,Vec xin,Vec xout)
.ve

+  st   - the spectral transformation context
.  xin  - input vector
-  xout - output vector

   Level: developer

.seealso: STShellSetBackTransform(), STShellSetApplyTranspose()
@*/
PetscErrorCode STShellSetApply(ST st,PetscErrorCode (*apply)(ST,Vec,Vec))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(st,ST_CLASSID,1);
  ierr = PetscTryMethod(st,"STShellSetApply_C",(ST,PetscErrorCode (*)(ST,Vec,Vec)),(st,apply));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STShellSetApplyTranspose"
/*@C
   STShellSetApplyTranspose - Sets routine to use as the application of the 
   transposed operator to a vector in the user-defined spectral transformation.

   Collective on ST

   Input Parameters:
+  st    - the spectral transformation context
-  applytrans - the application-provided transformation routine

   Calling sequence of apply:
.vb
   PetscErrorCode applytrans (ST st,Vec xin,Vec xout)
.ve

+  st   - the spectral transformation context
.  xin  - input vector
-  xout - output vector

   Level: developer

.seealso: STShellSetApply(), STShellSetBackTransform()
@*/
PetscErrorCode STShellSetApplyTranspose(ST st,PetscErrorCode (*applytrans)(ST,Vec,Vec))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(st,ST_CLASSID,1);
  ierr = PetscTryMethod(st,"STShellSetApplyTranspose_C",(ST,PetscErrorCode (*)(ST,Vec,Vec)),(st,applytrans));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STShellSetBackTransform"
/*@C
   STShellSetBackTransform - Sets the routine to be called after the 
   eigensolution process has finished in order to transform back the
   computed eigenvalues.

   Collective on ST

   Input Parameters:
+  st     - the spectral transformation context
-  backtr - the application-provided backtransform routine

   Calling sequence of backtr:
.vb
   PetscErrorCode backtr (ST st,PetscScalar *eigr,PetscScalar *eigi)
.ve

+  st   - the spectral transformation context
.  eigr - pointer ot the real part of the eigenvalue to transform back
-  eigi - pointer ot the imaginary part 

   Level: developer

.seealso: STShellSetApply(), STShellSetApplyTranspose()
@*/
PetscErrorCode STShellSetBackTransform(ST st,PetscErrorCode (*backtr)(ST,PetscInt,PetscScalar*,PetscScalar*))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(st,ST_CLASSID,1);
  ierr = PetscTryMethod(st,"STShellSetBackTransform_C",(ST,PetscErrorCode (*)(ST,PetscInt,PetscScalar*,PetscScalar*)),(st,backtr));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "STSetFromOptions_Shell"
PetscErrorCode STSetFromOptions_Shell(ST st) 
{
  PetscErrorCode ierr;
  PC             pc;
  const PCType   pctype;
  const KSPType  ksptype;

  PetscFunctionBegin;

  ierr = KSPGetPC(st->ksp,&pc);CHKERRQ(ierr);
  ierr = KSPGetType(st->ksp,&ksptype);CHKERRQ(ierr);
  ierr = PCGetType(pc,&pctype);CHKERRQ(ierr);
  if (!pctype && !ksptype) {
    if (st->shift_matrix == ST_MATMODE_SHELL) {
      /* in shell mode use GMRES with Jacobi as the default */
      ierr = KSPSetType(st->ksp,KSPGMRES);CHKERRQ(ierr);
      ierr = PCSetType(pc,PCJACOBI);CHKERRQ(ierr);
    } else {
      /* use direct solver as default */
      ierr = KSPSetType(st->ksp,KSPPREONLY);CHKERRQ(ierr);
      ierr = PCSetType(pc,PCREDUNDANT);CHKERRQ(ierr);
    }
  }

  PetscFunctionReturn(0);
}

/*MC
   STSHELL - Creates a new spectral transformation class.
          This is intended to provide a simple class to use with EPS.
	  You should not use this if you plan to make a complete class.

  Level: advanced

  Usage:
$             PetscErrorCode (*apply)(void*,Vec,Vec);
$             PetscErrorCode (*applytrans)(void*,Vec,Vec);
$             PetscErrorCode (*backtr)(void*,PetscScalar*,PetscScalar*);
$             STCreate(comm,&st);
$             STSetType(st,STSHELL);
$             STShellSetApply(st,apply);
$             STShellSetApplyTranspose(st,applytrans);
$             STShellSetBackTransform(st,backtr);    (optional)

M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "STCreate_Shell"
PetscErrorCode STCreate_Shell(ST st)
{
  PetscErrorCode ierr;
  ST_Shell       *shell;

  PetscFunctionBegin;
  st->ops->destroy = STDestroy_Shell;
  ierr = PetscNew(ST_Shell,&shell);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(st,sizeof(ST_Shell));CHKERRQ(ierr);

  st->data           = (void *) shell;

  st->ops->apply          = STApply_Shell;
  st->ops->applytrans     = STApplyTranspose_Shell;
  st->ops->backtr         = STBackTransform_Shell;
  st->ops->view           = PETSC_NULL;
  st->ops->setfromoptions = STSetFromOptions_Shell;

  shell->apply       = 0;
  shell->applytrans  = 0;
  shell->backtr      = 0;
  shell->ctx         = 0;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STShellSetApply_C","STShellSetApply_Shell",STShellSetApply_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STShellSetApplyTranspose_C","STShellSetApplyTranspose_Shell",STShellSetApplyTranspose_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)st,"STShellSetBackTransform_C","STShellSetBackTransform_Shell",STShellSetBackTransform_Shell);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
EXTERN_C_END

