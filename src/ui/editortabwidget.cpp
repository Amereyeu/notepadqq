#include "include/editortabwidget.h"
#include <QTabBar>
#include <QApplication>
#include <QFileInfo>

#ifdef QT_DEBUG
#include <QElapsedTimer>
#endif

EditorTabWidget::EditorTabWidget(QWidget *parent) :
    QTabWidget(parent)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setDocumentMode(true);
    this->setTabsClosable(true);
    this->setMovable(true);

    this->setTabBarHidden(false);
    this->setTabBarHighlight(false);

    QString style = QString("QTabBar::tab{min-width:100px; height:24px;}");
    setStyleSheet(style);
}

int EditorTabWidget::addEditorTab(bool setFocus, const QString &title)
{
    return this->rawAddEditorTab(setFocus, title, 0, 0);
}

void EditorTabWidget::connectEditorSignals(Editor *editor)
{
    connect(editor, &Editor::cleanChanged,
            this, &EditorTabWidget::on_cleanChanged);

    connect(editor, &Editor::gotFocus, this, &EditorTabWidget::gotFocus);

    connect(editor, &Editor::mouseWheel,
            this, &EditorTabWidget::on_editorMouseWheel);

    connect(editor, &Editor::fileNameChanged,
            this, &EditorTabWidget::on_fileNameChanged);
}

void EditorTabWidget::disconnectEditorSignals(Editor *editor)
{
    disconnect(editor, &Editor::cleanChanged,
               this, &EditorTabWidget::on_cleanChanged);

    disconnect(editor, &Editor::gotFocus, this, &EditorTabWidget::gotFocus);

    disconnect(editor, &Editor::mouseWheel,
               this, &EditorTabWidget::on_editorMouseWheel);

    disconnect(editor, &Editor::fileNameChanged,
               this, &EditorTabWidget::on_fileNameChanged);
}

int EditorTabWidget::transferEditorTab(bool setFocus, EditorTabWidget *source, int tabIndex)
{
    return this->rawAddEditorTab(setFocus, QString(), source, tabIndex);
}

/**
 * @brief Do NOT directly connect to Editor signals within this method,
 *        or they'll remain attached to this EditorTabWidget whenever the
 *        tab gets moved to another container. Use connectEditorSignals()
 *        and disconnectEditorSignals() methods instead.
 */
int EditorTabWidget::rawAddEditorTab(const bool setFocus, const QString &title, EditorTabWidget *source, const int sourceTabIndex)
{
#ifdef QT_DEBUG
    QElapsedTimer __aet_timer;
    __aet_timer.start();
#endif

    bool create = (source == 0);

    this->setUpdatesEnabled(false);

    Editor *editor;

    QString oldText;
    QIcon oldIcon;
    QString oldTooltip;

    if (create) {
        editor = Editor::getNewEditor();
        editor->setParent(this);
    } else {
        editor = (Editor *)source->widget(sourceTabIndex);

        oldText = source->tabText(sourceTabIndex);
        oldIcon = source->tabIcon(sourceTabIndex);
        oldTooltip = source->tabToolTip(sourceTabIndex);
    }

    int index = this->addTab(editor, create ? title : oldText);
    if (!create) {
        source->disconnectEditorSignals(editor);
    }
    this->connectEditorSignals(editor);

    if(setFocus) {
        this->setCurrentIndex(index);
        editor->setFocus();
    }

    if (create) {
        this->setSavedIcon(index, true);
    } else {
        this->setTabIcon(index, oldIcon);
        this->setTabToolTip(index, oldTooltip);
    }

    // Common setup
    editor->setZoomFactor(m_zoomFactor);

    this->setUpdatesEnabled(true);

    emit editorAdded(index);

#ifdef QT_DEBUG
    qint64 __aet_elapsed = __aet_timer.nsecsElapsed();
    qDebug() << QString("Tab opened in " + QString::number(__aet_elapsed / 1000 / 1000) + "msec").toStdString().c_str();
#endif

    return index;
}

int EditorTabWidget::findOpenEditorByUrl(const QUrl &filename)
{
    QUrl absFileName = filename;
    if (absFileName.isLocalFile())
        absFileName = QUrl::fromLocalFile(QFileInfo(filename.toLocalFile()).absoluteFilePath());

    for (int i = 0; i < this->count(); i++) {
        Editor *editor = (Editor *)this->widget(i);
        if (editor->fileName() == filename)
            return i;
    }

    return -1;
}

Editor *EditorTabWidget::editor(int index)
{
    return (Editor *)this->widget(index);
}

Editor *EditorTabWidget::currentEditor()
{
    return (Editor *)this->currentWidget();
}
qreal EditorTabWidget::zoomFactor() const
{
    return m_zoomFactor;
}

void EditorTabWidget::setZoomFactor(const qreal &zoomFactor)
{
    m_zoomFactor = zoomFactor;

    for (int i = 0; i < count(); i++) {
        editor(i)->setZoomFactor(zoomFactor);
    }
}


void EditorTabWidget::setSavedIcon(int index, bool saved)
{
    if(saved)
        this->setTabIcon(index, QIcon(":/icons/icons/saved.png"));
    else
        this->setTabIcon(index, QIcon(":/icons/icons/unsaved.png"));
}

void EditorTabWidget::setTabBarHidden(bool yes)
{
    tabBar()->setHidden(yes);
}

void EditorTabWidget::setTabBarHighlight(bool yes)
{
    // Get colors from palette so it doesn't look fugly.
    QPalette palette = tabBar()->palette();
    palette.setColor(QPalette::Highlight, yes ? QApplication::palette().highlight().color() : QApplication::palette().light().color());
    tabBar()->setPalette(palette);
}

void EditorTabWidget::on_cleanChanged(bool isClean)
{
    int index = indexOf((QWidget *)sender());
    if(index >= 0)
        setSavedIcon(index, isClean);
}

void EditorTabWidget::on_editorMouseWheel(QWheelEvent *ev)
{
    Editor *editor = (Editor *)sender();
    emit editorMouseWheel(indexOf(editor), ev);
}

void EditorTabWidget::on_fileNameChanged(const QUrl & /*oldFileName*/, const QUrl &newFileName)
{
    Editor *editor = (Editor *)sender();
    int index = indexOf(editor);

    QString fileName = QFileInfo(newFileName.toDisplayString(QUrl::RemoveScheme |
                                                   QUrl::RemovePassword |
                                                   QUrl::RemoveUserInfo |
                                                   QUrl::RemovePort |
                                                   QUrl::RemoveAuthority |
                                                   QUrl::RemoveQuery |
                                                   QUrl::RemoveFragment |
                                                   QUrl::PreferLocalFile
                                                   )).fileName();

    QString fullFileName = newFileName.toDisplayString(QUrl::PreferLocalFile |
                                                       QUrl::RemovePassword);

    setTabText(index, fileName);
    setTabToolTip(index, fullFileName);
}