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

template<typename T,typename S,typename Q, typename R>
inline QString i18n (const char *text, T param, S param2, Q param3, R param4)
{
    return QObject::tr(text).arg(param).arg(param2).arg(param3).arg(param4);
}
#endif
#endif
