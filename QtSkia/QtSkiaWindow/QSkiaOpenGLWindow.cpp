#include "QSkiaOpenGLWindow.h"

#include "core/SkImageInfo.h"
#include "core/SkSurface.h"
#include "gpu/GrContext.h"

#include <QOpenGLFunctions>
#include <QTime>
#include <QTimer>
#include <QDebug>
class QSkiaOpenGLWindowPrivate {
public:
    QOpenGLFunctions funcs;
    sk_sp<GrContext> context = nullptr;
    sk_sp<SkSurface> gpuSurface = nullptr;
    SkImageInfo info;
    QTimer timer;
    QTime lastTime;
    int oldW;
    int oldH;
};
QSkiaOpenGLWindow::QSkiaOpenGLWindow(QWindow* parent)
    : QOpenGLWindow(QOpenGLWindow::UpdateBehavior::NoPartialUpdate, parent)
    , m_dptr(new QSkiaOpenGLWindowPrivate)
{
    connect(&m_dptr->timer, &QTimer::timeout, this, QOverload<>::of(&QSkiaOpenGLWindow::update));
    m_dptr->timer.start(1000 / 60);
}

QSkiaOpenGLWindow::~QSkiaOpenGLWindow()
{
    makeCurrent();
    delete m_dptr;
    m_dptr = nullptr;
    doneCurrent();
}

void QSkiaOpenGLWindow::initializeGL()
{
    m_dptr->funcs.initializeOpenGLFunctions();
    m_dptr->context = GrContext::MakeGL();
    SkASSERT(m_dptr->context);
    init(this->width(), this->height());
    onInit(this->width(), this->height());
    m_dptr->lastTime = QTime::currentTime();
    m_dptr->oldW = width();
    m_dptr->oldW = height();
}

void QSkiaOpenGLWindow::resizeGL(int w, int h)
{
    if (w == m_dptr->oldW && h == m_dptr->oldH) {
        return;
    }
    //TODO 直接换Surface会闪烁，需要优化为快速切换。
    init(w, h);
    onResize(w, h);
}

void QSkiaOpenGLWindow::init(int w, int h)
{
    qWarning() << __FUNCTION__ << w << h;
    m_dptr->info = SkImageInfo::MakeN32Premul(w, h);
    m_dptr->gpuSurface = SkSurface::MakeRenderTarget(m_dptr->context.get(), SkBudgeted::kNo, m_dptr->info);
    if (!m_dptr->gpuSurface) {
        SkDebugf("SkSurface::MakeRenderTarget return null\n");
        return;
    }
    m_dptr->funcs.glViewport(0, 0, w, h);
}

void QSkiaOpenGLWindow::paintGL()
{
    if (!this->isVisible()) {
        return;
    }
    if (!m_dptr->gpuSurface) {
        return;
    }
    auto canvas = m_dptr->gpuSurface->getCanvas();
    if (!canvas) {
        return;
    }
    const auto elapsed = m_dptr->lastTime.elapsed();
    m_dptr->lastTime = QTime::currentTime();
    canvas->save();
    this->draw(canvas, elapsed);
    canvas->restore();
}
