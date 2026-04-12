#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ObjectTreeModel.h"
#include "QObjectViewer/QObjectViewer.h"

#include <QItemSelection>
#include <QKeySequence>
#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QObject *selectedObject();
    QKeySequence globalDisplayKeySequence() const;

private:
    void loadWindowSettings();
    void saveWindowSettings();
    void initializeHeirarchyView();

    void closeEvent(QCloseEvent *event) override;

signals:
    void visibleChanged();

public slots:
    void onExportResourceTriggered();
    void onExportAllResourcesTriggered();
    void onRefreshHeirarchyViewTriggered();
    void onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::MainWindow *ui;

    // TODO: Move into a aseparate component, and add filtering options for
    //          - Filtering objects by name
    //          - Filtering objects by type
    //          - Hiding invisible objects
    //          - Hiding the window that contains the component
    QToolBar m_heirarchyViewToolbar;
    ObjectTreeModel m_objectTreeModel;

    QObjectViewer m_qObjectViewer;
};

#endif // MAINWINDOW_H
