/*
      EPS routines related to monitors.

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

#include "private/epsimpl.h"   /*I "slepceps.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorSet"
/*@C
   EPSMonitorSet - Sets an ADDITIONAL function to be called at every 
   iteration to monitor the error estimates for each requested eigenpair.
      
   Collective on EPS

   Input Parameters:
+  eps     - eigensolver context obtained from EPSCreate()
.  monitor - pointer to function (if this is PETSC_NULL, it turns off monitoring)
.  mctx    - [optional] context for private data for the
             monitor routine (use PETSC_NULL if no context is desired)
-  monitordestroy - [optional] routine that frees monitor context
          (may be PETSC_NULL)

   Calling Sequence of monitor:
$     monitor (EPS eps, int its, int nconv, PetscScalar *eigr, PetscScalar *eigi, PetscReal* errest, int nest, void *mctx)

+  eps    - eigensolver context obtained from EPSCreate()
.  its    - iteration number
.  nconv  - number of converged eigenpairs
.  eigr   - real part of the eigenvalues
.  eigi   - imaginary part of the eigenvalues
.  errest - relative error estimates for each eigenpair
.  nest   - number of error estimates
-  mctx   - optional monitoring context, as set by EPSMonitorSet()

   Options Database Keys:
+    -eps_monitor          - print only the first error estimate
.    -eps_monitor_all      - print error estimates at each iteration
.    -eps_monitor_conv     - print the eigenvalue approximations only when
      convergence has been reached
.    -eps_monitor_draw     - sets line graph monitor for the first unconverged
      approximate eigenvalue
.    -eps_monitor_draw_all - sets line graph monitor for all unconverged
      approximate eigenvalue
-    -eps_monitor_cancel   - cancels all monitors that have been hardwired into
      a code by calls to EPSMonitorSet(), but does not cancel those set via
      the options database.

   Notes:  
   Several different monitoring routines may be set by calling
   EPSMonitorSet() multiple times; all will be called in the 
   order in which they were set.

   Level: intermediate

.seealso: EPSMonitorFirst(), EPSMonitorAll(), EPSMonitorLG(), EPSMonitorLGAll(), EPSMonitorCancel()
@*/
PetscErrorCode EPSMonitorSet(EPS eps,PetscErrorCode (*monitor)(EPS,PetscInt,PetscInt,PetscScalar*,PetscScalar*,PetscReal*,PetscInt,void*),
                             void *mctx,PetscErrorCode (*monitordestroy)(void*))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  if (eps->numbermonitors >= MAXEPSMONITORS) {
    SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"Too many EPS monitors set");
  }
  eps->monitor[eps->numbermonitors]           = monitor;
  eps->monitorcontext[eps->numbermonitors]    = (void*)mctx;
  eps->monitordestroy[eps->numbermonitors++]  = monitordestroy;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorCancel"
/*@
   EPSMonitorCancel - Clears all monitors for an EPS object.

   Collective on EPS

   Input Parameters:
.  eps - eigensolver context obtained from EPSCreate()

   Options Database Key:
.    -eps_monitor_cancel - Cancels all monitors that have been hardwired 
      into a code by calls to EPSMonitorSet(),
      but does not cancel those set via the options database.

   Level: intermediate

.seealso: EPSMonitorSet()
@*/
PetscErrorCode EPSMonitorCancel(EPS eps)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  for (i=0; i<eps->numbermonitors; i++) {
    if (eps->monitordestroy[i]) {
      ierr = (*eps->monitordestroy[i])(eps->monitorcontext[i]);CHKERRQ(ierr);
    }
  }
  eps->numbermonitors = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EPSGetMonitorContext"
/*@C
   EPSGetMonitorContext - Gets the monitor context, as set by 
   EPSSetMonitor() for the FIRST monitor only.

   Not Collective

   Input Parameter:
.  eps - eigensolver context obtained from EPSCreate()

   Output Parameter:
.  ctx - monitor context

   Level: intermediate

.seealso: EPSSetMonitor()
@*/
PetscErrorCode EPSGetMonitorContext(EPS eps, void **ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  *ctx =      (eps->monitorcontext[0]);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorAll"
/*@C
   EPSMonitorAll - Print the current approximate values and 
   error estimates at each iteration of the eigensolver.

   Collective on EPS

   Input Parameters:
+  eps    - eigensolver context
.  its    - iteration number
.  nconv  - number of converged eigenpairs so far
.  eigr   - real part of the eigenvalues
.  eigi   - imaginary part of the eigenvalues
.  errest - error estimates
.  nest   - number of error estimates to display
-  dummy  - unused monitor context 

   Level: intermediate

.seealso: EPSMonitorSet()
@*/
PetscErrorCode EPSMonitorAll(EPS eps,PetscInt its,PetscInt nconv,PetscScalar *eigr,PetscScalar *eigi,PetscReal *errest,PetscInt nest,void *dummy)
{
  PetscErrorCode          ierr;
  PetscInt                i;
  PetscScalar             er,ei;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;

  PetscFunctionBegin;
  if (its) {
    if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)eps)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3d EPS nconv=%d Values (Errors)",its,nconv);CHKERRQ(ierr);
    for (i=0;i<nest;i++) {
      er = eigr[i]; ei = eigi[i];
      ierr = STBackTransform(eps->OP, 1, &er, &ei); CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
      ierr = PetscViewerASCIIMonitorPrintf(viewer," %g%+gi",PetscRealPart(er),PetscImaginaryPart(er));CHKERRQ(ierr);
#else
      ierr = PetscViewerASCIIMonitorPrintf(viewer," %g",er);CHKERRQ(ierr);
      if (ei!=0.0) { ierr = PetscViewerASCIIMonitorPrintf(viewer,"%+gi",ei);CHKERRQ(ierr); }
#endif
      ierr = PetscViewerASCIIMonitorPrintf(viewer," (%10.8e)",errest[i]);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"\n");CHKERRQ(ierr);
    if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorFirst"
/*@C
   EPSMonitorFirst - Print the first approximate value and 
   error estimate at each iteration of the eigensolver.

   Collective on EPS

   Input Parameters:
+  eps    - eigensolver context
.  its    - iteration number
.  nconv  - number of converged eigenpairs so far
.  eigr   - real part of the eigenvalues
.  eigi   - imaginary part of the eigenvalues
.  errest - error estimates
.  nest   - number of error estimates to display
-  dummy  - unused monitor context 

   Level: intermediate

.seealso: EPSMonitorSet()
@*/
PetscErrorCode EPSMonitorFirst(EPS eps,PetscInt its,PetscInt nconv,PetscScalar *eigr,PetscScalar *eigi,PetscReal *errest,PetscInt nest,void *dummy)
{
  PetscErrorCode          ierr;
  PetscScalar             er,ei;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;

  PetscFunctionBegin;
  if (its && nconv<nest) {
    if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)eps)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3d EPS nconv=%d first unconverged value (error)",its,nconv);CHKERRQ(ierr);
    er = eigr[nconv]; ei = eigi[nconv];
    ierr = STBackTransform(eps->OP, 1, &er, &ei); CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
    ierr = PetscViewerASCIIMonitorPrintf(viewer," %g%+gi",PetscRealPart(er),PetscImaginaryPart(er));CHKERRQ(ierr);
#else
    ierr = PetscViewerASCIIMonitorPrintf(viewer," %g",er);CHKERRQ(ierr);
    if (ei!=0.0) { ierr = PetscViewerASCIIMonitorPrintf(viewer,"%+gi",ei);CHKERRQ(ierr); }
#endif
    ierr = PetscViewerASCIIMonitorPrintf(viewer," (%10.8e)\n",errest[nconv]);CHKERRQ(ierr);
    if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorConverged"
/*@C
   EPSMonitorConverged - Print the approximate values and 
   error estimates as they converge.

   Collective on EPS

   Input Parameters:
+  eps    - eigensolver context
.  its    - iteration number
.  nconv  - number of converged eigenpairs so far
.  eigr   - real part of the eigenvalues
.  eigi   - imaginary part of the eigenvalues
.  errest - error estimates
.  nest   - number of error estimates to display
-  dummy  - unused monitor context 

   Level: intermediate

.seealso: EPSMonitorSet()
@*/
PetscErrorCode EPSMonitorConverged(EPS eps,PetscInt its,PetscInt nconv,PetscScalar *eigr,PetscScalar *eigi,PetscReal *errest,PetscInt nest,void *dummy)
{
  PetscErrorCode  ierr;
  PetscInt        i;
  PetscScalar     er,ei;
  EPSMONITOR_CONV *ctx = (EPSMONITOR_CONV*) dummy;

  PetscFunctionBegin;
  if (!its) {
    ctx->oldnconv = 0;
  } else {
    for (i=ctx->oldnconv;i<nconv;i++) {
      ierr = PetscViewerASCIIMonitorPrintf(ctx->viewer,"%3d EPS converged value (error) #%d",its,i);CHKERRQ(ierr);
      er = eigr[i]; ei = eigi[i];
      ierr = STBackTransform(eps->OP, 1, &er, &ei); CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
      ierr = PetscViewerASCIIMonitorPrintf(ctx->viewer," %g%+gi",PetscRealPart(er),PetscImaginaryPart(er));CHKERRQ(ierr);
#else
      ierr = PetscViewerASCIIMonitorPrintf(ctx->viewer," %g",er);CHKERRQ(ierr);
      if (ei!=0.0) { ierr = PetscViewerASCIIMonitorPrintf(ctx->viewer,"%+gi",ei);CHKERRQ(ierr); }
#endif
      ierr = PetscViewerASCIIMonitorPrintf(ctx->viewer," (%10.8e)\n",errest[i]);CHKERRQ(ierr);
    }
    ctx->oldnconv = nconv;
  }
  PetscFunctionReturn(0);
}

PetscErrorCode EPSMonitorDestroy_Converged(EPSMONITOR_CONV *ctx)
{
  PetscErrorCode  ierr;
  PetscFunctionBegin;
  ierr = PetscViewerASCIIMonitorDestroy(ctx->viewer);CHKERRQ(ierr);
  ierr = PetscFree(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorLG"
PetscErrorCode EPSMonitorLG(EPS eps,PetscInt its,PetscInt nconv,PetscScalar *eigr,PetscScalar *eigi,PetscReal *errest,PetscInt nest,void *monctx)
{
  PetscViewer    viewer = (PetscViewer) monctx;
  PetscDraw      draw,draw1;
  PetscDrawLG    lg,lg1;
  PetscErrorCode ierr;
  PetscReal      x,y,myeigr,p;
  PetscScalar    er,ei;

  PetscFunctionBegin;

  if (!viewer) { viewer = PETSC_VIEWER_DRAW_(((PetscObject)eps)->comm); }

  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDrawLG(viewer,0,&lg);CHKERRQ(ierr);
  if (!its) {
    ierr = PetscDrawSetTitle(draw,"Error estimates");CHKERRQ(ierr);
    ierr = PetscDrawSetDoubleBuffer(draw);CHKERRQ(ierr);
    ierr = PetscDrawLGSetDimension(lg,1);CHKERRQ(ierr);
    ierr = PetscDrawLGReset(lg);CHKERRQ(ierr);
    ierr = PetscDrawLGSetLimits(lg,0,1.0,log10(eps->tol)-2,0.0);CHKERRQ(ierr);
  }

  /* In the hermitian case, the eigenvalues are real and can be plotted */
  if (eps->ishermitian) {
    ierr = PetscViewerDrawGetDraw(viewer,1,&draw1);CHKERRQ(ierr);
    ierr = PetscViewerDrawGetDrawLG(viewer,1,&lg1);CHKERRQ(ierr);
    if (!its) {
      ierr = PetscDrawSetTitle(draw1,"Approximate eigenvalues");CHKERRQ(ierr);
      ierr = PetscDrawSetDoubleBuffer(draw1);CHKERRQ(ierr);
      ierr = PetscDrawLGSetDimension(lg1,1);CHKERRQ(ierr);
      ierr = PetscDrawLGReset(lg1);CHKERRQ(ierr);
      ierr = PetscDrawLGSetLimits(lg1,0,1.0,1.e20,-1.e20);CHKERRQ(ierr);
    }
  }

  x = (PetscReal) its;
  if (errest[nconv] > 0.0) y = log10(errest[nconv]); else y = 0.0;
  ierr = PetscDrawLGAddPoint(lg,&x,&y);CHKERRQ(ierr);
  if (eps->ishermitian) {
    er = eigr[nconv]; ei = eigi[nconv];
    ierr = STBackTransform(eps->OP, 1, &er, &ei); CHKERRQ(ierr);
    myeigr = PetscRealPart(er);
    ierr = PetscDrawLGAddPoint(lg1,&x,&myeigr);CHKERRQ(ierr);
    ierr = PetscDrawGetPause(draw1,&p);CHKERRQ(ierr);
    ierr = PetscDrawSetPause(draw1,0);CHKERRQ(ierr);    
    ierr = PetscDrawLGDraw(lg1);CHKERRQ(ierr);
    ierr = PetscDrawSetPause(draw1,p);CHKERRQ(ierr);    
  }
  ierr = PetscDrawLGDraw(lg);CHKERRQ(ierr);
  PetscFunctionReturn(0);
} 


#undef __FUNCT__  
#define __FUNCT__ "EPSMonitorLGAll"
PetscErrorCode EPSMonitorLGAll(EPS eps,PetscInt its,PetscInt nconv,PetscScalar *eigr,PetscScalar *eigi,PetscReal *errest,PetscInt nest,void *monctx)
{
  PetscViewer    viewer = (PetscViewer) monctx;
  PetscDraw      draw,draw1;
  PetscDrawLG    lg,lg1;
  PetscErrorCode ierr;
  PetscReal      *x,*y,*myeigr,p;
  PetscInt       i,n = PetscMin(eps->nev,255);
  PetscScalar    er,ei;

  PetscFunctionBegin;

  if (!viewer) { viewer = PETSC_VIEWER_DRAW_(((PetscObject)eps)->comm); }

  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDrawLG(viewer,0,&lg);CHKERRQ(ierr);
  if (!its) {
    ierr = PetscDrawSetTitle(draw,"Error estimates");CHKERRQ(ierr);
    ierr = PetscDrawSetDoubleBuffer(draw);CHKERRQ(ierr);
    ierr = PetscDrawLGSetDimension(lg,n);CHKERRQ(ierr);
    ierr = PetscDrawLGReset(lg);CHKERRQ(ierr);
    ierr = PetscDrawLGSetLimits(lg,0,1.0,log10(eps->tol)-2,0.0);CHKERRQ(ierr);
  }

  /* In the hermitian case, the eigenvalues are real and can be plotted */
  if (eps->ishermitian) {
    ierr = PetscViewerDrawGetDraw(viewer,1,&draw1);CHKERRQ(ierr);
    ierr = PetscViewerDrawGetDrawLG(viewer,1,&lg1);CHKERRQ(ierr);
    if (!its) {
      ierr = PetscDrawSetTitle(draw1,"Approximate eigenvalues");CHKERRQ(ierr);
      ierr = PetscDrawSetDoubleBuffer(draw1);CHKERRQ(ierr);
      ierr = PetscDrawLGSetDimension(lg1,n);CHKERRQ(ierr);
      ierr = PetscDrawLGReset(lg1);CHKERRQ(ierr);
      ierr = PetscDrawLGSetLimits(lg1,0,1.0,1.e20,-1.e20);CHKERRQ(ierr);
    }
  }

  ierr = PetscMalloc(sizeof(PetscReal)*n,&x);CHKERRQ(ierr);
  ierr = PetscMalloc(sizeof(PetscReal)*n,&y);CHKERRQ(ierr);
  for (i=0;i<n;i++) {
    x[i] = (PetscReal) its;
    if (i < nest && errest[i] > 0.0) y[i] = log10(errest[i]);
    else y[i] = 0.0;
  }
  ierr = PetscDrawLGAddPoint(lg,x,y);CHKERRQ(ierr);
  if (eps->ishermitian) {
    ierr = PetscMalloc(sizeof(PetscReal)*n,&myeigr);CHKERRQ(ierr);
    for(i=0;i<n;i++) {
      er = eigr[i]; ei = eigi[i];
      ierr = STBackTransform(eps->OP, 1, &er, &ei); CHKERRQ(ierr);
      if (i < nest) myeigr[i] = PetscRealPart(er);
      else myeigr[i] = 0.0;
    }
    ierr = PetscDrawLGAddPoint(lg1,x,myeigr);CHKERRQ(ierr);
    ierr = PetscDrawGetPause(draw1,&p);CHKERRQ(ierr);
    ierr = PetscDrawSetPause(draw1,0);CHKERRQ(ierr);    
    ierr = PetscDrawLGDraw(lg1);CHKERRQ(ierr);
    ierr = PetscDrawSetPause(draw1,p);CHKERRQ(ierr);    
    ierr = PetscFree(myeigr);CHKERRQ(ierr);
  }
  ierr = PetscDrawLGDraw(lg);CHKERRQ(ierr);
  ierr = PetscFree(x);CHKERRQ(ierr);
  ierr = PetscFree(y);CHKERRQ(ierr);  
  PetscFunctionReturn(0);
} 

