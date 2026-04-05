#pragma once

#include <QMainWindow>

class QLabel;
class QPlainTextEdit;
class CrawlWindow;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QString currentEditingStatus() const;
    void    loadIntoEditor();
    void    setStatusForCurrentPath(const QString &prefix, const QString &overridePath = {});
    void    configureCrawlWindow(CrawlWindow *window);
    void    ensureCrawlWindow();
    bool    saveEditorContents();
    bool    confirmDiscardOrSave();
    void    enterShowMode();

    QPlainTextEdit *m_editor      = nullptr;
    QLabel         *m_statusLabel = nullptr;
    CrawlWindow    *m_crawlWindow = nullptr;
    bool            m_hasUnsavedChanges = false;
};
