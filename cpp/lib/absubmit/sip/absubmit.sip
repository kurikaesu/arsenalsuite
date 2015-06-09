
%Module blur.absubmit 0

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import blur/classes.sip
%Import blur/blurqt.sip

%DefaultSupertype sip.wrapper

%Include submitter.sip

%ModuleHeaderCode
#include "absubmit.h"
#include "pyembed.h"
%End