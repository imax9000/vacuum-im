#include "multiuserchatplugin.h"

#include <QInputDialog>

#define IN_GROUPCHAT          "psi/groupChat"
#define IN_INVITE             "psi/events"

#define ADR_STREAM_JID        Action::DR_StreamJid
#define ADR_HOST              Action::DR_Parametr1
#define ADR_ROOM              Action::DR_Parametr2
#define ADR_NICK              Action::DR_Parametr3
#define ADR_PASSWORD          Action::DR_Parametr4

MultiUserChatPlugin::MultiUserChatPlugin()
{
  FPluginManager = NULL;
  FMessenger = NULL;
  FRostersViewPlugin = NULL;
  FMainWindowPlugin = NULL;
  FTrayManager = NULL;
  FXmppStreams = NULL;
  FDiscovery = NULL;

  FChatMenu = NULL;
  FJoinAction = NULL;
}

MultiUserChatPlugin::~MultiUserChatPlugin()
{
  delete FChatMenu;
}

void MultiUserChatPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Implements multi-user text conferencing");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Multi-User Chat"); 
  APluginInfo->uid = MULTIUSERCHAT_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(MESSENGER_UUID);
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MultiUserChatPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("IMessenger").value(0,NULL);
  if (plugin)
  {
    FMessenger = qobject_cast<IMessenger *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
    if (FDiscovery)
    {
      connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),
        SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
    }
  }

  return FMessenger!=NULL;
}

bool MultiUserChatPlugin::initObjects()
{
  FChatMenu = new Menu(NULL);
  FChatMenu->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  FChatMenu->setTitle(tr("Conferences"));
  connect(FChatMenu->menuAction(),SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));

  FJoinAction = new Action(FChatMenu);
  FJoinAction->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  FJoinAction->setText(tr("Join conference"));
  connect(FJoinAction,SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));
  FChatMenu->addAction(FJoinAction,AG_DEFAULT+100,true);

  if (FMessenger)
  {
    FMessenger->insertMessageHandler(this,MHO_MULTIUSERCHAT_INVITE);
  }
  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FMainWindowPlugin->mainWindow()->topToolBarChanger()->addAction(FChatMenu->menuAction(),AG_MULTIUSERCHAT_MWTTB);
  }
  if (FTrayManager)
  {
    FTrayManager->addAction(FChatMenu->menuAction(),AG_MULTIUSERCHAT_TRAY,true);
  }
  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_MUC,this,DFO_DEFAULT);
  }
  return true;
}

bool MultiUserChatPlugin::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature==NS_MUC && ADiscoInfo.contactJid.resource().isEmpty())
  {
    IMultiUserChatWindow *chatWindow = multiChatWindow(AStreamJid,ADiscoInfo.contactJid);
    if (!chatWindow)
      showJoinMultiChatDialog(AStreamJid,ADiscoInfo.contactJid,"","");
    else
      chatWindow->showWindow();
    return true;
  }
  return false;
}

Action *MultiUserChatPlugin::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  if (AFeature == NS_MUC)
  {
    if (ADiscoInfo.identity.value(0).category == "conference")
    {
      Action *action = createJoinAction(AStreamJid,ADiscoInfo.contactJid,AParent);
      return action;
    }
    else
    {
      Menu *inviteMenu = createInviteMenu(ADiscoInfo.contactJid,AParent);
      return inviteMenu->isEmpty() ? (delete inviteMenu,NULL) : inviteMenu->menuAction();
    }
  }
  return NULL;
}

bool MultiUserChatPlugin::checkMessage(const Message &AMessage)
{
  return !AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite").isNull();
}

bool MultiUserChatPlugin::notifyOptions(const Message &AMessage, QIcon &AIcon, QString &AToolTip, int &AFlags)
{
  QDomElement inviteElem = AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");
  Jid roomJid = AMessage.from();
  if (!multiChatWindow(AMessage.to(),roomJid))
  {
    AIcon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_INVITE);
    AToolTip = tr("You are invited to the conference %1").arg(roomJid.bare());
    AFlags = 0;
    return true;
  }
  return false;
}

void MultiUserChatPlugin::receiveMessage(int AMessageId)
{
  FActiveInvites.append(AMessageId);
}

void MultiUserChatPlugin::showMessage(int AMessageId)
{
  Message message = FMessenger->messageById(AMessageId);
  QDomElement inviteElem = message.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");
  Jid roomJid = message.from();
  Jid fromJid = inviteElem.attribute("from");
  if (roomJid.isValid() && fromJid.isValid())
  {
    InviteFields fields;
    fields.streamJid = message.to();
    fields.roomJid = roomJid;
    fields.fromJid  = fromJid;
    fields.password = inviteElem.firstChildElement("password").text();

    QString reason = inviteElem.firstChildElement("reason").text();
    QString msg = tr("You are invited to the conference %1 by %2.<br>Reason: %3").arg(roomJid.hBare()).arg(fromJid.hFull()).arg(Qt::escape(reason));
    msg+="<br><br>";
    msg+=tr("Do you want to join this conference?");

    QMessageBox *inviteDialog = new QMessageBox(QMessageBox::Question,tr("Invite"),msg,QMessageBox::Yes|QMessageBox::No|QMessageBox::Ignore);
    inviteDialog->setAttribute(Qt::WA_DeleteOnClose,true);
    inviteDialog->setEscapeButton(QMessageBox::Ignore);
    inviteDialog->setModal(false);
    connect(inviteDialog,SIGNAL(finished(int)),SLOT(onInviteDialogFinished(int)));
    FInviteDialogs.insert(inviteDialog,fields);
    inviteDialog->show();
  }
  FActiveInvites.removeAt(FActiveInvites.indexOf(AMessageId ));
  FMessenger->removeMessage(AMessageId);
}

bool MultiUserChatPlugin::requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid)
{
  if (FDiscovery)
    return FDiscovery->requestDiscoInfo(AStreamJid,ARoomJid.bare(),MUC_NODE_ROOM_NICK);
  return false;
}

IMultiUserChat *MultiUserChatPlugin::getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
                                                      const QString &APassword, bool ADedicated)
{
  IMultiUserChat *chat = multiUserChat(AStreamJid,ARoomJid);
  if (!chat)
  {
    chat = new MultiUserChat(this,ADedicated ? NULL: FMessenger,AStreamJid,ARoomJid,ANick,APassword,this);
    connect(chat->instance(),SIGNAL(chatDestroyed()),SLOT(onMultiUserChatDestroyed()));
    FChats.append(chat);
  }
  return chat;
}

IMultiUserChat *MultiUserChatPlugin::multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const
{
  foreach(IMultiUserChat *chat, FChats)
    if (chat->streamJid() == AStreamJid && chat->roomJid() == ARoomJid)
      return chat;
  return NULL;
}

IMultiUserChatWindow *MultiUserChatPlugin::getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
                                                              const QString &APassword)
{
  IMultiUserChatWindow *chatWindow = multiChatWindow(AStreamJid,ARoomJid);
  if (!chatWindow)
  {
    IMultiUserChat *chat = getMultiUserChat(AStreamJid,ARoomJid,ANick,APassword,false);
    chatWindow = new MultiUserChatWindow(this,FMessenger,chat);
    connect(chatWindow,SIGNAL(multiUserContextMenu(IMultiUser *, Menu *)),SLOT(onMultiUserContextMenu(IMultiUser *, Menu *)));
    connect(chatWindow,SIGNAL(windowDestroyed()),SLOT(onMultiChatWindowDestroyed()));
    insertChatAction(chatWindow);
    FChatWindows.append(chatWindow);
    emit multiChatWindowCreated(chatWindow);
  }
  return chatWindow;
}

IMultiUserChatWindow *MultiUserChatPlugin::multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const
{
  foreach(IMultiUserChatWindow *chatWindow,FChatWindows)
    if (chatWindow->streamJid()==AStreamJid && chatWindow->roomJid()==ARoomJid)
      return chatWindow;
  return NULL;
}

void MultiUserChatPlugin::showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
  JoinMultiChatDialog *dialog = new JoinMultiChatDialog(this,AStreamJid,ARoomJid,ANick,APassword);
  dialog->show();
}

void MultiUserChatPlugin::insertChatAction(IMultiUserChatWindow *AWindow)
{
  Action *action = new Action(FChatMenu);
  action->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  action->setText(tr("%1 as %2").arg(AWindow->multiUserChat()->roomJid().bare()).arg(AWindow->multiUserChat()->nickName()));
  connect(action,SIGNAL(triggered(bool)),SLOT(onChatActionTriggered(bool)));
  FChatMenu->addAction(action,AG_DEFAULT,false);
  FChatActions.insert(AWindow,action);
}

void MultiUserChatPlugin::removeChatAction(IMultiUserChatWindow *AWindow)
{
  if (FChatActions.contains(AWindow))
    FChatMenu->removeAction(FChatActions.take(AWindow));
}

void MultiUserChatPlugin::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  QIcon icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_GROUPCHAT);

  dfeature.active = true;
  dfeature.icon = icon;
  dfeature.var = NS_MUC;
  dfeature.name = tr("Multi-user text conferencing");
  dfeature.actionName = tr("Join conference");
  dfeature.description = tr("Multi-user text conferencing");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.active = false;
  dfeature.icon = QIcon();
  dfeature.actionName = "";
  dfeature.description = "";

  dfeature.var = MUC_HIDDEN;
  dfeature.name = tr("Hidden room");
  dfeature.description = tr("A room that cannot be found by any user through normal means such as searching and service discovery");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_MEMBERSONLY;
  dfeature.name = tr("Members-only room");
  dfeature.description = tr("A room that a user cannot enter without being on the member list");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_MODERATED;
  dfeature.name = tr("Moderated room");
  dfeature.description = tr("A room in which only those with 'voice' may send messages to all occupants");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_NONANONYMOUS;
  dfeature.name = tr("Non-anonymous room");
  dfeature.description = tr("A room in which an occupant's full JID is exposed to all other occupants");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_OPEN;
  dfeature.name = tr("Open room");
  dfeature.description = tr("A room that anyone may enter without being on the member list");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_PASSWORD;
  dfeature.name = tr("Password-protected room");
  dfeature.description = tr("A room that a user cannot enter without first providing the correct password");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_PASSWORDPROTECTED;
  dfeature.name = tr("Password-protected room");
  dfeature.description = tr("A room that a user cannot enter without first providing the correct password");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_PERSISTENT;
  dfeature.name = tr("Persistent room");
  dfeature.description = tr("A room that is not destroyed if the last occupant exits");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_PUBLIC;
  dfeature.name = tr("Public room");
  dfeature.description = tr("A room that can be found by any user through normal means such as searching and service discovery");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_SEMIANONYMOUS;
  dfeature.name = tr("Semi-anonymous room");
  dfeature.description = tr("A room in which an occupant's full JID can be discovered by room admins only");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_TEMPORARY;
  dfeature.name = tr("Temporary room");
  dfeature.description = tr("A room that is destroyed if the last occupant exits");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_UNMODERATED;
  dfeature.name = tr("Unmoderated room");
  dfeature.description = tr("A room in which any occupant is allowed to send messages to all occupants");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = MUC_UNSECURED;
  dfeature.name = tr("Unsecured room");
  dfeature.description = tr("A room that anyone is allowed to enter without first providing the correct password");
  FDiscovery->insertDiscoFeature(dfeature);
}

Menu *MultiUserChatPlugin::createInviteMenu(const Jid &AContactJid, QWidget *AParent) const
{
  Menu *inviteMenu = new Menu(AParent);
  inviteMenu->setTitle(tr("Invite to"));
  inviteMenu->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  foreach(IMultiUserChatWindow *window,FChatWindows)
  {
    if (window->multiUserChat()->isOpen())
    {
      Action *action = new Action(inviteMenu);
      action->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
      action->setText(tr("%1 from %2").arg(window->roomJid().full()).arg(window->multiUserChat()->nickName()));
      action->setData(ADR_STREAM_JID,window->streamJid().full());
      action->setData(ADR_HOST,AContactJid.full());
      action->setData(ADR_ROOM,window->roomJid().full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onInviteActionTriggered(bool)));
      inviteMenu->addAction(action,AG_DEFAULT,true);
    }
  }
  return inviteMenu;
}

Action *MultiUserChatPlugin::createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QObject *AParent) const
{
  Action *action = new Action(AParent);
  action->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  action->setText(tr("Join conference"));
  action->setData(ADR_STREAM_JID,AStreamJid.full());
  action->setData(ADR_HOST,ARoomJid.domane());
  action->setData(ADR_ROOM,ARoomJid.node());
  connect(action,SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));
  return action;
}

void MultiUserChatPlugin::onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu)
{
  IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(sender());
  if (chatWindow)
    emit multiUserContextMenu(chatWindow,AUser,AMenu);
}

void MultiUserChatPlugin::onMultiUserChatDestroyed()
{
  IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
  if (FChats.contains(chat))
  {
    FChats.removeAt(FChats.indexOf(chat));
    emit multiUserChatDestroyed(chat);
  }
}

void MultiUserChatPlugin::onMultiChatWindowDestroyed()
{
  IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(sender());
  if (chatWindow)
  {
    removeChatAction(chatWindow);
    FChatWindows.removeAt(FChatWindows.indexOf(chatWindow));
    emit multiChatWindowCreated(chatWindow);
  }
}

void MultiUserChatPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  foreach(IMultiUserChatWindow *chatWindow,FChatWindows)
    if (chatWindow->streamJid() == AXmppStream->jid())
      chatWindow->exitMultiUserChat("");

  QList<QMessageBox *> inviteDialogs = FInviteDialogs.keys();
  foreach(QMessageBox * inviteDialog,inviteDialogs)
    if (FInviteDialogs.value(inviteDialog).streamJid == AXmppStream->jid())
      inviteDialog->done(QMessageBox::Ignore);

  for (int i=0; i<FActiveInvites.count();i++)
    if (AXmppStream->jid() == FMessenger->messageById(FActiveInvites.at(i)).to())
    {
      FMessenger->removeMessage(FActiveInvites.at(i));
      FActiveInvites.removeAt(i--);
    }
}

void MultiUserChatPlugin::onJoinActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString host = action->data(ADR_HOST).toString();
    QString room = action->data(ADR_ROOM).toString();
    QString nick = action->data(ADR_NICK).toString();
    QString password = action->data(ADR_PASSWORD).toString();
    Jid streamJid = action->data(Action::DR_StreamJid).toString();
    Jid roomJid(room,host,"");
    showJoinMultiChatDialog(streamJid,roomJid,nick,password);
  }
}

void MultiUserChatPlugin::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  int show = AIndex->data(RDR_Show).toInt();
  if (show!=IPresence::Offline && show!=IPresence::Error)
  {
    if (AIndex->type() == RIT_StreamRoot)
    {
      Action *action = createJoinAction(AIndex->data(RDR_Jid).toString(),Jid(),AMenu);
      AMenu->addAction(action,AG_MULTIUSERCHAT_ROSTER,true);
    }
  }
}

void MultiUserChatPlugin::onChatActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  IMultiUserChatWindow *window = FChatActions.key(action,NULL);
  if (window)
    window->showWindow();
}

void MultiUserChatPlugin::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node == MUC_NODE_ROOM_NICK)
  {
    QString nick;
    for (int i=0; i<ADiscoInfo.identity.count() && nick.isEmpty(); i++)
      if (ADiscoInfo.identity.at(i).category=="conference" && ADiscoInfo.identity.at(i).type=="text")
        nick = ADiscoInfo.identity.at(i).name;
    emit roomNickReceived(ADiscoInfo.streamJid,ADiscoInfo.contactJid,nick);
  }
}

void MultiUserChatPlugin::onInviteDialogFinished(int AResult)
{
  QMessageBox *inviteDialog = qobject_cast<QMessageBox *>(sender());
  if (inviteDialog)
  {
    InviteFields fields = FInviteDialogs.take(inviteDialog);
    if (AResult == QMessageBox::Yes)
    {
      showJoinMultiChatDialog(fields.streamJid,fields.roomJid,"",fields.password);
    }
    else if (AResult == QMessageBox::No)
    {
      Message decline;
      decline.setTo(fields.roomJid.eBare());
      Stanza &mstanza = decline.stanza();
      QDomElement declElem = mstanza.addElement("x",NS_MUC_USER).appendChild(mstanza.createElement("decline")).toElement();
      declElem.setAttribute("to",fields.fromJid.eFull());
      QString reason = tr("I`am too busy right now");
      reason = QInputDialog::getText(inviteDialog,tr("Decline invite"),tr("Enter a reason"),QLineEdit::Normal,reason);
      if (!reason.isEmpty())
        declElem.appendChild(mstanza.createElement("reason")).appendChild(mstanza.createTextNode(reason));
      FMessenger->sendMessage(decline,fields.streamJid);
    }
  }
}

void MultiUserChatPlugin::onInviteActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_HOST).toString();
    Jid roomJid = action->data(ADR_ROOM).toString();
    IMultiUserChatWindow *window = multiChatWindow(streamJid,roomJid);
    if (window && contactJid.isValid())
    {
      bool ok;
      QString reason = tr("You are welcome here");
      reason = QInputDialog::getText(window,tr("Invite user"),tr("Enter a reason"),QLineEdit::Normal,reason,&ok);
      if (ok)
        window->multiUserChat()->inviteContact(contactJid,reason);
    }
  }
}

Q_EXPORT_PLUGIN2(MultiUserChatPlugin, MultiUserChatPlugin)