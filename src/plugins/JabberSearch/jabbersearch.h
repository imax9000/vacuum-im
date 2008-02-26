#ifndef JABBERSEARCH_H
#define JABBERSEARCH_H

#include "../../definations/namespaces.h"
#include "../../definations/discofeatureorder.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ijabbersearch.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/ipresence.h"
#include "searchdialog.h"

class JabberSearch : 
  public QObject,
  public IPlugin,
  public IJabberSearch,
  public IIqStanzaOwner,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IJabberSearch IIqStanzaOwner IDiscoFeatureHandler);
public:
  JabberSearch();
  ~JabberSearch();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return JABBERSEARCH_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IJabberSearch
  virtual QString sendRequest(const Jid &AStreamJid, const Jid &AServiceJid);
  virtual QString sendSubmit(const Jid &AStreamJid, const ISearchSubmit &ASubmit);
  virtual void showSearchDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
signals:
  virtual void searchFields(const QString &AId, const ISearchFields &AFields);
  virtual void searchResult(const QString &AId, const ISearchResult &AResult);
  virtual void searchError(const QString &AId, const QString &AError);
protected:
  void registerDiscoFeatures();
protected slots:
  void onSearchActionTriggered(bool);
private:
  IPluginManager *FPluginManager;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
  IPresencePlugin *FPresencePlugin;
private:
  QList<QString> FRequests;
  QList<QString> FSubmits;
};

#endif // JABBERSEARCH_H