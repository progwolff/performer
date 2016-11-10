
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
