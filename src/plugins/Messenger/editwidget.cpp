#include <QDebug>
#include "editwidget.h"

#include <QKeyEvent>

EditWidget::EditWidget(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FSendMessageKey = Qt::Key_Return;
  ui.tedEditor->installEventFilter(this);

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
}

EditWidget::~EditWidget()
{

}

void EditWidget::sendMessage()
{
  emit messageAboutToBeSend();
  emit messageReady();
}

void EditWidget::setSendMessageKey(int AKey)
{
  FSendMessageKey = AKey;
  emit sendMessageKeyChanged(AKey);
}

void EditWidget::setStreamJid( const Jid &AStreamJid )
{
  Jid befour = FStreamJid;
  FStreamJid = AStreamJid;
  emit streamJidChanged(befour);
}

void EditWidget::setContactJid( const Jid &AContactJid )
{
  Jid befour = FContactJid;
  FContactJid = AContactJid;
  emit contactJidChanged(AContactJid);
}

void EditWidget::clearEditor()
{
  ui.tedEditor->clear();
  emit editorCleared();
}

bool EditWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  if (AWatched == ui.tedEditor && AEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
    if (keyEvent->modifiers() + keyEvent->key() == FSendMessageKey)
    {
      sendMessage();
      return true;
    }
  }
  return QWidget::eventFilter(AWatched,AEvent);
}

