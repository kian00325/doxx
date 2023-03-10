/*
 * VGMTrans (c) 2002-2021
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#include "VGMFileTreeView.h"

#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QApplication>
#include <VGMFile.h>
#include <VGMItem.h>
#include "QtVGMRoot.h"
#include "Helpers.h"

void VGMTreeDisplayItem::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
  QStyleOptionViewItem paintopt = option;
  initStyleOption(&paintopt, index);

  QStyle *style = paintopt.widget ? paintopt.widget->style() : QApplication::style();

  QTextDocument backing_doc;
  backing_doc.setHtml(paintopt.text);

  // Paint the item's background
  paintopt.text = QString{};
  style->drawControl(QStyle::CE_ItemViewItem, &paintopt, painter, paintopt.widget);

  QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &paintopt);
  painter->save();
  painter->translate(textRect.topLeft());
  backing_doc.drawContents(painter, textRect.translated(-textRect.topLeft()));

  painter->restore();
}

QSize VGMTreeDisplayItem::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const {
  QStyleOptionViewItem style_opt = option;
  initStyleOption(&style_opt, index);

  QTextDocument backing_doc;
  backing_doc.setHtml(style_opt.text);
  backing_doc.setTextWidth(style_opt.rect.width());
  return QSize(backing_doc.idealWidth(), backing_doc.size().height());
}

/*
 * The following is not actually a proper view on the data,
 * but actually an entirely new tree.
 * As long as we need to support the legacy version, this is fine.
 */

VGMFileTreeView::VGMFileTreeView(VGMFile *file, QWidget *parent) : QTreeWidget(parent) {
  setHeaderLabel("File structure");

  /* Items to be added to the top level have their parent set at the vgmfile */
  m_items[file] = invisibleRootItem();
  file->AddToUI(nullptr, this);

  setItemDelegate(new VGMTreeDisplayItem());
}

void VGMFileTreeView::addVGMItem(VGMItem *item, VGMItem *parent, const std::wstring &name) {
  auto item_name = QString::fromStdWString(name);
  auto tree_item = new VGMTreeItem(item_name, item, nullptr, parent);
  if (name == item->GetDescription()) {
    tree_item->setText(0, QString{"<b>%1</b><br>Offset: 0x%2 | Length: 0x%3"}
                              .arg(item_name)
                              .arg(item->dwOffset, 1, 16)
                              .arg(item->unLength, 1, 16));
  } else {
    tree_item->setText(0, QString{"<b>%1</b><br>%2<br>Offset: 0x%3 | Length: 0x%4"}
                              .arg(item_name)
                              .arg(QString::fromStdWString(item->GetDescription()))
                              .arg(item->dwOffset, 1, 16)
                              .arg(item->unLength, 1, 16));
  }
  tree_item->setIcon(0, iconForItemType(item->GetIcon()));
  tree_item->setToolTip(0, QString::fromStdWString(item->GetDescription()));

  if (parent != parent_cached) {
    parent_cached = parent;
    parent_item_cached = m_items[parent];
  }

  parent_item_cached->addChild(tree_item);
  m_items[item] = tree_item;
  tree_item->setData(0, Qt::UserRole, QVariant::fromValue((void *)item));
}