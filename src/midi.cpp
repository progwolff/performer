
#include "midi.h"
#include <QAction>
#include <QToolButton>
#include <QScrollBar>
#include <QComboBox>
#include <QDebug>

QMap<unsigned char, QAction*> MIDI::midi_cc_map = QMap<unsigned char, QAction*>();
QList<QAction*> MIDI::midi_cc_actions = QList<QAction*>();

QAction* MIDI::setLearnable(QWidget* widget, const QString& text, const QString& name, QObject* parent)
{
    QAction *action = new QAction(text, parent);
    action->setObjectName(name);
    midi_cc_actions << action;
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), parent, SLOT(midiContextMenuRequested(const QPoint&)));
    if(widget->inherits("QToolButton"))
    {
        QToolButton *button = static_cast<QToolButton*>(widget);
        action->setIcon(button->icon());
        action->setData("button");
        action->setCheckable(button->isCheckable());
        action->setStatusTip(button->statusTip());
        action->setToolTip(button->toolTip());
        action->setWhatsThis(button->whatsThis());
        button->addAction(action);
        button->setDefaultAction(action);
        connect(action, SIGNAL(triggered()), button, SIGNAL(clicked()));
    }
    else if(widget->inherits("QScrollBar"))
    {
        QScrollBar *scrollBar = static_cast<QScrollBar*>(widget);
        connect(action, &QAction::triggered, parent, [scrollBar,action](){
            scrollBar->setSliderPosition(
                (scrollBar->maximum()-scrollBar->minimum())*action->data().toInt()/128.+scrollBar->minimum()
            );
        });
        scrollBar->addAction(action);
    }
    else if(widget->inherits("QComboBox"))
    {
        QComboBox *box = static_cast<QComboBox*>(widget);
        connect(action, &QAction::triggered, parent, [box,action](){
            qDebug() << "combobox: " << action->data().toInt();
            box->setCurrentIndex((box->count()-1)*action->data().toInt()/128.);
        });
        box->addAction(action);
    }
    else
        qDebug() << "MIDI Actions are not implemented for this type of object." << name << text;
    
    return action;
}

unsigned int MIDI::cc(QAction* action) 
{
    unsigned char cc = 128;
    QMapIterator<unsigned char, QAction*> i(midi_cc_map);
    while (i.hasNext()) {
        i.next();
        if(i.value() == action)
        {
            cc = i.key();
            break;
        }
    }
    return cc;
}

void MIDI::setCc(QAction* action, unsigned int cc)
{
    midi_cc_map[cc] = action;
}

void MIDI::resetCc(QAction* action)
{
    if(!action)
        return;
    
    QMapIterator<unsigned char, QAction*> i(midi_cc_map);
    while (i.hasNext()) {
        i.next();
        if(i.value() == action)
            midi_cc_map[i.key()] = nullptr;
    }
}

QAction* MIDI::action(unsigned int cc)
{
    return midi_cc_map[cc];
}

void MIDI::addAction(QAction* action)
{
    midi_cc_actions << action;
}

const QList<QAction*>& MIDI::actions()
{
    return midi_cc_actions;
}
