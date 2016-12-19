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


#define MIDI_BUTTON_THRESHOLD_UPPER (128/2+2)
#define MIDI_BUTTON_THRESHOLD_LOWER (128/2-2)

#include <QList>
#include <QAction>


class MIDI : public QObject
{
    Q_OBJECT
public: 
    /**
     * Adds midi learn functionality to a QWidget.\n
     * On right click on the widget a midi learn context menu will be shown.\n
     * Once learned, a MIDI CC message will trigger the default action of the widget.
     * @param parent the parent object of the new action. This Object should implement the slot midiContextMenuRequested(const QPoint&).
     * @param widget the widget to add midi learn functionality to
     * @param name a locale independent identifier of the corresponding action
     * @param text a human readable (possibly localized) identifier of the corresponding action
     */
    static QAction* setLearnable(QWidget* widget, const QString& text, const QString& name, QObject* parent = nullptr);
    
    /**
     * Adds a QAction to the list of learnable actions
     * @param action the QAction to add
     */
    static void addAction(QAction* action);
    
    /**
     * Get a list of MIDI learnable actions
     * @return a list of MIDI learnable actions
     */
    static const QList<QAction*>& actions();
    
    /**
     * Get the action assigned to a given MIDI CC
     * @param cc the CC to get the assigned action for
     * @return the action assigned to the given MIDI CC
     */
    static QAction* action(unsigned char cc);
    
    /**
     * Get the corresponding MIDI CC to a given action
     * @param action the action for which a MIDI CC should be returned
     * @return If there is a MIDI CC assigned to the given action, the corresponding CC is returned. If there is no MIDI CC assigned to the given action, a value greater than 127 is returned.
     */
    static unsigned char cc(QAction* action);
    
    /**
     * Get the last known value of a given MIDI CC
     * @param cc the MIDI CC to get the last known value of
     * @return the last known value of the given MIDI CC
     */
    static unsigned char value(unsigned char cc);

public slots:    
    /**
     * Sets the MIDI CC for a given action.
     * @param action the action to assign a MIDI CC to
     * @param cc the CC to assign to the given action
     */
    static void setCc(QAction* action, unsigned char cc);
    
    /**
     * Resets all CCs for a given action.
     * @param action the action to remove all MIDI CCs from
     */
    static void resetCc(QAction* action);
    
    /**
     * Trigger the action that is assigned to MIDI CC cc.
     * @param cc the MIDI CC assigned to the action to trigger.
     * @param value the current value of the given MIDI CC
     */
    static void trigger(unsigned char cc, unsigned char value);
    
    /**
     * Update the value of a given MIDI CC
     * @param cc the MIDI CC to update
     * @param value the current value of the given MIDI CC
     */
    static void setValue(unsigned char cc, unsigned char value);
    
private:
    
    static QMap<unsigned char, QAction*> midi_cc_map;
    static QList<QAction*> midi_cc_actions;
    static QMap<unsigned char, unsigned char> midi_cc_value_map;
};



#endif //PERFORMER_MIDI_H
