#ifndef ROSTERSMODEL_H
#define ROSTERSMODEL_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/iaccountmanager.h>
#include <utils/jid.h>
#include <utils/options.h>
#include "rootindex.h"
#include "rosterindex.h"
#include "dataholder.h"

class RostersModel :
	public AdvancedItemModel,
	public IPlugin,
	public IRostersModel
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRostersModel);
public:
	RostersModel();
	~RostersModel();
	virtual AdvancedItemModel *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTERSMODEL_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IRostersModel
	virtual QList<Jid> streams() const;
	virtual IRosterIndex *addStream(const Jid &AStreamJid);
	virtual void removeStream(const Jid &AStreamJid);
	virtual int streamsLayout() const;
	virtual void setStreamsLayout(StreamsLayout ALayout);
	virtual IRosterIndex *rootIndex() const;
	virtual IRosterIndex *newRosterIndex(int AKind);
	virtual IRosterIndex *findStreamRoot(const Jid &AStreamJid) const;
	virtual IRosterIndex *findStreamIndex(const Jid &AStreamJid) const;
	virtual void removeRosterIndex(IRosterIndex *AIndex, bool ADestroy = true);
	virtual IRosterIndex *findGroupIndex(int AKind, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) const;
	virtual IRosterIndex *getGroupIndex(int AKind, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent);
	virtual QList<IRosterIndex *> getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate = false);
	virtual QModelIndex modelIndexFromRosterIndex(IRosterIndex *AIndex) const;
	virtual IRosterIndex *rosterIndexFromModelIndex(const QModelIndex &AIndex) const;
	virtual bool isGroupKind(int AKind) const;
	virtual QList<int> singleGroupKinds() const;
	virtual QString singleGroupName(int AKind) const;
	virtual void registerSingleGroup(int AKind, const QString &AName);
	virtual QMultiMap<int, IRosterDataHolder *> rosterDataHolders() const;
	virtual void insertRosterDataHolder(int AOrder, IRosterDataHolder *AHolder);
	virtual void removeRosterDataHolder(int AOrder, IRosterDataHolder *AHolder);
signals:
	void streamAdded(const Jid &AStreamJid);
	void streamRemoved(const Jid &AStreamJid);
	void streamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void streamsLayoutAboutToBeChanged(int AAfter);
	void streamsLayoutChanged(int ABefore);
	void indexCreated(IRosterIndex *AIndex);
	void indexInserted(IRosterIndex *AIndex);
	void indexRemoving(IRosterIndex *AIndex);
	void indexDestroyed(IRosterIndex *AIndex);
	void indexDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void updateStreamsLayout();
	void emitIndexDestroyed(IRosterIndex *AIndex);
	void removeEmptyGroup(IRosterIndex *AGroupIndex);
	QString getGroupName(int AKind, const QString &AGroup) const;
	bool isChildIndex(IRosterIndex *AIndex, IRosterIndex *AParent) const;
	QList<IRosterIndex *> findContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, IRosterIndex *AParent = NULL) const;
protected slots:
	void onAdvancedItemInserted(QStandardItem *AItem);
	void onAdvancedItemRemoving(QStandardItem *AItem);
	void onAdvancedItemDataChanged(QStandardItem *AItem, int ARole);
protected slots:
	void onAccountShown(IAccount *AAccount);
	void onAccountHidden(IAccount *AAccount);
	void onAccountOptionsChanged(const OptionsNode &ANode);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
private:
	friend class RosterIndex;
private:
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IAccountManager *FAccountManager;
private:
	StreamsLayout FLayout;
	RootIndex *FRootIndex;
	IRosterIndex *FStreamsRoot;
	QMap<int, QString> FSingleGroups;
	QMap<Jid,IRosterIndex *> FStreamIndexes;
	QMultiMap<int, IRosterDataHolder *> FRosterDataHolders;
	QMap<IRosterDataHolder *, DataHolder *> FAdvancedDataHolders;
private:
	// streamRoot->bareJid->index
	QHash<IRosterIndex *, QMultiHash<Jid, IRosterIndex *> > FContactsCache;
	// parent->name->index
	QHash<IRosterIndex *, QMultiHash<QString, IRosterIndex *> > FGroupsCache;
};

#endif // ROSTERSMODEL_H
