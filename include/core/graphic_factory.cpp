#include "graphic_factory.h"
#include "draw_strategy.h"
#include <stdexcept>

class ConcreteGraphic : public Graphic {
public:
    ConcreteGraphic(Graphic::GraphicType type, const std::vector<QPointF>& points) 
        : m_type(type), m_points(points) {
        // 修改 graphic_factory.cpp 中 ConcreteGraphic 的构造函数
        switch (m_type) {
            case Graphic::LINE:
                m_drawStrategy = std::make_shared<LineDrawStrategy>();
                break;
            case Graphic::RECTANGLE:
                m_drawStrategy = std::make_shared<RectangleDrawStrategy>();
                break;
            case Graphic::CIRCLE:
                m_drawStrategy = std::make_shared<CircleDrawStrategy>();
                break;
            case Graphic::ELLIPSE:
                m_drawStrategy = std::make_shared<EllipseDrawStrategy>();
                break;
            case Graphic::BEZIER:
                m_drawStrategy = std::make_shared<BezierDrawStrategy>();
                break;
            case Graphic::FLOWCHART_NODE:
                m_drawStrategy = std::make_shared<FlowchartNodeDrawStrategy>();
                break;
            default:
                throw std::runtime_error("Unsupported graphic type");
        }
    }

    void draw(QPainter& painter) const override {
        if (m_drawStrategy) {
            m_drawStrategy->draw(painter, m_points);
        }
    }

    void move(const QPointF& offset) override {
        for (auto& point : m_points) {
            point += offset;
        }
    }

    void rotate(double angle) override {
        QPointF center = getCenter();
        for (auto& point : m_points) {
            QPointF translated = point - center;
            double radians = angle * M_PI / 180.0;
            QPointF rotated(
                translated.x() * std::cos(radians) - translated.y() * std::sin(radians),
                translated.x() * std::sin(radians) + translated.y() * std::cos(radians)
            );
            point = rotated + center;
        }
    }

    void scale(double factor) override {
        QPointF center = getCenter();
        for (auto& point : m_points) {
            point = center + (point - center) * factor;
        }
    }

    QPointF getCenter() const override {
        if (m_points.empty()) return QPointF();
        
        QPointF sum(0, 0);
        for (const auto& point : m_points) {
            sum += point;
        }
        return sum / m_points.size();
    }

    GraphicType getType() const override {
        return m_type;
    }

    QRectF getBoundingBox() const override {
        if (m_points.empty()) return QRectF();
        
        double minX = m_points[0].x();
        double maxX = m_points[0].x();
        double minY = m_points[0].y();
        double maxY = m_points[0].y();
        
        for (const auto& point : m_points) {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());
            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }
        
        return QRectF(minX, minY, maxX - minX, maxY - minY);
    }

    void setDrawStrategy(std::shared_ptr<DrawStrategy> strategy) override {
        m_drawStrategy = strategy;
    }

    std::unique_ptr<Graphic> clone() const override {
        auto clonedGraphic = std::make_unique<ConcreteGraphic>(*this);
        return clonedGraphic;
    }

    void drawEllipse(const QPointF& center, double width, double height) {
        // 使用 QPainter 绘制椭圆
        QPainter painter; // 需要传入合适的 QPainter 对象
        painter.drawEllipse(center, width / 2, height / 2);
    }

private:
    GraphicType m_type;
    std::vector<QPointF> m_points;
    std::shared_ptr<DrawStrategy> m_drawStrategy;
};

std::unique_ptr<Graphic> DefaultGraphicFactory::createGraphic(Graphic::GraphicType type) {
    switch(type) {
        case Graphic::LINE: {
            std::vector<QPointF> points = {
                QPointF(0, 0), 
                QPointF(100, 100)
            };
            return std::make_unique<ConcreteGraphic>(type, points);
        }
        case Graphic::RECTANGLE: {
            std::vector<QPointF> points = {
                QPointF(0, 0), 
                QPointF(100, 100)
            };
            return std::make_unique<ConcreteGraphic>(type, points);
        }
        case Graphic::CIRCLE: {
            std::vector<QPointF> points = {
                QPointF(50, 50),  
                QPointF(100, 50)  
            };
            return std::make_unique<ConcreteGraphic>(type, points);
        }
        case Graphic::ELLIPSE: {
            std::vector<QPointF> points = {
                QPointF(50, 50),  
                QPointF(100, 50)  
            };
            return std::make_unique<ConcreteGraphic>(type, points);
        }
        case Graphic::TRIANGLE: {
            std::vector<QPointF> points = {
                QPointF(50, 0), 
                QPointF(0, 100), 
                QPointF(100, 100)
            };
            return std::make_unique<ConcreteGraphic>(type, points);
        }
        case Graphic::FLOWCHART_NODE: {
            std::vector<QPointF> points = {
                QPointF(50, 50),  
                QPointF(100, 50)  
            };
            auto graphic = std::make_unique<ConcreteGraphic>(type, points);
            graphic->setDrawStrategy(std::make_shared<FlowchartNodeDrawStrategy>());
            return graphic;
        }
        case Graphic::BEZIER: {
            return std::make_unique<ConcreteGraphic>(type, std::vector<QPointF>());
        }
        default:
            throw std::runtime_error("Unsupported graphic type");
    }
}

std::unique_ptr<Graphic> DefaultGraphicFactory::createCustomGraphic(Graphic::GraphicType type, const std::vector<QPointF>& points) {
        return std::make_unique<ConcreteGraphic>(type, points);
}
