#include "viewportobject.h"
#include "pointcloudrenderer.h"

ViewportObject::ViewportObject(const QString& name)
    : m_display(nullptr)
    , m_name(name)
{
    m_params.projectionMatrix.setToIdentity();
    m_params.projectionMatrix.perspective(45.0f, 1.0f, 0.01f, 1000.0f);
    m_params.modelViewMatrix.setToIdentity();
    m_params.cameraDistance = 5.0f;
    m_params.rotation = QVector3D(0.0f, 0.0f, 0.0f);
    m_params.modelCenter = QVector3D(0.0f, 0.0f, 0.0f);
    m_params.pointSize = 2.0f;
    m_params.boundingBoxMin = QVector3D(0.0f, 0.0f, 0.0f);
    m_params.boundingBoxMax = QVector3D(0.0f, 0.0f, 0.0f);
}

ViewportObject::~ViewportObject()
{
}

void ViewportObject::setParameters(const ViewportParameters& params)
{
    m_params = params;
}

void ViewportObject::applyViewport(PointCloudRenderer* renderer) const
{
    if (renderer) {
        // Apply the saved viewport parameters to the renderer
        renderer->setViewport(m_params);
        renderer->update();
    }
}
