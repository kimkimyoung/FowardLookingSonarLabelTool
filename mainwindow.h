#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTimer>

#include <cmath>
#include <string>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "eventlabel.h"
#include "labeldialog.h"

using namespace cv;
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void systemInit();
    ~MainWindow();

    /*flags*/
    int stop_flag = 0;
    int save_mode;

signals:
    void labelAdded(QString label);
    void labelDeleted(QString label);
    void labelCleared();

private slots:
    /*btn*/
    void on_btn_Start_clicked();
    void on_btn_Next_clicked();
    void on_btn_Previous_clicked();
    void on_btn_SpeedUp_clicked();
    void on_btn_SpeedDown_clicked();

    /*tool bar*/
    void openFile();
    void openDir();
    void appQuit();
    void saveAnnotation();

    /*files*/
    void douleClickFile(QListWidgetItem *item);

    /*label*/
    void newLabel(QString label);
    void newLabelfromDialog(QString label);
    void deleteLabel(QString label);
    void labelMenu(const QPoint &pos);

    /*annotation*/
    void setLabel(QString label);
    void setRect(QList<int>);
    void setAnnotation();

    /* picture solver*/
    void showImage(int num);
    void showNextImage();
    void showPrevImage();
    void autoPlay();

private:
    Ui::MainWindow *ui;
    std::string file_path;
    QString dir_path;
    QStringList file_list;
    QString saving_path;
    QString csv_path;

    EventLabel *p_label = new EventLabel();
    LabelDialog *dialog = new LabelDialog();
    QTimer *timer;

    QImage disImage;
    QString label_name;
    QList<int> current_rect;
    QVector<QString> label_names;
    QVector<QList<int>> current_rects;
    QString width;
    QString height;
    QString origin_x;
    QString origin_y;
    QString sonar_annotation;
    cv::Mat imageSaved;
    int curIdx = 0;
    int speed = 100;
    float ratio;

    /* Manager */
    void setupLabelManager();
    void setupFileManager();
    void setupmainToolBar();
    void setupLabelDialog();
    void setupAnnotationManager();

};

#endif // MAINWINDOW_H
