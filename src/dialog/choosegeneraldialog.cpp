#include "choosegeneraldialog.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "protocol.h"
#include "settings.h"
#include "skin-bank.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalMapper>
#include <QTabWidget>
#ifdef Q_OS_ANDROID
#include <QToolTip>
#endif

using namespace QSanProtocol;

OptionButton::OptionButton(QString icon_path, const QString &caption, QWidget *parent)
    : QToolButton(parent)
{
    QPixmap pixmap(icon_path);
    QIcon icon(pixmap);

    setIcon(icon);
    setIconSize(pixmap.size());

    if (!caption.isEmpty()) {
        setText(caption);
        setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        int fontId = QFontDatabase::addApplicationFont("font/home.ttf");
        QString fzlb = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont font = QFont(fzlb);
        font.setPixelSize(19);
        setFont(font);
    }
}

#ifdef Q_OS_ANDROID
void OptionButton::mousePressEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()))
        QToolTip::showText(event->globalPos() + QPoint(2, 5), toolTip());
}

void OptionButton::mouseMoveEvent(QMouseEvent *event)
{
    if (!rect().contains(event->pos()))
        QToolTip::hideText();
}

void OptionButton::mouseReleaseEvent(QMouseEvent *)
{
    QToolTip::hideText();
}
#endif

void OptionButton::mouseDoubleClickEvent(QMouseEvent *)
{
    emit double_clicked();
}

ChooseGeneralDialog::ChooseGeneralDialog(const QStringList &general_names, QWidget *parent, bool view_only,
                                         const QString &title)
    : QDialog(parent)
{
    m_freeChooseDialog = NULL;
    if (title.isEmpty())
        setWindowTitle(tr("Choose general"));
    else
        setWindowTitle(title);

    QString lord_name;

    QList<const General *> generals;
    foreach (QString general_name, general_names) {
        if (general_name.contains("(lord)")) {
            general_name.chop(6);
            lord_name = general_name;
            continue;
        }
        const General *general = Sanguosha->getGeneral(general_name);
        generals << general;
    }

    QSignalMapper *mapper = new QSignalMapper(this);
    QList<OptionButton *> buttons;
    bool tooManyManyGenerals = (generals.length() > G_COMMON_LAYOUT.m_chooseGeneralBoxNoIconThreshold);
    bool tooManyGenerals = (generals.length() > G_COMMON_LAYOUT.m_chooseGeneralBoxSwitchIconSizeThreshold);
    bool no_icon = false;
    QSize icon_size;
    QSanRoomSkin::GeneralIconSize icon_type = QSanRoomSkin::S_GENERAL_ICON_SIZE_CARD;
    if (tooManyManyGenerals) {
        no_icon = true;
    } else {
        if (tooManyGenerals) {
            icon_type = QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE;
            icon_size = G_COMMON_LAYOUT.m_chooseGeneralBoxDenseIconSize;
        } else {
            icon_type = QSanRoomSkin::S_GENERAL_ICON_SIZE_CARD;
            icon_size = G_COMMON_LAYOUT.m_chooseGeneralBoxSparseIconSize;
        }
    }
    foreach (const General *general, generals) {
        QString caption;
        if (general != NULL) {
            caption = Sanguosha->translate(general->objectName());
            if (caption.length() > 8) {
                QString name = "&" + general->objectName();
                QString translated = Sanguosha->translate(name);
                if (translated != name)
                    caption = translated;
            }
        }
        OptionButton *button = new OptionButton(QString(), caption);
        if (no_icon) {
            button->setIcon(QIcon("image/system/no-general-icon.png"));
            button->setIconSize(QSize(G_COMMON_LAYOUT.m_chooseGeneralBoxDenseIconSize.width(), 1));
        } else {
            button->setIcon(QIcon(G_ROOM_SKIN.getGeneralPixmap(general->objectName(), icon_type)));
            button->setIconSize(icon_size);
            button->setMaximumWidth(icon_size.width());
        }
        button->setToolTip(general->getSkillDescription(true));
        buttons << button;

        if (!view_only) {
            mapper->setMapping(button, general->objectName());
            connect(button, (void (OptionButton::*)())(&OptionButton::double_clicked), mapper,
                    (void (QSignalMapper::*)())(&QSignalMapper::map));
            connect(button, &OptionButton::double_clicked, this, &ChooseGeneralDialog::accept);
        }
    }

    if (!view_only && ServerInfo.EnableHegemony && ServerInfo.Enable2ndGeneral && generals.length() > 2) {
        int index = 0;
        foreach (const General *general, generals) {
            int party = 0;
            foreach (const General *other, generals)
                if (other->getKingdom() == general->getKingdom())
                    party++;
            if (party < 2)
                buttons.at(index)->setEnabled(false);
            if (Self->getGeneral())
                if (Self->getGeneral()->getKingdom() != general->getKingdom()
                    || Self->getGeneralName() == general->objectName())
                    buttons.at(index)->setEnabled(false);
            index++;
        }
    }

    QLayout *layout = NULL;
    const int columns = tooManyGenerals ? G_COMMON_LAYOUT.m_chooseGeneralBoxSwitchIconEachRowForTooManyGenerals
                                        : G_COMMON_LAYOUT.m_chooseGeneralBoxSwitchIconEachRow;
    if (generals.length() <= columns) {
        layout = new QHBoxLayout;

        if (lord_name.size() && !ServerInfo.EnableHegemony && !no_icon) {
            const General *lord = Sanguosha->getGeneral(lord_name);

#ifndef Q_OS_ANDROID
            QLabel *label = new QLabel;
            label->setPixmap(G_ROOM_SKIN.getGeneralPixmap(lord->objectName(), icon_type));
#else
            OptionButton *label = new OptionButton(QString(), lord->getTranslatedName());
            label->setIcon(QIcon(G_ROOM_SKIN.getGeneralPixmap(lord->objectName(), icon_type)));
            label->setIconSize(icon_size);
#endif
            label->setToolTip(lord->getSkillDescription(true));
            layout->addWidget(label);
        }

        foreach (OptionButton *button, buttons)
            layout->addWidget(button);
    } else {
        QGridLayout *grid_layout = new QGridLayout;
        QHBoxLayout *hlayout = new QHBoxLayout;
        QVBoxLayout *lord_layout = new QVBoxLayout;

        if (lord_name.size() && !ServerInfo.EnableHegemony && !no_icon) {
            const General *lord = Sanguosha->getGeneral(lord_name);

#ifndef Q_OS_ANDROID
            QLabel *label = new QLabel;
            label->setPixmap(G_ROOM_SKIN.getCardMainPixmap(lord->objectName()));
#else
            OptionButton *label = new OptionButton(QString(), lord->getTranslatedName());
            label->setIcon(QIcon(G_ROOM_SKIN.getCardMainPixmap(lord->objectName())));
            label->setIconSize(G_COMMON_LAYOUT.m_chooseGeneralBoxSparseIconSize);
#endif
            label->setToolTip(lord->getSkillDescription(true));
            lord_layout->addWidget(label);
        }
        lord_layout->addStretch();
        hlayout->addLayout(lord_layout);

        int columns_x = qMin(columns, (buttons.length() + 1) / 2);
        for (int i = 0; i < buttons.length(); i++) {
            int row = i / columns_x;
            int column = i % columns_x;
            grid_layout->addWidget(buttons.at(i), row, column);
        }
        hlayout->addLayout(grid_layout);
        layout = hlayout;
    }

    QString default_name = "random";

    if (!view_only) {
        mapper->setMapping(this, default_name);
        connect(this, (void (ChooseGeneralDialog::*)())(&ChooseGeneralDialog::rejected), mapper,
                (void (QSignalMapper::*)())(&QSignalMapper::map));

        connect(mapper, (void (QSignalMapper::*)(const QString &))(&QSignalMapper::mapped), ClientInstance,
                &Client::onPlayerChooseGeneral);
    }

    QVBoxLayout *dialog_layout = new QVBoxLayout;
    dialog_layout->addLayout(layout);

    if (!view_only && !ServerInfo.EnableHegemony) {
        // role prompt
        QLabel *role_label = new QLabel(tr("Your role is %1").arg(Sanguosha->translate(Self->getRole())));
        if (lord_name.size())
            role_label->setText(tr("The lord has chosen %1. Your seat is %2. %3")
                                    .arg(Sanguosha->translate(lord_name))
                                    .arg(Sanguosha->translate("CAPITAL(" + QString::number(Self->getSeat()) + ")"))
                                    .arg(role_label->text()));
        dialog_layout->addWidget(role_label);
    }

    // progress bar & free choose button
    QHBoxLayout *last_layout = new QHBoxLayout;
    if (view_only || ServerInfo.OperationTimeout == 0) {
        progress_bar = NULL;
    } else {
        progress_bar = new QSanCommandProgressBar();
        progress_bar->setFixedWidth(300);
        progress_bar->setTimerEnabled(true);
        progress_bar->setCountdown(S_COMMAND_CHOOSE_GENERAL);
        progress_bar->show();
        last_layout->addWidget(progress_bar);
    }

    bool free_choose
        = ServerInfo.FreeChoose || ServerInfo.GameMode.startsWith("_mini_") || ServerInfo.GameMode == "custom_scenario";

    if (!view_only && free_choose) {
        QPushButton *free_choose_button = new QPushButton(tr("Free choose ..."));
        connect(free_choose_button, &QPushButton::clicked, this, &ChooseGeneralDialog::freeChoose);
        last_layout->addWidget(free_choose_button);
    }

    last_layout->addStretch();

    if (last_layout->count() != 0) {
        dialog_layout->addLayout(last_layout);
    }

    setLayout(dialog_layout);
}

void ChooseGeneralDialog::done(int result)
{
    if (m_freeChooseDialog != NULL) {
        m_freeChooseDialog->reject();
        delete m_freeChooseDialog;
        m_freeChooseDialog = NULL;
    }
    QDialog::done(result);
}

void ChooseGeneralDialog::freeChoose()
{
    FreeChooseDialog *dialog = new FreeChooseDialog(this);

    connect(dialog, &FreeChooseDialog::accepted, this, &ChooseGeneralDialog::accept);
    connect(dialog, &FreeChooseDialog::general_chosen, ClientInstance, &Client::onPlayerChooseGeneral);

    m_freeChooseDialog = dialog;

    dialog->exec();
}

FreeChooseDialog::FreeChooseDialog(QWidget *parent, ButtonGroupType type)
    : QDialog(parent)
    , type(type)
{
    setWindowTitle(tr("Free choose generals"));

    QTabWidget *tab_widget = new QTabWidget;

    group = new QButtonGroup(this);
    group->setExclusive(type == Exclusive);

    QList<const General *> all_generals = Sanguosha->getGeneralList();
    QMap<QString, QList<const General *> > map;
    foreach (const General *general, all_generals) {
        if (general->isTotallyHidden())
            continue;

        map[general->getKingdom()] << general;
    }

    QStringList kingdoms = Sanguosha->getKingdoms();

    foreach (QString kingdom, kingdoms) {
        QList<const General *> generals = map[kingdom];

        if (!generals.isEmpty()) {
            QWidget *tab = createTab(generals);
            tab_widget->addTab(tab, QIcon(G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_KINGDOM_ICON, kingdom)),
                               Sanguosha->translate(kingdom));
        }
    }

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, &QPushButton::clicked, this, &FreeChooseDialog::chooseGeneral);

    QPushButton *cancel_button = new QPushButton(tr("Cancel"));
    connect(cancel_button, &QPushButton::clicked, this, &FreeChooseDialog::reject);

    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addStretch();
    button_layout->addWidget(ok_button);
    button_layout->addWidget(cancel_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(tab_widget);
    layout->addLayout(button_layout);

    setLayout(layout);

    if (type == Exclusive)
        group->buttons().first()->click();
}

void FreeChooseDialog::chooseGeneral()
{
    if (type == Pair) {
        QList<QAbstractButton *> buttons = group->buttons();
        QString first, second;
        foreach (QAbstractButton *button, buttons) {
            if (!button->isChecked())
                continue;

            if (first.isEmpty())
                first = button->objectName();
            else {
                second = button->objectName();
                emit pair_chosen(first, second);
                break;
            }
        }
        if (second.isEmpty())
            emit general_chosen(first);
    } else if (type == Multi) {
        QStringList general_names;
        foreach (QAbstractButton *button, group->buttons()) {
            if (button->isChecked())
                general_names << button->objectName();
        }
        if (!general_names.isEmpty())
            emit general_chosen(general_names.join("+"));
    } else {
        QAbstractButton *button = group->checkedButton();
        if (button)
            emit general_chosen(button->objectName());
    }

    accept();
}

QWidget *FreeChooseDialog::createTab(const QList<const General *> &generals)
{
    QWidget *tab = new QWidget;

    QGridLayout *layout = new QGridLayout;
    layout->setOriginCorner(Qt::TopLeftCorner);
    QIcon lord_icon("image/system/roles/lord.png");

    const int columns = 5;

    for (int i = 0; i < generals.length(); i++) {
        const General *general = generals.at(i);
        QString general_name = general->objectName();
        QString text
            = QString("%1[%2]").arg(Sanguosha->translate(general_name)).arg(Sanguosha->translate(general->getPackage()));

        QAbstractButton *button;
        if (type == Exclusive)
            button = new QRadioButton(text);
        else
            button = new QCheckBox(text);
        button->setObjectName(general_name);
        button->setToolTip(general->getSkillDescription(true));
        if (general->isLord())
            button->setIcon(lord_icon);

        group->addButton(button);

        int row = i / columns;
        int column = i % columns;
        layout->addWidget(button, row, column);
    }

    QVBoxLayout *layout2 = new QVBoxLayout;
    layout2->addStretch();

    QVBoxLayout *tablayout = new QVBoxLayout;
    tablayout->addLayout(layout);
    tablayout->addLayout(layout2);

    tab->setLayout(tablayout);

    if (type == Pair)
        connect(group, (void (QButtonGroup::*)(QAbstractButton *))(&QButtonGroup::buttonClicked), this,
                &FreeChooseDialog::uncheckExtraButton);

    return tab;
}

void FreeChooseDialog::uncheckExtraButton(QAbstractButton *click_button)
{
    QAbstractButton *first = NULL;
    QList<QAbstractButton *> buttons = group->buttons();
    foreach (QAbstractButton *button, buttons) {
        if (!button->isChecked())
            continue;

        if (button == click_button)
            continue;

        if (first == NULL)
            first = button;
        else {
            first->setChecked(false);
            break;
        }
    }
}
