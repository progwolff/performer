#ifndef UTIL_H
#define UTIL_H

#include <QTimer>
#include <QFuture>
#include <QtConcurrent>


/**
 * calls a function and cancels execution after timeout milliseconds.
 * @param timeout timeout in ms
 * @param function the function to execute
 */
template<typename T>
void try_run(int timeout, T function, const char* name = "")
{
    QTime timer;
    timer.start();
    QFuture<void> funct = QtConcurrent::run(function);
    while(timer.elapsed() < timeout && funct.isRunning());
    if(funct.isRunning())
    {
        qDebug() << "Canceled execution of function after" << timer.elapsed() << "ms" << name;
        funct.cancel();
    }
}

/**
 * replaces each occurence of a in str with b
 * @param str the haystack to search in
 * @param a the needle to search for
 * @param b the replacement string
 * @return a QByteArray holding the result after replacement
 */
QByteArray replace(const char* str, const char* a, const char* b);

/**
 * replaces each occurence of a in str with b
 * @param str the haystack to search in
 * @param a the needle to search for
 * @param b the replacement string
 * @return a QByteArray holding the result after replacement
 */
QByteArray replace(const char* str, const QString& a, const QString& b);

#endif
