#ifndef SYMLINKSTALLEDISSUE_H
#define SYMLINKSTALLEDISSUE_H

#include <StalledIssue.h>
#include <syncs/control/MegaIgnoreRules.h>
#include <megaapi.h>

class IgnoredStalledIssue : public StalledIssue
{
public:
    IgnoredStalledIssue(const mega::MegaSyncStall *stallIssue);
    ~IgnoredStalledIssue() = default;

    StalledIssue::AutoSolveIssueResult autoSolveIssue() override;
    bool isAutoSolvable() const override;

    void fillIssue(const mega::MegaSyncStall *stall) override;

    bool isSymLink() const;
    bool isSpecialLink() const;
    bool isHardLink() const;

    bool isExpandable() const override;
    bool checkForExternalChanges() override;

    static void clearIgnoredSyncs();

    mega::MegaSyncStall::SyncPathProblem linkType() const;

    struct IgnoredPath
    {
        QString path;
        enum class IgnorePathSide
        {
            LOCAL,
            REMOTE,
        };
        IgnorePathSide pathSide;
        MegaIgnoreNameRule::Target target;

        IgnoredPath(const QString& newpath, IgnorePathSide side, MegaIgnoreNameRule::Target newtarget)
            :path(newpath), pathSide(side), target(newtarget)
        {}
    };
    QList<IgnoredPath> getIgnoredFiles() const { return mIgnoredPaths;}

protected:
    QList<IgnoredPath> mIgnoredPaths;

private:
    mega::MegaSyncStall::SyncPathProblem mLinkType;
    static QMap<mega::MegaHandle, bool> mSymLinksIgnoredInSyncs;
};

class CloudNodeIsBlockedIssue : public IgnoredStalledIssue
{
public:
    CloudNodeIsBlockedIssue(const mega::MegaSyncStall *stallIssue);
    ~CloudNodeIsBlockedIssue() = default;

    bool isAutoSolvable() const override;
    void fillIssue(const mega::MegaSyncStall *stall) override;
    bool showDirectoryInHyperlink() const override;
    bool isExpandable() const override;
};

#endif // SYMLINKSTALLEDISSUE_H
