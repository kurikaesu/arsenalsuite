
#ifndef AB_SUBMIT_H
#define AB_SUBMIT_H

#include <qobject.h>

#ifdef AB_SUBMIT_MAKE_DLL
#define AB_SUBMIT_EXPORT Q_DECL_EXPORT
#else
#define AB_SUBMIT_EXPORT Q_DECL_IMPORT
#endif

#endif // AB_SUBMIT_H
