#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QToolButton>
#include <QLabel>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <QCollator>
#include <QXmlStreamWriter>

#define WINDOW_H 1000
#define WINDOW_W 2400
#define COMPENSARION_VALUE 0

#define ALFA 1 // sonar parameters
#define GAMA 0.01f // sonar parameters
#define GAIN 1.1f // sonar parameters

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupmainToolBar();
    setupFileManager();
    setupLabelManager();
    setupLabelDialog();
    setupAnnotationManager();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::systemInit()
{

}

/***********************************************************************************************
 *                                  set mainToolBar
 * *********************************************************************************************/
void MainWindow::setupmainToolBar()
{
    connect(ui->actionOpenFile, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(ui->actionOpenDir, SIGNAL(triggered(bool)), this, SLOT(openDir()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(appQuit()));
}

void MainWindow::openFile()
{
    QString str_path = QFileDialog::getOpenFileName(this, tr("select BMP file"));
    if (str_path != ""){
        file_path = str_path.toStdString();
    }
}

void MainWindow::openDir()
{
    dir_path = QFileDialog::getExistingDirectory(this, tr("select directory"));
    QString dir_name = dir_path.right(dir_path.length() - dir_path.lastIndexOf("/") -1);

    if (dir_path != ""){
        QDir dir(dir_path);
        QStringList nameFilters;
        nameFilters << "*.data" << "*.DATA";
        file_list = dir.entryList(nameFilters, QDir::Files|QDir::NoDot);
        if (file_list.empty())
        {
            QMessageBox::about(NULL, "ERROR", "No existed .data files in the directory");
            return;
        }
        QCollator collator;
        collator.setNumericMode(true);
        sort(file_list.begin(), file_list.end(), [& collator](const QString & str1, const QString & str2)
        {
            return collator.compare(str1, str2) < 0;
        });

        QString str_path = dir_path + "/" + file_list[0];
        file_path = str_path.toStdString();
        ui->Files_listWidget->addItems(file_list);
        ui->Files_listWidget->setCurrentRow(0);

        if (dir.cdUp())
        {
            csv_path = dir.absolutePath() + "/" + dir_name + ".csv";
        }
    }
}

void MainWindow::appQuit()
{
    if (!(QMessageBox::information(this,tr("Exit Warning"),tr("Do you really want exit ?"),tr("Yes"),tr("No"))))
    {
        qApp->quit();
    }
}

/***********************************************************************************************
 *                                        files box
 * *********************************************************************************************/
void MainWindow::setupFileManager()
{
    connect(ui->Files_listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::douleClickFile);
    connect(ui->Files_listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::on_btn_Start_clicked);
}

void MainWindow::douleClickFile(QListWidgetItem *item)
{
    file_path = (dir_path + "/" + item->text()).toStdString();
}

/***********************************************************************************************
 *                                        label box
 * *********************************************************************************************/
void MainWindow::setupLabelManager()
{
    ui->Label_listWidget->setProperty("contextMenuPolicy", Qt::CustomContextMenu);
    connect(ui->btn_AddLabel, &QPushButton::clicked, [this](){newLabel(ui->lineEdit_newLabel->text());});
    connect(ui->Label_listWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::labelMenu);
    connect(dialog, SIGNAL(dialoglabelAdded(QString)), this, SLOT(newLabelfromDialog(QString)));
}

void MainWindow::newLabel(QString label)
{
    if (label == "")
    {
        return;
    }
    if (!ui->Label_listWidget->findItems(label, Qt::MatchExactly).empty())
    {
        QMessageBox::about(NULL, "ERROR", "Exist Label, Please reset the label name");
        return;
    }
    ui->Label_listWidget->addItem(label);
    ui->lineEdit_newLabel->setText("");
    emit labelAdded(label);

}

void MainWindow::newLabelfromDialog(QString label)
{
    if (ui->Label_listWidget->findItems(label, Qt::MatchExactly).empty())
    {
        ui->Label_listWidget->addItem(label);
        return;
    }
}

void MainWindow::deleteLabel(QString label)
{
    QListWidgetItem* item = ui->Label_listWidget->findItems(label, Qt::MatchExactly)[0];
    ui->Label_listWidget->removeItemWidget(item);
    delete item;
    emit labelDeleted(label);
}

void MainWindow::labelMenu(const QPoint &pos)
{
    QPoint globalPos = ui->Label_listWidget->mapToGlobal(pos);
    QModelIndex modelIndex = ui->Label_listWidget->indexAt(pos);
    if (!modelIndex.isValid())
    {
        return;
    }
    int row = modelIndex.row();
    auto item = ui->Label_listWidget->item(row);

    QMenu menu;
    menu.addAction("Delete");
    menu.addAction("Clear all");
    QAction* click = menu.exec(globalPos);
    if (click)
    {
        if (click->text().contains("Delete"))
        {
            deleteLabel(item->text());
        }
        else if (click->text().contains("Clear all"))
        {
            ui->Label_listWidget->clear();
            emit labelCleared();
        }
    }
}

/***********************************************************************************************
 *                                     annotation box
 * *********************************************************************************************/
void MainWindow::setupAnnotationManager()
{
    connect(dialog, SIGNAL(dialoglabelAdded(QString)), this, SLOT(setLabel(QString)));
    connect(ui->label_image, SIGNAL(rectLocationSet(QList<int>)), this, SLOT(setRect(QList<int>)));
    connect(dialog, SIGNAL(dialoglabelAdded(QString)), this, SLOT(setAnnotation()));
}

void MainWindow::setAnnotation()
{
    /* original poing */
    origin_x = QString::number(int(current_rect[0] * ratio));
    origin_y = QString::number(int(current_rect[2] * ratio));

    /* box size */
    width = QString::number(int((current_rect[1] - current_rect[0]) * ratio));
    height = QString::number(int((current_rect[3] - current_rect[2]) * ratio));

    sonar_annotation = label_name + " " + "(" + origin_x + ", " + origin_y + ")" + " " + "(" + width + ", " + height + ")";
    ui->Anno_listWidget->addItem(sonar_annotation);
}

void MainWindow::setLabel(QString label)
{
    label_name = label;
    label_names.append(label_name);
}

void MainWindow::setRect(QList<int> rect)
{
    current_rect = rect;
    current_rects.append(current_rect);
}

/***********************************************************************************************
 *                                     label dialog
 * *********************************************************************************************/
void MainWindow::setupLabelDialog()
{
    connect(ui->label_image, SIGNAL(mouseReleaseSignal()), dialog, SLOT(dialogShow()));
    connect(this, SIGNAL(labelAdded(QString)), dialog, SLOT(getLabelFromManager(QString)));
    connect(this, SIGNAL(labelDeleted(QString)), dialog, SLOT(deleteLabelFromManager(QString)));
    connect(this, SIGNAL(labelCleared()), dialog, SLOT(clearLabelFromManager()));
    connect(dialog, SIGNAL(dialoglabelCanceled()), ui->label_image, SLOT(deleteLastRect()));
}

/***********************************************************************************************
 *                                  sonar image function
 * *********************************************************************************************/
void enhanceImage(Mat& img)
{
    Mat imgRGB[3];
    split(img, imgRGB);
    for (int i = 0; i < 3; i++)
    {
        equalizeHist(imgRGB[i], imgRGB[i]);
    }
    merge(imgRGB, 3, img);
}

void colorImage(Mat& img)
{
    for (int i = 0; i < img.rows; i++)
    {
        Vec3b *ptr_color = img.ptr<Vec3b>(i);
        for (int j = 0; j < img.cols; j++)
        {
            ptr_color[j][1] = abs(0.60 * ptr_color[j][0] + COMPENSARION_VALUE);
            ptr_color[j][2] = abs(0.10 * ptr_color[j][0] + COMPENSARION_VALUE);
            ptr_color[j][0] = abs(1.00 * ptr_color[j][0] + COMPENSARION_VALUE);
        }
    }
}

void MainWindow::showImage(int num)
{
    file_path = (dir_path + "/" + ui->Files_listWidget->item(num)->text()).toStdString();
    Mat img = imread(file_path);
    enhanceImage(img);
    colorImage(img);
    medianBlur(img, img, 1);
    Mat rgb;
    cvtColor(img, rgb, COLOR_BGR2RGB);
    disImage = QImage((const uchar*)(rgb.data), rgb.cols, rgb.rows, rgb.cols*rgb.channels(), QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(disImage);
    QPixmap scaled = pixmap.scaled(ui->label_image->width(), ui->label_image->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_image->setScaledContents(true);
    ui->label_image->setPixmap(scaled);
    ratio = img.cols / ui->label_image->width();
    QApplication::processEvents();
}

void MainWindow::showNextImage()
{
    int currentIndex = ui->Files_listWidget->currentRow();
    if (currentIndex != ui->Files_listWidget->count()-1)
    {
        showImage(currentIndex+1);
        ui->Files_listWidget->setCurrentRow(currentIndex+1);
    }
    else
    {
        QMessageBox::about(NULL, "Message", "Finish Reading");
        disconnect(this->timer, SIGNAL(timeout()), this, SLOT(showNextImage()));
        ui->btn_Start->setText("Start");
        ui->btn_SpeedDown->setEnabled(0);
        ui->btn_SpeedUp->setEnabled(0);
        ui->Files_listWidget->setCurrentRow(0);
        return;
    }
}

void MainWindow::showPrevImage()
{
    int currentIndex = ui->Files_listWidget->currentRow();
    if (currentIndex != 0)
    {
        showImage(currentIndex-1);
        ui->Files_listWidget->setCurrentRow(currentIndex-1);
    }
    else
    {
        QMessageBox::about(NULL, "Warn", "This is the first image");
    }
}

void MainWindow::autoPlay()
{
    if (ui->btn_Start->text() == "Start")
    {
        /* button status */
        ui->btn_SpeedDown->setEnabled(1);
        ui->btn_SpeedUp->setEnabled(1);
        ui->btn_Next->setEnabled(0);
        ui->btn_Previous->setEnabled(0);
        ui->btn_Start->setText("Stop");
        timer = new QTimer(this);
        timer->setInterval(speed);
        QObject::connect(this->timer, SIGNAL(timeout()), this, SLOT(showNextImage()));
        this->timer->start();
        showImage(ui->Files_listWidget->currentRow());
    }
    else
    {
        /* button status */
        this->timer->stop();
        ui->btn_SpeedDown->setEnabled(0);
        ui->btn_SpeedUp->setEnabled(0);
        ui->btn_Next->setEnabled(1);
        ui->btn_Previous->setEnabled(1);
        ui->btn_Start->setText("Start");
    }
}

/***********************************************************************************************
*                                  Button Function
************************************************************************************************/

void MainWindow::saveAnnotation()
{
    if (ui->Anno_listWidget->count() != 0)
    {
        // save pic and anno

        QString path;
        QDir dir;
        QDateTime time = QDateTime::currentDateTime();
        path = dir_path + "_annotaion";

        QDir dir_file;
        QString path_file;
        QString path_xml;
        if (!dir_file.exists(path))
        {
            dir_file.mkdir(path);
        }
        path_file = path + "/" + dir_path.right(dir_path.length() - dir_path.lastIndexOf("/") -1) + "annotaion_" + QString::number(curIdx) + ".data";
        path_xml = path + "/" + dir_path.right(dir_path.length() - dir_path.lastIndexOf("/") -1) + "annotaion_" + QString::number(curIdx) + ".xml";
        disImage.save(path_file, "bmp");

        /* sonar info */
        QFile csv_file(csv_path);
        QStringList csv_lines;
        QStringList csv_info;
        if (!csv_file.open(QFile::ReadWrite | QIODevice::Text))
        {
            QMessageBox::about(NULL, "WARNING", "No Sonar INFO!");
        }

        QTextStream csv_out(&csv_file);
        while (!csv_out.atEnd())
        {
            csv_lines.push_back(csv_out.readLine());
        }
        QString line = csv_lines.at(ui->Files_listWidget->currentRow());
        csv_info = line.split(",");

        /* annotation */
        QFile file(path_xml);
        if (!file.open(QFile::ReadWrite | QIODevice::Text))
        {
            return;
        }
        QXmlStreamWriter writer(&file);

        writer.setAutoFormatting(true);
        writer.writeStartDocument();

        writer.writeStartElement("sonar");
        writer.writeTextElement("tpye", csv_info.at(8));
        writer.writeTextElement("version", csv_info.at(5));
        writer.writeTextElement("range", csv_info.at(3));
        writer.writeTextElement("horiangle", csv_info.at(4));
        if (csv_info.at(7) == "1200")
            writer.writeTextElement("vertiangle", QString::number(12));
        else
            writer.writeTextElement("vertiangle", QString::number(20));
        writer.writeTextElement("soundspeed", csv_info.at(6));
        writer.writeTextElement("frequency", csv_info.at(7));

        writer.writeEndElement();

        writer.writeStartElement("annotation");
        writer.writeTextElement("folder", dir_path.right(dir_path.length() - dir_path.lastIndexOf("/") -1) + "annotaion");
        writer.writeTextElement("filename", dir_path.right(dir_path.length() - dir_path.lastIndexOf("/") -1) + "annotaion_" + QString::number(curIdx) + ".xml");
        writer.writeTextElement("path", path_file);

        writer.writeStartElement("size");
        writer.writeTextElement("width", QString::number(disImage.width()));
        writer.writeTextElement("height", QString::number(disImage.height()));
        writer.writeTextElement("depth", QString::number(disImage.depth()));
        writer.writeEndElement();

        for (int i = 0; i < ui->Anno_listWidget->count(); i++)
        {
            writer.writeStartElement("object");
            writer.writeTextElement("name", label_names[i]);
            writer.writeStartElement("bndbox");
            writer.writeTextElement("xmin", QString::number(current_rects[i][0]));
            writer.writeTextElement("ymin", QString::number(current_rects[i][2]));
            writer.writeTextElement("xmax", QString::number(current_rects[i][1]));
            writer.writeTextElement("ymax", QString::number(current_rects[i][3]));
            writer.writeEndElement();
            writer.writeEndElement();
        }

        writer.writeEndElement();
        writer.writeEndDocument();

        // clear
        label_names.clear();
        curIdx++;
        ui->Anno_listWidget->clear();
        ui->label_image->deleteRectlist();
    }
}

void MainWindow::on_btn_Start_clicked()
{
    if (file_path.empty())
    {
        QMessageBox::about(NULL, "ERROR", "Please Select BMP files");
        return;
    }
    saveAnnotation();
    autoPlay();
}

void MainWindow::on_btn_Next_clicked()
{
    saveAnnotation();
    showNextImage();
}

void MainWindow::on_btn_Previous_clicked()
{
    saveAnnotation();
    showPrevImage();
}

void MainWindow::on_btn_SpeedUp_clicked()
{
    speed -= 10;
    if (speed > 20)
    {
        this->timer->setInterval(speed);
    }
    else
    {
        QMessageBox::about(NULL, "Error", "out of maximun speed");
    }
}

void MainWindow::on_btn_SpeedDown_clicked()
{
    speed += 10;
    this->timer->setInterval(speed);
}
