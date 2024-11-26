#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "Preferences/Preferences.h"

#include <QObject>
#include <QString>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager* instance();
    QStringList themesAvailable() const;
    Preferences::ThemeType getSelectedTheme() const;
    QString getSelectedThemeString() const;
    void setTheme(Preferences::ThemeType theme);

signals:
    void themeChanged(Preferences::ThemeType theme);

private:
    ThemeManager();

    Preferences::ThemeType mCurrentTheme;
    static const QMap<Preferences::ThemeType, QString> mThemesMap;
};

#endif
