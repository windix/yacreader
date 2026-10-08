#include "comics_remover.h"

#include "QsLog.h"
#include "comic.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

ComicsRemover::ComicsRemover(QModelIndexList &il, QList<QString> &ps, qulonglong parentId, QObject *parent)
    : QObject(parent), indexList(il), paths(ps), parentId(parentId)
{
}

void ComicsRemover::process()
{
    QString currentComicPath;
    QListIterator<QModelIndex> i(indexList);
    QListIterator<QString> i2(paths);
    i.toBack();
    i2.toBack();

    while (i.hasPrevious() && i2.hasPrevious()) {
        QModelIndex mi = i.previous();
        currentComicPath = i2.previous();
        if (QFileInfo(currentComicPath).isDir()) {
            if (removeFolderComic(currentComicPath))
                emit remove(mi.row());
            else
                emit removeError();
        } else if (QFile::moveToTrash(currentComicPath))
            emit remove(mi.row());
        else if (QFile::remove(currentComicPath))
            emit remove(mi.row());
        else
            emit removeError();
    }

    emit finished();
    emit removedItemsFromFolder(parentId);
}

// A folder comic can share its folder with other folders and comic files, so only its
// pages are removed, and the folder itself only once nothing is left in it.
bool ComicsRemover::removeFolderComic(const QString &path)
{
    const auto pages = FolderComic::pageFiles(path);
    bool removed = true;
    for (const auto &page : pages) {
        const auto pagePath = page.absoluteFilePath();
        if (!QFile::moveToTrash(pagePath) && !QFile::remove(pagePath))
            removed = false;
    }

    if (removed)
        QDir().rmdir(path);

    return removed;
}

FoldersRemover::FoldersRemover(QModelIndexList &il, QList<QString> &ps, QObject *parent)
    : QObject(parent), indexList(il), paths(ps)
{
}

void FoldersRemover::process()
{
    QString currentFolderPath;
    QListIterator<QModelIndex> i(indexList);
    QListIterator<QString> i2(paths);
    i.toBack();
    i2.toBack();

    QLOG_DEBUG() << "Deleting folders" << paths.at(0);

    while (i.hasPrevious() && i2.hasPrevious()) {
        QModelIndex mi = i.previous();
        currentFolderPath = i2.previous();
        QDir d(currentFolderPath);
        if (QFile::moveToTrash(currentFolderPath))
            emit remove(mi);
        else if (d.removeRecursively() || !d.exists()) // the folder is in the DB but no in the drive...
            emit remove(mi);
        else
            emit removeError();
    }

    emit finished();
}
