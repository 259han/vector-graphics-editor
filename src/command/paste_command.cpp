#include "paste_command.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
#include <QGraphicsScene>
#include <QApplication>

PasteGraphicCommand::PasteGraphicCommand(DrawArea* drawArea, const QList<QGraphicsItem*>& items)
    : m_drawArea(drawArea)
    , m_pastedItems(items)
    , m_executed(true) // 构造时，项目已经被添加到场景，因此标记为已执行
{
    Logger::debug(QString("PasteGraphicCommand: 创建粘贴命令 - 项目数: %1").arg(items.size()));
    
    // 保存项目的初始状态
    saveItemStates();
}

PasteGraphicCommand::~PasteGraphicCommand()
{
    // 注意：不删除m_pastedItems中的项目，因为它们可能仍在场景中
    Logger::debug("PasteGraphicCommand: 销毁粘贴命令");
}

void PasteGraphicCommand::execute()
{
    if (m_executed || !m_drawArea) {
        Logger::warning("PasteGraphicCommand::execute: 命令已执行或DrawArea无效");
        return;
    }
    
    Logger::debug("PasteGraphicCommand::execute: 开始执行粘贴命令");
    
    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        Logger::error("PasteGraphicCommand::execute: 场景无效");
        return;
    }
    
    // 取消当前所有选择
    for (auto item : scene->selectedItems()) {
        item->setSelected(false);
    }
    
    // 重新添加之前粘贴的项目
    for (auto item : m_pastedItems) {
        if (!item->scene()) {
            scene->addItem(item);
            item->setSelected(true);
        }
    }
    
    m_executed = true;
    
    // 更新场景
    scene->update();
    if (m_drawArea) {
        m_drawArea->viewport()->update();
    }
    
    Logger::info(QString("PasteGraphicCommand::execute: 粘贴命令执行成功 - 项目数: %1").arg(m_pastedItems.size()));
}

void PasteGraphicCommand::undo()
{
    if (!m_executed || !m_drawArea) {
        Logger::warning("PasteGraphicCommand::undo: 命令未执行或DrawArea无效");
        return;
    }
    
    Logger::debug("PasteGraphicCommand::undo: 开始撤销粘贴命令");
    
    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        Logger::error("PasteGraphicCommand::undo: 场景无效");
        return;
    }
    
    // 从场景中移除所有粘贴的项目
    for (auto item : m_pastedItems) {
        if (item->scene()) {
            Logger::debug(QString("PasteGraphicCommand::undo: 移除项目 %1").arg(reinterpret_cast<quintptr>(item)));
            scene->removeItem(item);
        }
    }
    
    m_executed = false;
    
    // 更新场景
    scene->update();
    if (m_drawArea) {
        m_drawArea->viewport()->update();
    }
    
    Logger::info(QString("PasteGraphicCommand::undo: 撤销粘贴命令成功 - 项目数: %1").arg(m_pastedItems.size()));
}

QString PasteGraphicCommand::getDescription() const
{
    return QString("粘贴 %1 个图形项").arg(m_pastedItems.size());
}

QString PasteGraphicCommand::getType() const
{
    return "paste";
}

void PasteGraphicCommand::saveItemStates()
{
    m_itemStates.clear();
    
    for (QGraphicsItem* item : m_pastedItems) {
        ItemState state;
        state.item = item;
        state.position = item->pos();
        state.isSelected = item->isSelected();
        
        m_itemStates.append(state);
    }
    
    Logger::debug(QString("PasteGraphicCommand::saveItemStates: 已保存 %1 个项目的状态").arg(m_itemStates.size()));
} 