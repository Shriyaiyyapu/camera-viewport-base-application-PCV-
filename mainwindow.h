#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidget>
#include <QDockWidget>
#include "pointcloudrenderer.h"
#include "viewportobject.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openPointCloudFile();
    void resetView();
    void doActionSaveViewportAsObject();
    void doActionSaveViewportWithUserCoords();
    void onTreeWidgetItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void setupActions();
    void addToDB(ViewportObject* viewport);
    void updateTreeWidget(ViewportObject* viewport);

    Ui::MainWindow *ui;
    PointCloudRenderer *m_renderer;
    QAction *m_openAction;
    QAction *m_resetViewAction;
    QAction *m_saveViewportAction;
    QAction *m_saveViewportWithCoordsAction;
    QList<ViewportObject*> m_viewportList;
    QTreeWidget *m_dbTreeWidget;
    QDockWidget *m_dbDockWidget;
};

#endif // MAINWINDOW_H
