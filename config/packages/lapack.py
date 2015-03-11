#
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  SLEPc - Scalable Library for Eigenvalue Problem Computations
#  Copyright (c) 2002-2014, Universitat Politecnica de Valencia, Spain
#
#  This file is part of SLEPc.
#
#  SLEPc is free software: you can redistribute it and/or modify it under  the
#  terms of version 3 of the GNU Lesser General Public License as published by
#  the Free Software Foundation.
#
#  SLEPc  is  distributed in the hope that it will be useful, but WITHOUT  ANY
#  WARRANTY;  without even the implied warranty of MERCHANTABILITY or  FITNESS
#  FOR  A  PARTICULAR PURPOSE. See the GNU Lesser General Public  License  for
#  more details.
#
#  You  should have received a copy of the GNU Lesser General  Public  License
#  along with SLEPc. If not, see <http://www.gnu.org/licenses/>.
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#

import petscconf, log, package

class Lapack(package.Package):

  def __init__(self,argdb,log):
    self.packagename  = 'lapack'
    self.downloadable = False
    self.log          = log

  def ShowInfo(self):
    if hasattr(self,'missing'):
      log.Println('LAPACK missing functions:')
      log.Print('  ')
      for i in self.missing: log.Print(i)
      log.Println('')
      log.Println('')
      log.Println('WARNING: Some SLEPc functionality will not be available')
      log.Println('PLEASE reconfigure and recompile PETSc with a full LAPACK implementation')

  def Process(self,conf,vars,cmake,archdir=''):
    self.log.write('='*80)
    self.log.Println('Checking LAPACK library...')
    self.Check(conf,vars,cmake)

  def Check(self,conf,vars,cmake):

    # LAPACK standard functions
    l = ['laev2','gehrd','lanhs','lange','getri','trexc','trevc','geevx','ggevx','gelqf','gesdd','tgexc','tgevc','pbtrf','stedc','hsein','larfg','larf','trsen','tgsen','lacpy','lascl','lansy','laset']

    # LAPACK functions with different real and complex versions
    if petscconf.SCALAR == 'real':
      l += ['orghr','syevr','syevd','sytrd','sygvd','ormlq','orgqr','orgtr']
      if petscconf.PRECISION == 'single':
        prefix = 's'
      elif petscconf.PRECISION == '__float128':
        prefix = 'q'
      else:
        prefix = 'd'
    else:
      l += ['unghr','heevr','heevd','hetrd','hegvd','unmlq','ungqr','ungtr']
      if petscconf.PRECISION == 'single':
        prefix = 'c'
      elif petscconf.PRECISION == '__float128':
        prefix = 'w'
      else:
        prefix = 'z'

    # add prefix to LAPACK names
    functions = []
    for i in l:
      functions.append(prefix + i)

    # in this case, the real name represents both versions
    namesubst = {'unghr':'orghr', 'heevr':'syevr', 'heevd':'syevd', 'hetrd':'sytrd', 'hegvd':'sygvd', 'unmlq':'ormlq', 'ungqr':'orgqr', 'ungtr':'orgtr'}

    # LAPACK functions which are always used in real version
    if petscconf.PRECISION == 'single':
      functions += ['sstevr','sbdsdc','slamch','slag2','slasv2','slartg','slaln2','slaed4','slamrg','slapy2']
    elif petscconf.PRECISION == '__float128':
      functions += ['qstevr','qbdsdc','qlamch','qlag2','qlasv2','qlartg','qlaln2','qlaed4','qlamrg','qlapy2']
    else:
      functions += ['dstevr','dbdsdc','dlamch','dlag2','dlasv2','dlartg','dlaln2','dlaed4','dlamrg','dlapy2']

    # check for all functions at once
    all = []
    for i in functions:
      f =  '#if defined(PETSC_BLASLAPACK_UNDERSCORE)\n'
      f += i + '_\n'
      f += '#elif defined(PETSC_BLASLAPACK_CAPS) || defined(PETSC_BLASLAPACK_STDCALL)\n'
      f += i.upper() + '\n'
      f += '#else\n'
      f += i + '\n'
      f += '#endif\n'
      all.append(f)

    self.log.write('=== Checking all LAPACK functions...')
    if self.Link(all,[],[]):
      return

    # check functions one by one
    self.missing = []
    for i in functions:
      f =  '#if defined(PETSC_BLASLAPACK_UNDERSCORE)\n'
      f += i + '_\n'
      f += '#elif defined(PETSC_BLASLAPACK_CAPS) || defined(PETSC_BLASLAPACK_STDCALL)\n'
      f += i.upper() + '\n'
      f += '#else\n'
      f += i + '\n'
      f += '#endif\n'

      self.log.write('=== Checking LAPACK '+i+' function...')
      if not self.Link([f],[],[]):
        self.missing.append(i)
        # some complex functions are represented by their real names
        if i[1:] in namesubst:
          nf = namesubst[i[1:]]
        else:
          nf = i[1:]
        conf.write('#ifndef SLEPC_MISSING_LAPACK_' + nf.upper() + '\n#define SLEPC_MISSING_LAPACK_' + nf.upper() + ' 1\n#endif\n\n')
        cmake.write('set (SLEPC_MISSING_LAPACK_' + nf.upper() + ' YES)\n')

    if self.missing:
      cmake.write('mark_as_advanced (' + ' '.join([s.upper() for s in self.missing]) + ')\n')

