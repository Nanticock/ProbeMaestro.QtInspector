#ifndef QOBJECTVIEWER_H
#define QOBJECTVIEWER_H

#include "PropertyEditors/IPropertyEditor.h"
#include <UndoRedoStack/UndoRedoStack.hpp>

#include <QProgressBar>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

class QObjectViewer : public QWidget
{
    Q_OBJECT

    friend class IPropertyEditor;

private:
    struct UndoStackElement
    {
        QObject *currentObject = nullptr;
        const QMetaObject *currentMetaObject = nullptr;

        bool operator==(const UndoStackElement &other);
        bool operator!=(const UndoStackElement &other);

        QString toString() const;
    };

public:
    explicit QObjectViewer(QWidget *parent = nullptr);

    QObject *currentObject() const;
    void setCurrentObject(QObject *value, bool pushToUndoStack = true);

    bool currentObjectIsValid() const;

    const QMetaObject *currentMetaObject() const;
    void setCurrentMetaObject(const QMetaObject *value, bool pushToUndoStack = true);

    bool setCurrentObjectFromAddress(void *address);
    bool setCurrentObjectFromAddress(size_t address);

    bool setCurrentMetaObjectFromAddress(void *address);
    bool setCurrentMetaObjectFromAddress(size_t address);

    bool setCurrentObjectFromQmlId(const QString &id);
    bool setCurrentObjectFromQmlPath(const QString &path);
    bool setCurrentMetaObjectFromTypeName(const QString &typeName);
    bool setCurrentObjectFromQmlSingleton(const QString &singletonName);

    bool gotoAddress(size_t address);
    bool gotoString(const QString &string);

    void refresh();
    bool saveHeader();
    bool gotoParent();
    bool gotoSuperClass();
    bool gotoNextObject();
    bool gotoPreviousObject();

    void showGotoWindow(const QString &defaultValue = "");
    void showSearchWindow(const QString &defaultValue = "");

    IPropertyEditorPtr getPropertyEditorForType(int typeId) const;
    IPropertyEditorPtr getPropertyEditorForValue(const QVariant &value) const;
    IPropertyEditorPtr getPropertyEditorForProperty(const QMetaProperty &property) const;

    QTreeWidget &treeWidget();
    const QTreeWidget &treeWidget() const;

    /**
     * @brief getAddressFromString if you have a string that represents octal, decimal or hexadecimal number
     * this function parses this string and returns its value
     * @param string the input string that represents octal, decimal or hexadecimal value
     * @param value the numerical value of the string
     * @return true in the case of success, false otherwise
     */
    static bool getAddressFromString(const QString &string, size_t *value = nullptr);

private:
    void initToolbar();
    void initSearchBox();
    void initContextMenus();
    void initTreeView();
    void initProgressBar();

    // TODO: add connectionsItem
    void updateEnumsItem();
    void updateObjectItem();
    void updateMethodsItem();
    void updateMetaTypeItem();
    void updateClassInfoItem();
    void updatePropertiesItem();
    void updateConstructorsItem();
    void updateQmlSourceFileItem();

    void updateUndoRedoStack();

    QSharedPointer<UndoStackElement> currentState() const;

signals:
    void currentObjectChanged();
    void currentMetaObjectChanged();

private slots:
    void onGotoTriggered();
    void onSearchTriggered();
    void onCurrentObjectChanged();
    void onCurrentMetaObjectChanged();

private:
    QTreeWidget m_treeWidget;           // The tree widget to display the meta information
    QTreeWidgetItem m_objectItem;       // The top-level item for the object itself
    QTreeWidgetItem m_metaTypeItem;     // The top-level item for the metaType info
    QTreeWidgetItem m_classInfoItem;    // The top-level item for the class infos
    QTreeWidgetItem m_constructorsItem; // The top-level item for the constructors
    QTreeWidgetItem m_propertiesItem;   // The top-level item for the properties
    QTreeWidgetItem m_methodsItem;      // The top-level item for the methods
    QTreeWidgetItem m_enumsItem;        // The top-level item for the enums

    QToolBar m_toolbar;
    QVBoxLayout m_centralWidget;
    QProgressBar m_progressBar;

    QFont m_numbersFont;

    QAction *m_searchAction;
    QAction *m_refreshAction;
    QAction *m_saveHeaderAction;
    QAction *m_gotoParentAction;
    QAction *m_gotoSuperClassAction;
    QAction *m_gotoNextObjectAction;
    QAction *m_gotoPreviousObjectAction;
    UndoRedoStack<UndoStackElement> m_undoRedoStack;

    QList<IPropertyEditorPtr> m_propertyEditors;
};

#endif // QOBJECTVIEWER_H
