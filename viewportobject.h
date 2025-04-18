#ifndef VIEWPORTOBJECT_H
#define VIEWPORTOBJECT_H

#include <QMatrix4x4>
#include <QVector3D>
#include <QString>

// Forward declaration
class PointCloudRenderer;

class ViewportObject
{
public:
    struct ViewportParameters {
        QMatrix4x4 modelViewMatrix;
        QMatrix4x4 projectionMatrix;
        float cameraDistance;
        QVector3D rotation;
        QVector3D modelCenter;
        float pointSize;
        QVector3D boundingBoxMin;
        QVector3D boundingBoxMax;
    };

    ViewportObject(const QString& name = "Viewport");
    ~ViewportObject();

    void setParameters(const ViewportParameters& params);
    void setDisplay(PointCloudRenderer* display) { m_display = display; }
    PointCloudRenderer* getDisplay() const { return m_display; }
    const ViewportParameters& getParameters() const { return m_params; }
    QString getName() const { return m_name; }

    // Apply this viewport to the provided renderer
    void applyViewport(PointCloudRenderer* renderer) const;

private:
    ViewportParameters m_params;
    PointCloudRenderer* m_display; // Weak reference to the renderer (for potential restoration)
    QString m_name;
};

#endif // VIEWPORTOBJECT_H
