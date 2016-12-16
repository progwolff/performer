
#include "util.h"



QByteArray replace(const char* str, const char* a, const char* b)
{
    return replace(str, QString::fromLatin1(a), QString::fromLatin1(b));
}

QByteArray replace(const char* str, const QString& a, const QString& b)
{
    return QString::fromLatin1(str).replace(a, b).toLatin1();
}
