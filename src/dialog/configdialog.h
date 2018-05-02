#ifndef _CONFIG_DIALOG_H
#define _CONFIG_DIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();

private:
    Ui::ConfigDialog *ui;
    void showFont(QLineEdit *lineedit, const QFont &font);

private slots:
    void on_setTextEditColorButton_clicked();
    void on_setTextEditFontButton_clicked();
    void on_changeAppFontButton_clicked();
    void on_resetBgMusicButton_clicked();
    void on_browseBgMusicButton_clicked();
    void on_resetBgButton_clicked();
    void on_browseBgButton_clicked();
    void on_chooseGeneralButton_clicked();
    void saveConfig();
    void specifyGeneral(const QString &general);

signals:
    void bg_changed();
};

#endif
