#ifndef PERFORMER_MIDI_H
#define PERFORMER_MIDI_H

/**
 * true if MIDI message with status byte a is a MiDI Control Change message
 */
#define IS_MIDICC(a) ((0xF0 & (a)) == 0xB0)

/**
 * true if MIDI message with status byte a is a MiDI Program Change message
 */
#define IS_MIDIPC(a) ((0xF0 & (a)) == 0xC0)


#include <QAbstractTableModel>
#include <QList>
#include <QAction>
#include <QToolButton>


class MIDI : public QAbstractTableModel
{
    Q_OBJECT
    
protected:
    struct Parameter {
        QAction *action;
        unsigned char low;
        unsigned char high;
        unsigned char value;
        bool autorange;
    };
    
public: 
    enum Columns {
        ActionColumn = 0,
        CcColumn,
        LowColumn,
        HighColumn,
        ValueColumn,
        ColumnsCount
    };
    
    explicit MIDI(QObject *parent=0);
    
    /**
     * Adds midi learn functionality to a QWidget.\n
     * On right click on the widget a midi learn context menu will be shown.\n
     * Once learned, a MIDI CC message will trigger the default action of the widget.
     * @param parent the parent object of the new action.
     * @param widget the widget to add midi learn functionality to
     * @param name a locale independent identifier of the corresponding action
     * @param text a human readable (possibly localized) identifier of the corresponding action
     * @return a new action of parent that is assigned to widget. This action is added to the list of learnable actions and can be triggered with 
     */
    QAction* setLearnable(QWidget* widget, const QString& text, const QString& name, QObject* parent = nullptr);
    
    /**
     * Adds midi learn functionality to a QToolButton using the current defaultAction of this button.\n
     * On right click on the widget a midi learn context menu will be shown.\n
     * Once learned, a MIDI CC message will trigger the default action of the widget.
     * @param widget the widget to add midi learn functionality to
     */
    void setLearnable(QToolButton* widget);
    
    /**
     * Adds a QAction to the list of learnable actions
     * @param action the QAction to add
     */
    void addAction(QAction* action);
    
    /**
     * Returns a list of MIDI learnable actions
     * @return a list of MIDI learnable actions
     */
    
    const QList<QAction*>& actions() const;
    
    /**
     * Get the action assigned to a given MIDI CC
     * @param cc the CC to get the assigned action for
     * @return the action assigned to the given MIDI CC
     */
    QAction* action(unsigned char cc) const;
    
    /**
     * Returns the corresponding MIDI CC to a given action
     * @param action the action for which a MIDI CC should be returned
     * @return If there is a MIDI CC assigned to the given action, the corresponding CC is returned. If there is no MIDI CC assigned to the given action, a value greater than 127 is returned.
     */
    unsigned char cc(QAction* action) const;
    
    /**
     * Returns the last known value of a given MIDI CC
     * @param cc the MIDI CC to get the last known value of
     * @return the last known value of the given MIDI CC
     */
    unsigned char value(unsigned char cc) const;
    
    /**
     * Returns the minimum value in the range of a givven MIDI CC
     * @param cc the MIDI CC to get the minimum value of
     * @return the minimum value of the given CC
     */
    unsigned char min(unsigned char cc) const;
    
    /**
     * Returns the maximum value in the range of a givven MIDI CC
     * @param cc the MIDI CC to get the maximum value of
     * @return the maximum value of the given CC
     */
    unsigned char max(unsigned char cc) const;
    
    int rowCount(const QModelIndex& parent) const override;

    int columnCount(const QModelIndex& parent) const override;
    
    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex & index, const QVariant & value, int role) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    
    static QString description(unsigned char cc);
    
public slots:    
    /**
     * Sets the MIDI CC for a given action.
     * @param action the action to assign a MIDI CC to
     * @param cc the CC to assign to the given action
     */
    void setCc(QAction* action, unsigned char cc);
    
    /**
     * Resets all CCs for a given action.
     * @param action the action to remove all MIDI CCs from
     */
    void resetCc(QAction* action);
    
    /**
     * Updates the value of a given MIDI CC
     * @param cc the MIDI CC to update
     * @param value the current value of the given MIDI CC
     */
    void setValue(unsigned char cc, unsigned char value);
    
    /**
     * Sets the minimum value of the given CC
     * @param cc the MIDI CC to set the maximum value of
     * @param value the new minimum value of the given cc.
     */
    void setMin(unsigned char cc, unsigned char min);
    
    /**
     * Sets the maximum value of the given CC
     * @param cc the MIDI CC to set the maximum value of
     * @param value the new maximum value of the given cc.
     */
    void setMax(unsigned char cc, unsigned char max);
    
    /**
     * Triggers the action that is assigned to MIDI CC cc.
     * @param cc the MIDI CC assigned to the action to trigger.
     * @param value the current value of the given MIDI CC
     */
    void trigger(unsigned char cc, unsigned char value);
    
    /**
     * Let the range of a continuous control be learned from the values it receives
     * @param cc a continuous control
     */
    void autoRange(unsigned char cc);
    
    /**
     * Fix the range of a continuous control to its current min and max values
     * @param cc continuous control
     */
    void fixRange(unsigned char cc);
    
    /**
     * Process a MIDI message. Connect this slot to a MIDI signal source that should be used to learn and trigger MIDI events.
     * @param status status byte of a MIDI message
     * @param data1 first data byte of a MIDI message
     * @param data2 second data byte of a MIDI message
     */
    void message(unsigned char status, unsigned char data1, unsigned char data2);
    
    /**
     * Start learning a MIDI CC for a given action. 
     * @param action the action to learn a MIDI CC for. The next CC received through the message slot will be assigned to this action.
     */
    void learn(QAction* action);
    
signals:
    /**
     * A status message regarding MIDI learn or trigger events
     * @param msg a status message
     */
    void status(const QString& msg);
    
private slots:
    void midiContextMenuRequested(const QPoint& pos);
    
private:
    
    QMap<unsigned char, Parameter> m_params;
    QList<QAction*> m_actions;
    QAction *m_learn;
};



#endif //PERFORMER_MIDI_H
