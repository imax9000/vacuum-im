#include "rosterindexdelegate.h"
#include <QPainter>

RosterIndexDelegate::RosterIndexDelegate(QObject *AParent)
  : QItemDelegate(AParent)
{
  FRosterIconset.openFile(ROSTER_ICONSETFILE);
}

RosterIndexDelegate::~RosterIndexDelegate()
{

}

void RosterIndexDelegate::paint(QPainter *APainter, 
                                const QStyleOptionViewItem &AOption,  
                                const QModelIndex &AIndex) const
{
  QStyleOptionViewItem option = setOptions(AIndex,AOption);

  QRect freeRect = option.rect;
  APainter->save();

  drawBackground(APainter,option,AIndex,option.rect);

  QRect usedRect;

  usedRect = drawDecoration(APainter,option,AIndex,freeRect); 
  if (!usedRect.isNull())
    freeRect.setLeft(usedRect.right()+1);
  
  usedRect = drawDisplay(APainter,option,AIndex,freeRect);
  if (!usedRect.isNull())
    freeRect.setLeft(usedRect.right()+1);

  drawFocus(APainter,option,option.rect);

  APainter->restore();
}

QSize RosterIndexDelegate::sizeHint(const QStyleOptionViewItem &AOption,  
                                    const QModelIndex &AIndex) const
{
  return QItemDelegate::sizeHint(AOption,AIndex);
}

QRect RosterIndexDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                          const QModelIndex &AIndex, const QRect &/*ARect*/) const
{
  QItemDelegate::drawBackground(APainter,AOption,AIndex);
  return AOption.rect;
}

QRect RosterIndexDelegate::drawDecoration(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                          const QModelIndex &AIndex, const QRect &ARect ) const
{
  QVariant data = AIndex.data(Qt::DecorationRole);
  if (data.isValid() && data.canConvert(QVariant::Icon))
  {
    QRect iconRect = ARect.intersected(QRect(ARect.topLeft(),rect(AOption,AIndex,Qt::DecorationRole).size()));
    QItemDelegate::drawDecoration(APainter,AOption,iconRect,decoration(AOption,data));
    return iconRect;
  }
  return QRect();
}

QRect RosterIndexDelegate::drawDisplay(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                       const QModelIndex &AIndex, const QRect &ARect) const
{
  QVariant data = AIndex.data(Qt::DisplayRole);
  if (data.isValid() && data.canConvert(QVariant::String))
  {
    QString text = data.toString();
    if (!text.isEmpty())
    {
      QRect textRect = ARect.intersected(QRect(ARect.topLeft(),rect(AOption,AIndex,Qt::DisplayRole).size()));
      QItemDelegate::drawDisplay(APainter,AOption,textRect,text);
      return textRect;
    }
  }
  return QRect();
}

void RosterIndexDelegate::drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                     const QRect &ARect) const 
{
  QItemDelegate::drawFocus(APainter,AOption,ARect);
}

QStyleOptionViewItem RosterIndexDelegate::setOptions(const QModelIndex &AIndex,
                                                     const QStyleOptionViewItem &AOption) const
{
  QStyleOptionViewItem option = QItemDelegate::setOptions(AIndex,AOption);

  QVariant data = AIndex.data(IRosterIndex::DR_FontHint);
  if (data.isValid())
    option.font.setStyleHint((QFont::StyleHint)data.toInt());
  
  data = AIndex.data(IRosterIndex::DR_FontSize);
  if (data.isValid())
    option.font.setPointSize(data.toInt());

  data = AIndex.data(IRosterIndex::DR_FontWeight);
  if (data.isValid())
    option.font.setWeight(data.toInt());

  data = AIndex.data(IRosterIndex::DR_FontStyle);
  if (data.isValid())
    option.font.setStyle((QFont::Style)data.toInt());

  data = AIndex.data(IRosterIndex::DR_FontUnderline);
  if (data.isValid())
    option.font.setUnderline(data.toBool());

  return option;
}

QIcon::Mode RosterIndexDelegate::getIconMode( QStyle::State AState )
{
  if (!(AState & QStyle::State_Enabled)) return QIcon::Disabled;
  if (AState & QStyle::State_Selected) return QIcon::Selected;
  return QIcon::Normal;
}

QIcon::State RosterIndexDelegate::getIconState( QStyle::State AState )
{
  return AState & QStyle::State_Open ? QIcon::On : QIcon::Off;
}
