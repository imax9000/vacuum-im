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
	virtual IRosterIndex *addStream(const Jid &AStreamJid);
	virtual QList<Jid> streams() const;
	virtual void removeStream(const Jid &AStreamJid);
	virtual IRosterIndex *rootIndex() const;
	virtual IRosterIndex *streamRoot(const Jid &AStreamJid) const;
	virtual IRosterIndex *createRosterIndex(int AKind, IRosterIndex *AParent);
	virtual IRosterIndex *findGroupIndex(int AKind, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) const;
	virtual IRosterIndex *createGroupIndex(int AKind, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent);
	virtual QList<IRosterIndex *> getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate = false);
	virtual QModelIndex modelIndexFromRosterIndex(IRosterIndex *AIndex) const;
	virtual IRosterIndex *rosterIndexFromModelIndex(const QModelIndex &AIndex) const;
	virtual bool isGroupKind(int AKind) const;
	virtual QList<int> singleGroupKinds() const;
	virtual QString singleGroupName(int AKind) const;
	virtual void registerSingleGroup(int AKind, const QString &AName);
signals:
	void streamAdded(const Jid &AStreamJid);
	void streamRemoved(const Jid &AStreamJid);
	void streamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void indexCreated(IRosterIndex *AIndex);
	void indexInserted(IRosterIndex *AIndex);
	void indexRemoved(IRosterIndex *AIndex);
	void indexDestroyed(IRosterIndex *AIndex);
	void indexDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void emitIndexDestroyed(IRosterIndex *AIndex);
	QString getGroupName(int AKind, const QString &AGroup) const;
	bool isChildIndex(IRosterIndex *AIndex, IRosterIndex *AParent) const;
	QList<IRosterIndex *> findContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, bool ABare, IRosterIndex *AParent = NULL) const;
protected slots:
	void onAdvancedItemInserted(QStandardItem *AItem);
	void onAdvancedItemRemoved(QStandardItem *AItem);
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
	RootIndex *FRootIndex;
	QMap<int, QString> FSingleGroups;
	QHash<Jid,IRosterIndex *> FStreamRoots;
private:
	// streamRoot->bareJid->index
	QHash<IRosterIndex *, QMultiHash<Jid, IRosterIndex *> > FContactsCache;
	// parent->name->index
	QHash<IRosterIndex *, QMultiHash<QString, IRosterIndex *> > FGroupsCache;
};

#endif // ROSTERSMODEL_H
