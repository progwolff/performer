#ifndef FALLBACK_H
#define FALLBACK_H
#ifndef WITH_KF5

#include <QObject>
inline QString i18n (const char *text)
{
    return QObject::tr(text);
}

template<typename T>
inline QString i18n (const char *text, T param)
{
    return QObject::tr(text).arg(param);
}

template<typename T,typename S>
inline QString i18n (const char *text, T param, S param2)
{
    return QObject::tr(text).arg(param).arg(param2);
}

template<typename T,typename S,typename Q>
inline QString i18n (const char *text, T param, S param2, Q param3)
{
    return QObject::tr(text).arg(param).arg(param2).arg(param3);
}
#endif
#endif
