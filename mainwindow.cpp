#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
    this->setAcceptDrops(true);
    this->connect(ui->tabWidget_Rules, &QTabWidget::currentChanged, this, &MainWindow::enableRunOrNot);
    ui->tabWidget_Rules->setDocumentMode(true);
    ui->tabWidget_Rules->setCurrentIndex(0);
    int maxOrdinalDigits = 9;
    this->connect(ui->lineEdit_Ordinal, &QLineEdit::textChanged, this, &MainWindow::changeDigitsByOrdinal);
    this->connect(ui->spinBox_Digits, &QSpinBox::valueChanged, this, &MainWindow::changeOrdinalByDigits);
    ui->lineEdit_Ordinal->setMaxLength(maxOrdinalDigits);
    ui->spinBox_Digits->setMaximum(maxOrdinalDigits);
    ui->lineEdit_Ordinal->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), ui->lineEdit_Ordinal));
    ui->spinBox_Digits->setValue(3);
    this->connect(ui->radioButton_Delete, &QRadioButton::toggled, this, &MainWindow::switchToDelete);
    this->connect(ui->radioButton_Insert, &QRadioButton::toggled, this, &MainWindow::switchToInsert);
    this->switchToDelete(ui->radioButton_Delete->isChecked());
    ui->radioButton_Insert->setChecked(true);
    ui->spinBox_Delete->setValue(1);
    ui->radioButton_Head->setChecked(true);
    ui->spinBox_Indexof->setMinimum(1);
    ui->radioButton_ToUnicode->setChecked(true);
    this->connect(ui->comboBox_Locale, &QComboBox::currentIndexChanged, this, &MainWindow::enableRunOrNot);
    ui->comboBox_Locale->setCurrentIndex(-1);
    this->connect(ui->pushButton_RenameExtExcluded, &QPushButton::clicked, this, &MainWindow::renameExtExcluded);
    this->connect(ui->pushButton_RenameExtOnly, &QPushButton::clicked, this, &MainWindow::renameExtOnly);
    ui->checkBox_Test->setChecked(true);
    ui->textBrowser_TaskView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->textBrowser_TaskView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->textBrowser_TaskView->setLineWrapMode(QTextEdit::NoWrap);
    this->newTask();
    this->enableRunOrNot();
    this->setTaskView();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::hasLocalFileInUrls(const QList<QUrl>& urls)
{
    foreach (const auto& u, urls)
        if (u.isLocalFile())
            return true;
    return false;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (this->hasLocalFileInUrls(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    this->newTask();
    const QList<QUrl>& urls = event->mimeData()->urls();
    foreach (const auto& u, urls)
        if (u.isLocalFile())
            this->taskHistory.top().append(u.toLocalFile());
    this->enableRunOrNot();
    this->setTaskView();
    event->acceptProposedAction();
}

void MainWindow::newTask()
{
    this->taskHistory.clear();
    this->taskHistory.push(Task());
}

void MainWindow::rename(Task::Mask mask)
{
    this->enableRun(false);
    Task::Rule rule;
    QList<QString> strings;
    QList<int> numbers;
    switch (ui->tabWidget_Rules->currentIndex())
    {
        case 0:
            rule = Task::Rename;
            strings.append(ui->lineEdit_RenameTo->text());
        break;
        case 1:
            if (ui->checkBox_Reverse->isChecked())
                rule = Task::OrdinalWithPrefixReverse;
            else
                rule = Task::OrdinalWithPrefix;
            strings.append(ui->lineEdit_Prefix->text());
            strings.append(ui->lineEdit_Ordinal->text());
        break;
        case 2:
            rule = Task::Replace;
            strings.append(ui->lineEdit_ReplaceFrom->text());
            strings.append(ui->lineEdit_ReplaceTo->text());
        break;
        case 3:
            if (ui->radioButton_Insert->isChecked())
            {
                if (ui->radioButton_Head->isChecked())
                {
                    rule = Task::Insert;
                }
                else if (ui->radioButton_Tail->isChecked())
                {
                    rule = Task::InsertLast;
                }
                else
                {
                    qWarning() << "Neither head nor tail specified.";
                    this->enableRunOrNot();
                    return;
                }
                numbers.append(ui->spinBox_Indexof->value() - 1);
                strings.append(ui->lineEdit_Insert->text());
            }
            else if (ui->radioButton_Delete->isChecked())
            {
                if (ui->radioButton_Head->isChecked())
                {
                    rule = Task::Delete;
                }
                else if (ui->radioButton_Tail->isChecked())
                {
                    rule = Task::DeleteLast;
                }
                else
                {
                    qWarning() << "Neither head nor tail specified.";
                    this->enableRunOrNot();
                    return;
                }
                numbers.append(ui->spinBox_Indexof->value() - 1);
                numbers.append(ui->spinBox_Delete->value());
            }
            else
            {
                qWarning() << "Neither insert nor delete specified.";
                this->enableRunOrNot();
                return;
            }
        break;
        case 4:
            if (ui->radioButton_ToUnicode->isChecked())
            {
                rule = Task::ToUnicode;
            }
            else if (ui->radioButton_ToLocale->isChecked())
            {
                rule = Task::ToLocale;
            }
            else
            {
                qWarning() << "Neither Unicode nor locale specified.";
                this->enableRunOrNot();
                return;
            }
            switch (ui->comboBox_Locale->currentIndex())
            {
                case 0:
                    strings.append("Shift-JIS");
                break;
                case 1:
                    strings.append("GBK");
                break;
                case 2:
                    strings.append("CP949");
                break;
                case 3:
                    strings.append("Big5");
                break;
                case 4:
                    strings.append("Windows-1252");
                break;
                default:
                    qWarning() << "Outbound combobox at " << ui->comboBox_Locale->currentIndex();
                    this->enableRunOrNot();
                return;
            }
        break;
        default:
            qWarning() << "Outbound tab widget at " << ui->tabWidget_Rules->currentIndex();
            this->enableRunOrNot();
        return;
    }
    if (ui->checkBox_Test->isChecked())
        this->taskHistory.top().renameTestAll(mask, rule, strings, numbers);
    else
        this->taskHistory.top().renameAll(mask, rule, strings, numbers);
    if (ui->checkBox_RunThenClose->isChecked())
    {
        this->close();
    }
    else
    {
        this->enableRunOrNot();
        this->setTaskView();
    }
}

void MainWindow::setTaskView()
{
    QString text;
    if (this->taskHistory.isEmpty())
    {
        text = tr("目前沒有任何項目。");
        text += "\n\n";
        text += tr("請先從桌面或資料夾，拖曳檔案或目錄到此處。");
    }
    else
    {
        switch (this->taskHistory.top().getStatus())
        {
            case Task::Ready:
                text = tr("目前沒有任何項目。");
                text += "\n\n";
                text += tr("請先從桌面或資料夾，拖曳檔案或目錄到此處。");
            break;
            case Task::Pending:
                text = tr("%1 個項目待處理").arg(QString::number(this->taskHistory.top().size()));
                text += "\n\n";
                /*
                {
                    Task::Filelist::const_iterator file = this->taskHistory.top().cbegin();
                    while (file < this->taskHistory.top().cend())
                    {
                        text += file.value().first() + "\n";
                        ++file;
                    }
                }
                */
                foreach (const auto& history, this->taskHistory.top().getFilelist())
                    text += history.first() + "\n";
            break;
            case Task::Tested:
            case Task::Finished:
                foreach (const auto& history, this->taskHistory.top().getFilelist())
                {
                    if (!history.isEmpty())
                    {
                        switch (this->taskHistory.top().getStatus())
                        {
                            case Task::Tested:
                                text += tr("測試");
                            break;
                            case Task::Finished:
                                text += tr("改名");
                            break;
                            default:
                                qWarning() << "Status makes no sense.";
                            return;
                        }
                        text += "　";
                        if (Task::isAllInOneDir(history))
                        {
                            text += QFileInfo(history.first()).fileName();
                            for (int i = 1; i < history.size(); ++i)
                                text += "　→　" + QFileInfo(history.at(i)).fileName();
                            text += "　";
                            text += tr("在目錄");
                            text += ": " + QFileInfo(history.first()).path();
                        }
                        else
                        {
                            text += history.first();
                            for (int i = 1; i < history.size(); ++i)
                                text += "　→　" + history.at(i);
                        }
                    }
                    text += "\n";
                }
            break;
            default:
                qWarning() << "No such task status: " << this->taskHistory.top().getStatus();
            return;
        }
    }
    ui->textBrowser_TaskView->setPlainText(text);
}

void MainWindow::changeDigitsByOrdinal(const QString& text)
{
    int sizeofOrdinal = text.size();
    if (sizeofOrdinal <= ui->spinBox_Digits->maximum())
        ui->spinBox_Digits->setValue(sizeofOrdinal);
    else
        this->changeOrdinalByDigits(ui->spinBox_Digits->value());
}

void MainWindow::changeOrdinalByDigits(int i)
{

    int sizeofOrdinal = ui->lineEdit_Ordinal->text().size();
    if (ui->lineEdit_Ordinal->maxLength() < i)
    {
        this->changeDigitsByOrdinal(ui->lineEdit_Ordinal->text());
    }
    else if (sizeofOrdinal > i)
    {
        ui->lineEdit_Ordinal->setText(ui->lineEdit_Ordinal->text().last(i));
    }
    else if (sizeofOrdinal < i)
    {
        QString text = QString("%1").arg(ui->lineEdit_Ordinal->text(), i, QChar('0'));
        if (!sizeofOrdinal)
            text[text.size() - 1] = QChar('1');
        ui->lineEdit_Ordinal->setText(text);
    }
}

void MainWindow::enableRun(bool enabled)
{
    if (!enabled || this->taskHistory.isEmpty())
    {
        ui->pushButton_RenameExtExcluded->setEnabled(false);
        ui->pushButton_RenameExtOnly->setEnabled(false);
    }
    else
    {
        switch (this->taskHistory.top().getStatus())
        {
            case Task::Pending:
            case Task::Tested:
                if (ui->comboBox_Locale->isVisible())
                {
                    int localeIndex = ui->comboBox_Locale->currentIndex();
                    if (localeIndex >= 0 && localeIndex < ui->comboBox_Locale->count())
                    {
                        ui->pushButton_RenameExtExcluded->setEnabled(true);
                        ui->pushButton_RenameExtOnly->setEnabled(true);
                    }
                    else
                    {
                        ui->pushButton_RenameExtExcluded->setEnabled(false);
                        ui->pushButton_RenameExtOnly->setEnabled(false);
                    }
                }
                else
                {
                    ui->pushButton_RenameExtExcluded->setEnabled(true);
                    ui->pushButton_RenameExtOnly->setEnabled(true);
                }
            break;
            default:
                ui->pushButton_RenameExtExcluded->setEnabled(false);
                ui->pushButton_RenameExtOnly->setEnabled(false);
                ui->checkBox_Test->setChecked(true);
            break;
        }
    }
}

void MainWindow::enableRunOrNot()
{
    this->enableRun(true);
}

void MainWindow::renameExtExcluded()
{
    this->rename(Task::ExtExcluded);
}

void MainWindow::renameExtOnly()
{
    this->rename(Task::ExtOnly);
}

void MainWindow::switchToDelete(bool checked)
{
    if (checked)
    {
        ui->label_InsDelHint->setText(tr("刪除字數"));
        ui->groupBox_Index->setTitle(tr("從何處開始"));
    }
    ui->spinBox_Delete->setVisible(checked);
}

void MainWindow::switchToInsert(bool checked)
{
    if (checked)
    {
        ui->label_InsDelHint->setText(tr("插入文字"));
        ui->groupBox_Index->setTitle(tr("將文字插入到"));
    }
    ui->lineEdit_Insert->setVisible(checked);
}
