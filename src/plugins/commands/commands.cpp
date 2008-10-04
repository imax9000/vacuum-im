#include "commands.h"

#define COMMAND_TAG_NAME              "command"
#define COMMANDS_TIMEOUT              60000

#define SHC_COMMANDS                  "/iq[@type='set']/query[@xmlns='" NS_COMMANDS "']"

#define IN_COMMAND                    "psi/command"

#define ADR_STREAMJID                 Action::DR_StreamJid
#define ADR_COMMAND_JID               Action::DR_Parametr1
#define ADR_COMMAND_NODE              Action::DR_Parametr2

Commands::Commands()
{
  FDataForms = NULL;
  FStanzaProcessor = NULL;
  FDiscovery = NULL;
  FPresencePlugin = NULL;
}

Commands::~Commands()
{

}

void Commands::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Plugin for advertising and executing application-specific commands");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Ad-Hoc Commands"); 
  APluginInfo->uid = COMMANDS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(DATAFORMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Commands::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
    if (FDiscovery)
    {
      connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),SLOT(onPresenceAdded(IPresence *)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved (IPresence *)));
    }
  }

  return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool Commands::initObjects()
{
  ErrorHandler::addErrorItem("malformed-action",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Can not understand the specified action"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-action",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Can not accept the specified action"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-locale",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Can not accept the specified language/locale"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-payload",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("The data form did not provide one or more required fields"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-sessionid",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Specified session not present"),NS_COMMANDS);
  ErrorHandler::addErrorItem("session-expired",ErrorHandler::CANCEL,ErrorHandler::NOT_ALLOWED,
    tr("Specified session is no longer active"),NS_COMMANDS);

  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertDiscoHandler(this);
    FDiscovery->insertFeatureHandler(NS_COMMANDS,this,DFO_DEFAULT);
  }
  return true;
}

bool Commands::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FStanzaHandlers.contains(AHandlerId))
  {
    ICommandRequest request;
    request.streamJid = AStreamJid;
    request.commandJid = AStanza.from();
    request.stanzaId = AStanza.id();

    QDomElement cmdElem = AStanza.firstElement(COMMAND_TAG_NAME,NS_COMMANDS);
    request.sessionId = cmdElem.attribute("sessionid");
    request.node = cmdElem.attribute("node");
    request.action = cmdElem.attribute("action",COMMAND_ACTION_EXECUTE);

    if (FDataForms)
    {
      QDomElement formElem = cmdElem.firstChildElement("x");
      while(!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
        formElem = formElem.nextSiblingElement("x");
      if (!formElem.isNull())
        request.form = FDataForms->dataForm(formElem);
    }
    
    ICommandServer *server = FCommands.value(request.node);
    if (!server || !server->receiveCommandRequest(request))
    {
      Stanza reply = AStanza.replyError("malformed-action",NS_COMMANDS,ErrorHandler::BAD_REQUEST);
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
    AAccept = true;
  }
  return false;
}

void Commands::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{
  if (FRequests.contains(AStanza.id()))
  {
    FRequests.removeAt(FRequests.indexOf(AStanza.id()));
    if (AStanza.type() == "result")
    {
      ICommandResult result;
      result.streamJid = AStreamJid;
      result.commandJid = AStanza.from();
      result.stanzaId = AStanza.id();
      
      QDomElement cmdElem = AStanza.firstElement(COMMAND_TAG_NAME,NS_COMMANDS);
      result.sessionId = cmdElem.attribute("sessionid");
      result.node = cmdElem.attribute("node");
      result.status = cmdElem.attribute("status");
      
      QDomElement actElem = cmdElem.firstChildElement("actions");
      result.execute = actElem.attribute("execute");
      actElem = actElem.firstChildElement();
      while (!actElem.isNull())
      {
        result.actions.append(actElem.tagName());
        actElem = actElem.nextSiblingElement();
      }

      QDomElement noteElem = cmdElem.firstChildElement("note");
      while(!noteElem.isNull())
      {
        ICommandNote note;
        note.type = noteElem.attribute("type",COMMAND_NOTE_INFO);
        note.message = noteElem.text();
        result.notes.append(note);
        noteElem = noteElem.nextSiblingElement("note");
      }

      if (FDataForms)
      {
        QDomElement formElem = cmdElem.firstChildElement("x");
        while(!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
          formElem = formElem.nextSiblingElement("x");
        if (!formElem.isNull())
          result.form = FDataForms->dataForm(formElem);
      }

      foreach(ICommandClient *client, FClients)
        if (client->receiveCommandResult(result))
          break;
    }
    else
    {
      ICommandError error;
      error.stanzaId = AStanza.id();
      ErrorHandler err(AStanza.element(),NS_COMMANDS);
      error.code = err.code();
      error.condition = err.condition();
      error.message = err.message();
      foreach(ICommandClient *client, FClients)
        if (client->receiveCommandError(error))
          break;
    }
  }
}

void Commands::iqStanzaTimeOut(const QString &AId)
{
  if (FRequests.contains(AId))
  {
    ICommandError error;
    error.stanzaId = AId;
    ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
    error.code = err.code();
    error.condition = err.condition();
    error.message = err.message();
    foreach(ICommandClient *client, FClients)
      if (client->receiveCommandError(error))
        break;
  }
}

void Commands::fillDiscoInfo(IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node == NS_COMMANDS)
  {
    IDiscoIdentity identity;
    identity.category = "automation";
    identity.type = "command-list";
    identity.name = "Commands";
    ADiscoInfo.identity.append(identity);
    
    if (ADiscoInfo.features.contains(NS_COMMANDS))
      ADiscoInfo.features.append(NS_COMMANDS);
  }
  else if (FCommands.contains(ADiscoInfo.node))
  {
    IDiscoIdentity identity;
    identity.category = "automation";
    identity.type = "command-node";
    identity.name = FCommands.value(ADiscoInfo.node)->commandName(ADiscoInfo.node);
    ADiscoInfo.identity.append(identity);

    if (ADiscoInfo.features.contains(NS_COMMANDS))
      ADiscoInfo.features.append(NS_COMMANDS);
    if (ADiscoInfo.features.contains(NS_JABBER_DATA))
      ADiscoInfo.features.append(NS_JABBER_DATA);
  }
}

void Commands::fillDiscoItems(IDiscoItems &ADiscoItems)
{
  if (!FCommands.isEmpty())
  {
    if (ADiscoItems.node == NS_COMMANDS)
    {
      QList<QString> nodes = FCommands.keys();
      foreach(QString node, nodes)
      {
        QString name = FCommands.value(node)->commandName(node);
        if (!name.isEmpty())
        {
          IDiscoItem ditem;
          ditem.itemJid = ADiscoItems.streamJid;
          ditem.node = node;
          ditem.name = name;
          ADiscoItems.items.append(ditem);
        }
      }
    }
    else if (ADiscoItems.node.isEmpty())
    {
      IDiscoItem ditem;
      ditem.itemJid = ADiscoItems.streamJid;
      ditem.node = NS_COMMANDS;
      ditem.name = "Commands";
    }
  }
}

bool Commands::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature==NS_COMMANDS && !ADiscoInfo.node.isEmpty() && ADiscoInfo.identity.value(0).type == "command-node")
  {
    executeCommnad(AStreamJid,ADiscoInfo.contactJid,ADiscoInfo.node);
    return true;
  }
  return false;
}

Action *Commands::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen() && AFeature==NS_COMMANDS)
  {
    IDiscoIdentity ident = ADiscoInfo.identity.value(0);
    if (ident.type == "command-node")
    {
      if (!ADiscoInfo.node.isEmpty())
      {
        Action *action = new Action(AParent);
        action->setText(tr("Execute"));
        action->setIcon(SYSTEM_ICONSETFILE,IN_COMMAND);
        action->setData(ADR_STREAMJID,AStreamJid.full());
        action->setData(ADR_COMMAND_JID,ADiscoInfo.contactJid.full());
        action->setData(ADR_COMMAND_NODE,ADiscoInfo.node);
        connect(action,SIGNAL(triggered(bool)),SLOT(onExecuteActionTriggered(bool)));
        return action;
      }
    }
    else
    {
      Menu *execMenu = new Menu(AParent);
      execMenu->setTitle(tr("Commands"));
      execMenu->setIcon(SYSTEM_ICONSETFILE,IN_COMMAND);
      IDiscoItems ditems = FDiscovery->discoItems(ADiscoInfo.contactJid,NS_COMMANDS);
      foreach (IDiscoItem ditem,ditems.items)
      {
        if (!ditem.node.isEmpty())
        {
          Action *action = new Action(execMenu);
          action->setText(ditem.name.isEmpty() ? ditem.node : ditem.name);
          action->setData(ADR_STREAMJID,AStreamJid.full());
          action->setData(ADR_COMMAND_JID,ditem.itemJid.full());
          action->setData(ADR_COMMAND_NODE,ditem.node);
          connect(action,SIGNAL(triggered(bool)),SLOT(onExecuteActionTriggered(bool)));
          execMenu->addAction(action,AG_DEFAULT,false);
        }
      }
      if (execMenu->isEmpty())
      {
        delete execMenu;
        return NULL;
      }
      return execMenu->menuAction();
    }
  }
  return NULL;
}

void Commands::insertCommand(const QString &ANode, ICommandServer *AServer)
{
  if (AServer && !FCommands.contains(ANode))
  {
    FCommands.insert(ANode,AServer);
    emit commandInserted(ANode, AServer);
  }
}

void Commands::removeCommand(const QString &ANode)
{
  if (FCommands.contains(ANode))
  {
    FCommands.remove(ANode);
    emit commandRemoved(ANode);
  }
}

void Commands::insertCommandClient(ICommandClient *AClient)
{
  if (AClient && !FClients.contains(AClient))
  {
    FClients.append(AClient);
    emit clientInserted(AClient);
  }
}

void Commands::removeCommandClient(ICommandClient *AClient)
{
  if (FClients.contains(AClient))
  {
    FClients.removeAt(FClients.indexOf(AClient));
    emit clientRemoved(AClient);
  }
}

QString Commands::sendCommandRequest(const ICommandRequest &ARequest)
{
  Stanza request("iq");
  request.setTo(ARequest.commandJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
  QDomElement cmdElem = request.addElement(COMMAND_TAG_NAME,NS_COMMANDS);
  cmdElem.setAttribute("node",ARequest.node);
  if (!ARequest.sessionId.isEmpty())
    cmdElem.setAttribute("sessionid",ARequest.sessionId);
  if (!ARequest.action.isEmpty())
    cmdElem.setAttribute("action",ARequest.action);
  if (FDataForms && !ARequest.form.type.isEmpty())
    FDataForms->xmlForm(ARequest.form,cmdElem);
  if (FStanzaProcessor->sendIqStanza(this,ARequest.streamJid,request,COMMANDS_TIMEOUT))
  {
    FRequests.append(request.id());
    return request.id();
  }
  return QString();
}

bool Commands::sendCommandResult(const ICommandResult &AResult)
{
  Stanza result("iq");
  result.setTo(AResult.commandJid.eFull()).setType("result").setId(AResult.stanzaId);
  
  QDomElement cmdElem = result.addElement(COMMAND_TAG_NAME,NS_COMMANDS);
  cmdElem.setAttribute("node",AResult.node);
  cmdElem.setAttribute("sessionid",AResult.sessionId);
  cmdElem.setAttribute("status",AResult.status);

  if (!AResult.actions.isEmpty())
  {
    QDomElement actElem = cmdElem.appendChild(result.createElement("actions")).toElement();
    actElem.setAttribute("execute",AResult.execute);
    foreach(QString action,AResult.actions)
      actElem.appendChild(result.createElement(action));
  }
  
  if (FDataForms && !AResult.form.type.isEmpty())
    FDataForms->xmlForm(AResult.form,cmdElem);

  foreach(ICommandNote note,AResult.notes)
  {
    QDomElement noteElem = cmdElem.appendChild(result.createElement("note")).toElement();
    noteElem.setAttribute("type",note.type);
    noteElem.appendChild(result.createTextNode(note.message));
  }

  return FStanzaProcessor->sendStanzaOut(AResult.streamJid,result);
}

void Commands::executeCommnad(const Jid &AStreamJid, const Jid &ACommandJid, const QString &ANode)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
  {
    CommandDialog *dialog = new CommandDialog(this,FDataForms,AStreamJid,ACommandJid,ANode,NULL);
    connect(presence->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
    dialog->executeCommand();
    dialog->show();
  }
}

void Commands::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = true;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_COMMAND);
  dfeature.var = NS_COMMANDS;
  dfeature.name = tr("Ad-Hoc Commands");
  dfeature.description = tr("Advertising and executing application-specific commands");
  FDiscovery->insertDiscoFeature(dfeature);
}

void Commands::onExecuteActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAMJID).toString();
    Jid commandJid = action->data(ADR_COMMAND_JID).toString();
    QString node = action->data(ADR_COMMAND_NODE).toString();
    executeCommnad(streamJid,commandJid,node);
  }
}

void Commands::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
  if (AInfo.contactJid.node().isEmpty())
    if (AInfo.features.contains(NS_COMMANDS) && !FDiscovery->hasDiscoItems(AInfo.contactJid,NS_COMMANDS))
      FDiscovery->requestDiscoItems(AInfo.streamJid,AInfo.contactJid,NS_COMMANDS);
}

void Commands::onPresenceAdded(IPresence *APresence)
{
  int handler = FStanzaProcessor->insertHandler(this,SHC_COMMANDS,IStanzaProcessor::DirectionIn,SHP_DEFAULT,APresence->streamJid());
  FStanzaHandlers.insert(handler,APresence);
}

void Commands::onPresenceRemoved(IPresence *APresence)
{
  int handler = FStanzaHandlers.key(APresence);
  FStanzaHandlers.remove(handler);
  FStanzaProcessor->removeHandler(handler);
}

Q_EXPORT_PLUGIN2(CommandsPlugin, Commands)