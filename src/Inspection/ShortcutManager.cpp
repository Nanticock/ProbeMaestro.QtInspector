#include "ShortcutManager.h"

#include <QKeyEvent>

using namespace PM::QtInspector;

void ShortcutManager::registerSequence(const QKeySequence &seq, const std::function<void()> &callback)
{
    m_sequences.insert(seq, callback);
}

bool ShortcutManager::eventFilter(QObject *, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return false;

    auto *keyEvent = static_cast<QKeyEvent *>(event);
    const QKeySequence sequence = keySequenceFromEvent(keyEvent);

    if (!m_sequences.contains(sequence))
        return false;

    const auto callback = m_sequences[sequence];
    if (callback)
        callback();

    emit sequenceTriggered(sequence);
    return true; // consume event
}

QKeySequence ShortcutManager::keySequenceFromEvent(QKeyEvent *event)
{
    int result = event->key();

    Qt::KeyboardModifiers modifierKeys = event->modifiers();
    if (modifierKeys & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (modifierKeys & Qt::AltModifier)
        result |= Qt::ALT;
    if (modifierKeys & Qt::ShiftModifier)
        result |= Qt::SHIFT;
    if (modifierKeys & Qt::MetaModifier)
        result |= Qt::META;

    return QKeySequence(result);
}
