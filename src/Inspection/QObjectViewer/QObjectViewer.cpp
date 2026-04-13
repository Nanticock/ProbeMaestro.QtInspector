#include "QObjectViewer.h"

#include "Exporters/QObjectToCppExporter_p.h"
#include "ObjectLocator/ObjectLocator.h"
#include "PointerChecker/PointerChecker.h"
#include "PropertyEditors/IPropertyEditor_p.h"
#include <Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h>
#include <compat_Qt.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaProperty>
#include <QProgressDialog>
#include <QPushButton>
#include <QShortcut>
#include <QtQml>

QObjectViewer::QObjectViewer(QWidget *parent) :
    QWidget(parent),
    m_treeWidget(this),
    m_objectItem(&m_treeWidget),
    m_metaTypeItem(&m_treeWidget),
    m_classInfoItem(&m_treeWidget),
    m_constructorsItem(&m_treeWidget),
    m_propertiesItem(&m_treeWidget),
    m_methodsItem(&m_treeWidget),
    m_enumsItem(&m_treeWidget),
    m_toolbar(this),
    m_centralWidget(this),
    m_numbersFont(QFont(QApplication::font().family(), QApplication::font().pointSize(), QFont::Black, true)),
    m_gotoParentAction(nullptr),
    m_gotoSuperClassAction(nullptr),
    m_gotoNextObjectAction(nullptr),
    m_gotoPreviousObjectAction(nullptr),
    m_propertyEditors(IPropertyEditor::defaultEditors())
{
    // Add top-level items for properties, methods and enums
    m_enumsItem.setText(0, "Enums");
    m_enumsItem.setFont(2, m_numbersFont);
    m_methodsItem.setText(0, "Methods");
    m_methodsItem.setFont(2, m_numbersFont);
    m_metaTypeItem.setText(0, "MetaType");
    m_metaTypeItem.setFont(2, m_numbersFont);
    m_classInfoItem.setText(0, "Class info");
    m_classInfoItem.setFont(2, m_numbersFont);
    m_propertiesItem.setText(0, "Properties");
    m_propertiesItem.setFont(2, m_numbersFont);
    m_constructorsItem.setText(0, "Constructors");
    m_constructorsItem.setFont(2, m_numbersFont);

    connect(this, &QObjectViewer::currentObjectChanged, this, &QObjectViewer::onCurrentObjectChanged);
    connect(this, &QObjectViewer::currentMetaObjectChanged, this, &QObjectViewer::onCurrentMetaObjectChanged);

    initToolbar();
    initTreeView();
    initSearchBox();
    initProgressBar();
    initContextMenus();
}

QObject *QObjectViewer::currentObject() const
{
    if (currentState().isNull())
        return nullptr;

    return currentState()->currentObject;
}

void QObjectViewer::setCurrentObject(QObject *value, bool pushToUndoStack)
{
    if (currentState().isNull())
        return;

    // if there is no current object set and the value isn't nullptr, continue the function
    // if there is a current object and it equals the current value, return
    if ((currentObject() == nullptr && value != nullptr) && currentObject() == value)
        return;

    if (pushToUndoStack)
        m_undoRedoStack.pushToUndoStack();

    currentState()->currentObject = value;
    emit currentObjectChanged();

    updateUndoRedoStack();
}

bool QObjectViewer::currentObjectIsValid() const
{
    return currentObject() != nullptr;
}

const QMetaObject *QObjectViewer::currentMetaObject() const
{
    if (currentState().isNull())
        return nullptr;

    if (currentObjectIsValid())
        return currentObject()->metaObject();

    return currentState()->currentMetaObject;
}

void QObjectViewer::setCurrentMetaObject(const QMetaObject *value, bool pushToUndoStack)
{
    if (currentState().isNull())
        return;

    // if there is no current object, continue the function
    // if there is a current object and its value quals the new value, return
    if (!currentObject() && currentMetaObject() == value)
        return;

    if (pushToUndoStack)
        m_undoRedoStack.pushToUndoStack();

    setCurrentObject(nullptr, false);
    currentState()->currentMetaObject = value;
    emit currentMetaObjectChanged();

    updateUndoRedoStack();
}

bool QObjectViewer::setCurrentObjectFromAddress(void *address)
{
    // check to see if the given address is pointing to a QObject
    QObject *object = ObjectLocator::getQObjectFromAddress(address);
    if (!object)
        return false;

    setCurrentObject(object);
    return true;
}

bool QObjectViewer::setCurrentObjectFromAddress(size_t address)
{
    return setCurrentObjectFromAddress(reinterpret_cast<void *>(address));
}

bool QObjectViewer::setCurrentMetaObjectFromAddress(void *address)
{
    // check to see if the given address is pointing to a QMetaObject
    const QMetaObject *metaObject = ObjectLocator::getQMetaObjectFromAddress(address);
    if (!metaObject)
        return false;

    setCurrentMetaObject(metaObject);
    return true;
}

bool QObjectViewer::setCurrentMetaObjectFromAddress(size_t address)
{
    return setCurrentMetaObjectFromAddress(reinterpret_cast<void *>(address));
}

bool QObjectViewer::setCurrentObjectFromQmlId(const QString &id)
{
    // check to see if the given string is the id of a QML object in any of the application windows
    QObject *object = ObjectLocator::getQmlObjectById(id);
    if (!object)
        return false;

    setCurrentObject(object);
    return true;
}

bool QObjectViewer::setCurrentObjectFromQmlPath(const QString &path)
{
    // check to see if the given string is the id of a QML object in any of the application windows
    QObject *object = ObjectLocator::getQmlObjectByPath(path);
    if (!object)
        return false;

    setCurrentObject(object);
    return true;
}

bool QObjectViewer::setCurrentMetaObjectFromTypeName(const QString &typeName)
{
    // check to see if the given string is a name to a valid QMetaType
    const QMetaObject *metaObject = ObjectLocator::getQMetaObjectFromTypeName(typeName);
    if (!metaObject)
        return false;

    setCurrentMetaObject(metaObject);
    return true;
}

bool QObjectViewer::setCurrentObjectFromQmlSingleton(const QString &singletonName)
{
    // check to see if the given string is the name of a QML singletone
    QObject *object = ObjectLocator::getQmlSingleton(singletonName);
    if (!object)
        return false;

    setCurrentObject(object);
    return true;
}

bool QObjectViewer::gotoAddress(size_t address)
{
    if (setCurrentObjectFromAddress(address))
        return true;

    if (setCurrentMetaObjectFromAddress(address))
        return true;

    return false;
}

bool QObjectViewer::gotoString(const QString &string)
{
    if (setCurrentObjectFromQmlSingleton(string))
        return true;

    if (setCurrentObjectFromQmlId(string))
        return true;

    if (setCurrentMetaObjectFromTypeName(string))
        return true;

    if (setCurrentObjectFromQmlPath(string))
        return true;

    return false;
}

void QObjectViewer::refresh()
{
    emit currentMetaObjectChanged();
}

bool QObjectViewer::saveHeader()
{
    static QObjectToCppExporter exporter;

    QString fileName = QObjectToCppExporterPrivate::classNameToFileName(currentMetaObject() ? currentMetaObject()->className() : "");

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), fileName,
                                                    "C/C++ header file (*.h *.hpp *.hxx);;C++ source file (*.cpp *.cxx);;All files (*.*)");

    if (filePath.isEmpty())
        return false;

    if (currentObjectIsValid())
        return exporter.exportToFile(QVariant::fromValue(currentObject()), filePath);

    if (currentMetaObject() != nullptr)
        return exporter.exportToFile(QVariant::fromValue(currentMetaObject()), filePath);

    return false;
}

bool QObjectViewer::gotoParent()
{
    if (!currentObjectIsValid())
        return false;

    if (!currentObject()->parent())
        return false;

    setCurrentObject(currentObject()->parent());
    return true;
}

bool QObjectViewer::gotoSuperClass()
{
    if (!currentMetaObject())
        return false;

    const QMetaObject *superClass = currentMetaObject()->superClass();
    if (superClass == nullptr)
        return false;

    setCurrentMetaObject(superClass);
    return true;
}

bool QObjectViewer::gotoNextObject()
{
    if (!m_undoRedoStack.redo())
        return false;

    updateUndoRedoStack();

    emit currentObjectChanged();
    return true;
}

bool QObjectViewer::gotoPreviousObject()
{
    if (!m_undoRedoStack.undo())
        return false;

    updateUndoRedoStack();

    emit currentObjectChanged();
    return true;
}

void QObjectViewer::showGotoWindow(const QString &defaultValue)
{
    QString input = QInputDialog::getText(nullptr, "Goto object",
                                          "Enter any of the following:\n"
                                          " * the name of a QMetaType\n"
                                          " * the id of a QML Object in any of the application windows\n"
                                          " * the path of a QML object in any of the application windows\n"
                                          " * the name of a QML singleton in the form of \"{uri}.{name}\" e.g. \"Common.BuildType\"\n"
                                          " * the memory address of a QObject\n"
                                          " * the memory address of a QMetaObject",
                                          QLineEdit::Normal, defaultValue)
                        .trimmed();

    if (input.trimmed().isEmpty())
        return;

    size_t address = 0;
    if (getAddressFromString(input, &address))
    {
        if (gotoAddress(address))
            return;

        QMessageBox("Error", QString("address: 0x%1 cannot be read.").arg(address, 0, 16), QMessageBox::Critical, QMessageBox::Ok,
                    QMessageBox::NoButton, QMessageBox::NoButton)
            .exec();
    }
    else // treat the input as a normal string
    {
        if (gotoString(input))
            return;

        QMessageBox("Error", "Couldn't locate \"" + input + "\"", QMessageBox::Critical, QMessageBox::Ok, QMessageBox::NoButton,
                    QMessageBox::NoButton)
            .exec();
    }

    showGotoWindow(input);
}

void QObjectViewer::showSearchWindow(const QString &defaultValue)
{
    QString input = QInputDialog::getText(nullptr, "Search for an object of the given type", "Enter the type name of the object:\n",
                                          QLineEdit::Normal, defaultValue)
                        .trimmed();

    if (input.isEmpty())
        return;

    m_progressBar.setMaximum(0);
    m_progressBar.setTextVisible(false);

    QMessageBox messageBox;

    m_searchAction->setEnabled(false);
    m_progressBar.show();
    {
        // currently, running the search function in a thread other than the main thread is unstable and can cause the application to crash, but by
        // default the search function can detect that it's running in the main thread and would call the
        QObjectList searchResult = ObjectLocator::searchForQmlObjectsOfType(input);

        messageBox.setWindowTitle("Search result");
        messageBox.setIcon(QMessageBox::Critical);
        // messageBox.setTextFormat(Qt::MarkdownText);

        if (!searchResult.isEmpty())
        {
            messageBox.setIconPixmap(QPixmap(":/resources/icons/search-icon-black.svg"));
            messageBox.setText(QString("Objects found: **%1**\n\nObjects type: **%2**").arg(searchResult.count()).arg(input));
            messageBox.setStandardButtons(QMessageBox::NoButton);

            QString detailedText = input + ":\n";
            for (QObject *object : searchResult)
                detailedText += QString("0x%1\n").arg(size_t(object), 0, 16);

            messageBox.setDetailedText(detailedText);
        }
        else
        {
            messageBox.setText("Couldn't find any objects of type: **" + input + "**");
        }
    }
    m_progressBar.hide();
    m_searchAction->setEnabled(true);

    messageBox.exec();
}

QTreeWidget &QObjectViewer::treeWidget()
{
    return m_treeWidget;
}

void QObjectViewer::initToolbar()
{
    m_centralWidget.addWidget(&m_toolbar);

    // gotoPreviousObjectAction
    m_gotoPreviousObjectAction =
        m_toolbar.addAction(QIcon(":/resources/icons/previous-icon-blue.svg"), "Previous", this, [this]() { gotoPreviousObject(); });
    m_gotoPreviousObjectAction->setEnabled(false);

    // gotoNextObjectAction
    m_gotoNextObjectAction = m_toolbar.addAction(QIcon(":/resources/icons/next-icon-blue.svg"), "Next", this, [this]() { gotoNextObject(); });
    m_gotoNextObjectAction->setEnabled(false);

    // refreshAction
    m_refreshAction = m_toolbar.addAction(QIcon(":/resources/icons/refresh_icon.svg"), "Refresh", this, [this]() { refresh(); });
    m_refreshAction->setEnabled(false);

    // gotoParentAction
    m_gotoParentAction = m_toolbar.addAction(QIcon(":/resources/icons/goto-parent-icon-black.svg"), "Goto parent", this, [this]() { gotoParent(); });
    m_gotoParentAction->setEnabled(false);

    // gotoSuperClassAction
    m_gotoSuperClassAction =
        m_toolbar.addAction(QIcon(":/resources/icons/goto-super-class-icon-red-black.svg"), "Goto super-class", this, [this]() { gotoSuperClass(); });
    m_gotoSuperClassAction->setEnabled(false);

    m_toolbar.addSeparator();

    m_toolbar.addAction("Goto", this, &QObjectViewer::onGotoTriggered);
    m_searchAction = m_toolbar.addAction(QIcon(":/resources/icons/search-icon-black.svg"), "Search", this, &QObjectViewer::onSearchTriggered);

    m_toolbar.addSeparator();

    // saveHeaderAction
    m_saveHeaderAction = m_toolbar.addAction(QIcon(":/resources/icons/export-header.svg"), "Save header file", this, [this]() { saveHeader(); });
    m_saveHeaderAction->setEnabled(false);
}

// Refer to:
// https://sl.bing.net/iHYGxNWyVDE
// https://sl.bing.net/dcyrvOEhFaC
// https://sl.bing.net/bbNKcPe4tye
void QObjectViewer::initSearchBox()
{
    // Create a line edit for searching
    QLineEdit *searchEdit = new QLineEdit(this);
    searchEdit->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    // Create a button for hiding the search bar
    QPushButton *hideButton = new QPushButton(QIcon(":/resources/icons/close-icon-blue.svg"), "", this);
    hideButton->setFlat(true);
    hideButton->setStyleSheet("QPushButton { margin: 0px; padding: 0px; }");

    // Create a widget to hold the search edit and the hide button
    QWidget *searchBar = new QWidget(this);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchBar);
    searchLayout->addWidget(hideButton);
    searchLayout->addWidget(searchEdit);
    searchLayout->setSpacing(2);
    searchLayout->setContentsMargins(0, 0, 0, 0);

    // Set the initial visibility of the search bar to false
    searchBar->setVisible(false);

    // Create a shortcut for toggling the search bar
    QShortcut *searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);

    // Connect the shortcut to a lambda function that shows the search bar
    connect(searchShortcut, &QShortcut::activated, this,
            [this, searchBar, searchEdit]()
            {
                if (window() && !this->isAncestorOf(window()->focusWidget()))
                    return;

                searchBar->setVisible(true);
                searchEdit->setFocus();
            });

    // Connect the hide button to a lambda function that hides the search bar and clears the search edit
    connect(hideButton, &QPushButton::clicked, this,
            [this, searchBar, searchEdit]()
            {
                searchEdit->clear();
                searchBar->setVisible(false);
                m_treeWidget.setFocus();
            });

    // Create a menu for the search edit
    QMenu *searchMenu = new QMenu(this);

    // Create actions for case sensitivity and whole word matching
    QAction *caseSensitiveAction = new QAction("Case sensitive", this);
    caseSensitiveAction->setCheckable(true);
    QAction *wholeWordAction = new QAction("Whole word", this);
    wholeWordAction->setCheckable(true);

    // Add actions to the menu
    searchMenu->addAction(caseSensitiveAction);
    searchMenu->addAction(wholeWordAction);

    // Connect the custom context menu requested signal of the search edit to a lambda function that shows the menu
    connect(searchEdit, &QLineEdit::customContextMenuRequested, this,
            [searchEdit, searchMenu](const QPoint &pos) { searchMenu->exec(searchEdit->mapToGlobal(pos)); });

    auto searchFunction = [this, caseSensitiveAction, wholeWordAction](const QString &text)
    {
        // Loop through all the items in the tree widget
        QTreeWidgetItemIterator it(&m_treeWidget);
        while (*it)
        {
            // Get the current item
            QTreeWidgetItem *item = *it;
            // Check if the item contains the text in any column
            bool matchFound = false;
            for (int i = 0; i < m_treeWidget.columnCount(); ++i)
            {
                // Refer to: https://sl.bing.net/h38QirGpuYS
                QString searchString = wholeWordAction->isChecked() ? "\\b" + text + "\\b" : text;

                // Create a regular expression based on the text and options
                QRegExp regex(searchString, caseSensitiveAction->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

                matchFound = item->text(i).contains(regex);

                if (matchFound)
                    break;
            }

            if (matchFound)
            {
                // We need to show all the parents of any found item
                // because any hidden item doesn't draw any of its children
                QTreeWidgetItem *parent = item->parent();

                if (parent)
                    parent->setExpanded(true);

                while (parent)
                {
                    parent->setHidden(false);
                    parent = parent->parent();
                }
            }

            // Show or hide the item depending on the match
            item->setHidden(!matchFound);
            // Go to the next item
            ++it;
        }
    };

    // Connect the text changed signal of the search edit to a lambda function that filters the tree widget
    connect(searchEdit, &QLineEdit::textChanged, this, searchFunction);

    connect(wholeWordAction, &QAction::toggled, this, [searchEdit, searchFunction]() { searchFunction(searchEdit->text()); });
    connect(caseSensitiveAction, &QAction::toggled, this, [searchEdit, searchFunction]() { searchFunction(searchEdit->text()); });

    m_centralWidget.addWidget(searchBar);
    m_centralWidget.setSpacing(1);
}

// Refer to:
// https://sl.bing.net/d5cs0e5nejA
// https://sl.bing.net/PwU46fFrcO
void QObjectViewer::initContextMenus()
{
    // Create a menu for the tree widget
    QMenu *treeMenu = new QMenu(this);

    // Create actions for copying cell, copying row, expanding/collapsing row and expanding/collapsing all
    QAction *copyCellAction = new QAction("Copy", this);
    QAction *copyRowAction = new QAction("Copy row", this);
    QAction *expandRowAction = new QAction("Expand", this);
    QAction *collapseRowAction = new QAction("Collapse", this);
    QAction *expandAllAction = new QAction("Expand all", this);
    QAction *collapseAllAction = new QAction("Collapse all", this);
    QAction *followPointerAction = new QAction("Follow pointer", this);

    // Add actions to the menu
    treeMenu->addAction(followPointerAction);
    treeMenu->addSeparator();
    treeMenu->addAction(copyCellAction);
    treeMenu->addAction(copyRowAction);
    treeMenu->addSeparator();
    treeMenu->addAction(expandRowAction);
    treeMenu->addAction(collapseRowAction);
    treeMenu->addSeparator();
    treeMenu->addAction(expandAllAction);
    treeMenu->addAction(collapseAllAction);

    m_treeWidget.setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect the custom context menu requested signal of the tree widget to a lambda function that shows the menu
    connect(&m_treeWidget, &QTreeWidget::customContextMenuRequested, this,
            [this, treeMenu, copyCellAction, copyRowAction, expandRowAction, collapseRowAction, followPointerAction](const QPoint &pos)
            {
                bool hasValidSelection = m_treeWidget.currentItem() != nullptr;
                bool currentItemHasChildren = hasValidSelection && m_treeWidget.currentItem()->childCount() > 0;

                // copy actions
                copyRowAction->setEnabled(hasValidSelection);
                copyCellAction->setEnabled(hasValidSelection);

                // expand / collapse actions
                expandRowAction->setEnabled(currentItemHasChildren);
                collapseRowAction->setEnabled(currentItemHasChildren);

                size_t value = 0;
                bool isPointerValue =
                    hasValidSelection && QObjectViewer::getAddressFromString(m_treeWidget.currentItem()->text(m_treeWidget.currentColumn()), &value);

                bool isReadableAddress = isPointerValue && BasicPointerChecker::isReadableAddress(value);

                followPointerAction->setEnabled(isReadableAddress);
                followPointerAction->setToolTip(isReadableAddress ? "" : "Warning: This address points to an unreadable memory location");

                treeMenu->exec(m_treeWidget.mapToGlobal(pos));
            });

    // Connect the copy cell action to a lambda function that copies the text of the current item
    connect(copyCellAction, &QAction::triggered, this,
            [this]()
            {
                QTreeWidgetItem *item = m_treeWidget.currentItem();
                if (item)
                {
                    // Get the text of the current item
                    QString text = item->text(m_treeWidget.currentColumn());
                    // Copy the text to the clipboard
                    QClipboard *clipboard = QApplication::clipboard();
                    clipboard->setText(text);
                }
            });

    // Connect the copy row action to a lambda function that copies the text of the current row
    connect(copyRowAction, &QAction::triggered, this,
            [this]()
            {
                QTreeWidgetItem *item = m_treeWidget.currentItem();
                if (item)
                {
                    // Get the text of all columns of the current item
                    QStringList texts;
                    for (int i = 0; i < m_treeWidget.columnCount(); ++i)
                    {
                        texts << item->text(i);
                    }
                    // Join the texts with tabs and copy to the clipboard
                    QString text = texts.join("\t");
                    QClipboard *clipboard = QApplication::clipboard();
                    clipboard->setText(text);
                }
            });

    // Connect the expand row action to a lambda function that expands the current item
    connect(expandRowAction, &QAction::triggered, this,
            [this]()
            {
                QTreeWidgetItem *item = m_treeWidget.currentItem();
                if (item)
                {
                    m_treeWidget.expandItem(item);
                }
            });

    // Connect the collapse row action to a lambda function that collapses the current item
    connect(collapseRowAction, &QAction::triggered, this,
            [this]()
            {
                QTreeWidgetItem *item = m_treeWidget.currentItem();

                if (item != nullptr)
                    m_treeWidget.collapseItem(item);
            });

    // Connect the expand all action to a lambda function that expands all items
    connect(expandAllAction, &QAction::triggered, this, [this]() { m_treeWidget.expandAll(); });

    // Connect the collapse all action to a lambda function that collapses all items
    connect(collapseAllAction, &QAction::triggered, this, [this]() { m_treeWidget.collapseAll(); });

    // Connect the follow pointer action
    connect(followPointerAction, &QAction::triggered, this,
            [this]()
            {
                if (!m_treeWidget.currentItem())
                    return;

                size_t value = 0;
                if (!getAddressFromString(m_treeWidget.currentItem()->text(m_treeWidget.currentColumn()), &value))
                    return;

                gotoAddress(value);
            });
}

void QObjectViewer::initTreeView()
{
    // Create a tree widget to display the meta information
    m_treeWidget.setHeaderLabels(QStringList() << "Name"
                                               << "Type"
                                               << "Value");

    m_treeWidget.header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

    // Refer to: https://sl.bing.net/deFlVShptPo
    m_treeWidget.setAlternatingRowColors(true);
    m_treeWidget.setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget.setSelectionBehavior(QAbstractItemView::SelectRows);

    // Add a top-level item for the object itself
    m_objectItem.setText(0, "Object");

    m_centralWidget.addWidget(&m_treeWidget);
}

void QObjectViewer::initProgressBar()
{
    m_progressBar.hide();
    m_progressBar.setMaximum(0);

    m_centralWidget.addWidget(&m_progressBar);
}

void QObjectViewer::updateEnumsItem()
{
    m_enumsItem.takeChildren();
    m_enumsItem.setText(2, "");

    if (!currentMetaObject())
        return;

    m_enumsItem.setText(2, QString::number(currentMetaObject()->enumeratorCount()));

    // Loop through the enums of the object
    for (int i = 0; i < currentMetaObject()->enumeratorCount(); ++i)
    {
        // Get the meta-enum
        QMetaEnum metaEnum = currentMetaObject()->enumerator(i);

        // Create a child item for the enum
        QTreeWidgetItem *enumItem = new QTreeWidgetItem(&m_enumsItem);
        enumItem->setText(0, metaEnum.name());
        // Get the key-value pairs of the enum
        QStringList keyValues;

        for (int j = 0; j < metaEnum.keyCount(); ++j)
        {
            keyValues << QString("%1 = %2").arg(metaEnum.key(j)).arg(metaEnum.value(j));
        }

        // Set the key-value pairs as the value of the enum item
        enumItem->setText(2, keyValues.join(", "));
    }
}

void QObjectViewer::updateObjectItem()
{
    m_objectItem.takeChildren();
    m_objectItem.setText(1, "");
    m_objectItem.setText(2, "");

    if (!currentMetaObject())
        return;

    // Set the class name and address of the object
    m_objectItem.setText(1, currentMetaObject()->className());

    quintptr value = currentObject() ? quintptr(currentObject()) : quintptr(currentMetaObject());
    m_objectItem.setText(2, QString("0x%1").arg(value, QT_POINTER_SIZE * 2, 16, QChar('0')));

    QTreeWidgetItem *superClassItem = new QTreeWidgetItem(&m_objectItem);
    superClassItem->setText(0, "Inherits");
    superClassItem->setText(2, currentMetaObject()->superClass() ? currentMetaObject()->superClass()->className() : "None");

    if (!currentObject())
        return;

    QTreeWidgetItem *qmlParentItem = new QTreeWidgetItem(&m_objectItem);
    qmlParentItem->setText(0, "Parent");
    qmlParentItem->setText(2, QString("0x%1").arg(size_t(currentObject()->parent()), 0, 16));
    qmlParentItem->setFont(0, m_numbersFont);

    // Refer to: https://sl.bing.net/hCqkRVXvpwO
    //
    // see if the current object has any QML context,
    // and if so, get its "id" in this context
    QQmlContext *context = qmlContext(currentObject());
    if (context != nullptr)
    {
        QTreeWidgetItem *qmlIdItem = new QTreeWidgetItem(&m_objectItem);
        qmlIdItem->setText(0, "id");
        qmlIdItem->setText(2, context->nameForObject(currentObject()));
        qmlIdItem->setFont(0, m_numbersFont);

        QTreeWidgetItem *qmlObjectPathItem = new QTreeWidgetItem(&m_objectItem);
        qmlObjectPathItem->setText(0, "Object path");
        qmlObjectPathItem->setText(2, ObjectLocator::getQmlObjectPath(currentObject()));
        qmlObjectPathItem->setFont(0, m_numbersFont);

        QTreeWidgetItem *qmlEngineItem = new QTreeWidgetItem(&m_objectItem);
        qmlEngineItem->setText(0, "QML engine");
        qmlEngineItem->setText(2, QString("0x%1").arg(size_t(context->engine()), 0, 16));
        qmlEngineItem->setFont(0, m_numbersFont);

        updateQmlSourceFileItem();
    }
}

void QObjectViewer::updateMethodsItem()
{
    m_methodsItem.takeChildren();
    m_methodsItem.setText(2, "");

    if (!currentMetaObject())
        return;

    m_methodsItem.setText(2, QString::number(currentMetaObject()->methodCount()));

    // Loop through the methods of the object
    static QHash<QMetaMethod::MethodType, QString> s_methodTypeMap{
        {QMetaMethod::MethodType::Slot, "Slot"},
        {QMetaMethod::MethodType::Signal, "Signal"},
        {QMetaMethod::MethodType::Method, "Method"},
        {QMetaMethod::MethodType::Constructor, "Constructor"},
    };

    // Loop through the methods of the object
    static QHash<QMetaMethod::Access, QString> s_methodAccessMap{
        {QMetaMethod::Access::Private, "Private"}, {QMetaMethod::Access::Protected, "Protected"}, {QMetaMethod::Access::Public, "Public"}};

    for (int i = 0; i < currentMetaObject()->methodCount(); ++i)
    {
        // Get the meta-method
        QMetaMethod metaMethod = currentMetaObject()->method(i);

        // Create a child item for the method
        QTreeWidgetItem *methodItem = new QTreeWidgetItem(&m_methodsItem);
        methodItem->setText(0, metaMethod.name());
        methodItem->setText(1, s_methodAccessMap[metaMethod.access()] + " " + s_methodTypeMap[metaMethod.methodType()]);
        methodItem->setText(2, metaMethod.methodSignature());

        methodItem->setFont(1, QFont("", -1, QFont::Light, true));
        methodItem->setForeground(1, Qt::darkGray);

        // Get the return type and parameters of the method
        QString returnType = metaMethod.typeName();
        QList<QByteArray> parameterTypes = metaMethod.parameterTypes();
        QList<QByteArray> parameterNames = metaMethod.parameterNames();

        // Create a child item for the return type
        QTreeWidgetItem *returnTypeItem = new QTreeWidgetItem(methodItem);
        returnTypeItem->setText(0, "Return type");
        returnTypeItem->setText(1, returnType);

        // Loop through the parameters of the method
        for (int j = 0; j < parameterTypes.size(); ++j)
        {
            // Create a child item for each parameter
            QTreeWidgetItem *parameterItem = new QTreeWidgetItem(methodItem);
            parameterItem->setText(0, parameterNames.at(j));
            parameterItem->setText(1, parameterTypes.at(j));
        }
    }
}

void QObjectViewer::updateMetaTypeItem()
{
    m_metaTypeItem.takeChildren();
    m_metaTypeItem.setText(2, "");

    const QMetaObject *mo = currentMetaObject();
    if (!mo)
        return;

    QString className = mo->className();
    QString normalizedName = QMetaObject::normalizedType(className.toUtf8().constData());

    int typeId = QMetaType::type(className.toUtf8().constData());

    // Try normalized name
    if (typeId == QMetaType::UnknownType)
    {
        QByteArray norm = normalizedName.toUtf8();
        typeId = QMetaType::type(norm.constData());
    }

    // Try pointer type
    if (typeId == QMetaType::UnknownType)
    {
        QString ptrName = normalizedName + " *";
        ptrName = QMetaObject::normalizedType(ptrName.toUtf8().constData());

        QByteArray ptr = ptrName.toUtf8();
        typeId = QMetaType::type(ptr.constData());
    }

    QMetaType metaType(typeId);

    const bool isValid = metaType.isValid();
    m_metaTypeItem.setText(2, isValid ? "Valid" : "Invalid");

    if (!isValid)
        return;

    auto makeItem = [&](const QString &name, const QString &value)
    {
        auto *item = new QTreeWidgetItem(&m_metaTypeItem);
        item->setText(0, name);
        item->setText(2, value);
    };

    makeItem("name", metaType.metaObject() ? metaType.metaObject()->className() : "<null>");

    makeItem("sizeOf", QString::number(metaType.sizeOf()));
    makeItem("flags", QString::number(metaType.flags()));
    makeItem("id", QString::number(PM::internal::getMetaTypeId(metaType)));
    makeItem("isRegistered", metaType.isRegistered() ? "true" : "false");
    makeItem("isValid", metaType.isValid() ? "true" : "false");

    makeItem("hasRegisteredComparators", metaType.hasRegisteredComparators(PM::internal::getMetaTypeId(metaType)) ? "true" : "false");
    makeItem("hasRegisteredDebugStreamOperator", metaType.hasRegisteredDebugStreamOperator(PM::internal::getMetaTypeId(metaType)) ? "true" : "false");
}

void QObjectViewer::updateClassInfoItem()
{
    m_classInfoItem.takeChildren();
    m_classInfoItem.setText(2, "");

    if (!currentMetaObject())
        return;

    m_classInfoItem.setText(2, QString::number(currentMetaObject()->classInfoCount()));

    // Loop through the class infos
    for (int i = 0; i < currentMetaObject()->classInfoCount(); ++i)
    {
        // Get the meta-class info
        QMetaClassInfo metaClassInfo = currentMetaObject()->classInfo(i);
        // Create a child item for the class info
        QTreeWidgetItem *classInfoItem = new QTreeWidgetItem(&m_classInfoItem);
        classInfoItem->setText(0, metaClassInfo.name());
        classInfoItem->setText(2, metaClassInfo.value());
    }
}

void QObjectViewer::updatePropertiesItem()
{
    m_propertiesItem.takeChildren();
    m_propertiesItem.setText(2, "");

    if (!currentMetaObject())
        return;

    m_propertiesItem.setText(2, QString::number(currentMetaObject()->propertyCount()));

    // collapsing the properties item before adding the properties
    // and then expanding it after that, makes the control more responsive
    // and eleminates the loading lag when changing the currentObject
    // while the properties item is already expanded
    bool expanded = m_propertiesItem.isExpanded();
    m_propertiesItem.setExpanded(false);
    {
        // Loop through the properties of the object
        for (int i = 0; i < currentMetaObject()->propertyCount(); ++i)
        {
            // Get the meta-property
            QMetaProperty metaProperty = currentMetaObject()->property(i);

            // Create a child item for the property
            QSharedPointer<IPropertyEditor> propertyEditor = getPropertyEditorForProperty(metaProperty);
            propertyEditor->createPropertyTreeItem(metaProperty, this, &m_propertiesItem);
        }
    }
    m_propertiesItem.setExpanded(expanded);
}

void QObjectViewer::updateConstructorsItem()
{
    m_constructorsItem.takeChildren();
    m_constructorsItem.setText(2, "");

    if (!currentMetaObject())
        return;

    m_constructorsItem.setText(2, QString::number(currentMetaObject()->constructorCount()));

    // Loop through the constructors
    for (int i = 0; i < currentMetaObject()->constructorCount(); ++i)
    {
        // Get the meta-method
        QMetaMethod metaMethod = currentMetaObject()->constructor(i);
        // Create a child item for the constructor
        QTreeWidgetItem *constructorItem = new QTreeWidgetItem(&m_constructorsItem);
        constructorItem->setText(0, metaMethod.name());
        constructorItem->setText(1, metaMethod.methodSignature());
        constructorItem->setText(2, metaMethod.typeName());

        // Get the parameters of the constructor
        QList<QByteArray> parameterTypes = metaMethod.parameterTypes();
        QList<QByteArray> parameterNames = metaMethod.parameterNames();

        // Loop through the parameters of the constructor
        for (int j = 0; j < parameterTypes.size(); ++j)
        {
            // Create a child item for each parameter
            QTreeWidgetItem *parameterItem = new QTreeWidgetItem(constructorItem);
            parameterItem->setText(0, parameterNames.at(j));
            parameterItem->setText(1, parameterTypes.at(j));
        }
    }
}

void QObjectViewer::updateQmlSourceFileItem()
{
    QUrl qmlFilePath = ObjectLocator::getQmlObjectSourceFilePath(currentObject());

    if (!qmlFilePath.isValid())
        return;

    QTreeWidgetItem *qmlFilePathItem = new QTreeWidgetItem(&m_objectItem);
    qmlFilePathItem->setText(0, "Qml file path");
    qmlFilePathItem->setText(2, qmlFilePath.toString());
    qmlFilePathItem->setFont(0, m_numbersFont);

    // TODO: add a button to view and edit the qml source file
}

void QObjectViewer::updateUndoRedoStack()
{
    if (m_gotoNextObjectAction)
        m_gotoNextObjectAction->setEnabled(m_undoRedoStack.canRedo());

    if (m_gotoPreviousObjectAction)
        m_gotoPreviousObjectAction->setEnabled(m_undoRedoStack.canUndo());

    m_refreshAction->setEnabled(currentObjectIsValid() || m_undoRedoStack.currentState()->currentMetaObject != nullptr);
    m_saveHeaderAction->setEnabled(m_refreshAction->isEnabled());
}

QSharedPointer<QObjectViewer::UndoStackElement> QObjectViewer::currentState() const
{
    return m_undoRedoStack.currentState();
}

bool QObjectViewer::getAddressFromString(const QString &string, size_t *value)
{
    bool gotAddress = false;
    size_t address = string.toLongLong(&gotAddress); // try to get a decimal address

    if (!gotAddress) // if this isn't a decimal address, try to treat it as a hexadecimal address
        address = string.toLongLong(&gotAddress, 16);

    if (!gotAddress) // if this isn't a hexadecimal address, try to treat it as a octal address
        address = string.toLongLong(&gotAddress, 8);

    if (!gotAddress)
        return false;

    if (value != nullptr)
        *value = address;

    return true;
}

QSharedPointer<IPropertyEditor> QObjectViewer::getPropertyEditorForType(int typeId) const
{
    return IPropertyEditorPrivate::getPropertyEditorForType(typeId, m_propertyEditors);
}

QSharedPointer<IPropertyEditor> QObjectViewer::getPropertyEditorForValue(const QVariant &value) const
{
    return IPropertyEditorPrivate::getPropertyEditorForValue(value, m_propertyEditors);
}

QSharedPointer<IPropertyEditor> QObjectViewer::getPropertyEditorForProperty(const QMetaProperty &property) const
{
    return IPropertyEditorPrivate::getPropertyEditorForProperty(property, m_propertyEditors);
}

void QObjectViewer::onCurrentObjectChanged()
{
    emit currentMetaObjectChanged();

    // ConnectionInspector::test(currentObject());
}

void QObjectViewer::onCurrentMetaObjectChanged()
{
    // FIXME: get rid of this
    Compat::QtObjectsMemoryMap::addMetaObject(currentMetaObject());

    auto currentIndex = m_treeWidget.currentIndex();
    QVector<bool> topLevelItemsExpandedStates(m_treeWidget.topLevelItemCount());
    for (int i = 0; i < m_treeWidget.topLevelItemCount(); i++)
    {
        auto currentTopLevelItem = m_treeWidget.topLevelItem(i);
        topLevelItemsExpandedStates[i] = currentTopLevelItem->isExpanded();
        currentTopLevelItem->setExpanded(false);
    }

    // Refer to:
    // https://sl.bing.net/dhJxCXViHoi
    // https://sl.bing.net/eAhRuFehyG4

    updateObjectItem();
    updateMetaTypeItem();
    updateClassInfoItem();
    updatePropertiesItem();
    updateConstructorsItem();
    updateMethodsItem();
    updateEnumsItem();

    for (int i = 0; i < m_treeWidget.topLevelItemCount(); i++)
        m_treeWidget.topLevelItem(i)->setExpanded(topLevelItemsExpandedStates[i]);

    m_treeWidget.setCurrentIndex(currentIndex);
    m_treeWidget.scrollTo(currentIndex);

    if (m_gotoParentAction)
        m_gotoParentAction->setEnabled(currentObjectIsValid() && currentObject()->parent());

    if (m_gotoSuperClassAction)
        m_gotoSuperClassAction->setEnabled(currentMetaObject() && currentMetaObject()->superClass());
}

const QTreeWidget &QObjectViewer::treeWidget() const
{
    return m_treeWidget;
}

void QObjectViewer::onGotoTriggered()
{
    showGotoWindow();
}

void QObjectViewer::onSearchTriggered()
{
    showSearchWindow();
}

bool QObjectViewer::UndoStackElement::operator==(const UndoStackElement &other)
{
    return currentObject == other.currentObject && currentMetaObject == other.currentMetaObject;
}

bool QObjectViewer::UndoStackElement::operator!=(const UndoStackElement &other)
{
    return !(*this == other);
}

QString QObjectViewer::UndoStackElement::toString() const
{
    QString result = "{currentObject = %1(0x%2), currentMetaObject = %3(0x%4)}";

    // currentObject
    result = result.arg(currentObject ? currentObject->staticMetaObject.className() : "");
    result = result.arg(size_t(currentObject), 0, 16);

    // currentMetaObject
    result = result.arg(currentMetaObject ? currentMetaObject->className() : "");
    result = result.arg(size_t(currentMetaObject), 0, 16);

    return result;
}
