#ifndef POINTCLOUDRENDERER_H
#define POINTCLOUDRENDERER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include "viewportobject.h" // Add this line

class PointCloudRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum class ColorMode {
        Original,
        Unicolor,
        XGradient,
        YGradient,
        ZGradient
    };

    enum class ViewOrientation {
        Custom,
        Top,
        Front,
        Side
    };

    explicit PointCloudRenderer(QWidget *parent = nullptr);
    ~PointCloudRenderer();

    bool loadPtsFile(const QString &filename);
    bool loadPlyFile(const QString &filename);
    bool savePtsFile(const QString &filename);
    bool savePlyFile(const QString &filename);

    void resetView();
    void setTopView();
    void setFrontView();
    void setSideView();

    void setPointSize(float size);
    float getPointSize() const { return m_pointSize; }

    void setColorMode(ColorMode mode);
    ColorMode getColorMode() const { return m_colorMode; }

    void setShowCoordinateSystem(bool show);
    bool isShowingCoordinateSystem() const { return m_showCoordinateSystem; }

    void setShowBoundingBox(bool show);
    bool isShowingBoundingBox() const { return m_showBoundingBox; }

    void setPerspectiveMode(bool enabled);
    bool isPerspectiveModeEnabled() const { return m_perspectiveMode; }

    void setBackgroundColor(const QColor &color);
    QColor getBackgroundColor() const { return m_backgroundColor; }

    int getPointCount() const { return m_vertices.size(); }
    QVector3D getBoundingBoxSize() const { return m_boundingBoxMax - m_boundingBoxMin; }
    QMatrix4x4 getProjectionMatrix() const { return m_projection; }
    QMatrix4x4 getModelViewMatrix() const { return m_modelView; }
    float getCameraDistance() const { return m_distance; }
    QVector3D getRotation() const { return m_rotation; }
    QVector3D getModelCenter() const { return m_modelCenter; }

    // Corrected method declaration
    ViewportObject::ViewportParameters getViewportParameters() const;

    void setViewport(const ViewportObject::ViewportParameters& params);

    // Filter functions
    void applyPassThroughFilter(int axis, float minValue, float maxValue);
    void resetFilters();

    // Measurement tools
    void enableMeasureTool(bool enable);
    void enablePickPointTool(bool enable);
    float getMeasuredDistance() const { return m_measuredDistance; }
    QVector3D getPickedPoint() const { return m_pickedPoint; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    struct Vertex {
        QVector3D position;
        QVector3D color;
    };

    void setupShaders();
    void setupVertexBuffers();
    void updateModelViewMatrix();
    void drawCoordinateSystem(QPainter &painter);
    void drawBoundingBox();
    void drawMeasurementLine(QPainter &painter);
    void drawPickedPointInfo(QPainter &painter);
    void updateColorBuffer();
    QVector3D unprojectPoint(const QPoint &screenPos);
    bool rayIntersectsModel(const QVector3D &rayOrigin, const QVector3D &rayDirection, QVector3D &intersection);

    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    QVector<Vertex> m_vertices;
    QVector<Vertex> m_filteredVertices;
    QVector<Vertex> m_originalVertices;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_modelView;
    float m_distance;
    QVector3D m_rotation;
    QPoint m_lastMousePos;
    bool m_perspectiveMode;

    float m_pointSize;
    QVector3D m_boundingBoxMin;
    QVector3D m_boundingBoxMax;
    QVector3D m_modelCenter;
    ColorMode m_colorMode;
    QColor m_backgroundColor;
    bool m_showCoordinateSystem;
    bool m_showBoundingBox;

    bool m_measureToolEnabled;
    bool m_pickPointToolEnabled;
    bool m_isFirstPointSelected;
    QVector3D m_measurePointStart;
    QVector3D m_measurePointEnd;
    float m_measuredDistance;
    QVector3D m_pickedPoint;
};

#endif // POINTCLOUDRENDERER_H
