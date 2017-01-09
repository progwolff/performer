
#include "util.h"



QByteArray replace(const char* str, const char* a, const char* b)
{
    return replace(str, QString::fromLocal8Bit(a), QString::fromLocal8Bit(b));
}

QByteArray replace(const char* str, const QString& a, const QString& b)
{
    return QString::fromLocal8Bit(str).replace(a, b).toLatin1();
}
