#ifndef CODEEDITORDIALOG_H
#define CODEEDITORDIALOG_H

#include <QMainWindow>
#include "codeeditor.h"

class QAction;
class QLabel;

class CodeEditorDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit CodeEditorDialog(QWidget *parent = nullptr);
    
    // Static helper to launch new instances
    static void openEditor(QWidget *parent, const QString &fileName = QString());
    
    bool openFile(const QString &fileName);
    bool saveFile();
    bool saveFileAs();
    
    QString currentFile() const { return m_editor->currentFile(); }

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onSave();
    void onSaveAs();
    void onDocumentModified();

private:
    bool maybeSave();
    void createActions();
    void createToolBar();
    void createStatusBar();
    
    CodeEditor *m_editor;
    
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeAction; // Optional, usually X button is enough
    
    QLabel *m_statusLabel;
};

#endif // CODEEDITORDIALOG_H
