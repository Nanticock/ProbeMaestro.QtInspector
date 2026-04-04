#ifndef UNDOREDOSTACK_HPP
#define UNDOREDOSTACK_HPP

#include <QSharedPointer>
#include <QVector>

/**
 * @ref https://github.com/prusa3d/PrusaSlicer/blob/master/src/slic3r/Utils/UndoRedo.hpp
 */

template <typename T>
class UndoRedoStack
{
public:
    UndoRedoStack() : UndoRedoStack(T())
    {
    }

    explicit UndoRedoStack(const T &initialState)
    {
        setCurrentState(initialState);
    }

    bool undo()
    {
        if (!canUndo())
            return false;

        if (!m_currentState.isNull())
            m_redoStack << *m_currentState;

        m_currentState = QSharedPointer<T>::create(m_undoStack.takeLast());

        return !m_currentState.isNull();
    }

    bool redo()
    {
        if (!canRedo())
            return false;

        if (!m_currentState.isNull())
            m_undoStack << *m_currentState;

        m_currentState = QSharedPointer<T>::create(m_redoStack.takeLast());

        return !m_currentState.isNull();
    }

    bool popUndoStack(T *value = nullptr)
    {
        if (m_undoStack.isEmpty())
            return false;

        if (value != nullptr)
            *value = m_undoStack.takeLast();
        else
            m_undoStack.takeLast();

        return true;
    }

    bool popRedoStack(T *value = nullptr)
    {
        if (m_redoStack.isEmpty())
            return false;

        if (value != nullptr)
            *value = m_redoStack.takeLast();
        else
            m_redoStack.takeLast();

        return true;
    }

    void clearUndoStack()
    {
        m_undoStack.clear();
    }

    void clearRedoStack()
    {
        m_redoStack.clear();
    }

    void pushToUndoStack(const T &state, bool clearRedoStack = true)
    {
        if (clearRedoStack)
            m_redoStack.clear();

        m_undoStack << state;
    }

    void pushToUndoStack(bool clearRedoStack = true)
    {
        if (m_currentState.isNull())
            return;

        pushToUndoStack(*m_currentState, clearRedoStack);
    }

    void clearAll()
    {
        clearUndoStack();
        clearRedoStack();
    }

    QSharedPointer<T> currentState() const
    {
        return m_currentState;
    }

    void setCurrentState(const QSharedPointer<T> &currentState)
    {
        m_currentState = currentState;
    }

    void setCurrentState(const T &currentState)
    {
        setCurrentState(QSharedPointer<T>::create(currentState));
    }

    int undoStackLength() const
    {
        return m_undoStack.count();
    }

    int redoStackLength() const
    {
        return m_redoStack.count();
    }

    bool canUndo() const
    {
        return !m_undoStack.isEmpty();
    }

    bool canRedo() const
    {
        return !m_redoStack.isEmpty();
    }

private:
    QVector<T> m_undoStack;
    QVector<T> m_redoStack;

    QSharedPointer<T> m_currentState;
};

#endif // UNDOREDOSTACK_HPP
