#ifndef PROCESSGENERATOR_H
#define PROCESSGENERATOR_H

#include <QString>
#include <QMap>
#include "mapdata.h"

class ProcessGenerator
{
public:
    // Generate process code for an entity type (legacy)
    static QString generateProcessCode(const QString &processName, 
                                      const QString &assetPath,
                                      const QString &type,
                                      const QString &wrapperOpen = "",
                                      const QString &wrapperClose = "");
    
    // Generate process code with behavior (NEW)
    static QString generateProcessCodeWithBehavior(const EntityInstance &entity,
                                                  const QString &wrapperOpen = "",
                                                  const QString &wrapperClose = "");
    
    // Generate includes section for main.prg
    static QString generateIncludesSection(const QVector<EntityInstance> &entities);
    
    // Generate ALL process definitions inline (to avoid include issues)
    static QString generateAllProcessesCode(const QVector<EntityInstance> &entities,
                                          const QString &wrapperOpen = "",
                                          const QString &wrapperClose = "");
    
    // Generate declarations section (DECLARE PROCESS ...)
    static QString generateDeclarationsSection(const QVector<EntityInstance> &entities);
    
    // Generate spawn calls for main.prg
    static QString generateSpawnCalls(const QVector<EntityInstance> &entities);
    
    // Save process file to src/includes/
    static bool saveProcessFile(const QString &projectPath,
                               const QString &processName,
                               const QString &code);
    
    // Get unique process names from entities
    static QStringList getUniqueProcessNames(const QVector<EntityInstance> &entities);
};

#endif // PROCESSGENERATOR_H
