
#include "midi.h"
#include <QAction>
#include <QToolButton>
#include <QScrollBar>
#include <QComboBox>
#include <QDebug>

QMap<unsigned char, QAction*> MIDI::midi_cc_map = QMap<unsigned char, QAction*>();
QList<QAction*> MIDI::midi_cc_actions = QList<QAction*>();
QMap<unsigned char, unsigned char> MIDI::midi_cc_value_map = QMap<unsigned char, unsigned char>();

QAction* MIDI::setLearnable(QWidget* widget, const QString& text, const QString& name, QObject* parent)
{
    QAction *action = new QAction(text, parent);
    action->setObjectName(name);
    if(widget->inherits("QToolButton"))
    {
        QToolButton *button = static_cast<QToolButton*>(widget);
        action->setIcon(button->icon());
        action->setData("button");
        action->setCheckable(button->isCheckable());
        action->setStatusTip(button->statusTip());
        action->setToolTip(button->toolTip());
        action->setWhatsThis(button->whatsThis());
        button->setDefaultAction(action);
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
    {
        qDebug() << "MIDI Actions are not implemented for this type of object." << name << text;
        delete action;
        action = nullptr;
        return action;
    }
    
    midi_cc_actions << action;
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), parent, SLOT(midiContextMenuRequested(const QPoint&)));
    
    return action;
}

unsigned char MIDI::cc(QAction* action) 
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

void MIDI::setCc(QAction* action, unsigned char cc)
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

QAction* MIDI::action(unsigned char cc)
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

unsigned char MIDI::value(unsigned char cc)
{
    return midi_cc_value_map[cc];
}

void MIDI::setValue(unsigned char cc, unsigned char value)
{
    midi_cc_value_map[cc] = value;
}

void MIDI::trigger(unsigned char cc, unsigned char value)
{
    QAction* action = MIDI::action(cc);
    if(action && action->data().toString() == "button")
    {
        unsigned char olddata2 = MIDI::value(cc);
        if(value < MIDI_BUTTON_THRESHOLD_LOWER || value >= MIDI_BUTTON_THRESHOLD_UPPER)
            setValue(cc, value);
        if(olddata2 < MIDI_BUTTON_THRESHOLD_LOWER && value >= MIDI_BUTTON_THRESHOLD_UPPER)
        {
            for(QWidget *widget : action->associatedWidgets())
                if(widget->inherits("QToolButton"))
                {
                    ((QToolButton*)widget)->emit clicked();
                    ((QToolButton*)widget)->setDown(true);
                }
        }
        if(olddata2 >= MIDI_BUTTON_THRESHOLD_UPPER && value < MIDI_BUTTON_THRESHOLD_LOWER)
        {
            for(QWidget *widget : action->associatedWidgets())
                if(widget->inherits("QToolButton"))
                    ((QToolButton*)widget)->setDown(false);
        }
    }
    else if(action)
    {
        action->setData(value);
        action->trigger();
    }
}
