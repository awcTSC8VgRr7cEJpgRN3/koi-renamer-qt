#ifndef TASK_H
#define TASK_H

#include <QStack>

class Task
{
public:
    enum Mask {ExtExcluded, ExtOnly};
    enum Rule {Rename, OrdinalWithPrefix, OrdinalWithPrefixReverse, Replace, Insert, InsertLast, Delete, DeleteLast, ToUnicode, ToLocale};
    enum Status {Ready, Pending, Tested, Finished};
    typedef QStack<QString> RenameHistory;
    typedef QList<RenameHistory> Filelist;
    //typedef Filelist::const_iterator const_iterator;
    //typedef Filelist::iterator iterator;
    Task();
    static qsizetype indexofUtf8(QByteArray&, qsizetype, qsizetype);
    static bool isAllInOneDir(const RenameHistory&);
    void append(const QString&);
    //iterator begin();
    //const_iterator begin() const;
    //const_iterator cbegin() const;
    //const_iterator cend() const;
    void clear();
    //iterator end();
    //const_iterator end() const;
    Filelist getFilelist() const;
    Status getStatus() const;
    bool isEmpty() const;
    void renameAll(Mask, Rule, const QList<QString>&, const QList<int>&);
    void renameTestAll(Mask, Rule, const QList<QString>&, const QList<int>&);
    qsizetype size() const;

private:
    Filelist filelist;
    Status status;
    void renameTest(Mask, Rule, const QList<QString>&, const QList<int>&, Filelist::iterator&);
    void resetHistory(Filelist::iterator&);
    void resetHistoryAll();
    void setStatus(Status);
};

#endif // TASK_H
