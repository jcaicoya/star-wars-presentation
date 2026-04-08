#pragma once

#include <QPlainTextEdit>

class LineNumberArea;

class LineNumberTextEdit final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit LineNumberTextEdit(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void setColumnGuide(int column);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void paintLineNumbers(QPaintEvent *event);

    LineNumberArea *m_lineNumberArea;
    int m_columnGuide = -1;
    friend class LineNumberArea;
};

class LineNumberArea final : public QWidget {
public:
    explicit LineNumberArea(LineNumberTextEdit *editor)
        : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override {
        return {m_editor->lineNumberAreaWidth(), 0};
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        m_editor->paintLineNumbers(event);
    }

private:
    LineNumberTextEdit *m_editor;
};
