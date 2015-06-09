
%Module Bach 0

%Import QtCore/QtCoremod.sip
%Import blur/blurqt.sip

%DefaultSupertype sip.wrapper

%Include qlist.sip
%Include auto.sip

void bach_loader();

Schema * bachSchema();
Database * bachDb();

%ModuleHeaderCode
#include "bach.h"
#include "pyembed.h"
%End

%PostInitialisationCode
addSchemaCastNamedModule( bachSchema(), "Bach" );
%End

