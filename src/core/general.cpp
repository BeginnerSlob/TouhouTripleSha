#include "general.h"
#include "client.h"
#include "engine.h"
#include "package.h"
#include "skill.h"

#include <QFile>
#include <QSize>

General::General(Package *package, const QString &name, const QString &kingdom, int max_hp, int start_hp, bool male,
                 bool hidden, bool never_shown)
    : QObject(package)
    , kingdom(kingdom)
    , max_hp(max_hp)
    , start_hp(start_hp)
    , gender(male ? Male : Female)
    , hidden(hidden)
    , never_shown(never_shown)
{
    static QChar lord_symbol('$');
    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        lord = true;
        setObjectName(copy);
    } else {
        lord = false;
        setObjectName(name);
    }
}

General::General(Package *package, const QString &name, const QString &kingdom, int max_hp, bool male, bool hidden,
                 bool never_shown)
    : QObject(package)
    , kingdom(kingdom)
    , max_hp(max_hp)
    , start_hp(max_hp)
    , gender(male ? Male : Female)
    , hidden(hidden)
    , never_shown(never_shown)
{
    static QChar lord_symbol('$');
    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        lord = true;
        setObjectName(copy);
    } else {
        lord = false;
        setObjectName(name);
    }
}

int General::getMaxHp() const
{
    return max_hp;
}

int General::getStartHp() const
{
    return start_hp;
}

QString General::getKingdom() const
{
    return kingdom;
}

bool General::isMale() const
{
    return gender == Male;
}

bool General::isFemale() const
{
    return gender == Female;
}

bool General::isNeuter() const
{
    return gender == Neuter;
}

void General::setGender(Gender gender)
{
    this->gender = gender;
}

General::Gender General::getGender() const
{
    return gender;
}

bool General::isLord() const
{
    return lord;
}

bool General::isHidden() const
{
    return hidden;
}

bool General::isTotallyHidden() const
{
    return never_shown;
}

#include <QMessageBox>
void General::addSkill(Skill *skill)
{
    if (!skill) {
        QMessageBox::warning(NULL, "", tr("Invalid skill added to general %1").arg(objectName()));
        return;
    }
    if (!skillname_list.contains(skill->objectName())) {
        skill->setParent(this);
        skillname_list << skill->objectName();
    }
}

void General::addSkill(const QString &skill_name)
{
    if (!skillname_list.contains(skill_name)) {
        extra_set.insert(skill_name);
        skillname_list << skill_name;
    }
}

bool General::hasSkill(const QString &skill_name) const
{
    return skillname_list.contains(skill_name);
}

QList<const Skill *> General::getSkillList() const
{
    QList<const Skill *> skills;
    foreach (QString skill_name, skillname_list) {
        if (skill_name == "thjibu" && ServerInfo.DuringGame && ServerInfo.GameMode == "02_1v1"
            && ServerInfo.GameRuleMode != "Classical")
            skill_name = "xiaoxi";
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill)
            skills << skill;
    }
    return skills;
}

QList<const Skill *> General::getVisibleSkillList() const
{
    QList<const Skill *> skills;
    foreach (const Skill *skill, getSkillList()) {
        if (skill->isVisible())
            skills << skill;
    }

    return skills;
}

QSet<const Skill *> General::getVisibleSkills() const
{
    return getVisibleSkillList().toSet();
}

QSet<const TriggerSkill *> General::getTriggerSkills() const
{
    QSet<const TriggerSkill *> skills;
    foreach (QString skill_name, skillname_list) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if (skill)
            skills << skill;
    }
    return skills;
}

void General::addRelateSkill(const QString &skill_name)
{
    related_skills << skill_name;
}

QStringList General::getRelatedSkillNames() const
{
    return related_skills;
}

QString General::getPackage() const
{
    QObject *p = parent();
    if (p)
        return p->objectName();
    else
        return QString(); // avoid null pointer exception;
}

QString General::getSkillDescription(bool include_name) const
{
    QString description;

    foreach (const Skill *skill, getVisibleSkillList()) {
        QString skill_name = Sanguosha->translate(skill->objectName());
        QString desc = skill->getDescription();
        desc.replace("\n", "<br/>");
        description.append(QString("<b>%1</b>: %2 <br/> <br/>").arg(skill_name).arg(desc));
    }

    if (include_name) {
        QString color_str = Sanguosha->getKingdomColor(kingdom).name();
        QString name = QString("<br/><font color=%1><b>%2-%3</b></font>")
                           .arg(color_str)
                           .arg(objectName().toUpper())
                           .arg(Sanguosha->translate(objectName()));
        for (int i = 0; i < max_hp; i++)
            name.prepend("<img src='image/system/magatamas/4.png' height = 12/>");
        name.prepend(QString("<img src='image/kingdom/icon/%1.png'/>    ").arg(kingdom));
        name.append("<br/> <br/>");
        description.prepend(name);
    }

    return description;
}

QString General::getTranslatedName() const
{
    QString name = "&" + objectName();
    QString translated = Sanguosha->translate(name);
    if (translated == name)
        translated = Sanguosha->translate(objectName());
    return translated;
}

void General::lastWord() const
{
    QString filename = QString("audio/death/%1.ogg").arg(objectName());
    bool fileExists = QFile::exists(filename);
    if (!fileExists) {
        QStringList origin_generals = objectName().split("_");
        if (origin_generals.length() > 1)
            filename = QString("audio/death/%1.ogg").arg(origin_generals.last());
    }
    Sanguosha->playAudioEffect(filename);
}
