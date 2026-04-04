#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QObjectViewer/QObjectViewer.h"
#include "WindowChildrenTreeModel/WindowChildrenTreeItem.h"
#include "WindowChildrenTreeModel/WindowChildrenTreeModel.h"

#include <QItemSelection>
#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

// For the *.ui file Refer to:
// https://sl.bing.net/e4aomzbCBky
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QObject *rootObject();
    void setRootObject(QObject *value);

    QObject *selectedObject();

private:
    void loadWindowSettings();
    void saveWindowSettings();

    void closeEvent(QCloseEvent *event) override;

public slots:
    void onObjectChanged();
    void onInvokeActionTriggered();
    void onListAllActionsTriggered();
    void onExportResourceTriggered();
    void onExportAllResourcesTriggered();
    void onDebugCommand1Triggered();
    void onDebugCommand2Triggered();
    void onDebugCommand3Triggered();
    void onDebugCommand4Triggered();
    void onDebugCommand5Triggered();
    void onDebugCommand6Triggered();
    void onObjectViewerObjectChanged();
    void onRefreshHeirarchyViewTriggered();
    void onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    Ui::MainWindow *ui;

    QObjectViewer m_qObjectViewer;
    WindowChildrenTreeModel m_windowChildrenTreeModel;
    WindowChildrenTreeItem m_windowChildrenTreeRootItem;
};

#endif // MAINWINDOW_H
