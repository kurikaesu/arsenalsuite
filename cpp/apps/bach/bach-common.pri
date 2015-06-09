
SOURCES += \
	view/bachmainwindow.cpp \
	view/bachassetlistview.cpp \
	view/bachassettree.cpp \
	view/bachitems.cpp \
	view/BachCollectionView.cpp \
	view/BachFolderBrowserTreeView.cpp \
	view/BachKeywordView.cpp \
	view/BachSearchBox.cpp \
	view/BachDirModel.cpp \
	view/BachDisplayOptions.cpp \
	bach.cpp \
	core/bachthumbnailloader.cpp \
	core/main.cpp

HEADERS += \
	view/bachmainwindow.h \
	view/bachassetlistview.h \
	view/bachassettree.h \
	view/bachitems.h \
	view/BachCollectionView.h \
	view/BachFolderBrowserTreeView.h \
	view/BachKeywordView.h \
	view/BachSearchBox.h \
	view/BachDirModel.h \
	view/BachDisplayOptions.h \
	view/DragDropHelper.h \
	core/bachthumbnailloader.h \
	bach.h

FORMS = \
	ui/bachmainwindow.ui \
	ui/bachdisplayoptions.ui \
	ui/BachSearchBox.ui

unix:!macx{
    INCLUDEPATH+=base core view ui autocore .
    INCLUDEPATH+=/drd/software/ext/stone/lin64/9100/include
	LIBS+=-L/drd/software/ext/stone/lin64/9100/ -lstonegui -lstone
	LIBS+=-L./.out
}

macx{
    INCLUDEPATH+=base core view ui autocore .
    INCLUDEPATH+=/drd/software/ext/stone/osx/9100/include
	LIBS+=-L/drd/software/ext/stone/osx/9100/ -lstonegui -lstone
	LIBS+=-L./.out
}

