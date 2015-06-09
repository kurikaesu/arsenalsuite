
import QVariantTest
from PyQt4.QtCore import QVariant

if QVariantTest.returnVariantList( [None,1,"test",QVariant(1)] ) == [None,1,"test",1]:
	print "QList<QVariant> conversion succeeded"
else:
	print "QList<QVariant> conversion failed"
	


