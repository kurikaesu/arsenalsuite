
import sys,os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
smoke_path = os.path.join(path,'smoke/qt')
perlqt_path = os.path.join(path,'PerlQt')

win = sys.platform == 'win32'

if not win or os.path.exists('c:/perl/bin/perl.exe'):
	# Install the libraries
	#if sys.platform != 'win32':
	#	All_Targets.append(LibInstallTarget("stoneinstall","lib/stone","stone","/usr/lib/"))
	
	# Python module targets
	gen_cmd = './generate.pl'
	if win:
		gen_cmd = 'c:/perl/bin/perl generate.pl'
	smoke_generate = StaticTarget( "smoke_generate", smoke_path, gen_cmd, ["classmaker","classes"] )
	smoke = QMakeTarget( "smoke", smoke_path, "smoke.pro", [smoke_generate] )
	perlqt_lib = QMakeTarget( "perlqt_lib", perlqt_path, "perlqt.pro", [smoke] )
	perlqt = Target( "perlqt", path, [perlqt_lib], [] )

	RPMTarget("perlqtrpm",'blur-perlqt',path,'../../../rpm/spec/perlqt.spec.template','1.0')

if __name__ == "__main__":
	build()
