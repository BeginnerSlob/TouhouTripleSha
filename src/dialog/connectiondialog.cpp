#include "connectiondialog.h"
#ifdef Q_OS_ANDROID
#include "ui_connectiondialog_android.h"
#else
#include "ui_connectiondialog.h"
#endif
#include "detector.h"
#include "engine.h"
#include "settings.h"
#include "skin-bank.h"

#include <QBoxLayout>
#include <QMessageBox>
#include <QRadioButton>
#include <QTimer>

static const int ShrinkWidth = 285;
static const int ExpandWidth = 826;

void ConnectionDialog::hideAvatarList()
{
    if (!ui->avatarList->isVisible())
        return;
    ui->avatarList->hide();
    ui->avatarList->clear();
}

void ConnectionDialog::showAvatarList()
{
    if (ui->avatarList->isVisible())
        return;
    ui->avatarList->clear();
    QList<const General *> generals = Sanguosha->getGeneralList();
    foreach (const General *general, generals) {
        QIcon icon(G_ROOM_SKIN.getGeneralPixmap(general->objectName(), QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
        QListWidgetItem *item = new QListWidgetItem(icon, QString(), ui->avatarList);
        item->setData(Qt::UserRole, general->objectName());
    }
    ui->avatarList->show();
}

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);

    ui->nameLineEdit->setText(Config.UserName);
    ui->nameLineEdit->setMaxLength(8);
#ifdef Q_OS_ANDROID
    ui->nameLineEdit->setText(Config.HostAddress);
#else
    ui->hostComboBox->addItems(Config.HistoryIPs);
    ui->hostComboBox->lineEdit()->setText(Config.HostAddress);
#endif

    ui->connectButton->setFocus();

    ui->avatarPixmap->setPixmap(G_ROOM_SKIN.getGeneralPixmap(Config.UserAvatar, QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));

    hideAvatarList();

    ui->reconnectionCheckBox->setChecked(Config.value("EnableReconnection", false).toBool());

    setFixedHeight(height());
    setFixedWidth(ShrinkWidth);
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::on_connectButton_clicked()
{
    QString username = ui->nameLineEdit->text();

    if (username.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("The user name can not be empty!"));
        return;
    }

    Config.UserName = username;
#ifdef Q_OS_ANDROID
    Config.HostAddress = ui->hostLineEdit->text();
#else
    Config.HostAddress = ui->hostComboBox->lineEdit()->text();
#endif

    Config.setValue("UserName", Config.UserName);
    Config.setValue("HostAddress", Config.HostAddress);
    Config.setValue("EnableReconnection", ui->reconnectionCheckBox->isChecked());

    accept();
}

void ConnectionDialog::on_changeAvatarButton_clicked()
{
    if (ui->avatarList->isVisible()) {
        QListWidgetItem *selected = ui->avatarList->currentItem();
        if (selected)
            on_avatarList_itemDoubleClicked(selected);
        else {
            hideAvatarList();
            setFixedWidth(ShrinkWidth);
        }
    } else {
        showAvatarList();
        setFixedWidth(ExpandWidth);
    }
}

void ConnectionDialog::on_avatarList_itemDoubleClicked(QListWidgetItem *item)
{
    QString general_name = item->data(Qt::UserRole).toString();
    QPixmap avatar(G_ROOM_SKIN.getGeneralPixmap(general_name, QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
    ui->avatarPixmap->setPixmap(avatar);
    Config.UserAvatar = general_name;
    Config.setValue("UserAvatar", general_name);
    hideAvatarList();

    setFixedWidth(ShrinkWidth);
}

void ConnectionDialog::on_clearHistoryButton_clicked()
{
#ifdef Q_OS_ANDROID
    ui->hostLineEdit->clear();
#else
    ui->hostComboBox->clear();
    ui->hostComboBox->lineEdit()->clear();
#endif

    Config.HistoryIPs.clear();
    Config.remove("HistoryIPs");
}

void ConnectionDialog::on_detectLANButton_clicked()
{
    UdpDetectorDialog *detector_dialog = new UdpDetectorDialog(this);
#ifdef Q_OS_ANDROID
    connect(detector_dialog, &UdpDetectorDialog::address_chosen, ui->hostLineEdit, &QLineEdit::setText);
#else
    connect(detector_dialog, &UdpDetectorDialog::address_chosen, ui->hostComboBox->lineEdit(), &QLineEdit::setText);
#endif

    detector_dialog->exec();
}

// -----------------------------------

UdpDetectorDialog::UdpDetectorDialog(QDialog *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Detect available server's addresses at LAN"));
    detect_button = new QPushButton(tr("Refresh"));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(detect_button);

    list = new QListWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(list);
    layout->addLayout(hlayout);

    setLayout(layout);

    detector = NULL;
    connect(detect_button, SIGNAL(clicked()), this, SLOT(startDetection()));
    connect(list, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(chooseAddress(QListWidgetItem *)));

    detect_button->click();
}

void UdpDetectorDialog::startDetection()
{
    list->clear();
    detect_button->setEnabled(false);

    detector = new UdpDetector;
    connect(detector, SIGNAL(detected(QString, QString)), this, SLOT(addServerAddress(QString, QString)));
    QTimer::singleShot(2000, this, SLOT(stopDetection()));

    detector->detect();
}

void UdpDetectorDialog::stopDetection()
{
    detect_button->setEnabled(true);
    detector->stop();
    delete detector;
    detector = NULL;
}

void UdpDetectorDialog::addServerAddress(const QString &server_name, const QString &address_)
{
    QString address = address_;
    if (address.startsWith("::ffff:"))
        address.remove(0, 7);
    QString label = QString("%1 [%2]").arg(server_name).arg(address);
    QListWidgetItem *item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, address);

    list->addItem(item);
}

void UdpDetectorDialog::chooseAddress(QListWidgetItem *item)
{
    accept();

    QString address = item->data(Qt::UserRole).toString();
    emit address_chosen(address);
}
