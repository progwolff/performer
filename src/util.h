/*
 *    Copyright 2016-2017 by Julian Wolff <wolff@julianwolff.de>
 * 
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 *   
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *   
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTIL_H
#define UTIL_H

#include <QTimer>
#include <QFuture>
#include <QtConcurrent>

#if defined(_MSC_VER)
    //  Microsoft 
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    //  do nothing and hope for the best?
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

/**
 * calls a function and cancels execution after timeout milliseconds.
 * @param timeout timeout in ms
 * @param function the function to execute
 * @return true if function returned before timeout, false if function was cancelled after timeout
 */
template<typename T>
bool try_run(int timeout, T function, const char* name = "")
{
    QTime timer;
    timer.start();
    QFuture<void> funct = QtConcurrent::run(function);
    while(timer.elapsed() < timeout && funct.isRunning()) QThread::msleep(10);
    if(funct.isRunning())
    {
        qWarning() << "Canceled execution of function after" << timer.elapsed() << "ms" << name;
        funct.cancel();
        return false;
    }
    return true;
}

/**
 * calls a function and measures it's execution time.
 * @param timeout timeout in ms
 * @param function the function to execute
 * @return true if function returned before timeout, false if function returned after timeout
 */
template<typename T>
bool measure_run(int timeout, T function, const char* name = "")
{
    QTime timer;
    timer.start();
    function();
    if(timer.elapsed() > timeout)
    {
        qWarning() << "Execution of function took " << timer.elapsed() << "ms" << name;
        return false;
    }
    return true;
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
