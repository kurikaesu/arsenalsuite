from PyQt4 import QtCore
from itertools import ifilter

def is_autoconnect_slot((name, attr)):
    return callable(attr) and name.startswith("on_")

def signals(child):
    meta = child.metaObject()
    for idx in xrange(meta.methodOffset(),
                      meta.methodOffset() + meta.methodCount()):
        methodinfo = meta.method(idx)
        if methodinfo.methodType() == QtCore.QMetaMethod.Signal:
            yield methodinfo
    
def connectSlotsByName(ui_impl):
    for name, slot in ifilter(is_autoconnect_slot,
                              ui_impl.__class__.__dict__.iteritems()):
        try:
            # is it safe to assume that there are
            # never underscores in signals?
            idx = name.rindex("_")
            objectname, signalname = name[3:idx], name[idx+1:]
            child = ui_impl.findChild(QtCore.QObject, objectname)
            assert child != None
            for signal in signals(child):
                if signal.signature().startswith(signalname):
                    QtCore.QObject.connect(child,
                                           QtCore.SIGNAL(signal.signature()),
                                           getattr(ui_impl, name))
                    break
        except:
            pass
    
