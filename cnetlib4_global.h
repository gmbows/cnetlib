#ifndef CNETLIB4_GLOBAL_H
#define CNETLIB4_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CNETLIB4_LIBRARY)
#  define CNETLIB4_EXPORT Q_DECL_EXPORT
#else
#  define CNETLIB4_EXPORT Q_DECL_IMPORT
#endif

#endif // CNETLIB4_GLOBAL_H
