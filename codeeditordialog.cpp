#include "codeeditordialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QCloseEvent>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QIcon>

CodeEditorDialog::CodeEditorDialog(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Editor de Código");
    resize(900, 700);
    
    // Create editor
    m_editor = new CodeEditor(this);
    setCentralWidget(m_editor);
    
    createActions();
    createToolBar();
    createStatusBar();
    
    // Connect signals
    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &CodeEditorDialog::onDocumentModified);
            
    // Set window attributes to allow multiple independent windows
    // Note: We don't set WA_DeleteOnClose here by default because sometimes we might want to keep it
    // But for "Open new window" usage, we will.
}

void CodeEditorDialog::openEditor(QWidget *parent, const QString &fileName)
{
    // Create a new instance that deletes itself on close
    CodeEditorDialog *editor = new CodeEditorDialog(parent); // Parent can be null for top-level
    editor->setAttribute(Qt::WA_DeleteOnClose);
    
    if (!fileName.isEmpty()) {
        if (editor->openFile(fileName)) {
            editor->show();
        } else {
            editor->close(); // Will delete
        }
    } else {
        editor->show();
    }
}

void CodeEditorDialog::createActions()
{
    // Save Action
    // Try to find standard icons. If fail, text will show.
    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    m_saveAction = new QAction(saveIcon, "Guardar", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip("Guardar el archivo actual");
    connect(m_saveAction, &QAction::triggered, this, &CodeEditorDialog::onSave);
    
    // Save As Action
    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    m_saveAsAction = new QAction(saveAsIcon, "Guardar Como...", this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setStatusTip("Guardar el archivo con otro nombre");
    connect(m_saveAsAction, &QAction::triggered, this, &CodeEditorDialog::onSaveAs);
    
    // Close Action
    const QIcon closeIcon = QIcon::fromTheme("window-close");
    m_closeAction = new QAction(closeIcon, "Cerrar", this);
    m_closeAction->setShortcut(QKeySequence::Close);
    connect(m_closeAction, &QAction::triggered, this, &CodeEditorDialog::close);
}

void CodeEditorDialog::createToolBar()
{
    QToolBar *toolBar = addToolBar("Archivo");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24)); // Nice size
    
    toolBar->addAction(m_saveAction);
    toolBar->addAction(m_saveAsAction);
    
    // Add separator
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);
    
    // Right setup (optional) or just close button
    // toolBar->addAction(m_closeAction); // Close in toolbar is weird, usually X is enough
}

void CodeEditorDialog::createStatusBar()
{
    m_statusLabel = new QLabel("Listo", this);
    statusBar()->addWidget(m_statusLabel);
}

bool CodeEditorDialog::openFile(const QString &fileName)
{
    if (!m_editor->loadFile(fileName)) {
        QMessageBox::warning(this, "Error",
            QString("No se pudo abrir el archivo:\n%1").arg(fileName));
        return false;
    }
    
    QFileInfo fi(fileName);
    setWindowFilePath(fileName); // This helps with window management on some OS
    setWindowTitle(QString("%1 - Editor de Código").arg(fi.fileName()));
    m_statusLabel->setText("Abierto: " + fileName);
    
    return true;
}

bool CodeEditorDialog::saveFile()
{
    if (m_editor->currentFile().isEmpty()) {
        return saveFileAs();
    }
    
    if (!m_editor->saveFile()) {
        QMessageBox::warning(this, "Error",
            QString("No se pudo guardar el archivo:\n%1\nVerifique permisos o si el archivo está en uso.").arg(m_editor->currentFile()));
        return false;
    }
    
    m_statusLabel->setText(QString("Guardado: %1").arg(m_editor->currentFile()));
    setWindowTitle(QFileInfo(m_editor->currentFile()).fileName() + " - Editor de Código"); // Reset title (remove *)
    return true;
}

bool CodeEditorDialog::saveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Guardar Código",
        m_editor->currentFile().isEmpty() ? "" : m_editor->currentFile(),
        "Archivos BennuGD (*.prg *.inc *.h);;Todos los archivos (*)"
    );
    
    if (fileName.isEmpty()) {
        return false;
    }
    
    if (!m_editor->saveFile(fileName)) {
        QMessageBox::warning(this, "Error",
            QString("No se pudo guardar el archivo:\n%1").arg(fileName));
        return false;
    }
    
    setWindowFilePath(fileName);
    setWindowTitle(QString("%1 - Editor de Código").arg(QFileInfo(fileName).fileName()));
    m_statusLabel->setText(QString("Guardado: %1").arg(fileName));
    
    return true;
}

void CodeEditorDialog::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void CodeEditorDialog::onSave()
{
    saveFile();
}

void CodeEditorDialog::onSaveAs()
{
    saveFileAs();
}

void CodeEditorDialog::onDocumentModified()
{
    QString title = windowTitle();
    if (m_editor->isModified()) {
        if (!title.startsWith("*")) {
             setWindowTitle("*" + title);
        }
    } else {
        if (title.startsWith("*")) {
             setWindowTitle(title.mid(1));
        }
    }
}

bool CodeEditorDialog::maybeSave()
{
    if (!m_editor->isModified()) {
        return true;
    }
    
    QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        "Código Modificado",
        "El código ha sido modificado.\n¿Deseas guardar los cambios?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    
    if (ret == QMessageBox::Save) {
        return saveFile();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    
    return true;
}
