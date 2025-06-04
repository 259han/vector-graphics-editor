#ifndef SCENE_UTILS_H
#define SCENE_UTILS_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include "../core/connection_manager.h"
#include "../core/connection_point_overlay.h"
#include "../core/selection_manager.h"
#include "../utils/logger.h"

class SceneUtils {
public:
    /**
     * @brief 清空场景中的所有图形项，同时保持连接点覆盖层的状态
     * @param scene 要清空的场景
     * @param view 关联的视图
     * @param connectionManager 连接管理器
     * @param connectionOverlay 连接点覆盖层
     * @param selectionManager 选择管理器
     */
    static void clearScene(QGraphicsScene* scene,
                          QGraphicsView* view,
                          ConnectionManager* connectionManager,
                          ConnectionPointOverlay* connectionOverlay,
                          SelectionManager* selectionManager);
};

#endif // SCENE_UTILS_H 