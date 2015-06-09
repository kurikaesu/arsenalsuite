#!/usr/bin/python
# $Author$
# $LastChangedDate: 2007-11-27 09:20:51 +1100 (Tue, 27 Nov 2007) $
# $Rev: 5270 $
# $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/build.py $

import os
import sys
import string

import lib
import apps
import docs

if __name__ == "__main__":
	sys.path.insert(0,os.path.join(os.path.split(os.getcwd())[0],"python"))

from blur.build import *

if __name__ == "__main__":
	build()

## \mainpage
## \brief Arsenal is a toolkit to help run a network of computers for the purpose of generating 3D visual effects. It includes components for distributed rendering, host management, data management, shot tracking, scheduling and artist collaboration.
## \details
## Arsenal is an open source effort to build a common toolkit for 3d studios. Most studios require the same core components to run their computer infrastructure. By having a common toolkit studios can focus their energies on where it matters: production. The open nature of Arsenal allows integration into any pipeline. Built on top of the Qt library, there is equal access from C++, Python and Perl to the entire codebase.
## 
## Please refer to the <a href="http://code.google.com/p/arsenalsuite/w/list">Wiki</a> for user docs, and the @ref modules page for a list of component docs.
## 
## \author Matt Newell (newellm@blur.com)
## \author Duane Powell (duane@blur.com)
## \author Barry Robison (barry.robison@gmail.com)
## \author Abe Shelton
## 

