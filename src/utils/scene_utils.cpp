#include "scene_utils.h"

void SceneUtils::clearScene(QGraphicsScene* scene,
                          QGraphicsView* view,
                          ConnectionManager* connectionManager,
                          ConnectionPointOverlay* connectionOverlay,
                          SelectionManager* selectionManager)
{
    Logger::info("SceneUtils::clearScene: 开始清空场景");

    // 清除选择状态
    if (selectionManager) {
        Logger::debug("SceneUtils::clearScene: 清除选择状态");
        selectionManager->clearSelection();
    }

    // 准备 ConnectionManager 进行场景清空
    if (connectionManager) {
        Logger::debug("SceneUtils::clearScene: 准备 ConnectionManager 以进行场景清空");
        connectionManager->prepareForSceneClear();
    }

    // 处理连接点覆盖层
    bool overlayWasInScene = false;
    if (connectionOverlay && scene && scene->items().contains(connectionOverlay)) {
        Logger::debug("SceneUtils::clearScene: 从场景中临时移除 ConnectionPointOverlay");
        scene->removeItem(connectionOverlay);
        overlayWasInScene = true;
    }
    if (connectionOverlay) {
        connectionOverlay->setConnectionPointsVisible(false);
        connectionOverlay->clearHighlight();
    }

    // 清空场景中的所有其他图形项
    if (scene) {
        int itemCount = scene->items().count();
        Logger::debug(QString("SceneUtils::clearScene: 准备使用 QGraphicsScene::clear() 清除 %1 个项目").arg(itemCount));
        scene->clear();
        Logger::debug("SceneUtils::clearScene: QGraphicsScene::clear() 执行完毕");
    } else {
        Logger::warning("SceneUtils::clearScene: 场景为空，无需清除");
    }

    // 重新添加连接点覆盖层
    if (connectionOverlay && scene && overlayWasInScene) {
        Logger::debug("SceneUtils::clearScene: 将 ConnectionPointOverlay 重新添加到场景");
        scene->addItem(connectionOverlay);
        connectionOverlay->setZValue(1000);
        connectionOverlay->updateOverlay();
    } else if (connectionOverlay && !overlayWasInScene) {
        Logger::debug("SceneUtils::clearScene: 更新未在场景中的 ConnectionPointOverlay");
        connectionOverlay->updateOverlay();
    }

    if (scene) {
        scene->update();
    }
    if (view && view->viewport()) {
        view->viewport()->update();
    }

    Logger::info("SceneUtils::clearScene: 场景清空完成");
} 