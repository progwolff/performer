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

#include "util.h"



QByteArray replace(const char* str, const char* a, const char* b)
{
    return replace(str, QString::fromLocal8Bit(a), QString::fromLocal8Bit(b));
}

QByteArray replace(const char* str, const QString& a, const QString& b)
{
    return QString::fromLocal8Bit(str).replace(a, b).toLatin1();
}
