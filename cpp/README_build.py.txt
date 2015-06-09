
cpp repository script build.py demystified



cpp\build.py imports most functionality from python\blur\build.py. 

python\blur\build.py main data structure, All_Targets, is built using 
python's default module import default declaration __init__.py -- for 
any subdirectory of cpp\ that is built with cpp\build.py (most of 'em), 
[subdir]\__init__.py  calls [subdir]\build.py which then adds dependencies 
to All_Targets. (In other words, All_Targets is built by python's 
module initialization mechanism that scans all subdirectories for 
__init__.py. Confused yet?)


python\blur\build.py contains the base class Target and Target 
subclasses StaticTarget, CopyTarget, SigTarget, QMakeTarget, 
NSISTarget, WCRevTarget, RevCopyTarget, LibVersionTarget and 
LibInstallTarget


Example: you run "build.py resin clean"

In short --

- build.py parses thru all subdirs via python's module init, catches 
  __init__.py's which call subdir's build.py's which add targets to 
  All_Targets.
- build.py matches argv specified build Target against All_Targets entry
- build.py calls Target.build(args)
- Target.build runs checks, calls Target.build_deps, which 
  builds a specific Target's pre_deps (which are set at runtime during 
  All_Targets initialization via __init__.py/build.py in target dir)
- Target.build_run (main target qmake/make) is ran
- Target.post_deps (like pre_deps) are ran
- Done


Details via a dry run --


C:\source\blur\cpp>build.py resin clean
blur/build/Target class ctor called on  stonelibversion
blur/build/Target class ctor called on  pystone
blur/build/Target class ctor called on  pystonestatic
blur/build/Target class ctor called on  pystonecopy
blur/build/Target class ctor called on  stone
blur/build/Target class ctor called on  classeslibversion
blur/build/Target class ctor called on  pyclasses
blur/build/Target class ctor called on  pyclassesstatic
blur/build/Target class ctor called on  pyclassescopy
blur/build/Target class ctor called on  classes
blur/build/Target class ctor called on  libs_installer
blur/build/Target class ctor called on  pyqt_installer
blur/build/Target class ctor called on  pyqt
blur/build/Target class ctor called on  qt_installer
blur/build/Target class ctor called on  sip
blur/build/Target class ctor called on  sipstatic
blur/build/Target class ctor called on  stonegui
blur/build/Target class ctor called on  stoneguistatic
blur/build/Target class ctor called on  libfreezer
blur/build/Target class ctor called on  classesui
blur/build/Target class ctor called on  classesuistatic
blur/build/Target class ctor called on  assburnersvnrev
blur/build/Target class ctor called on  assburnersvnrevnsi
blur/build/Target class ctor called on  assburner_installer
blur/build/Target class ctor called on  pyassburner
blur/build/Target class ctor called on  abgui
blur/build/Target class ctor called on  abslave
blur/build/Target class ctor called on  abpsmon
blur/build/Target class ctor called on  assburner
blur/build/Target class ctor called on  assburner_1_3svnrev
blur/build/Target class ctor called on  assburner_1_3svnrevnsi
blur/build/Target class ctor called on  assburner_1_3installer
blur/build/Target class ctor called on  pyassburner_1_3
blur/build/Target class ctor called on  assburner
blur/build/Target class ctor called on  abpsmon
blur/build/Target class ctor called on  assburner_1_3
blur/build/Target class ctor called on  stoneclasseslibcopy
blur/build/Target class ctor called on  stoneguiclasseslibcopy
blur/build/Target class ctor called on  runclassmaker
blur/build/Target class ctor called on  classmaker
blur/build/Target class ctor called on  resinaxsvnrev
blur/build/Target class ctor called on  resinaxsvnrevnsi
blur/build/Target class ctor called on  resinaxsvnrevpri
blur/build/Target class ctor called on  resinax_installer
blur/build/Target class ctor called on  resinax
blur/build/Target class ctor called on  freezersvnrev
blur/build/Target class ctor called on  freezersvnrevnsi
blur/build/Target class ctor called on  assfreezer_installer
blur/build/Target class ctor called on  assfreezer
blur/build/Target class ctor called on  resinsvnrev
blur/build/Target class ctor called on  resinsvnrevnsi
blur/build/Target class ctor called on  resin_installer
blur/build/Target class ctor called on  resinuic
blur/build/Target class ctor called on  sipresin
blur/build/Target class ctor called on  resin
blur/build/Target class ctor called on  absubmitsvnrevnsi
blur/build/Target class ctor called on  absubmitsvnrevtxt
blur/build/Target class ctor called on  absubmit_installer
blur/build/Target class ctor called on  absubmit
__init__.py construction done
argv =  ['C:\\source\\blur\\cpp\\build.py', 'resin', 'clean']
Args:  ['C:\\source\\blur\\cpp\\build.py', 'clean']
Targets:  [resin]
Entering Target.build(args) on target  resin
Target pre_deps are:  ['sipstatic', 'pystonestatic', 'classes', 'pyclassesstatic
', 'libassfreezer', 'classesuistatic', 'stoneguistatic', resinsvnrev, sipresin]
Target post_deps are:  [resin_installer]

 [build process starts here]

Target.build: doing  sipstatic
Target.build: chdir to  C:\source\blur\cpp\lib/sip
SipTarget.build_run:   configure.py -k -p win32-g++
SipTarget.build_run:  make clean
SipTarget.build_run:  make
Target.build: doing  pystonestatic
Target.build: chdir to  C:\source\blur\cpp\lib/stone
SipTarget.build_run:   configure.py -k
SipTarget.build_run:  make clean
SipTarget.build_run:  make
Target.build: doing  pystonecopy
Target.build: chdir to  C:\source\blur\cpp\lib/stone/sipStone
Target.build: doing  stone
Target.build: chdir to  C:\source\blur\cpp\lib/stone
QMakeTarget.build_run:   qmake CONFIG-=debug stone.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  stonelibversion
Target.build: chdir to  C:\source\blur\cpp\lib/stone
Target.build: doing  stonegui
Target.build: chdir to  C:\source\blur\cpp\lib/stonegui
QMakeTarget.build_run:   qmake CONFIG-=debug stonegui.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  stoneclasseslibcopy
Target.build: chdir to  C:\source\blur\cpp\apps/classmaker
Target.build: doing  stoneguiclasseslibcopy
Target.build: chdir to  C:\source\blur\cpp\apps/classmaker
Target.build: doing  classmaker
Target.build: chdir to  C:\source\blur\cpp\apps/classmaker
QMakeTarget.build_run:   qmake CONFIG-=debug classmaker.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  runclassmaker
Target.build: chdir to  C:\source\blur\cpp\apps/classmaker
Target.build_run:   classmaker -s ../../lib/classes/schema.xml -o ../../lib/classes/
Target.build: doing  classes
Target.build: chdir to  C:\source\blur\cpp\lib/classes
QMakeTarget.build_run:   qmake CONFIG-=debug classes.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  classeslibversion
Target.build: chdir to  C:\source\blur\cpp\lib/classes
Target.build: doing  pyclassesstatic
Target.build: chdir to  C:\source\blur\cpp\lib/classes
SipTarget.build_run:   configure.py -k
SipTarget.build_run:  make clean
SipTarget.build_run:  make
Target.build: doing  pyclassescopy
Target.build: chdir to  C:\source\blur\cpp\lib/classes/sipClasses
Target.build: doing  classesui
Target.build: chdir to  C:\source\blur\cpp\lib/classesui
QMakeTarget.build_run:   qmake CONFIG-=debug classesui.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  libassfreezer
Target.build: chdir to  C:\source\blur\cpp\lib/assfreezer
QMakeTarget.build_run:   qmake CONFIG-=debug libassfreezer.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  classesuistatic
Target.build: chdir to  C:\source\blur\cpp\lib/classesui
QMakeTarget.build_run:   qmake CONFIG+=staticlib CONFIG-=debug classesui.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  stoneguistatic
Target.build: chdir to  C:\source\blur\cpp\lib/stonegui
QMakeTarget.build_run:   qmake CONFIG+=staticlib CONFIG-=debug stonegui.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  resinsvnrev
Target.build: chdir to  C:\source\blur\cpp\apps/resin
WCRevTarget.build_run:  os.spawnl( subwcrev.exe ) on  core/svnrev-template.h
Target.build: doing  resinuic
Target.build: chdir to  C:\source\blur\cpp\apps/resin
QMakeTarget.build_run:   qmake CONFIG-=debug resin.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make -f Makefile.release compiler_uic_make_all
Target.build: doing  sipresin
Target.build: chdir to  C:\source\blur\cpp\apps/resin
SipTarget.build_run:   configure.py -k
SipTarget.build_run:  make clean
SipTarget.build_run:  make
Target.build: doing  resin
Target.build: chdir to  C:\source\blur\cpp\apps/resin
QMakeTarget.build_run:   qmake CONFIG-=debug resin.pro
QMakeTarget.build_run:  make clean
QMakeTarget.build_run:  make
Target.build: doing  resinsvnrevnsi
Target.build: chdir to  C:\source\blur\cpp\apps/resin
WCRevTarget.build_run:  os.spawnl( subwcrev.exe ) on  resin-svnrev-template.nsi
Target.build: doing  resin_installer
Target.build: chdir to  C:\source\blur\cpp\apps/resin
NSISTarget.build_run:  os.spawnl( makensis.exe ) on  resin.nsi

C:\source\blur\cpp>
