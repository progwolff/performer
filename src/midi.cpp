
#include "midi.h"
#include <QAction>
#include <QToolButton>
#include <QScrollBar>
#include <QComboBox>
#include <QDebug>

#ifdef WITH_KF5
#include <KLocalizedString>
#else
#include "fallback.h"
#endif

#include <QTimer>

MIDI::MIDI(QObject* parent)
: QAbstractTableModel(parent)
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout , this, [this](){
        emit dataChanged(index(0,0), index(0,0));
    });
    timer->start();
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
    
    addAction(action);
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), parent, SLOT(midiContextMenuRequested(const QPoint&)));
    
    return action;
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
                if(widget->inherits("QToolButton"))
                {
                    ((QToolButton*)widget)->emit clicked();
                    ((QToolButton*)widget)->setDown(true);
                }
        }
        if(dec)
        {
            for(QWidget *widget : action->associatedWidgets())
                if(widget->inherits("QToolButton"))
                    ((QToolButton*)widget)->setDown(false);
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
