#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include <QGraphicsScene>
#include <QImage>
#include <QPoint>
#include <QColor>
#include <QRectF>
#include <QGraphicsItem>
#include <QString>
#include <QDebug>
#include <QStack>
#include <QVector>

/**
 * @brief 提供通用图形和图像处理工具的静态类
 * 
 * 该类包含各种通用的图形处理功能，无需实例化即可使用
 */
class GraphicsUtils {
public:
    /**
     * @brief 将图形场景渲染到图像
     * @param scene 要渲染的场景
     * @param disableAntialiasing 是否禁用抗锯齿
     * @return 渲染后的图像
     */
    static QImage renderSceneToImage(QGraphicsScene* scene, bool disableAntialiasing = true);
    
    /**
     * @brief 将场景的指定矩形区域渲染到图像
     * @param scene 要渲染的场景
     * @param sceneRect 场景中要渲染的矩形区域
     * @param transparent 是否使用透明背景
     * @param enableAntialiasing 是否启用抗锯齿
     * @return 渲染后的图像
     */
    static QImage renderSceneRectToImage(QGraphicsScene* scene, const QRectF& sceneRect, 
                                      bool transparent = true, bool enableAntialiasing = true);
    
    /**
     * @brief 渲染场景的指定部分到指定的绘图器上
     * @param painter 目标绘图器
     * @param targetRect 目标矩形（绘图器上的位置）
     * @param sourceRect 源矩形（场景中的区域）
     * @param scene 要渲染的场景
     * @param enableAntialiasing 是否启用抗锯齿
     */
    static void renderScenePart(QPainter* painter, const QRectF& targetRect, 
                             const QRectF& sourceRect, QGraphicsScene* scene,
                             bool enableAntialiasing = true);
    
    /**
     * @brief 使用扫描线填充算法对图像区域进行填充
     * @param image 要填充的图像
     * @param seedPoint 种子点（开始填充的位置）
     * @param targetColor 目标颜色（要替换的颜色）
     * @param fillColor 填充颜色
     * @return 填充的像素数量
     */
    static int fillImageRegion(QImage& image, const QPoint& seedPoint, 
                             const QColor& targetColor, const QColor& fillColor);
    
    /**
     * @brief 计算图像中填充区域占总图像的比例
     * @param filledPixels 填充的像素数量
     * @param width 图像宽度
     * @param height 图像高度
     * @return 填充比例（0.0-1.0）
     */
    static double calculateFillRatio(int filledPixels, int width, int height);
    
    /**
     * @brief 从填充后的图像创建填充结果图层
     * @param originalImage 原始图像
     * @param filledImage 填充后的图像
     * @param fillColor 填充颜色
     * @return 只包含填充部分的图像
     */
    static QImage createFillResultLayer(const QImage& originalImage, 
                                      const QImage& filledImage, 
                                      const QColor& fillColor);
    
    /**
     * @brief 检查点是否在图像有效范围内
     * @param point 要检查的点
     * @param width 图像宽度
     * @param height 图像高度
     * @return 如果点在图像范围内则返回true
     */
    static bool isPointInImageBounds(const QPoint& point, int width, int height);
    
    /**
     * @brief 将场景坐标转换为图像坐标
     * @param scenePos 场景坐标
     * @param sceneRect 场景矩形
     * @return 图像坐标
     */
    static QPoint sceneToImageCoordinates(const QPointF& scenePos, const QRectF& sceneRect);
    
    /**
     * @brief 记录填充区域的统计信息到调试输出
     * @param filledPixels 填充的像素数量
     * @param minX 区域最小X坐标
     * @param minY 区域最小Y坐标
     * @param maxX 区域最大X坐标
     * @param maxY 区域最大Y坐标
     */
    static void logFillAreaStats(int filledPixels, int minX, int minY, int maxX, int maxY);
};

#endif // GRAPHICS_UTILS_H 