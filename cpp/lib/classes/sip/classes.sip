
%Module blur.Classes 0

%Import QtCore/QtCoremod.sip
%Import blur/blurqt.sip

%DefaultSupertype sip.wrapper

%Include qlist.sip
%Include auto.sip

void classes_loader();

Schema * classesSchema();
Database * classesDb();

%ModuleHeaderCode
#include "classes.h"
#include "pyembed.h"
%End

%PostInitialisationCode
addSchemaCastNamedModule( classesSchema(), "blur.Classes" );
%End
