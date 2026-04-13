#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QObjectViewer/ObjectLocator/ObjectLocator.h>

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

static const char m_settingsGroupName[] = "probemaestro.qt_inspector.gui.main_window";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), m_heirarchyViewToolbar(this)
{
    ui->setupUi(this);

    initializeHeirarchyView();
    ui->dockWidget2->setWidget(&m_qObjectViewer);

    // menu connections
    ui->actionWindows->setMenu(new QMenu());
    ui->actionWindows->menu()->addAction(ui->dockWidget1->toggleViewAction());
    ui->actionWindows->menu()->addAction(ui->dockWidget2->toggleViewAction());

    connect(ui->actionExport_resource, &QAction::triggered, this, &MainWindow::onExportResourceTriggered);
    connect(ui->actionExport_all_resources, &QAction::triggered, this, &MainWindow::onExportAllResourcesTriggered);
    connect(ui->actionRefresh_hierarchy_view, &QAction::triggered, this, &MainWindow::onRefreshHeirarchyViewTriggered);
    connect(this, &MainWindow::visibleChanged, this,
            [this]()
            {
                //
                m_objectTreeModel.refresh();
            });

    loadWindowSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QObject *MainWindow::selectedObject()
{
    const QModelIndex index = ui->WindowChildrenTreeView->currentIndex();

    if (!index.isValid())
        return nullptr;

    // Handle proxy model if present
    auto *proxy = qobject_cast<QSortFilterProxyModel *>(ui->WindowChildrenTreeView->model());

    QModelIndex sourceIndex = proxy ? proxy->mapToSource(index) : index;

    return sourceIndex.data(ObjectTreeModel::ObjectRole).value<QObject *>();
}

QKeySequence MainWindow::globalDisplayKeySequence() const
{
    static const QKeySequence result(Qt::CTRL, Qt::ALT, Qt::Key_Q);

    return result;
}

void MainWindow::loadWindowSettings()
{
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
    QSettings settings;

    settings.beginGroup(m_settingsGroupName);
    {
        settings.setValue("geometry", saveGeometry());
        settings.setValue("state", saveState());
    }
    settings.endGroup();
}

void MainWindow::initializeHeirarchyView()
{
    ui->WindowChildrenTreeView->setHeaderHidden(true);
    ui->WindowChildrenTreeView->setModel(&m_objectTreeModel);
    ui->WindowChildrenTreeView->setRootIndex({});

    connect(ui->WindowChildrenTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onTreeViewSelectionChanged);

    // --- Toolbar setup ---
    m_heirarchyViewToolbar.setParent(ui->dockWidget1);
    m_heirarchyViewToolbar.setMovable(false);
    m_heirarchyViewToolbar.setFloatable(false);
    m_heirarchyViewToolbar.addAction(ui->actionRefresh_hierarchy_view);

    // --- Container ---
    auto *container = new QWidget(ui->dockWidget1);
    auto *layout = new QVBoxLayout(container);

    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(&m_heirarchyViewToolbar);
    layout->addWidget(ui->WindowChildrenTreeView);

    ui->dockWidget1->setWidget(container);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowSettings();
}

void MainWindow::onRefreshHeirarchyViewTriggered()
{
    m_objectTreeModel.refresh();
}

void MainWindow::onExportResourceTriggered()
{
    QString url;
    QFile resourceFile;

    while (true) // Ask until valid or canceled
    {
        bool ok = false;
        url = QInputDialog::getText(this, "Resource URL", "Enter the URL for the resource in the format\n\":/path/to/resource\"", QLineEdit::Normal,
                                    url, &ok);

        if (!ok)
            return;

        resourceFile.setFileName(url);

        if (resourceFile.open(QFile::ReadOnly))
            break;

        QMessageBox::critical(this, "Error", "Cannot find resource\n\"" + url + "\"");
    }

    const QString fileName = QFileDialog::getSaveFileName(this, "Save", QFileInfo(url).fileName());

    if (fileName.isEmpty())
        return;

    QFile outputFile(fileName);
    if (!outputFile.open(QFile::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Cannot save file\n\"" + outputFile.fileName() + "\"");
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

void MainWindow::onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    m_qObjectViewer.setCurrentObject(selectedObject());
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    emit visibleChanged();
}
