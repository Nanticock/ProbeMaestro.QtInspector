#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ObjectTreeModel.h"
#include "QObjectViewer/QObjectViewer.h"

#include <QItemSelection>
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

private:
    void loadWindowSettings();
    void saveWindowSettings();

    void closeEvent(QCloseEvent *event) override;

signals:
    void visibleChanged();

public slots:
    void onExportResourceTriggered();
    void onExportAllResourcesTriggered();
    void onRefreshHeirarchyViewTriggered();
    void onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::MainWindow *ui;

    QObjectViewer m_qObjectViewer;
    ObjectTreeModel m_objectTreeModel;
};

#endif // MAINWINDOW_H
