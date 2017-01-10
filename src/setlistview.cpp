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

#include "setlistview.h"
#include <QDebug>

RemoveSelectionDelegate::RemoveSelectionDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    //qDebug() << "RemoveSelectionDelegate::RemoveSelectionDelegate(...);";
}

void RemoveSelectionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    QStyleOptionViewItem newOption = option;
    newOption.state = option.state & (~QStyle::State_Selected);
    QStyledItemDelegate::paint(painter,newOption,index);
    //qDebug() << "RemoveSelectionDelegate::paint(...);";
}
