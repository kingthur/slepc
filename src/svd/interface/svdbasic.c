/*
     The basic SVD routines, Create, View, etc. are here.

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

#include <slepc-private/svdimpl.h>      /*I "slepcsvd.h" I*/

PetscFunctionList SVDList = 0;
PetscBool         SVDRegisterAllCalled = PETSC_FALSE;
PetscClassId      SVD_CLASSID = 0;
PetscLogEvent     SVD_SetUp = 0,SVD_Solve = 0;
static PetscBool  SVDPackageInitialized = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "SVDFinalizePackage"
/*@C
   SVDFinalizePackage - This function destroys everything in the Slepc interface
   to the SVD package. It is called from SlepcFinalize().

   Level: developer

.seealso: SlepcFinalize()
@*/
PetscErrorCode SVDFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&SVDList);CHKERRQ(ierr);
  SVDPackageInitialized = PETSC_FALSE;
  SVDRegisterAllCalled  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDInitializePackage"
/*@C
   SVDInitializePackage - This function initializes everything in the SVD package. It is called
   from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to SVDCreate()
   when using static libraries.

   Level: developer

.seealso: SlepcInitialize()
@*/
PetscErrorCode SVDInitializePackage(void)
{
  char           logList[256];
  char           *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (SVDPackageInitialized) PetscFunctionReturn(0);
  SVDPackageInitialized = PETSC_TRUE;
  /* Register Classes */
  ierr = PetscClassIdRegister("Singular Value Solver",&SVD_CLASSID);CHKERRQ(ierr);
  /* Register Constructors */
  ierr = SVDRegisterAll();CHKERRQ(ierr);
  /* Register Events */
  ierr = PetscLogEventRegister("SVDSetUp",SVD_CLASSID,&SVD_SetUp);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("SVDSolve",SVD_CLASSID,&SVD_Solve);CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,"-info_exclude",logList,256,&opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList,"svd",&className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(SVD_CLASSID);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,"-log_summary_exclude",logList,256,&opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList,"svd",&className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(SVD_CLASSID);CHKERRQ(ierr);
    }
  }
  ierr = PetscRegisterFinalize(SVDFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDView"
/*@C
   SVDView - Prints the SVD data structure.

   Collective on SVD

   Input Parameters:
+  svd - the singular value solver context
-  viewer - optional visualization context

   Options Database Key:
.  -svd_view -  Calls SVDView() at end of SVDSolve()

   Note:
   The available visualization contexts include
+     PETSC_VIEWER_STDOUT_SELF - standard output (default)
-     PETSC_VIEWER_STDOUT_WORLD - synchronized standard
         output where only the first processor opens
         the file.  All other processors send their
         data to the first processor to print.

   The user can open an alternative visualization context with
   PetscViewerASCIIOpen() - output to a specified file.

   Level: beginner

.seealso: STView(), PetscViewerASCIIOpen()
@*/
PetscErrorCode SVDView(SVD svd,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      isascii,isshell;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)svd));
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(svd,1,viewer,2);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscObjectPrintClassNamePrefixType((PetscObject)svd,viewer);CHKERRQ(ierr);
    if (svd->ops->view) {
      ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
      ierr = (*svd->ops->view)(svd,viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
    }
    switch (svd->transmode) {
      case SVD_TRANSPOSE_EXPLICIT:
        ierr = PetscViewerASCIIPrintf(viewer,"  transpose mode: explicit\n");CHKERRQ(ierr);
        break;
      case SVD_TRANSPOSE_IMPLICIT:
        ierr = PetscViewerASCIIPrintf(viewer,"  transpose mode: implicit\n");CHKERRQ(ierr);
        break;
      default:
        ierr = PetscViewerASCIIPrintf(viewer,"  transpose mode: not yet set\n");CHKERRQ(ierr);
    }
    if (svd->which == SVD_LARGEST) {
      ierr = PetscViewerASCIIPrintf(viewer,"  selected portion of the spectrum: largest\n");CHKERRQ(ierr);
    } else {
      ierr = PetscViewerASCIIPrintf(viewer,"  selected portion of the spectrum: smallest\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer,"  number of singular values (nsv): %D\n",svd->nsv);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  number of column vectors (ncv): %D\n",svd->ncv);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  maximum dimension of projected problem (mpd): %D\n",svd->mpd);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  maximum number of iterations: %D\n",svd->max_it);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  tolerance: %g\n",(double)svd->tol);CHKERRQ(ierr);
    if (svd->nini) {
      ierr = PetscViewerASCIIPrintf(viewer,"  dimension of user-provided initial space: %D\n",PetscAbs(svd->nini));CHKERRQ(ierr);
    }
    if (svd->ninil) {
      ierr = PetscViewerASCIIPrintf(viewer,"  dimension of user-provided initial left space: %D\n",PetscAbs(svd->ninil));CHKERRQ(ierr);
    }
  } else {
    if (svd->ops->view) {
      ierr = (*svd->ops->view)(svd,viewer);CHKERRQ(ierr);
    }
  }
  ierr = PetscObjectTypeCompareAny((PetscObject)svd,&isshell,SVDCROSS,SVDCYCLIC,"");CHKERRQ(ierr);
  if (!isshell) {
    if (!svd->ip) { ierr = SVDGetIP(svd,&svd->ip);CHKERRQ(ierr); }
    ierr = IPView(svd->ip,viewer);CHKERRQ(ierr);
    if (!svd->ds) { ierr = SVDGetDS(svd,&svd->ds);CHKERRQ(ierr); }
    ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_INFO);CHKERRQ(ierr);
    ierr = DSView(svd->ds,viewer);CHKERRQ(ierr);
    ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDPrintSolution"
/*@
   SVDPrintSolution - Prints the computed singular values.

   Collective on SVD

   Input Parameters:
+  svd - the singular value solver context
-  viewer - optional visualization context

   Options Database Key:
.  -svd_terse - print only minimal information

   Note:
   By default, this function prints a table with singular values and associated
   relative errors. With -svd_terse only the singular values are printed.

   Level: intermediate

.seealso: PetscViewerASCIIOpen()
@*/
PetscErrorCode SVDPrintSolution(SVD svd,PetscViewer viewer)
{
  PetscBool      terse,errok,isascii;
  PetscReal      error,sigma;
  PetscInt       i,j;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)svd));
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(svd,1,viewer,2);
  if (!svd->sigma) SETERRQ(PetscObjectComm((PetscObject)svd),PETSC_ERR_ARG_WRONGSTATE,"SVDSolve must be called first");
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (!isascii) PetscFunctionReturn(0);

  ierr = PetscOptionsHasName(NULL,"-svd_terse",&terse);CHKERRQ(ierr);
  if (terse) {
    if (svd->nconv<svd->nsv) {
      ierr = PetscViewerASCIIPrintf(viewer," Problem: less than %D singular values converged\n\n",svd->nsv);CHKERRQ(ierr);
    } else {
      errok = PETSC_TRUE;
      for (i=0;i<svd->nsv;i++) {
        ierr = SVDComputeRelativeError(svd,i,&error);CHKERRQ(ierr);
        errok = (errok && error<svd->tol)? PETSC_TRUE: PETSC_FALSE;
      }
      if (errok) {
        ierr = PetscViewerASCIIPrintf(viewer," All requested singular values computed up to the required tolerance:");CHKERRQ(ierr);
        for (i=0;i<=(svd->nsv-1)/8;i++) {
          ierr = PetscViewerASCIIPrintf(viewer,"\n     ");CHKERRQ(ierr);
          for (j=0;j<PetscMin(8,svd->nsv-8*i);j++) {
            ierr = SVDGetSingularTriplet(svd,8*i+j,&sigma,NULL,NULL);CHKERRQ(ierr);
            ierr = PetscViewerASCIIPrintf(viewer,"%.5f",(double)sigma);CHKERRQ(ierr);
            if (8*i+j+1<svd->nsv) { ierr = PetscViewerASCIIPrintf(viewer,", ");CHKERRQ(ierr); }
          }
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n\n");CHKERRQ(ierr);
      } else {
        ierr = PetscViewerASCIIPrintf(viewer," Problem: some of the first %D relative errors are higher than the tolerance\n\n",svd->nsv);CHKERRQ(ierr);
      }
    }
  } else {
    ierr = PetscViewerASCIIPrintf(viewer," Number of converged approximate singular triplets: %D\n\n",svd->nconv);CHKERRQ(ierr);
    if (svd->nconv>0) {
      ierr = PetscViewerASCIIPrintf(viewer,
           "          sigma            relative error\n"
           "   --------------------- ------------------\n");CHKERRQ(ierr);
      for (i=0;i<svd->nconv;i++) {
        ierr = SVDGetSingularTriplet(svd,i,&sigma,NULL,NULL);CHKERRQ(ierr);
        ierr = SVDComputeRelativeError(svd,i,&error);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"       % 6f          %12g\n",(double)sigma,(double)error);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDCreate"
/*@C
   SVDCreate - Creates the default SVD context.

   Collective on MPI_Comm

   Input Parameter:
.  comm - MPI communicator

   Output Parameter:
.  svd - location to put the SVD context

   Note:
   The default SVD type is SVDCROSS

   Level: beginner

.seealso: SVDSetUp(), SVDSolve(), SVDDestroy(), SVD
@*/
PetscErrorCode SVDCreate(MPI_Comm comm,SVD *outsvd)
{
  PetscErrorCode ierr;
  SVD            svd;

  PetscFunctionBegin;
  PetscValidPointer(outsvd,2);
  *outsvd = 0;
  ierr = SVDInitializePackage();CHKERRQ(ierr);
  ierr = SlepcHeaderCreate(svd,_p_SVD,struct _SVDOps,SVD_CLASSID,"SVD","Singular Value Decomposition","SVD",comm,SVDDestroy,SVDView);CHKERRQ(ierr);

  svd->OP             = NULL;
  svd->ip             = NULL;
  svd->ds             = NULL;
  svd->A              = NULL;
  svd->AT             = NULL;
  svd->transmode      = (SVDTransposeMode)PETSC_DECIDE;
  svd->sigma          = NULL;
  svd->perm           = NULL;
  svd->U              = NULL;
  svd->V              = NULL;
  svd->IS             = NULL;
  svd->ISL            = NULL;
  svd->tl             = NULL;
  svd->tr             = NULL;
  svd->rand           = NULL;
  svd->which          = SVD_LARGEST;
  svd->n              = 0;
  svd->nconv          = 0;
  svd->nsv            = 1;
  svd->ncv            = 0;
  svd->mpd            = 0;
  svd->nini           = 0;
  svd->ninil          = 0;
  svd->its            = 0;
  svd->max_it         = 0;
  svd->tol            = PETSC_DEFAULT;
  svd->errest         = NULL;
  svd->data           = NULL;
  svd->setupcalled    = 0;
  svd->reason         = SVD_CONVERGED_ITERATING;
  svd->numbermonitors = 0;
  svd->matvecs        = 0;
  svd->trackall       = PETSC_FALSE;

  ierr = PetscRandomCreate(comm,&svd->rand);CHKERRQ(ierr);
  ierr = PetscRandomSetSeed(svd->rand,0x12345678);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)svd,(PetscObject)svd->rand);CHKERRQ(ierr);
  *outsvd = svd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDReset"
/*@
   SVDReset - Resets the SVD context to the setupcalled=0 state and removes any
   allocated objects.

   Collective on SVD

   Input Parameter:
.  svd - singular value solver context obtained from SVDCreate()

   Level: advanced

.seealso: SVDDestroy()
@*/
PetscErrorCode SVDReset(SVD svd)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  if (svd->ops->reset) { ierr = (svd->ops->reset)(svd);CHKERRQ(ierr); }
  if (svd->ip) { ierr = IPReset(svd->ip);CHKERRQ(ierr); }
  if (svd->ds) { ierr = DSReset(svd->ds);CHKERRQ(ierr); }
  ierr = MatDestroy(&svd->OP);CHKERRQ(ierr);
  ierr = MatDestroy(&svd->A);CHKERRQ(ierr);
  ierr = MatDestroy(&svd->AT);CHKERRQ(ierr);
  ierr = VecDestroy(&svd->tl);CHKERRQ(ierr);
  ierr = VecDestroy(&svd->tr);CHKERRQ(ierr);
  if (svd->n) {
    ierr = PetscFree3(svd->sigma,svd->perm,svd->errest);CHKERRQ(ierr);
    ierr = VecDestroyVecs(svd->n,&svd->U);CHKERRQ(ierr);
    ierr = VecDestroyVecs(svd->n,&svd->V);CHKERRQ(ierr);
  }
  svd->transmode   = (SVDTransposeMode)PETSC_DECIDE;
  svd->matvecs     = 0;
  svd->setupcalled = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDDestroy"
/*@C
   SVDDestroy - Destroys the SVD context.

   Collective on SVD

   Input Parameter:
.  svd - singular value solver context obtained from SVDCreate()

   Level: beginner

.seealso: SVDCreate(), SVDSetUp(), SVDSolve()
@*/
PetscErrorCode SVDDestroy(SVD *svd)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*svd) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*svd,SVD_CLASSID,1);
  if (--((PetscObject)(*svd))->refct > 0) { *svd = 0; PetscFunctionReturn(0); }
  ierr = SVDReset(*svd);CHKERRQ(ierr);
  if ((*svd)->ops->destroy) { ierr = (*(*svd)->ops->destroy)(*svd);CHKERRQ(ierr); }
  ierr = IPDestroy(&(*svd)->ip);CHKERRQ(ierr);
  ierr = DSDestroy(&(*svd)->ds);CHKERRQ(ierr);
  /* just in case the initial vectors have not been used */
  ierr = SlepcBasisDestroy_Private(&(*svd)->nini,&(*svd)->IS);CHKERRQ(ierr);
  ierr = SlepcBasisDestroy_Private(&(*svd)->ninil,&(*svd)->ISL);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&(*svd)->rand);CHKERRQ(ierr);
  ierr = SVDMonitorCancel(*svd);CHKERRQ(ierr);
  ierr = PetscHeaderDestroy(svd);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDSetType"
/*@C
   SVDSetType - Selects the particular solver to be used in the SVD object.

   Logically Collective on SVD

   Input Parameters:
+  svd      - the singular value solver context
-  type     - a known method

   Options Database Key:
.  -svd_type <method> - Sets the method; use -help for a list
    of available methods

   Notes:
   See "slepc/include/slepcsvd.h" for available methods. The default
   is SVDCROSS.

   Normally, it is best to use the SVDSetFromOptions() command and
   then set the SVD type from the options database rather than by using
   this routine.  Using the options database provides the user with
   maximum flexibility in evaluating the different available methods.
   The SVDSetType() routine is provided for those situations where it
   is necessary to set the iterative solver independently of the command
   line or options database.

   Level: intermediate

.seealso: SVDType
@*/
PetscErrorCode SVDSetType(SVD svd,SVDType type)
{
  PetscErrorCode ierr,(*r)(SVD);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  PetscValidCharPointer(type,2);

  ierr = PetscObjectTypeCompare((PetscObject)svd,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr = PetscFunctionListFind(SVDList,type,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PetscObjectComm((PetscObject)svd),PETSC_ERR_ARG_UNKNOWN_TYPE,"Unknown SVD type given: %s",type);

  if (svd->ops->destroy) { ierr = (*svd->ops->destroy)(svd);CHKERRQ(ierr); }
  ierr = PetscMemzero(svd->ops,sizeof(struct _SVDOps));CHKERRQ(ierr);

  svd->setupcalled = 0;
  ierr = PetscObjectChangeTypeName((PetscObject)svd,type);CHKERRQ(ierr);
  ierr = (*r)(svd);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDGetType"
/*@C
   SVDGetType - Gets the SVD type as a string from the SVD object.

   Not Collective

   Input Parameter:
.  svd - the singular value solver context

   Output Parameter:
.  name - name of SVD method

   Level: intermediate

.seealso: SVDSetType()
@*/
PetscErrorCode SVDGetType(SVD svd,SVDType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)svd)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDRegister"
/*@C
   SVDRegister - Adds a method to the singular value solver package.

   Not Collective

   Input Parameters:
+  name - name of a new user-defined solver
-  function - routine to create the solver context

   Notes:
   SVDRegister() may be called multiple times to add several user-defined solvers.

   Sample usage:
.vb
   SVDRegister("my_solver",MySolverCreate);
.ve

   Then, your solver can be chosen with the procedural interface via
$     SVDSetType(svd,"my_solver")
   or at runtime via the option
$     -svd_type my_solver

   Level: advanced

.seealso: SVDRegisterAll()
@*/
PetscErrorCode SVDRegister(const char *name,PetscErrorCode (*function)(SVD))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&SVDList,name,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDSetIP"
/*@
   SVDSetIP - Associates an inner product object to the
   singular value solver.

   Collective on SVD

   Input Parameters:
+  svd - singular value solver context obtained from SVDCreate()
-  ip  - the inner product object

   Note:
   Use SVDGetIP() to retrieve the inner product context (for example,
   to free it at the end of the computations).

   Level: advanced

.seealso: SVDGetIP()
@*/
PetscErrorCode SVDSetIP(SVD svd,IP ip)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  PetscValidHeaderSpecific(ip,IP_CLASSID,2);
  PetscCheckSameComm(svd,1,ip,2);
  ierr = PetscObjectReference((PetscObject)ip);CHKERRQ(ierr);
  ierr = IPDestroy(&svd->ip);CHKERRQ(ierr);
  svd->ip = ip;
  ierr = PetscLogObjectParent((PetscObject)svd,(PetscObject)svd->ip);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDGetIP"
/*@C
   SVDGetIP - Obtain the inner product object associated
   to the singular value solver object.

   Not Collective

   Input Parameters:
.  svd - singular value solver context obtained from SVDCreate()

   Output Parameter:
.  ip - inner product context

   Level: advanced

.seealso: SVDSetIP()
@*/
PetscErrorCode SVDGetIP(SVD svd,IP *ip)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  PetscValidPointer(ip,2);
  if (!svd->ip) {
    ierr = IPCreate(PetscObjectComm((PetscObject)svd),&svd->ip);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)svd,(PetscObject)svd->ip);CHKERRQ(ierr);
  }
  *ip = svd->ip;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDSetDS"
/*@
   SVDSetDS - Associates a direct solver object to the singular value solver.

   Collective on SVD

   Input Parameters:
+  svd - singular value solver context obtained from SVDCreate()
-  ds  - the direct solver object

   Note:
   Use SVDGetDS() to retrieve the direct solver context (for example,
   to free it at the end of the computations).

   Level: advanced

.seealso: SVDGetDS()
@*/
PetscErrorCode SVDSetDS(SVD svd,DS ds)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  PetscValidHeaderSpecific(ds,DS_CLASSID,2);
  PetscCheckSameComm(svd,1,ds,2);
  ierr = PetscObjectReference((PetscObject)ds);CHKERRQ(ierr);
  ierr = DSDestroy(&svd->ds);CHKERRQ(ierr);
  svd->ds = ds;
  ierr = PetscLogObjectParent((PetscObject)svd,(PetscObject)svd->ds);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SVDGetDS"
/*@C
   SVDGetDS - Obtain the direct solver object associated to the singular value
   solver object.

   Not Collective

   Input Parameters:
.  svd - singular value solver context obtained from SVDCreate()

   Output Parameter:
.  ds - direct solver context

   Level: advanced

.seealso: SVDSetDS()
@*/
PetscErrorCode SVDGetDS(SVD svd,DS *ds)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(svd,SVD_CLASSID,1);
  PetscValidPointer(ds,2);
  if (!svd->ds) {
    ierr = DSCreate(PetscObjectComm((PetscObject)svd),&svd->ds);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)svd,(PetscObject)svd->ds);CHKERRQ(ierr);
  }
  *ds = svd->ds;
  PetscFunctionReturn(0);
}

