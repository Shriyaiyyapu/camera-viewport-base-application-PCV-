#include "pointcloudrenderer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QPaintEvent>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>

PointCloudRenderer::PointCloudRenderer(QWidget *parent)
    : QOpenGLWidget(parent),
    m_vbo(QOpenGLBuffer::VertexBuffer),
    m_distance(5.0f),
    m_pointSize(2.0f),
    m_rotation(0.0f, 0.0f, 0.0f),
    m_boundingBoxMin(0.0f, 0.0f, 0.0f),
    m_boundingBoxMax(0.0f, 0.0f, 0.0f),
    m_backgroundColor(0.1f, 0.2f, 0.3f, 1.0f)
{
    setMouseTracking(true);
}

PointCloudRenderer::~PointCloudRenderer()
{
    makeCurrent();
    m_vbo.destroy();
    m_vao.destroy();
    m_program.deleteLater();
    doneCurrent();
}

void PointCloudRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), m_backgroundColor.alphaF());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    setupShaders();
    setupVertexBuffers();

    resetView();
}

void PointCloudRenderer::setupShaders()
{
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 position;
        layout (location = 1) in vec3 color;

        uniform mat4 projection;
        uniform mat4 modelView;
        uniform float pointSize;

        out vec3 vertexColor;

        void main()
        {
            gl_Position = projection * modelView * vec4(position, 1.0);
            gl_PointSize = pointSize;
            vertexColor = color;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 fragColor;

        void main()
        {
            vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
            if (dot(circCoord, circCoord) > 1.0) {
                discard;
            }
            fragColor = vec4(vertexColor, 1.0);
        }
    )";

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qDebug() << "Failed to compile vertex shader";
    }

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qDebug() << "Failed to compile fragment shader";
    }

    if (!m_program.link()) {
        qDebug() << "Failed to link shader program";
    }
}

void PointCloudRenderer::setupVertexBuffers()
{
    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);

    if (!m_vertices.isEmpty()) {
        m_vbo.allocate(m_vertices.constData(), m_vertices.size() * sizeof(Vertex));
    }

    m_program.bind();

    m_program.enableAttributeArray(0);
    m_program.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));

    m_program.enableAttributeArray(1);
    m_program.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, sizeof(Vertex));

    m_program.release();
    m_vao.release();
}

void PointCloudRenderer::resizeGL(int w, int h)
{
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, float(w) / float(h), 0.01f, 1000.0f);
}

void PointCloudRenderer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_vertices.isEmpty()) {
        m_program.bind();
        m_vao.bind();

        updateModelViewMatrix();

        m_program.setUniformValue("projection", m_projection);
        m_program.setUniformValue("modelView", m_modelView);
        m_program.setUniformValue("pointSize", m_pointSize);

        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_vertices.size()));

        m_vao.release();
        m_program.release();
    }
}

void PointCloudRenderer::paintEvent(QPaintEvent *event)
{
    QOpenGLWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_showCoordinateSystem) {
        drawCoordinateSystem(painter);
    }
}

void PointCloudRenderer::drawCoordinateSystem(QPainter &painter)
{
    const int csysSize = 80;
    const int margin = 10;

    const int xPos = width() - csysSize - margin;
    const int yPos = height() - csysSize - margin;

    const int centerX = xPos + csysSize / 2;
    const int centerY = yPos + csysSize / 2;

    const int axisLength = csysSize / 3;

    QMatrix4x4 rotMatrix;
    rotMatrix.rotate(m_rotation.x(), 1.0f, 0.0f, 0.0f);
    rotMatrix.rotate(m_rotation.y(), 0.0f, 1.0f, 0.0f);

    QVector3D xAxis(axisLength, 0, 0);
    QVector3D yAxis(0, axisLength, 0);
    QVector3D zAxis(0, 0, axisLength);

    xAxis = rotMatrix.map(xAxis);
    yAxis = rotMatrix.map(yAxis);
    zAxis = rotMatrix.map(zAxis);

    QPen xPen(QColor(255, 0, 0));
    xPen.setWidth(2);
    painter.setPen(xPen);
    painter.drawLine(centerX, centerY, centerX + xAxis.x(), centerY - xAxis.y());

    QPen yPen(QColor(0, 255, 0));
    yPen.setWidth(2);
    painter.setPen(yPen);
    painter.drawLine(centerX, centerY, centerX + yAxis.x(), centerY - yAxis.y());

    QPen zPen(QColor(0, 0, 255));
    zPen.setWidth(2);
    painter.setPen(zPen);
    painter.drawLine(centerX, centerY, centerX + zAxis.x(), centerY - zAxis.y());

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(8);
    painter.setFont(font);

    painter.setPen(QColor(255, 0, 0));
    painter.drawText(centerX + xAxis.x() + 5, centerY - xAxis.y() + 5, "X");

    painter.setPen(QColor(0, 255, 0));
    painter.drawText(centerX + yAxis.x() + 5, centerY - yAxis.y() + 5, "Y");

    painter.setPen(QColor(0, 0, 255));
    painter.drawText(centerX + zAxis.x() + 5, centerY - zAxis.y() + 5, "Z");

    const int scaleLength = 60;
    const int scaleY = yPos + csysSize - 5;

    painter.setPen(QColor(255, 255, 255));
    painter.drawLine(xPos + 10, scaleY, xPos + 10 + scaleLength, scaleY);

    painter.drawLine(xPos + 10, scaleY - 2, xPos + 10, scaleY + 2);
    painter.drawLine(xPos + 10 + scaleLength, scaleY - 2, xPos + 10 + scaleLength, scaleY + 2);

    painter.drawText(xPos + 10 + scaleLength / 2 - 10, scaleY - 5, "0.2");
}

bool PointCloudRenderer::loadPtsFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open .pts file:" << filename;
        return false;
    }

    m_vertices.clear();

    m_boundingBoxMin = QVector3D(std::numeric_limits<float>::max(),
                                 std::numeric_limits<float>::max(),
                                 std::numeric_limits<float>::max());
    m_boundingBoxMax = QVector3D(std::numeric_limits<float>::lowest(),
                                 std::numeric_limits<float>::lowest(),
                                 std::numeric_limits<float>::lowest());

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() >= 3) {
            float x = parts[0].toFloat();
            float y = parts[1].toFloat();
            float z = parts[2].toFloat();

            QVector3D pos(x, y, z);
            QVector3D color(1.0f, 1.0f, 1.0f);

            if (parts.size() >= 6) {
                color = QVector3D(
                    parts[3].toFloat() / 255.0f,
                    parts[4].toFloat() / 255.0f,
                    parts[5].toFloat() / 255.0f
                    );
            }

            Vertex vertex = {pos, color};
            m_vertices.append(vertex);

            m_boundingBoxMin.setX(std::min(m_boundingBoxMin.x(), x));
            m_boundingBoxMin.setY(std::min(m_boundingBoxMin.y(), y));
            m_boundingBoxMin.setZ(std::min(m_boundingBoxMin.z(), z));

            m_boundingBoxMax.setX(std::max(m_boundingBoxMax.x(), x));
            m_boundingBoxMax.setY(std::max(m_boundingBoxMax.y(), y));
            m_boundingBoxMax.setZ(std::max(m_boundingBoxMax.z(), z));
        }
    }

    file.close();

    m_modelCenter = (m_boundingBoxMin + m_boundingBoxMax) * 0.5f;
    QVector3D size = m_boundingBoxMax - m_boundingBoxMin;
    m_distance = size.length() * 1.5f;

    makeCurrent();
    setupVertexBuffers();
    doneCurrent();

    resetView();
    update();

    qDebug() << "Loaded" << m_vertices.size() << "points from .pts file";
    return true;
}

bool PointCloudRenderer::loadPlyFile(const QString &filename)
{
    std::ifstream file(filename.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        qDebug() << "Failed to open .ply file:" << filename;
        return false;
    }

    m_vertices.clear();

    m_boundingBoxMin = QVector3D(std::numeric_limits<float>::max(),
                                 std::numeric_limits<float>::max(),
                                 std::numeric_limits<float>::max());
    m_boundingBoxMax = QVector3D(std::numeric_limits<float>::lowest(),
                                 std::numeric_limits<float>::lowest(),
                                 std::numeric_limits<float>::lowest());

    std::string line;
    int numVertices = 0;
    bool isBinary = false;
    bool isBigEndian = false;
    bool hasColors = false;
    bool headerEnd = false;

    enum PropertyType { PROP_NONE, PROP_FLOAT, PROP_UCHAR };
    struct PropertyInfo {
        std::string name;
        PropertyType type;
    };
    std::vector<PropertyInfo> properties;
    int xIndex = -1, yIndex = -1, zIndex = -1;
    int redIndex = -1, greenIndex = -1, blueIndex = -1;

    while (std::getline(file, line)) {
        std::istringstream ls(line);
        std::string keyword;
        ls >> keyword;

        if (keyword == "end_header") {
            headerEnd = true;
            break;
        } else if (keyword == "format") {
            std::string format, version;
            ls >> format >> version;
            if (format == "ascii") {
                isBinary = false;
            } else if (format == "binary_little_endian") {
                isBinary = true;
                isBigEndian = false;
            } else if (format == "binary_big_endian") {
                isBinary = true;
                isBigEndian = true;
                qDebug() << "Warning: Big endian binary PLY files might not be correctly supported.";
            }
        } else if (keyword == "element" && line.find("vertex") != std::string::npos) {
            ls >> keyword >> numVertices;
        } else if (keyword == "property") {
            std::string type, name;
            ls >> type >> name;

            PropertyInfo prop;
            prop.name = name;

            if (type == "float" || type == "float32") {
                prop.type = PROP_FLOAT;
            } else if (type == "uchar" || type == "uint8") {
                prop.type = PROP_UCHAR;
            } else {
                prop.type = PROP_NONE;
            }

            if (prop.type != PROP_NONE) {
                int propIndex = properties.size();
                if (name == "x") xIndex = propIndex;
                else if (name == "y") yIndex = propIndex;
                else if (name == "z") zIndex = propIndex;
                else if (name == "red") {
                    redIndex = propIndex;
                    hasColors = true;
                }
                else if (name == "green") greenIndex = propIndex;
                else if (name == "blue") blueIndex = propIndex;

                properties.push_back(prop);
            }
        }
    }

    if (!headerEnd || xIndex == -1 || yIndex == -1 || zIndex == -1) {
        qDebug() << "Invalid PLY file format or missing coordinate properties";
        file.close();
        return false;
    }

    m_vertices.reserve(numVertices);

    if (!isBinary) {
        for (int i = 0; i < numVertices; i++) {
            std::getline(file, line);
            std::istringstream ss(line);

            std::vector<float> values(properties.size(), 0.0f);

            for (size_t j = 0; j < properties.size(); j++) {
                if (properties[j].type == PROP_FLOAT) {
                    float val;
                    ss >> val;
                    values[j] = val;
                } else if (properties[j].type == PROP_UCHAR) {
                    int val;
                    ss >> val;
                    values[j] = static_cast<float>(val);
                }
            }

            float x = values[xIndex];
            float y = values[yIndex];
            float z = values[zIndex];

            QVector3D pos(x, y, z);
            QVector3D color(1.0f, 1.0f, 1.0f);

            if (hasColors && redIndex != -1 && greenIndex != -1 && blueIndex != -1) {
                float r = values[redIndex];
                float g = values[greenIndex];
                float b = values[blueIndex];

                if (properties[redIndex].type == PROP_UCHAR) {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                }

                color = QVector3D(r, g, b);
            }

            Vertex vertex = {pos, color};
            m_vertices.append(vertex);

            m_boundingBoxMin.setX(std::min(m_boundingBoxMin.x(), x));
            m_boundingBoxMin.setY(std::min(m_boundingBoxMin.y(), y));
            m_boundingBoxMin.setZ(std::min(m_boundingBoxMin.z(), z));

            m_boundingBoxMax.setX(std::max(m_boundingBoxMax.x(), x));
            m_boundingBoxMax.setY(std::max(m_boundingBoxMax.y(), y));
            m_boundingBoxMax.setZ(std::max(m_boundingBoxMax.z(), z));
        }
    } else {
        const int MAX_PROPS = 32;
        char buffer[MAX_PROPS * sizeof(float)];

        for (int i = 0; i < numVertices; i++) {
            std::vector<float> values(properties.size(), 0.0f);

            for (size_t j = 0; j < properties.size(); j++) {
                if (properties[j].type == PROP_FLOAT) {
                    float val;
                    file.read(reinterpret_cast<char*>(&val), sizeof(float));
                    values[j] = val;
                } else if (properties[j].type == PROP_UCHAR) {
                    unsigned char val;
                    file.read(reinterpret_cast<char*>(&val), sizeof(unsigned char));
                    values[j] = static_cast<float>(val);
                }
            }

            if (file.fail()) {
                qDebug() << "Error reading binary PLY data";
                file.close();
                return false;
            }

            float x = values[xIndex];
            float y = values[yIndex];
            float z = values[zIndex];

            QVector3D pos(x, y, z);
            QVector3D color(1.0f, 1.0f, 1.0f);

            if (hasColors && redIndex != -1 && greenIndex != -1 && blueIndex != -1) {
                float r = values[redIndex];
                float g = values[greenIndex];
                float b = values[blueIndex];

                if (properties[redIndex].type == PROP_UCHAR) {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                }

                color = QVector3D(r, g, b);
            }

            Vertex vertex = {pos, color};
            m_vertices.append(vertex);

            m_boundingBoxMin.setX(std::min(m_boundingBoxMin.x(), x));
            m_boundingBoxMin.setY(std::min(m_boundingBoxMin.y(), y));
            m_boundingBoxMin.setZ(std::min(m_boundingBoxMin.z(), z));

            m_boundingBoxMax.setX(std::max(m_boundingBoxMax.x(), x));
            m_boundingBoxMax.setY(std::max(m_boundingBoxMax.y(), y));
            m_boundingBoxMax.setZ(std::max(m_boundingBoxMax.z(), z));
        }
    }

    file.close();

    m_modelCenter = (m_boundingBoxMin + m_boundingBoxMax) * 0.5f;
    QVector3D size = m_boundingBoxMax - m_boundingBoxMin;
    m_distance = size.length() * 1.5f;

    makeCurrent();
    setupVertexBuffers();
    doneCurrent();

    resetView();
    update();

    qDebug() << "Loaded" << m_vertices.size() << "points from .ply file";
    return true;
}

void PointCloudRenderer::setViewport(const ViewportObject::ViewportParameters& params)
{
    m_modelView = params.modelViewMatrix;
    m_projection = params.projectionMatrix;
    m_distance = params.cameraDistance;
    m_rotation = params.rotation;
    m_modelCenter = params.modelCenter;
    m_pointSize = params.pointSize;
    m_boundingBoxMin = params.boundingBoxMin;
    m_boundingBoxMax = params.boundingBoxMax;

    updateModelViewMatrix();
}

void PointCloudRenderer::resetView()
{
    m_rotation = QVector3D(0.0f, 0.0f, 0.0f);
    updateModelViewMatrix();
    update();
}

void PointCloudRenderer::updateModelViewMatrix()
{
    m_modelView.setToIdentity();

    m_modelView.translate(0.0f, 0.0f, -m_distance);
    m_modelView.rotate(m_rotation.x(), 1.0f, 0.0f, 0.0f);
    m_modelView.rotate(m_rotation.y(), 0.0f, 1.0f, 0.0f);
    m_modelView.translate(-m_modelCenter);
}

ViewportObject::ViewportParameters PointCloudRenderer::getViewportParameters() const
{
    ViewportObject::ViewportParameters params;
    params.modelViewMatrix = m_modelView;
    params.projectionMatrix = m_projection;
    params.cameraDistance = m_distance;
    params.rotation = m_rotation;
    params.modelCenter = m_modelCenter;
    params.pointSize = m_pointSize;
    params.boundingBoxMin = m_boundingBoxMin;
    params.boundingBoxMax = m_boundingBoxMax;
    return params;
}

void PointCloudRenderer::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
}

void PointCloudRenderer::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_rotation.setY(m_rotation.y() + delta.x() * 0.5f);
        m_rotation.setX(m_rotation.x() + delta.y() * 0.5f);

        updateModelViewMatrix();
        update();
    }

    m_lastMousePos = event->pos();
}

void PointCloudRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    QOpenGLWidget::mouseReleaseEvent(event);
}

void PointCloudRenderer::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    m_distance *= std::pow(0.9f, delta);

    m_distance = std::max(0.1f, std::min(m_distance, 1000.0f));

    updateModelViewMatrix();
    update();
}
