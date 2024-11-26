#ifndef THEME_WIDGET_MANAGER_H
#define THEME_WIDGET_MANAGER_H

#include "Preferences/Preferences.h"

#include <QObject>
#include <QIcon>

class TokenParserWidgetManager : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<TokenParserWidgetManager> instance();

    void applyCurrentTheme();
    void applyCurrentTheme(QWidget* dialog);

private:
    using ColorTokens = QMap<QString, QString>;

    explicit TokenParserWidgetManager(QObject *parent = nullptr);
    void loadColorThemeJson();
    void loadStandardStyleSheetComponents();
    void onThemeChanged(Preferences::ThemeType theme);
    void onUpdateRequested();
    void applyTheme(QWidget* widget);
    bool replaceThemeTokens(QString& styleSheet, const QString& currentTheme);
    bool replaceIconColorTokens(QWidget* widget, QString& styleSheet, const ColorTokens& colorTokens);
    bool replaceColorTokens(QString& styleSheet, const ColorTokens& colorTokens);
    void removeFrameOnDialogCombos(QWidget* widget);

    QMap<QString, ColorTokens> mColorThemedTokens;
    QString mStandardComponentsStyleSheet;
};

#endif // THEMEWIDGET_H
