#ifndef MEGAIGNOREMANAGER_H
#define MEGAIGNOREMANAGER_H

#include "MegaIgnoreRules.h"

#include <QString>
#include <QFile>
#include <QMap>
#include <QMetaEnum>
#include <memory>

class MegaIgnoreManager
{
public:
    explicit MegaIgnoreManager(const QString& syncLocalFolder, bool createIfNotExist);
    explicit MegaIgnoreManager() = default;

    static bool isValid(const QString& syncLocalFolder);

    std::shared_ptr<MegaIgnoreSizeRule> getLowLimitRule() const;

    std::shared_ptr<MegaIgnoreSizeRule> getHighLimitRule() const;

    std::shared_ptr<MegaIgnoreNameRule> getIgnoreSymLink() const;

    QList<std::shared_ptr<MegaIgnoreRule>> getAllRules() const;

    std::shared_ptr<MegaIgnoreRule> getNameRule(int index) const;

    void enableAllNameRules(bool enable);

    int enabledRulesCount();

    void removeRule(std::shared_ptr<MegaIgnoreRule> rule);

    std::shared_ptr<MegaIgnoreRule> findRule(const QString& ruleToCompare);
    static MegaIgnoreRule::RuleType getRuleType(const QString& line);
    QStringList getExcludedExtensions() const;

    void parseIgnoresFile();

    std::shared_ptr<MegaIgnoreNameRule> addIgnoreSymLinksRule();
    std::shared_ptr<MegaIgnoreNameRule> addIgnoreSymLinkRule(const QString& pattern);
    std::shared_ptr<MegaIgnoreNameRule> addNameRule(MegaIgnoreNameRule::Class classType
                                                    ,const QString& pattern
                                                    ,MegaIgnoreNameRule::Target targetType = MegaIgnoreNameRule::Target::NONE, MegaIgnoreNameRule::WildCardType wildCard = MegaIgnoreNameRule::WildCardType::EQUAL);
    std::shared_ptr<MegaIgnoreExtensionRule> addExtensionRule(MegaIgnoreNameRule::Class classType, const QString& pattern);

    void restreDefaults();

    enum ApplyChangesError
    {
        OK,
        NO_UPDATE_NEEDED,
        NO_WRITE_PERMISSION
    };

    MegaIgnoreManager::ApplyChangesError applyChanges(bool updateExtensions = false, const QStringList& updatedExtensions = {});

    void setOutputIgnorePath(const QString& outputPath);

    void setInputDirPath(const QString& inputDir, bool createIfNotExist = true);

    bool hasChanged() const;

    int getNameRulesCount() const;

private:
    template <class Type>
    static const std::shared_ptr<Type> convert(const std::shared_ptr<MegaIgnoreRule> data)
    {
        return std::dynamic_pointer_cast<Type>(data);
    }

    template <class Type>
    bool addRule(std::shared_ptr<Type> rule)
    {
        const auto ruleText = rule->getModifiedRule();
        auto alreadyExists = findRule(ruleText);
        if (!alreadyExists || ruleText.isEmpty())
        {
            mRules.append(rule);
        }
        const auto type = rule->ruleType();
        if (type == MegaIgnoreRule::EXTENSIONRULE || type == MegaIgnoreRule::NAMERULE)
        {
            mNameRules.append(rule);
        }
        //Return if the addition was succesful
        return !alreadyExists;
    }

    QString mMegaIgnoreFile;
    QString mOutputMegaIgnoreFile;
    QList<std::shared_ptr<MegaIgnoreRule>> mRules;
    QList<std::shared_ptr<MegaIgnoreRule>> mNameRules;
    QMap<QString, std::shared_ptr<MegaIgnoreRule> > mExtensionRules;

    std::shared_ptr<MegaIgnoreSizeRule> mLowLimitRule;
    std::shared_ptr<MegaIgnoreSizeRule> mHighLimitRule;

    std::shared_ptr<MegaIgnoreNameRule> mIgnoreSymLinkRule;
    
    QString mIgnoreCRC;
};

#endif // MEGAIGNOREMANAGER_H
