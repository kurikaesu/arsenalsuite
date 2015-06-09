VPATH = /storage/arsenalsuite/cpp/lib/PyQt4/designer
CONFIG      += designer plugin release warn
TARGET      = pythonplugin
TEMPLATE    = lib

INCLUDEPATH += /usr/include/python2.7
LIBS        += -L/usr/lib -lpython2.7
DEFINES     += PYTHON_LIB=\\\"libpython2.7.so\\\"

SOURCES     = pluginloader.cpp
HEADERS     = pluginloader.h

# Install.
target.path = /usr/lib/x86_64-linux-gnu/qt4/plugins/designer

python.path = /usr/lib/x86_64-linux-gnu/qt4/plugins/designer
python.files = python

INSTALLS    += target python
