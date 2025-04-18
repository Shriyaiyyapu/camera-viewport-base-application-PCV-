#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>

static unsigned s_viewportIndex = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_renderer = ui->rendererWidget;
    m_dbDockWidget = ui->dbDockWidget;
    m_dbTreeWidget = ui->dbTreeWidget;
    m_dbTreeWidget->setHeaderHidden(true);
    addDockWidget(Qt::LeftDockWidgetArea, m_dbDockWidget);
    setupActions();

    connect(m_dbTreeWidget, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onTreeWidgetItemDoubleClicked);
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_viewportList);
    delete ui;
}

void MainWindow::setupActions()
{
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openPointCloudFile);
    connect(ui->actionResetView, &QAction::triggered, this, &MainWindow::resetView);
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionSave_Viewport_As_Object, &QAction::triggered, this, &MainWindow::doActionSaveViewportAsObject);
    connect(ui->actionSave_Viewport_with_User_defined_co_ords, &QAction::triggered, this, &MainWindow::doActionSaveViewportWithUserCoords);

    m_openAction = ui->actionOpen;
    m_resetViewAction = ui->actionResetView;
    m_saveViewportAction = ui->actionSave_Viewport_As_Object;
    m_saveViewportWithCoordsAction = ui->actionSave_Viewport_with_User_defined_co_ords;
}

void MainWindow::openPointCloudFile()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Open Point Cloud File",
        QString(),
        "Point Cloud Files (*.pts *.ply);;All Files (*.*)"
        );

    if (filename.isEmpty()) {
        return;
    }

    bool success = false;

    if (filename.endsWith(".pts", Qt::CaseInsensitive)) {
        success = m_renderer->loadPtsFile(filename);
    } else if (filename.endsWith(".ply", Qt::CaseInsensitive)) {
        success = m_renderer->loadPlyFile(filename);
    } else {
        QMessageBox::warning(this, "Unsupported Format", "The selected file format is not supported.");
        return;
    }

    if (!success) {
        QMessageBox::warning(this, "Load Error", "Failed to load the point cloud file.");
    } else {
        m_renderer->update();
    }
}

void MainWindow::resetView()
{
    m_renderer->resetView();
    m_renderer->update();
}

void MainWindow::doActionSaveViewportAsObject()
{
    if (!m_renderer || m_renderer->getPointCount() == 0) {
        return;
    }

    ViewportObject* viewportObject = new ViewportObject(QString("Viewport #%1").arg(++s_viewportIndex));
    viewportObject->setParameters(m_renderer->getViewportParameters());
    viewportObject->setDisplay(m_renderer);

    addToDB(viewportObject);
}

void MainWindow::doActionSaveViewportWithUserCoords()
{
    if (!m_renderer || m_renderer->getPointCount() == 0) {
        QMessageBox::warning(this, "Warning", "No point cloud loaded.");
        return;
    }

    // Get current viewport parameters
    ViewportObject::ViewportParameters params = m_renderer->getViewportParameters();
    QVector3D currentCameraCenter = params.modelCenter;

    // Get bounding box for scene size
    QVector3D bboxSize = m_renderer->getBoundingBoxSize();
    float sceneSize = bboxSize.length();

    if (sceneSize <= 0.0f) {
        QMessageBox::warning(this, "Warning", "No valid bounding box detected in the scene.");
        return;
    }

    // Show current camera coordinates
    QMessageBox::information(this, "Current Camera Position",
                             QString("Current Camera Position:\nX: %1\nY: %2\nZ: %3")
                                 .arg(currentCameraCenter.x(), 0, 'f', 2)
                                 .arg(currentCameraCenter.y(), 0, 'f', 2)
                                 .arg(currentCameraCenter.z(), 0, 'f', 2));

    // Ask user for new camera coordinates
    bool ok;
    double x = QInputDialog::getDouble(this, "Enter New X", "New X coordinate:", currentCameraCenter.x(), -10000, 10000, 2, &ok);
    if (!ok) return;
    double y = QInputDialog::getDouble(this, "Enter New Y", "New Y coordinate:", currentCameraCenter.y(), -10000, 10000, 2, &ok);
    if (!ok) return;
    double z = QInputDialog::getDouble(this, "Enter New Z", "New Z coordinate:", currentCameraCenter.z(), -10000, 10000, 2, &ok);
    if (!ok) return;

    // Update viewport parameters with new camera center
    params.modelCenter = QVector3D(x, y, z);
    params.cameraDistance = sceneSize * 1.5f; // Adjust distance based on scene size

    // Apply updated parameters to renderer
    m_renderer->setViewport(params);
    m_renderer->update();

    // Create and save new viewport object
    ViewportObject* viewportObject = new ViewportObject(QString("Viewport #%1").arg(++s_viewportIndex));
    viewportObject->setParameters(params);
    viewportObject->setDisplay(m_renderer);

    addToDB(viewportObject);
}

void MainWindow::addToDB(ViewportObject* viewport)
{
    m_viewportList.append(viewport);
    updateTreeWidget(viewport);
}

void MainWindow::updateTreeWidget(ViewportObject* viewport)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(m_dbTreeWidget);
    item->setText(0, viewport->getName());
    item->setData(0, Qt::UserRole, QVariant::fromValue(viewport));
    m_dbTreeWidget->addTopLevelItem(item);
    m_dbTreeWidget->expandAll();
}

void MainWindow::onTreeWidgetItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    QVariant itemData = item->data(0, Qt::UserRole);
    if (itemData.isValid()) {
        ViewportObject* viewport = itemData.value<ViewportObject*>();
        if (viewport) {
            viewport->applyViewport(m_renderer);
        }
    }
}
