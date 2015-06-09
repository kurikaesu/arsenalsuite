
from PyQt4.QtGui import *
import sys

app = QApplication([])
QApplication.addLibraryPath("c:/blur/common/")

imgTestPath ='/mnt/morlock006/assfreezer_Display/E_Key_0443.exr'
if sys.platform=='win32':
	imgTestPath='c:/assfreezer_Display/E_Key_0443.exr'
	
def readWithBlackBG( format = '' ):
	imgReader = QImageReader(imgTestPath,format)
	img = imgReader.read()
	imgRet = QImage(img.width(),img.height(),QImage.Format_ARGB32)
	p = QPainter(imgRet)
	p.fillRect(0,0,imgRet.width(),imgRet.height(),QColor(0,0,0))
	p.drawImage(0,0,img)
	p.end()
	return QPixmap.fromImage(imgRet)
	
	
lbl = QLabel(None)
lbl.setWindowTitle( 'EXR gamma adjusted' )
lbl.setPixmap( readWithBlackBG() )
lbl.show()

lbl2 = QLabel(None)
lbl2.setWindowTitle( 'EXR not gamma adjusted' )
lbl2.setPixmap( readWithBlackBG('exr_nogamma') )
lbl2.show()

app.exec_()
