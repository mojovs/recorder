#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioRecorder>
#include <QAudioProbe>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QAudioRecorder *recorder;		//录音
    QAudioProbe *probe;	//探测

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    /*--自定义槽函数--*/
    void onStateChanged(QMediaRecorder::State state);
    void onDurationChanged(qint64 duration);
    void processBuffer(const QAudioBuffer & buffer);

    void on_btnGetFile_clicked();

    void on_actStart_triggered();

    void on_actPause_triggered();

    void on_actStop_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
