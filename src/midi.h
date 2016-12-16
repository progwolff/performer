#ifndef PERFORMER_MIDI_H
#define PERFORMER_MIDI_H


#define IS_MIDICC(a) ((0xB0 & (a)) == 0xB0)


#define MIDI_BUTTON_THRESHOLD_UPPER (55)
#define MIDI_BUTTON_THRESHOLD_LOWER (45)

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
    static QAction* action(unsigned int cc);
    
    /**
     * Get the corresponding MIDI CC to a given action
     * @param action the action for which a MIDI CC should be returned
     * @return If there is a MIDI CC assigned to the given action, the corresponding CC is returned. If there is no MIDI CC assigned to the given action, a value greater than 127 is returned.
     */
    static unsigned int cc(QAction* action);
    
    /**
     * Sets the MIDI CC for a given action.
     * @param action the action to assign a MIDI CC to
     * @param cc the CC to assign to the given action
     */
    static void setCc(QAction* action, unsigned int cc);
    
    /**
     * Resets all CCs for a given action.
     * @param action the action to remove all MIDI CCs from
     */
    static void resetCc(QAction* action);
    
private:
    
    static QMap<unsigned char, QAction*> midi_cc_map;
    static QList<QAction*> midi_cc_actions;
};



#endif //PERFORMER_MIDI_H
