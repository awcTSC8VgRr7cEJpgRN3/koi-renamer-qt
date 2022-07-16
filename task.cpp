#include <QDir>
#include <QTextCodec>
#include "task.h"

Task::Task()
{
    this->clear();
}

qsizetype Task::indexofUtf8(QByteArray& stringUtf8, qsizetype base, qsizetype offsetUtf8)
{
    if (offsetUtf8 > 0)
    {
        ++offsetUtf8;
        while (offsetUtf8)
        {
            if (base >= 0 && base < stringUtf8.size())
            {
                if ((stringUtf8.at(base) & char(0xc0)) ^ char(0x80))
                    --offsetUtf8;
            }
            else
            {
                --offsetUtf8;
            }
            ++base;
        }
        --base;
    }
    else if (offsetUtf8 < 0)
    {
        while (offsetUtf8)
        {
            --base;
            if (base >= 0 && base < stringUtf8.size())
            {
                if ((stringUtf8.at(base) & char(0xc0)) ^ char(0x80))
                    ++offsetUtf8;
            }
            else
            {
                ++offsetUtf8;
            }
        }
    }
    return base;
}

bool Task::isAllInOneDir(const RenameHistory & history)
{
    if (history.isEmpty())
    {
        qInfo() << "Empty rename history.";
        return false;
    }
    else
    {
        QDir firstDir = QFileInfo(history.first()).dir();
        for (int i = 1; i < history.size(); ++i)
            if (firstDir != QFileInfo(history.at(i)).dir())
                return false;
        return true;
    }
}

void Task::append(const QString& filename)
{
    RenameHistory history;
    history.push(QString(filename));
    this->filelist.append(history);
    switch (this->status)
    {
        case Task::Pending:
        break;
        default:
            this->setStatus(Task::Pending);
        break;
    }
}

void Task::clear()
{
    this->filelist.clear();
    this->setStatus(Task::Ready);
}

Task::Filelist Task::getFilelist() const
{
    return this->filelist;
}

Task::Status Task::getStatus() const
{
    return this->status;
}

bool Task::isEmpty() const
{
    return this->filelist.isEmpty();
}

void Task::renameAll(Mask mask, Rule rule, const QList<QString>& strings, const QList<int>& numbers)
{
    this->renameTestAll(mask, rule, strings, numbers);
    int maxHistory = 2;
    for (int i = 1; i < maxHistory; ++i)
    {
        for (Filelist::iterator history = this->filelist.begin(); history < this->filelist.end(); ++history)
        {
            int size = history->size();
            if (size > i)
            {
                QFile file(history->at(i - 1));
                if (file.rename(history->at(i)))
                {
                    if (size > maxHistory)
                        maxHistory = size;
                }
                else
                {
                    history->remove(i, size - i);
                }
            }
        }
    }
    this->setStatus(Task::Finished);
}

void Task::renameTestAll(Mask mask, Rule rule, const QList<QString>& strings, const QList<int>& numbers)
{
    Filelist::iterator history = this->filelist.begin();
    switch (rule)
    {
        case Task::Rename:
        case Task::Replace:
        case Task::Insert:
        case Task::InsertLast:
        case Task::Delete:
        case Task::DeleteLast:
        case Task::ToUnicode:
        case Task::ToLocale:
            this->resetHistoryAll();
            while (history < this->filelist.end())
            {
                this->renameTest(mask, rule, strings, numbers, history);
                ++history;
            }
        break;
        case Task::OrdinalWithPrefix:
        {
            this->resetHistoryAll();
            int i = 0;
            while (history < this->filelist.end())
            {
                QList<int> newNumbers = numbers;
                newNumbers.append(i++);
                this->renameTest(mask, rule, strings, newNumbers, history);
                ++history;
            }
        }
        break;
        case Task::OrdinalWithPrefixReverse:
        {
            this->resetHistoryAll();
            int i = 0;
            while (history < this->filelist.end())
            {
                QList<int> newNumbers = numbers;
                newNumbers.append(i--);
                this->renameTest(mask, rule, strings, newNumbers, history);
                ++history;
            }
        }
        break;
        default:
            qWarning() << "No rename rule specified: " << rule;
        return;
    }
    if (this->filelist.isEmpty())
        this->setStatus(Task::Ready);
    else
        this->setStatus(Task::Tested);
}

qsizetype Task::size() const
{
    return this->filelist.size();
}

void Task::renameTest(Mask mask, Rule rule, const QList<QString>& strings, const QList<int>& numbers, Filelist::iterator& history)
{
    if (!history->isEmpty())
    {
        QFileInfo file(history->top());
        QString modText;
        switch (mask)
        {
            case Task::ExtExcluded:
                modText = file.completeBaseName();
            break;
            case Task::ExtOnly:
                modText = file.suffix();
            break;
            default:
                qWarning() << "No such rename mask: " << mask;
            return;
        }
        switch (rule)
        {
            case Task::Rename:
                if (strings.size() == 1)
                {
                    modText = strings.at(0);
                }
                else
                {
                    qWarning() << "Insufficient arguments: Rename.";
                    return;
                }
            break;
            case Task::OrdinalWithPrefix:
            case Task::OrdinalWithPrefixReverse:
                if (strings.size() == 2 && numbers.size() == 1)
                {
                    bool ok;
                    QString ordinal = strings.at(1).trimmed();
                    int suffix = ordinal.toInt(&ok) + numbers.at(0);
                    if (ordinal.size() >= QString::number(INT_MAX).size())
                        ok = false;
                    if (ok)
                    {
                        int base = 10;
                        int max = qPow(base, ordinal.size());
                        if (suffix < 0)
                            suffix = (suffix % max + max) % max;
                        modText = QString("%1%2").arg(strings.at(0)).arg(suffix, ordinal.size(), base, QChar('0'));
                    }
                    else
                    {
                        qWarning() << "Insufficient arguments: " << ordinal;
                        return;
                    }
                }
                else
                {
                    qWarning() << "Insufficient arguments: OrdinalWithPrefix.";
                    return;
                }
            break;
            case Task::Replace:
                if (strings.size() == 2)
                {
                    if (strings.at(0).size())
                        modText.replace(strings.at(0), strings.at(1));
                }
                else
                {
                    qWarning() << "Insufficient arguments: Replace.";
                    return;
                }
            break;
            case Task::Insert:
                if (strings.size() == 1 && numbers.size() == 1)
                {
                    //modText.insert(numbers.at(0), strings.at(0));
                    QByteArray modTextUtf8 = modText.toUtf8();
                    modText = QString::fromUtf8(modTextUtf8.insert(this->indexofUtf8(modTextUtf8, 0, numbers.at(0)), strings.at(0).toUtf8()));
                }
                else
                {
                    qWarning() << "Insufficient arguments: Insert.";
                    return;
                }
            break;
            case Task::InsertLast:
                if (strings.size() == 1 && numbers.size() == 1)
                {
                    //QChar space(' ');
                    //modText.prepend(&space, numbers.at(0) - modText.size());
                    //modText.insert(modText.size() - numbers.at(0), strings.at(0));
                    QByteArray modTextUtf8 = modText.toUtf8();
                    int posUtf8 = this->indexofUtf8(modTextUtf8, modTextUtf8.size(), 0 - numbers.at(0));
                    if (posUtf8 < 0)
                        modTextUtf8.prepend(0 - posUtf8, char(' ')).insert(0, strings.at(0).toUtf8());
                    else
                        modTextUtf8.insert(posUtf8, strings.at(0).toUtf8());
                    modText = QString::fromUtf8(modTextUtf8);
                }
                else
                {
                    qWarning() << "Insufficient arguments: InsertLast.";
                    return;
                }
            break;
            case Task::Delete:
                if (numbers.size() == 2)
                {
                    //modText.remove(numbers.at(0), numbers.at(1));
                    QByteArray modTextUtf8 = modText.toUtf8();
                    int posUtf8begin = this->indexofUtf8(modTextUtf8, 0, numbers.at(0));
                    int posUtf8end = this->indexofUtf8(modTextUtf8, posUtf8begin, numbers.at(1));
                    modText = QString::fromUtf8(modTextUtf8.remove(posUtf8begin, posUtf8end - posUtf8begin));
                }
                else
                {
                    qWarning() << "Insufficient arguments: Delete.";
                    return;
                }
            break;
            case Task::DeleteLast:
                if (numbers.size() == 2)
                {
                    /*
                    int pos = modText.size() - numbers.at(0) - numbers.at(1);
                    if (pos < 0)
                        modText.remove(0, numbers.at(1) + pos);
                    else
                        modText.remove(pos, numbers.at(1));
                    */
                    QByteArray modTextUtf8 = modText.toUtf8();
                    int posUtf8end = this->indexofUtf8(modTextUtf8, modTextUtf8.size(), 0 - numbers.at(0));
                    int posUtf8begin = this->indexofUtf8(modTextUtf8, posUtf8end, 0 - numbers.at(1));
                    if (posUtf8begin < 0)
                        posUtf8begin = 0;
                    modText = QString::fromUtf8(modTextUtf8.remove(posUtf8begin, posUtf8end - posUtf8begin));
                }
                else
                {
                    qWarning() << "Insufficient arguments: DeleteLast.";
                    return;
                }
            break;
            case Task::ToUnicode:
                if (strings.size() == 1)
                {
                    QTextCodec *codec = QTextCodec::codecForLocale();
                    QByteArray encodedString = codec->fromUnicode(modText);
                    codec = QTextCodec::codecForName(strings.at(0).toLatin1());
                    modText = codec->toUnicode(encodedString);
                }
                else
                {
                    qWarning() << "Insufficient arguments: ToUnicode.";
                    return;
                }
            break;
            case Task::ToLocale:
                if (strings.size() == 1)
                {
                    QTextCodec *codec = QTextCodec::codecForName(strings.at(0).toLatin1());
                    QByteArray encodedString = codec->fromUnicode(modText);
                    codec = QTextCodec::codecForLocale();
                    modText = codec->toUnicode(encodedString);
                }
                else
                {
                    qWarning() << "Insufficient arguments: ToLocale.";
                    return;
                }
            break;
            default:
                qWarning() << "No such rename rule: " << rule;
            return;
        }
        QString modFileName;
        switch (mask)
        {
            case Task::ExtExcluded:
                modFileName += modText;
                if (file.fileName().contains(QChar('.')) || !file.suffix().isEmpty())
                    modFileName += "." + file.suffix();
            break;
            case Task::ExtOnly:
                modFileName += file.completeBaseName();
                if (file.fileName().contains(QChar('.')) || !modText.isEmpty())
                    modFileName += "." + modText;
            break;
            default:
                qWarning() << "No such rename mask: " << mask;
            return;
        }
        file.setFile(file.dir(), modFileName);
        history->push(file.filePath());
    }
}

void Task::resetHistory(Filelist::iterator& history)
{
    if (history->size() > 1)
        history->remove(1, history->size() - 1);
}

void Task::resetHistoryAll()
{
    for (Filelist::iterator history = this->filelist.begin(); history < this->filelist.end(); ++history)
        this->resetHistory(history);
    if (this->filelist.isEmpty())
        this->setStatus(Task::Ready);
    else
        this->setStatus(Task::Pending);
}

void Task::setStatus(Task::Status status)
{
    this->status = status;
}
