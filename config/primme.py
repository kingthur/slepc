#
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  SLEPc - Scalable Library for Eigenvalue Problem Computations
#  Copyright (c) 2002-2010, Universidad Politecnica de Valencia, Spain
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

def Check(conf,cmake,directory,libs):
  
  log.Write('='*80)
  log.Println('Checking PRIMME library...')

  if petscconf.PRECISION == 'single':
    log.Exit('ERROR: PRIMME does not support single precision.')
 
  functions_base = ['primme_set_method','primme_Free','primme_initialize']
  if directory:
    dirs = [directory]
  else:
    dirs = check.GenerateGuesses('Primme')

  include = 'PRIMMESRC/COMMONSRC'
  if not libs:
    libs = ['-lprimme']
  if petscconf.SCALAR == 'real':
    functions = functions_base + ['dprimme']
  else:
    functions = functions_base + ['zprimme']

  for d in dirs:
    if d:
      l = ['-L' + d] + libs
      f = ['-I' + d + '/' + include]
    else:
      l =  libs
      f = []
    if check.Link(functions,[],l+f):
      conf.write('SLEPC_HAVE_PRIMME = -DSLEPC_HAVE_PRIMME\n')
      conf.write('PRIMME_LIB =' + str.join(' ', l) + '\n')
      conf.write('PRIMME_FLAGS =' + str.join(' ', f) + '\n')
      cmake.write('set (SLEPC_HAVE_PRIMME YES)\n')
      cmake.write('find_library (PRIMME_LIB primme HINTS '+ d +')\n')
      return l+f

  log.Println('ERROR: Unable to link with PRIMME library')
  log.Println('ERROR: In directories '+''.join([s+' ' for s in dirs]))
  log.Println('ERROR: With flags '+''.join([s+' ' for s in libs]))
  log.Exit('')
