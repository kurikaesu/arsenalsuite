__init__ ReadMe

This File:  __init__.py  Should go Here:
[PythonInstallDir]\Lib\site-packages\win32com\client

This is the Default __init__ that installs with Python but with some Changes 
to get dispatches to always return properly in XSI

The changes made from the default are:
	inside the function: __WrapDispatch
	Comment all lines except the last one that begins with "return dynamic.Dispatch( ... "


This __init__.py file is from Python 2.41