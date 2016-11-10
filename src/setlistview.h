#ifndef SETLISTVIEW_H
#define SETLISTVIEW_H

#include <QtWidgets>

class RemoveSelectionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit RemoveSelectionDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
};

#endif //SETLISTVIEW_H
