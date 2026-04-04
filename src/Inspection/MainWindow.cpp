#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <Compat/MemoryMaps/FunctionsMemoryMaps/FunctionsMemoryMap.h>
#include <Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h>
#include <MainThreadHijacker.h>
#include <QObjectViewer/ConnectionInspector/ConnectionInspector.h>
#include <QObjectViewer/ObjectLocator/ObjectLocator.h>

#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QJSValue>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QSettings>
#include <QThread>
#include <QTimer>

static const char m_settingsGroupName[] = "AuxilliaryMainWindow";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), m_windowChildrenTreeRootItem(QVariantList())
{
    ui->setupUi(this);

    ui->WindowChildrenTreeView->setModel(&m_windowChildrenTreeModel);
    // Refer to: https://sl.bing.net/f4z7PDyUCEC
    ui->WindowChildrenTreeView->setRootIndex(QModelIndex());
    ui->dockWidget3->setWidget(&m_qObjectViewer);

    connect(&m_qObjectViewer, &QObjectViewer::currentObjectChanged, this, &MainWindow::onObjectViewerObjectChanged);
    connect(ui->WindowChildrenTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onTreeViewSelectionChanged);

    // menu connections

    // Refer to:
    // https://sl.bing.net/eLrE9p3QmjI
    ui->actionWindows->setMenu(new QMenu());
    ui->actionWindows->menu()->addAction(ui->dockWidget1->toggleViewAction());
    ui->actionWindows->menu()->addAction(ui->dockWidget2->toggleViewAction());
    ui->actionWindows->menu()->addAction(ui->dockWidget3->toggleViewAction());

    connect(ui->actionInvoke_action, &QAction::triggered, this, &MainWindow::onInvokeActionTriggered);
    connect(ui->actionDebug_command_1, &QAction::triggered, this, &MainWindow::onDebugCommand1Triggered);
    connect(ui->actionDebug_command_2, &QAction::triggered, this, &MainWindow::onDebugCommand2Triggered);
    connect(ui->actionDebug_command_3, &QAction::triggered, this, &MainWindow::onDebugCommand3Triggered);
    connect(ui->actionDebug_command_4, &QAction::triggered, this, &MainWindow::onDebugCommand4Triggered);
    connect(ui->actionDebug_command_5, &QAction::triggered, this, &MainWindow::onDebugCommand5Triggered);
    connect(ui->actionDebug_command_6, &QAction::triggered, this, &MainWindow::onDebugCommand6Triggered);
    connect(ui->actionExport_resource, &QAction::triggered, this, &MainWindow::onExportResourceTriggered);
    connect(ui->actionExport_all_resources, &QAction::triggered, this, &MainWindow::onExportAllResourcesTriggered);
    connect(ui->actionList_all_available_actions, &QAction::triggered, this, &MainWindow::onListAllActionsTriggered);
    connect(ui->actionRefresh_hierarchy_view, &QAction::triggered, this, &MainWindow::onRefreshHeirarchyViewTriggered);

    loadWindowSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QObject *MainWindow::rootObject()
{
    return m_windowChildrenTreeModel.rootObject();
}

void MainWindow::setRootObject(QObject *value)
{
    if (rootObject() == value)
        return;

    //    m_propertyGrid.setObject(value);
    m_qObjectViewer.setCurrentObject(value);
    m_windowChildrenTreeModel.setRootObject(value);
}

QObject *MainWindow::selectedObject()
{
    // Refer to:
    // https://sl.bing.net/byG9N2EPR6W

    // Get a pointer to the selection model
    QItemSelectionModel *selectionModel = ui->WindowChildrenTreeView->selectionModel();

    // Get a list of selected indexes
    QModelIndexList indexes = selectionModel->selectedIndexes();

    // Check if there is any selected index
    if (indexes.size() == 0)
        return nullptr;

    // Get the first index in the list
    QModelIndex selectedIndex = indexes.at(0);

    // Get a pointer to the item from the index
    auto selectedItem = m_windowChildrenTreeModel.itemFromIndex(selectedIndex);

    if (!selectedItem)
        return nullptr;

    return selectedItem->data(0).value<QObject *>();
}

void MainWindow::loadWindowSettings()
{
    //    // Refer to: https://sl.bing.net/expGAVYoO16
    //    tabifyDockWidget(ui->dockWidget3, ui->dockWidget2);
    //    // Refer to: https://sl.bing.net/h0i8BQ9NaKW
    //    ui->dockWidget3->raise();

    // Refer to:
    // https://sl.bing.net/fE30ocPaj6a

    QSettings settings;

    settings.beginGroup(m_settingsGroupName);
    {
        restoreGeometry(settings.value("geometry").toByteArray());
        restoreState(settings.value("state").toByteArray());
    }
    settings.endGroup();
}

void MainWindow::saveWindowSettings()
{
    // Refer to:
    // https://sl.bing.net/fE30ocPaj6a

    QSettings settings;

    settings.beginGroup(m_settingsGroupName);
    {
        settings.setValue("geometry", saveGeometry());
        settings.setValue("state", saveState());
    }
    settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowSettings();
}

void MainWindow::onRefreshHeirarchyViewTriggered()
{
    // FIXME: fix the usage of raw pointers in WindowChildrenTreeItem class
    QObject *rootObject = m_windowChildrenTreeModel.rootObject();

    m_windowChildrenTreeModel.setRootObject(nullptr);
    m_windowChildrenTreeModel.setRootObject(rootObject);
}

void MainWindow::onObjectViewerObjectChanged()
{
    //    m_propertyGrid.setObject(m_qObjectViewer.currentObject());
    //    setRootObject(m_qObjectViewer.object());
}

void MainWindow::onObjectChanged()
{
    //    m_propertyGrid.setObject(selectedObject());
    m_qObjectViewer.setCurrentObject(selectedObject());
}

void MainWindow::onInvokeActionTriggered()
{
    QString input = QInputDialog::getText(nullptr, "Invoke action", "Enter the name, qml id or the path of an action", QLineEdit::Normal).trimmed();

    if (input.isEmpty())
        return;

    // Try to get action by id
    QObject *selectedAction = ObjectLocator::getQmlObjectById(input);

    // try to get it by path
    if (!selectedAction)
        selectedAction = ObjectLocator::getQmlObjectByPath(input);

    // go and search for it in the allActions list
    if (!selectedAction)
    {
    }

    if (!selectedAction)
    {
        QMessageBox("Error", "Couldn't locate action: \"" + input + "\"", QMessageBox::Critical, QMessageBox::Ok, QMessageBox::NoButton,
                    QMessageBox::NoButton)
            .exec();

        return;
    }

    bool result = true;

    result &= selectedAction->setProperty("guard", false);
    result &= selectedAction->setProperty("enabled", true);
    result &= selectedAction->metaObject()->invokeMethod(selectedAction, "trigger");

    if (!result)
        QMessageBox("Error", "Couldn't invoke action: \"" + input + "\"", QMessageBox::Critical, QMessageBox::Ok, QMessageBox::NoButton,
                    QMessageBox::NoButton)
            .exec();
}

void MainWindow::onListAllActionsTriggered()
{
}

void MainWindow::onExportResourceTriggered()
{
    bool ok = true;
    bool error = false;
    QString url = "";

    QFile resourceFile;

    while ((ok == true && url.isEmpty()) || error == true)
    {
        error = false;
        url = QInputDialog::getText(this, "Resource URL",
                                    "Enter the URL for the resource in the format\n"
                                    "\":/path/to/resource\"",
                                    QLineEdit::Normal, url, &ok);

        if (!ok)
            break;

        resourceFile.setFileName(url);

        if (resourceFile.open(QFile::ReadOnly))
            break;

        QMessageBox::critical(this, "Error",
                              "Cannot find resource\n"
                              "\"" +
                                  url + "\"");
        error = true;
    }

    if (!resourceFile.isOpen())
        return;

    QFileDialog fileDialog(nullptr, "Save");
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.selectFile(QFileInfo(url).fileName());

    if (!fileDialog.exec())
        return;

    QFile outputFile(fileDialog.selectedFiles().first());
    if (!outputFile.open(QFile::WriteOnly))
    {
        QMessageBox::critical(this, "Error",
                              "Cannot save file\n"
                              "\"" +
                                  outputFile.fileName() + "\"");

        return;
    }

    outputFile.write(resourceFile.readAll());
}

void MainWindow::onExportAllResourcesTriggered()
{
    QString resourcesBaseDirPath = ":/";
    QString outputPath;

    QFileDialog fileDialog(nullptr, "Select a folder to extract the application resources into");
    fileDialog.setFileMode(QFileDialog::Directory);

    if (!fileDialog.exec())
        return;

    outputPath = fileDialog.selectedFiles().first();

    QVector<QDir> subdirs;
    subdirs << QDir(resourcesBaseDirPath);

    for (int i = 0; i < subdirs.count(); i++)
    {
        QDir currentDir = subdirs[i];
        for (const QString &subdirName : currentDir.entryList(QDir::Filter::AllDirs | QDir::NoDotAndDotDot))
        {
            QString filePath = currentDir.filePath(subdirName);
            QDir subdir(filePath);
            if (subdirs.contains(subdir))
                continue;

            subdirs << subdir;

            filePath = subdirs.first().relativeFilePath(filePath);
            filePath = QDir(outputPath).filePath(filePath);

            QDir().mkpath(filePath);
        }

        for (const QString &fileName : currentDir.entryList(QDir::Filter::Files))
        {
            QString filePath = currentDir.filePath(fileName);

            QFile currentResourceFile(filePath);
            currentResourceFile.open(QFile::ReadOnly);

            filePath = subdirs.first().relativeFilePath(filePath);
            filePath = QDir(outputPath).filePath(filePath);

            QFile outputFile(filePath);
            outputFile.open(QFile::WriteOnly);
            outputFile.write(currentResourceFile.readAll());
        }
    }
}

void MainWindow::onDebugCommand1Triggered()
{
}

void MainWindow::onDebugCommand2Triggered()
{
    // create a QML object using (uri, versionMajor, versionMinor, qmlName)

    auto createQmlObject = [](const QString &uri, int versionMajor, int versionMinor, const QString &qmlName)
    {
        QString qmlFileData = "import %1 %2.%3\n"
                              "%4{}\n";

        qmlFileData = qmlFileData.arg(uri);
        qmlFileData = qmlFileData.arg(versionMajor);
        qmlFileData = qmlFileData.arg(versionMinor);
        qmlFileData = qmlFileData.arg(qmlName);

        QQmlComponent component(MainThreadHijacker::mainWindowEngine());
        component.setData(qmlFileData.toUtf8(), QUrl());

        QObject *result = component.create();

        if (result == nullptr || component.status() != QQmlComponent::Status::Ready)
            qWarning() << component.errorString();

        return result;
    };

    QObject *result = createQmlObject("UI.MaterialSelection", 1, 0, "MaterialSelectionManager");
    m_qObjectViewer.setCurrentObject(result);
}

void MainWindow::onDebugCommand3Triggered()
{
}

void MainWindow::onDebugCommand4Triggered()
{
}

void MainWindow::onDebugCommand5Triggered()
{
}

void MainWindow::onDebugCommand6Triggered()
{
    QObject obj1;
    QObject obj2;

    connect(&obj1, &QObject::objectNameChanged, &obj2, &QObject::objectNameChanged);
    connect(&obj2, &QObject::destroyed, [](QObject *) {});

    ConnectionInspector::test(&obj1, "obj1");
    ConnectionInspector::test(&obj2, "obj2");

    obj2.setObjectName("obj2");
}

void MainWindow::onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    emit onObjectChanged();
}
