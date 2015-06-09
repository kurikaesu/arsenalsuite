from PyQt4.uic import properties
from PyQt4.uic.Compiler import qtproxies

class ProxyProperties(properties.Properties):
    def __init__(self, qobject_creator):
        properties.Properties.__init__(self, qobject_creator,
                                       qtproxies.QtCore, qtproxies.QtGui)
