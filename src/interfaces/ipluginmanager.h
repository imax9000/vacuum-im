#ifndef IPLUGINMANAGER_H
#define IPLUGINMANAGER_H

#include <QApplication>
#include <QtPlugin>
#include <QUuid>
#include <QUrl>
#include <QIcon>
#include <QVector>

struct PluginInfo 
{
  QUuid uid;
  QString name;
  QIcon icon;
  QString description;
  QString version;
  QString author;
  QList<QUuid> implements;
  QList<QUuid> dependences;
  QList<QUuid> conflicts;
  QUrl homePage;
};

class IPluginManager;

class IPlugin  {
public:
  virtual QObject *instance()=0;
  virtual QUuid getPluginUuid() const=0;
  virtual void getPluginInfo(PluginInfo *info)=0;
  virtual bool initPlugin(IPluginManager *)=0;
  virtual bool startPlugin()=0;
};

class IPluginManager  {
public:
  virtual QObject* instance() =0;
  virtual void loadPlugins() =0;
  virtual void initPlugins() =0;
  virtual void startPlugins() =0;
  virtual bool unloadPlugin(const QUuid &) =0;
  virtual QApplication *application() =0;
  virtual QList<IPlugin *> getPlugins() =0;
  virtual QList<IPlugin *> getPlugins(const QString &) =0;
  virtual IPlugin* getPlugin(const QUuid &) =0;
  virtual const PluginInfo *getPluginInfo(const QUuid &) const =0;
  virtual QVector<QUuid> getDependencesOn(const QUuid &) =0;
  virtual QVector<QUuid> getDependencesFor(const QUuid &) =0;
signals:
  virtual void aboutToQuit() =0;
public slots:
  virtual void quit() =0;
};

Q_DECLARE_INTERFACE(IPluginManager,"Vacuum.Core.IPluginManager/1.0")
Q_DECLARE_INTERFACE(IPlugin,"Vacuum.Core.IPlugin/1.0")

#endif
