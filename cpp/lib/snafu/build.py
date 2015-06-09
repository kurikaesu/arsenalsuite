
import os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))

QMakeTarget("libsnafu",path,"libsnafu.pro",["stone","stonegui","classesui","libabsubmit"])
QMakeTarget("libsnafustatic",path,"libsnafu.pro",["stonestatic","stoneguistatic","classesuistatic","libabsubmit"],[],True)

if __name__ == "__main__":
	build()
