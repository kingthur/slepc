#
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  SLEPc - Scalable Library for Eigenvalue Problem Computations
#  Copyright (c) 2002-2011, Universitat Politecnica de Valencia, Spain
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

import os
import sys

import petscconf
import log
import check

def Check(conf,vars,cmake,tmpdir):
  log.write('='*80)
  log.Println('Checking LAPACK library...')

  # LAPACK standard functions
  l = ['laev2','gehrd','lanhs','lange','getri','trexc','trevc','geevx','ggevx','gelqf','gesdd','tgexc','tgevc','geqrf','pbtrf']

  # LAPACK functions with different real and complex versions
  if petscconf.SCALAR == 'real':
    l += ['orghr','syevr','sygvd','ormlq','orgqr','symm']
    if petscconf.PRECISION == 'single':
      prefix = 's'
    if petscconf.PRECISION == '__float128':
      prefix = 'q'
    else:
      prefix = 'd'
  else:
    l += ['unghr','heevr','hegvd','unmlq','ungqr','hemm']
    if petscconf.PRECISION == 'single':
      prefix = 'c'
    if petscconf.PRECISION == '__float128':
      prefix = 'w'
    else:
      prefix = 'z'

  # add prefix to LAPACK names  
  functions = []
  for i in l:
    functions.append(prefix + i)

  # in this case, the real name represents both versions
  namesubst = {'unghr':'orghr', 'heevr':'syevr', 'hegvd':'sygvd', 'unmlq':'ormlq', 'ungqr':'orgqr', 'hemm':'symm'}

  # LAPACK functions which are always used in real version 
  if petscconf.PRECISION == 'single':
    functions += ['sstevr','sbdsdc','ssteqr','sorgtr','ssytrd','slamch','slag2','slasv2']
  elif petscconf.PRECISION == '__float128':
    functions += ['qstevr','qbdsdc','qsteqr','qorgtr','qsytrd','qlamch','qlag2','qlasv2']
  else:
    functions += ['dstevr','dbdsdc','dsteqr','dorgtr','dsytrd','dlamch','dlag2','dlasv2']
   
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

  log.write('=== Checking all LAPACK functions...')
  if check.Link(tmpdir,all,[],[]):
    return []

  # check functions one by one
  missing = []
  for i in functions:
    f =  '#if defined(PETSC_BLASLAPACK_UNDERSCORE)\n'
    f += i + '_\n'
    f += '#elif defined(PETSC_BLASLAPACK_CAPS) || defined(PETSC_BLASLAPACK_STDCALL)\n'
    f += i.upper() + '\n'
    f += '#else\n'
    f += i + '\n'
    f += '#endif\n'
  
    log.write('=== Checking LAPACK '+i+' function...')
    if not check.Link(tmpdir,[f],[],[]):
      missing.append(i)
      # some complex functions are represented by their real names
      if i[1:] in namesubst:
        nf = namesubst[i[1:]]
      else:
        nf = i[1:]
      conf.write('#ifndef SLEPC_MISSING_LAPACK_' + nf.upper() + '\n#define SLEPC_MISSING_LAPACK_' + nf.upper() + ' 1\n#endif\n\n')
      cmake.write('set (SLEPC_MISSING_LAPACK_' + nf.upper() + ' YES)\n')

  if missing:
    cmake.write('mark_as_advanced (' + ''.join([s.upper()+' ' for s in missing]) + ')\n')
  return missing
