#include "configdialog.h"
#include "audio.h"
#include "choosegeneraldialog.h"
#include "roomscene.h"
#include "settings.h"
#include "ui_configdialog.h"

#include <QColorDialog>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDialog>

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    // tab 1
    QString bg_path = Config.value("BackgroundImage").toString();
    if (!bg_path.startsWith(":"))
        ui->bgPathLineEdit->setText(bg_path);

    ui->bgMusicPathLineEdit->setText(Config.value("BackgroundMusic", Config.m_defaultMusicPath).toString());

    ui->enableEffectCheckBox->setChecked(Config.EnableEffects);

    ui->enableLastWordCheckBox->setEnabled(Config.EnableEffects);
    ui->enableLastWordCheckBox->setChecked(Config.EnableLastWord);
    connect(ui->enableEffectCheckBox, &QCheckBox::toggled, ui->enableLastWordCheckBox, &QCheckBox::setEnabled);

    ui->enableBgMusicCheckBox->setChecked(Config.EnableBgMusic);

    //bool enabled_full = QFile::exists("skins/fulldefaultSkin.layout.json");
    //temp disabled
    bool enabled_full = false;
    ui->fullSkinCheckBox->setToolTip(tr("Temp Disabled"));
    ui->fullSkinCheckBox->setEnabled(enabled_full);
    ui->fullSkinCheckBox->setChecked(enabled_full && Config.value("UseFullSkin", false).toBool());
    ui->noIndicatorCheckBox->setChecked(Config.value("NoIndicator", true).toBool());
    ui->noEquipAnimCheckBox->setChecked(Config.value("NoEquipAnim", false).toBool());
    ui->noEffectsAnimCheckBox->setChecked(Config.value("NoEffectsAnim", false).toBool());

    ui->bgmVolumeSlider->setValue(100 * Config.BGMVolume);
    ui->effectVolumeSlider->setValue(100 * Config.EffectVolume);

    // tab 2
    ui->neverNullifyMyTrickCheckBox->setChecked(Config.NeverNullifyMyTrick);
    ui->autoTargetCheckBox->setChecked(Config.EnableAutoTarget);
    ui->intellectualSelectionCheckBox->setChecked(Config.EnableIntellectualSelection);
    ui->doubleClickCheckBox->setChecked(Config.EnableDoubleClick);
    ui->superDragCheckBox->setChecked(Config.EnableSuperDrag);
    ui->bubbleChatBoxKeepSpinBox->setSuffix(tr(" millisecond"));
    ui->bubbleChatBoxKeepSpinBox->setValue(Config.BubbleChatBoxKeepTime);
    // save replays
    bool open = Config.value("EnableAutoSaveRecord", true).toBool();
    ui->autoSaveReplaysCheckBox->setChecked(open);
    ui->replaysLayout->setEnabled(open);
    ui->replaysPathLineEdit->setText(Config.value("ReplaySavePaths", "replays/").toString());
    if (open)
        ui->saveNetworkOnlyCheckBox->setChecked(Config.value("SaveNetworkOnly", true).toBool());
    else
        ui->saveNetworkOnlyCheckBox->setChecked(false);
    open = Config.value("EnableAutoSpecifyGeneral", false).toBool();
    ui->specifyGeneralCheckBox->setChecked(open);
    ui->generalNameLineEdit->setEnabled(open);
    ui->chooseGeneralButton->setEnabled(open);

    connect(ui->specifyGeneralCheckBox, &QCheckBox::toggled, ui->generalNameLineEdit, &QLineEdit::setEnabled);
    connect(ui->specifyGeneralCheckBox, &QCheckBox::toggled, ui->chooseGeneralButton, &QPushButton::setEnabled);

    ui->generalNameLineEdit->setText(Config.value("AutoSpecifyGeneralName", "kaze001").toString());

    connect(this, &ConfigDialog::accepted, this, &ConfigDialog::saveConfig);

    QFont font = Config.AppFont;
    showFont(ui->appFontLineEdit, font);

    font = Config.UIFont;
    showFont(ui->textEditFontLineEdit, font);

    QPalette palette;
    palette.setColor(QPalette::Text, Config.TextEditColor);
    QColor color = Config.TextEditColor;
    int aver = (color.red() + color.green() + color.blue()) / 3;
    palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
    ui->textEditFontLineEdit->setPalette(palette);
}

void ConfigDialog::showFont(QLineEdit *lineedit, const QFont &font)
{
    lineedit->setFont(font);
    lineedit->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_browseBgButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a background image"), "image/system/backdrop/",
                                                    tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        QString app_path = QApplication::applicationDirPath();
        if (filename.startsWith(app_path))
            filename = filename.right(filename.length() - app_path.length() - 1);
        ui->bgPathLineEdit->setText(filename);

        Config.BackgroundImage = filename;
        Config.setValue("BackgroundImage", filename);

        emit bg_changed();
    }
}

void ConfigDialog::on_chooseGeneralButton_clicked()
{
    FreeChooseDialog *choose_general = new FreeChooseDialog(this);
    connect(choose_general, &FreeChooseDialog::general_chosen, this, &ConfigDialog::specifyGeneral);
    choose_general->exec();
}

void ConfigDialog::on_resetBgButton_clicked()
{
    ui->bgPathLineEdit->clear();

    QString filename = "image/system/backdrop/default.jpg";
    Config.BackgroundImage = filename;
    Config.setValue("BackgroundImage", filename);

    emit bg_changed();
}

void ConfigDialog::saveConfig()
{
    float volume = ui->bgmVolumeSlider->value() / 100.0;

    Audio::setBackgroundMusicVolume(volume);

    Config.BGMVolume = volume;
    Config.setValue("BGMVolume", volume);
    volume = ui->effectVolumeSlider->value() / 100.0;

    Audio::setEffectVolume(volume);

    Config.EffectVolume = volume;
    Config.setValue("EffectVolume", volume);

    bool enabled = ui->enableEffectCheckBox->isChecked();
    Config.EnableEffects = enabled;
    Config.setValue("EnableEffects", enabled);

    enabled = ui->enableLastWordCheckBox->isChecked();
    Config.EnableLastWord = enabled;
    Config.setValue("EnableLastWord", enabled);

    enabled = ui->enableBgMusicCheckBox->isChecked();

    if (!enabled)
        Audio::stopBackgroundMusic();

    Config.EnableBgMusic = enabled;
    Config.setValue("EnableBgMusic", enabled);

    QString newMusicPath = ui->bgMusicPathLineEdit->text();
    QString currentMusicPath = Config.value("BackgroundMusic", Config.m_defaultMusicPath).toString();
    if (newMusicPath != currentMusicPath) {
        Config.setValue("BackgroundMusic", newMusicPath);
        Audio::resetCustomBackgroundMusicFileName();

        if (Config.EnableBgMusic && Audio::isBackgroundMusicPlaying() && RoomSceneInstance != NULL
            && RoomSceneInstance->isGameStarted()) {
            Audio::stopBackgroundMusic();
            Audio::playBackgroundMusic(newMusicPath, true);
        }
    } else {
        if (Config.EnableBgMusic && NULL != RoomSceneInstance && RoomSceneInstance->isGameStarted()
            && !Audio::isBackgroundMusicPlaying()) {
            Audio::playBackgroundMusic(currentMusicPath, true);
        }
    }

    Config.setValue("UseFullSkin", ui->fullSkinCheckBox->isChecked());
    Config.setValue("NoIndicator", ui->noIndicatorCheckBox->isChecked());
    Config.setValue("NoEquipAnim", ui->noEquipAnimCheckBox->isChecked());
    Config.setValue("NoEffectsAnim", ui->noEffectsAnimCheckBox->isChecked());

    Config.NeverNullifyMyTrick = ui->neverNullifyMyTrickCheckBox->isChecked();
    Config.setValue("NeverNullifyMyTrick", Config.NeverNullifyMyTrick);

    Config.EnableAutoTarget = ui->autoTargetCheckBox->isChecked();
    Config.setValue("EnableAutoTarget", Config.EnableAutoTarget);

    Config.EnableIntellectualSelection = ui->intellectualSelectionCheckBox->isChecked();
    Config.setValue("EnableIntellectualSelection", Config.EnableIntellectualSelection);

    Config.EnableDoubleClick = ui->doubleClickCheckBox->isChecked();
    Config.setValue("EnableDoubleClick", Config.EnableDoubleClick);

    Config.EnableSuperDrag = ui->superDragCheckBox->isChecked();
    Config.setValue("EnableSuperDrag", Config.EnableSuperDrag);

    Config.BubbleChatBoxKeepTime = ui->bubbleChatBoxKeepSpinBox->value();
    Config.setValue("BubbleChatBoxKeepTime", Config.BubbleChatBoxKeepTime);

    Config.setValue("EnableAutoSaveRecord", ui->autoSaveReplaysCheckBox->isChecked());
    Config.setValue("SaveNetworkOnly", ui->saveNetworkOnlyCheckBox->isChecked());
    Config.setValue("ReplaySavePaths", ui->replaysPathLineEdit->text());

    Config.setValue("EnableAutoSpecifyGeneral", ui->specifyGeneralCheckBox->isChecked());
    Config.setValue("AutoSpecifyGeneralName", ui->generalNameLineEdit->text());

    if (RoomSceneInstance)
        RoomSceneInstance->updateVolumeConfig();
}

void ConfigDialog::specifyGeneral(const QString &general)
{
    ui->generalNameLineEdit->setText(general);
}

void ConfigDialog::on_browseBgMusicButton_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select a background music"), "audio/system",
                                                          tr("Audio files (*.wav *.mp3 *.ogg)"));
    QString app_path = QApplication::applicationDirPath();
    app_path.replace("\\", "/");
    int app_path_len = app_path.length();
    foreach (const QString &name, fileNames) {
        const_cast<QString &>(name).replace("\\", "/");
        if (name.startsWith(app_path)) {
            const_cast<QString &>(name) = name.right(name.length() - app_path_len - 1);
        }
    }
    QString filename = fileNames.join(";");
    if (!filename.isEmpty()) {
        ui->bgMusicPathLineEdit->setText(filename);
    }
}

void ConfigDialog::on_resetBgMusicButton_clicked()
{
    ui->bgMusicPathLineEdit->setText(Config.m_defaultMusicPath);
}

void ConfigDialog::on_changeAppFontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config.AppFont, this);
    if (ok) {
        Config.AppFont = font;
        showFont(ui->appFontLineEdit, font);

        Config.setValue("AppFont", font);
        QApplication::setFont(font);
    }
}

void ConfigDialog::on_setTextEditFontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config.UIFont, this);
    if (ok) {
        Config.UIFont = font;
        showFont(ui->textEditFontLineEdit, font);

        Config.setValue("UIFont", font);
        QApplication::setFont(font, "QTextEdit");
    }
}

void ConfigDialog::on_setTextEditColorButton_clicked()
{
    QColor color = QColorDialog::getColor(Config.TextEditColor, this);
    if (color.isValid()) {
        Config.TextEditColor = color;
        Config.setValue("TextEditColor", color);
        QPalette palette;
        palette.setColor(QPalette::Text, color);
        int aver = (color.red() + color.green() + color.blue()) / 3;
        palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
        ui->textEditFontLineEdit->setPalette(palette);
    }
}
