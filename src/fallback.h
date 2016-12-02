#ifndef FALLBACK_H
#define FALLBACK_H
#ifndef WITH_KF5

#include <QObject>
inline QString i18n (const char *text)
{
    return QObject::tr(text);
}

#endif
#endif
