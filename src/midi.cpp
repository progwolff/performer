
#include "midi.h"
#include <QAction>
#include <QToolButton>
#include <QScrollBar>
#include <QComboBox>
#include <QDebug>
#include <QMenu>
#include <QApplication>
#include <QGraphicsEffect>

#ifdef WITH_KF5
#include <KLocalizedString>
#else
#include "fallback.h"
#endif

MIDI::MIDI(QObject* parent)
: QAbstractTableModel(parent)
,m_learn(nullptr)
{
}

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
    
    addAction(action);
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
    return action;
}

void MIDI::setLearnable(QToolButton* button)
{
    if(!button->defaultAction())
        return;
    
    button->defaultAction()->setData("button");
    
    addAction(button->defaultAction());
    
    button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(button, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
}

unsigned char MIDI::cc(QAction* action) const
{
    unsigned char cc = 128;
    QMapIterator<unsigned char, Parameter> i(m_params);
    while (i.hasNext()) {
        i.next();
        if(i.value().action == action)
        {
            cc = i.key();
            break;
        }
    }
    return cc;
}

void MIDI::setCc(QAction* action, unsigned char cc)
{
    qDebug() << "set cc of" << action << "with index" << m_actions.indexOf(action) << "to" << cc;
    m_params[MIDI::cc(action)].action = nullptr;
    m_params[cc].action = action;
    if(action->data() == "button")
    {
        m_params[cc].low = 0;
        m_params[cc].high = 127;
    }
    else
    {
        m_params[cc].autorange = true;
        m_params[cc].low = 127;
        m_params[cc].high = 0;
    }
    emit dataChanged(index(m_actions.indexOf(action), 0), index(m_actions.indexOf(action), ColumnsCount-1));
}

void MIDI::resetCc(QAction* action)
{
    if(!action)
        return;
    
    QMapIterator<unsigned char, Parameter> i(m_params);
    while (i.hasNext()) {
        i.next();
        if(i.value().action == action)
            m_params[i.key()].action = nullptr;
    }
    emit dataChanged(index(m_actions.indexOf(action), 0), index(m_actions.indexOf(action), ColumnsCount-1));
}

QAction* MIDI::action(unsigned char cc) const
{
    return m_params[cc].action;
}

void MIDI::addAction(QAction* action)
{
    m_actions << action;
    
    beginInsertRows(QModelIndex(), m_actions.indexOf(action), m_actions.indexOf(action));
    endInsertRows();
    
    emit dataChanged(index(m_actions.indexOf(action), 0), index(m_actions.indexOf(action), ColumnsCount-1));
    
    qDebug() << "index of" << action << "is" << m_actions.indexOf(action);
}

const QList<QAction*>& MIDI::actions() const
{
    return m_actions;
}

unsigned char MIDI::value(unsigned char cc) const
{
    return m_params[cc].value;
}

void MIDI::setValue(unsigned char cc, unsigned char value)
{
    m_params[cc].value = value;
    emit dataChanged(index(m_actions.indexOf(action(cc)), ValueColumn), index(m_actions.indexOf(action(cc)), ValueColumn));
}

unsigned char MIDI::max(unsigned char cc) const
{
    return m_params[cc].high;
}

void MIDI::setMax(unsigned char cc, unsigned char max)
{
    m_params[cc].high = max;
    emit dataChanged(index(m_actions.indexOf(action(cc)), HighColumn), index(m_actions.indexOf(action(cc)), HighColumn));
}

unsigned char MIDI::min(unsigned char cc) const
{
    return m_params[cc].low;
}

void MIDI::setMin(unsigned char cc, unsigned char min)
{
    m_params[cc].low = min;
    emit dataChanged(index(m_actions.indexOf(action(cc)), LowColumn), index(m_actions.indexOf(action(cc)), LowColumn));
}

void MIDI::autoRange(unsigned char cc)
{
    m_params[cc].autorange = true;
}

void MIDI::fixRange(unsigned char cc)
{
    m_params[cc].autorange = false;
}

void MIDI::trigger(unsigned char cc, unsigned char val)
{
    QAction* action = MIDI::action(cc);
    
    int range = max(cc) - min(cc);
    
    int value = val;
    
    if(action && action->data().toString() == "button")
    {
        int low_thresh, high_thresh;
        bool outofrange, inc, dec;
        
        unsigned char olddata2 = MIDI::value(cc);
        
        if(range > 0)
        {
            low_thresh = min(cc) + (1+range)/2 - ((range >= 4)?2:0);
            high_thresh = min(cc) + (1+range)/2 + ((range >= 4)?2:0);
            outofrange = value < low_thresh || value >= high_thresh;
            inc = olddata2 < low_thresh && value >= high_thresh;
            dec = olddata2 >= high_thresh && value < low_thresh;
        }
        else
        {
            range = -range;
            low_thresh = max(cc) + (1+range)/2 - ((range >= 4)?2:0);
            high_thresh = max(cc) + (1+range)/2 + ((range >= 4)?2:0);
            outofrange = value >= low_thresh || value < high_thresh;
            inc = olddata2 >= low_thresh && value < high_thresh;
            dec = olddata2 < high_thresh && value >= low_thresh;
        }
        
        if(outofrange)
            setValue(cc, value);
        if(inc)
        {
            for(QWidget *widget : action->associatedWidgets())
            {
                if(widget->inherits("QToolButton"))
                {
                    if(static_cast<QToolButton*>(widget)->isCheckable())
                        static_cast<QToolButton*>(widget)->toggle();
                    else
                    {
                        static_cast<QToolButton*>(widget)->emit clicked();
                        static_cast<QToolButton*>(widget)->setDown(true);
                    }
                }
            }
        }
        if(dec)
        {
            for(QWidget *widget : action->associatedWidgets())
            {
                if(widget->inherits("QToolButton") && !static_cast<QToolButton*>(widget)->isCheckable())
                    static_cast<QToolButton*>(widget)->setDown(false);
            }
        }
    }
    else if(action)
    {
        if(value > max(cc) && m_params[cc].autorange)
            setMax(cc, value);
        if(value < min(cc) && m_params[cc].autorange)
            setMin(cc, value);
        setValue(cc, value);
        range = max(cc) - min(cc);
        if(range != 0)
        {
            action->setData((value-min(cc)) * 127/range);
            action->trigger();
        }
    }
}

int MIDI::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return ColumnsCount;
}

int MIDI::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_actions.size();
}

QVariant MIDI::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();
    
    QAction *action = m_actions[index.row()];
    unsigned char cc = this->cc(action);

    switch(role) {
        case Qt::DisplayRole:
            
            switch(index.column())
            {
                case CcColumn:
                    if(cc < 128)
                        return QString::number(cc);
                    else
                        return "";
                    break;
                case ValueColumn:
                    return QString::number(value(cc));
                    break;
                case ActionColumn:
                    return action->text().replace("&","");
                    break;
                case LowColumn:
                    return QString::number(min(cc));
                    break;
                case HighColumn:
                    return QString::number(max(cc));
                    break;
            }  
            
        case Qt::ToolTipRole:
            
            switch(index.column())
            {
                case CcColumn:
                    return description(cc);
                case ActionColumn:
                    return action->toolTip();
                default:
                    return QVariant();
            }
                
        default:
            return QVariant();
    }
}

bool MIDI::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid())
        return false;
    
    if (role == Qt::EditRole)
    {
        switch (index.column())
        {
            case ActionColumn:
            case ValueColumn:
                break;
            case CcColumn:
                if(QString::number(value.toInt()) != value.toString()) return false;
                if(value.toInt() < 0 || value.toInt() > 127) return false;
                setCc(m_actions[index.row()], value.toInt());
                break;
            case LowColumn:
                if(QString::number(value.toInt()) != value.toString()) return false;
                if(value.toInt() < 0 || value.toInt() > 127) return false;
                setMin(cc(m_actions[index.row()]), value.toInt());
                m_params[cc(m_actions[index.row()])].autorange = false;
                break;
            case HighColumn:
                if(QString::number(value.toInt()) != value.toString()) return false;
                if(value.toInt() < 0 || value.toInt() > 127) return false;
                setMax(cc(m_actions[index.row()]), value.toInt());
                m_params[cc(m_actions[index.row()])].autorange = false;
                break;
        }
    }
    return true;
}

QVariant MIDI::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case ActionColumn:
                return i18n("Action");
            case ValueColumn:
                return i18n("Value");
            case LowColumn:
                return i18n("Min");
            case HighColumn:
                return i18n("Max");
            case CcColumn:
                return i18n("CC");
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags MIDI::flags(const QModelIndex& index) const
{
    if(!index.isValid())
        return 0;
    
    if(index.column() != CcColumn && m_params[cc(m_actions[index.row()])].action == nullptr)
        return 0;
    
    switch (index.column())
    {
        case ActionColumn:
        case ValueColumn:
            return Qt::ItemIsEnabled;
        case CcColumn:
        case LowColumn:
        case HighColumn:
            return Qt::ItemIsEditable | Qt::ItemIsEnabled;
    }
    return 0;
}

void MIDI::midiContextMenuRequested(const QPoint& pos)
{
    if(QObject::sender()->inherits("QToolButton") || QObject::sender()->inherits("QScrollBar") || QObject::sender()->inherits("QComboBox"))
    {
        QWidget *sender = (QWidget*)QObject::sender();
        if(sender)
        {
            QPoint globalPos = sender->mapToGlobal(pos);
            QMenu myMenu;
            QAction *action;
            
            unsigned int cc = this->cc(sender->actions()[0]);
            
            if(cc <= 127)
            {
                action = myMenu.addAction(QIcon::fromTheme("tag-assigned", QApplication::style()->standardIcon(QStyle::SP_CommandLink)), i18n("CC %1 Assigned", cc), this, [](){});
                action->setEnabled(false);
                myMenu.addSeparator();
            }
            action = myMenu.addAction(QIcon::fromTheme("configure-shortcuts", QApplication::style()->standardIcon(QStyle::SP_CommandLink)), i18n("Learn MIDI CC"), this, [sender,this](){
                resetCc(sender->actions()[0]); 
                learn(sender->actions()[0]);
                emit status(i18n("Learning MIDI CC for action %1", sender->actions()[0]->text()));
            });
            action = myMenu.addAction(QIcon::fromTheme("remove", QApplication::style()->standardIcon(QStyle::SP_TrashIcon)), i18n("Clear MIDI CC"), this, [sender,this](){
                resetCc(sender->actions()[0]);
                emit status(i18n("MIDI CC cleared for action %1", sender->actions()[0]->text()));
            });
            if(cc > 127) 
                action->setEnabled(false);
            
            // Show context menu at handling position
            myMenu.exec(globalPos);
        }
    }
    else
        qDebug() << "MIDI context menu not implemented for this type of object.";
}

void MIDI::learn(QAction* action)
{
    if(!action)
        return;
    
    visualizeMidiLearn(action);
    
    m_learn = action;
}

void MIDI::visualizeMidiLearn(QAction *action)
{
    if(m_learn)
    for(QWidget* widget : m_learn->associatedWidgets())
    {
        widget->setGraphicsEffect(nullptr);
    }
    
    if(action)
    for(QWidget* widget : action->associatedWidgets())
    {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
        effect->setColor(QApplication::palette().color(QPalette::Highlight));
        widget->setGraphicsEffect(effect);
    }
}

void MIDI::message(unsigned char status, unsigned char data1, unsigned char data2)
{
    
    if(IS_MIDICC(status))
    {
        
        //qDebug() << "received MIDI event" << QString::number(status) << QString::number(data1) << QString::number(data2);
        if(m_learn)
        {
            setCc(m_learn, data1);
            emit this->status(i18n("MIDI CC %1 (%3) assigned to action %2", QString::number(data1), m_learn->text(), description(data1)));
            visualizeMidiLearn();
            m_learn = nullptr;
            setValue(data1, data2);
        }
        else
        {
            trigger(data1, data2);
        }
    }
}

QString MIDI::description(unsigned char cc)
{
    switch(cc) {
        case 0:
            return i18n("Bank select MSB");
        case 1:
            return i18n("Modulation MSB");
        case 2:
            return i18n("Breath Controller");
        case 3:
            return i18n("Undefined");
        case 4:
            return i18n("Foot Controller MSB");
        case 5:
            return i18n("Portamento Time MSB");
        case 6:
            return i18n("Data Byte");
        case 7:
            return i18n("Main volume");
        case 8:
            return i18n("Balance");
        case 9:
            return i18n("Undefined");
        case 10:
            return i18n("Panorama");
        case 11:
            return i18n("Expression");
        case 12:
            return i18n("Effect Control 1");
        case 13:
            return i18n("Effect Control 2");
        case 14:
        case 15:
            return i18n("Undefined");
        case 16:
            return i18n("General Purpose Controller 1");
        case 17:
            return i18n("General Purpose Controller 2");
        case 18:
            return i18n("General Purpose Controller 3");
        case 19:
            return i18n("General Purpose Controller 4");
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25: 
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
            return i18n("Undefined");
        case 32:
            return i18n("Bank select LSB");
        case 33:
            return i18n("LSB for CC 1");
        case 34:
            return i18n("LSB for CC 2");
        case 35:
            return i18n("LSB for CC 3");
        case 36:
            return i18n("LSB for CC 4");
        case 37:
            return i18n("LSB for CC 5");
        case 38:
            return i18n("LSB for CC 6");
        case 39:
            return i18n("LSB for CC 7");
        case 40:
            return i18n("LSB for CC 8");
        case 41:
            return i18n("LSB for CC 9");
        case 42:
            return i18n("LSB for CC 10");
        case 43:
            return i18n("LSB for CC 11");
        case 44:
            return i18n("LSB for CC 12");
        case 45:
            return i18n("LSB for CC 13");
        case 46:
            return i18n("LSB for CC 14");
        case 47:
            return i18n("LSB for CC 15");
        case 48:
            return i18n("LSB for CC 16");
        case 49:
            return i18n("LSB for CC 17");
        case 50:
            return i18n("LSB for CC 18");
        case 51:
            return i18n("LSB for CC 19");
        case 52:
            return i18n("LSB for CC 20");
        case 53:
            return i18n("LSB for CC 21");
        case 54:
            return i18n("LSB for CC 22");
        case 55:
            return i18n("LSB for CC 23");
        case 56:
            return i18n("LSB for CC 24");
        case 57:
            return i18n("LSB for CC 25");
        case 58:
            return i18n("LSB for CC 26");
        case 59:
            return i18n("LSB for CC 27");
        case 60:
            return i18n("LSB for CC 28");
        case 61:
            return i18n("LSB for CC 29");
        case 62:
            return i18n("LSB for CC 30");
        case 63:
            return i18n("LSB for CC 31");
        case 64:
            return i18n("Hold 1");
        case 65:
            return i18n("Portamento");
        case 66:
            return i18n("Sostenuto");
        case 67:
            return i18n("Soft Pedal");
        case 68:
            return i18n("Legato Footswitch");
        case 69:
            return i18n("Hold 2");
        case 70:
            return i18n("Sound Variation");
        case 71:
            return i18n("Harmonic Content");
        case 72:
            return i18n("Release Time");
        case 73:
            return i18n("Attack Time");
        case 74:
            return i18n("Brightness");
        case 75:
            return i18n("Sound Controller 6");
        case 76:
            return i18n("Sound Controller 7");
        case 77:
            return i18n("Sound Controller 8");
        case 78:
            return i18n("Sound Controller 9");
        case 79:
            return i18n("Sound Controller 10");
        case 80:
        case 81:
        case 82:
        case 83:
            return i18n("General Purpose Controller");
        case 84:
            return i18n("Portamento Control");
        case 85:
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
            return i18n("Undefined");
        case 91:
            return i18n("Effects 1 Depth");
        case 92:
            return i18n("Effects 2 Depth");
        case 93:
            return i18n("Effects 3 Depth");
        case 94:
            return i18n("Effects 4 Depth");
        case 95:
            return i18n("Effects 5 Depth");
        case 96:
            return i18n("Data Increment RPN/NRPN");
        case 97:
            return i18n("Data Decrement RPN/NRPN");
        case 98:
            return i18n("NRPN LSB");
        case 99:
            return i18n("NRPN MSB");
        case 100:
            return i18n("RPN LSB");
        case 101:
            return i18n("RPN MSB");
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 116:
        case 117:
        case 118:
        case 119:
            return i18n("Undefined");
        case 120:
            return i18n("all sounds off");
        case 121:
            return i18n("Controller Reset");
        case 122:
            return i18n("Local Control on/off");
        case 123:
            return i18n("all notes off");
        case 124:
            return i18n("omni off");
        case 125:
            return i18n("omni on");
        case 126:
            return i18n("mono on / poly off");
        case 127:
            return i18n("poly on / mono off");
        default:
            return "";
    }
}

