#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QKeyEvent>
#include <QKeySequence>
#include <QSet>

namespace PM
{
namespace QtInspector
{
    class ShortcutManager : public QObject
    {
        Q_OBJECT

    public:
        void registerSequence(const QKeySequence &seq, const std::function<void()> &callback = nullptr);

    signals:
        void sequenceTriggered(const QKeySequence &seq);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        QKeySequence keySequenceFromEvent(QKeyEvent *event);

    private:
        QHash<QKeySequence, std::function<void()>> m_sequences;
    };
} // namespace QtInspector
} // namespace PM

#endif // SHORTCUTMANAGER_H
