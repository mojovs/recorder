#include "mainwindow.h"

#include "ui_mainwindow.h"
#pragma execution_character_set("utf-8")
/*-------------------------------------构造------------------------------------------------*/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*--录音实例化以及函数绑定--*/
    recorder = new QAudioRecorder(this);
    connect(recorder, SIGNAL(stateChanged(QMediaRecorder::State)), this, SLOT(onStateChanged(QMediaRecorder::State)));
    connect(recorder, SIGNAL(durationChanged(qint64)), this, SLOT(onDurationChanged(qint64)));

    /*--探测器实例化以及函数绑定--*/
    probe = new QAudioProbe;  //探测器
    connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)), this, SLOT(processBuffer(QAudioBuffer)));
    probe->setSource(recorder);  //制定探测对象

    if (recorder->defaultAudioInput().isEmpty())  //如果没有默认的音频设置
        return;

    foreach (const QString &device, recorder->audioInputs()) {
        qDebug() << device << endl;
        ui->comboBoxAudioInput->addItem(device);  //音频输入设备列表
    }

    foreach (const QString &codecName, recorder->supportedAudioCodecs()) {
        ui->comboBoxCodec->addItem(codecName);  //音频编码列表
    }

    foreach (int sampleRate, recorder->supportedAudioSampleRates()) {
        ui->comboBoxSampleRate->addItem(QString::number(sampleRate));  //音频设备采样率列表
    }

    /*--设置音频通道列表--*/
    ui->comboBoxChannelCount->addItem("1");
    ui->comboBoxChannelCount->addItem("2");
    ui->comboBoxChannelCount->addItem("4");
    /*--音频质量--*/
    ui->sliderQuality->setRange(0, int(QMultimedia::VeryHighQuality));
    ui->sliderQuality->setValue(int(QMultimedia::NormalQuality));

    /*--比特率--*/
    ui->comboBoxBytesPerFrame->addItem("32000");
    ui->comboBoxBytesPerFrame->addItem("64000");
    ui->comboBoxBytesPerFrame->addItem("96000");
    ui->comboBoxBytesPerFrame->addItem("128000");
}

/*-------------------------------------构造------------------------------------------------*/
MainWindow::~MainWindow() { delete ui; }

/*-------------------------------------槽 状态改变------------------------------------------------*/
void MainWindow::onStateChanged(QMediaRecorder::State state)
{
    ui->actStart->setEnabled(QMediaRecorder::RecordingState != state);
    ui->actPause->setEnabled(QMediaRecorder::RecordingState == state);
    ui->actStop->setEnabled(QMediaRecorder::RecordingState == state);

    ui->btnGetFile->setEnabled(QMediaRecorder::StoppedState == state);
    ui->lineEditOutPutFile->setEnabled(QMediaRecorder::StoppedState == state);
    qDebug()<<"录音状态发生变化"<<endl;
}
/*-------------------------------------槽 时长改变------------------------------------------------*/
void MainWindow::onDurationChanged(qint64 duration) {

    ui->labTime->setText(QString("已录制 %1秒").arg(duration / 1000));
    qDebug()<<duration/1000;
}

/*-------------------------------------槽 处处理探测到的缓冲区------------------------------------------------*/
void MainWindow::processBuffer(const QAudioBuffer &buffer)
{
    ui->spinBoxByteCounts->setValue(buffer.byteCount());	//缓冲区字节数
    ui->spinBoxDuration->setValue(buffer.duration() / 1000);  //缓冲区时长
    ui->spinBoxBufferFrames->setValue(buffer.frameCount());		//缓冲区帧数
    ui->spinBoxSampleCounts->setValue(buffer.sampleCount());	//采样数

    QAudioFormat audioFormat = buffer.format();	//缓冲区格式
    /*--通道数，采样大小 率--*/
    ui->spinBoxChannels->setValue(audioFormat.channelCount());
    ui->spinBoxSampleSize->setValue(audioFormat.sampleSize());
    ui->spinBoxSampleRate->setValue(audioFormat.sampleRate());
    ui->spinBoxBytesPerFrame->setValue(audioFormat.bytesPerFrame());	//字节帧率
    if(audioFormat.byteOrder() == QAudioFormat::LittleEndian)	//字节序
        ui->lineEditByteOrder->setText("LittleEndian");
    else
        ui->lineEditByteOrder->setText("BigEndian");
    ui->lineEditCodex->setText(audioFormat.codec());	//编码
    /*--采样类型--*/
    if(audioFormat.sampleType() == QAudioFormat::SignedInt)	//采样为有符号整数
        ui->lineEditSampleType->setText("SignIn");
    else if(audioFormat.sampleType() == QAudioFormat::UnSignedInt)	//采样为无符号整数
        ui->lineEditSampleType->setText("UnSignedInt");
    else if(audioFormat.sampleType() == QAudioFormat::Float)	//采样为无符号整数
        ui->lineEditSampleType->setText("Float");
    else
        ui->lineEditSampleType->setText("unKnow");

}

/*-------------------------------------槽 获取路径，显示路径------------------------------------------------*/
void MainWindow::on_btnGetFile_clicked()
{
    QString path     = QDir::currentPath();
    QString pathName = QFileDialog::getExistingDirectory(this, "选择一个路径", path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    ui->lineEditOutPutFile->setText(pathName + "/OutPutFile");
}

/*-------------------------------------槽 开始录制------------------------------------------------*/
void MainWindow::on_actStart_triggered()
{
    /*--如果已经停止了--*/
    if (recorder->state() == QMediaRecorder::StoppedState) {
        /*--先获取文件路径--*/
        QString outputFile = ui->lineEditOutPutFile->text().trimmed();
        if (outputFile.isEmpty()) {
            QMessageBox::critical(this, "错误", "请先设置录音输出文件");
            return;
        }
        /*--如果这个文件存在，那么就把这个文件删除--*/
        if (QFile::exists(outputFile)) {
            if (!QFile::remove(outputFile)) {
                QMessageBox::critical(this, "致命错误", "设置的文件输出文件无法被删除");
                return;
            }
        }

        recorder->setOutputLocation(QUrl::fromLocalFile(outputFile));    //设置录音默认输出位置
        recorder->setAudioInput(ui->comboBoxAudioInput->currentText());  //输入设备
        if(!recorder->isAvailable())
            qDebug()<<"record "<<recorder->audioInput()<<" not available";
        /*--编解码设置--*/
        QAudioEncoderSettings settings;
        settings.setCodec(ui->comboBoxCodec->currentText());
        settings.setBitRate(ui->comboBoxBytesPerFrame->currentText().toInt());
        settings.setSampleRate(ui->comboBoxSampleRate->currentText().toInt());
        settings.setChannelCount(ui->comboBoxChannelCount->currentText().toInt());
        settings.setQuality(QMultimedia::EncodingQuality(ui->sliderQuality->value()));

        if (ui->radioBtnQuality->isChecked())  //如果为固定品质
            settings.setEncodingMode(QMultimedia::ConstantQualityEncoding);
        else
            settings.setEncodingMode(QMultimedia::ConstantBitRateEncoding);
        recorder->setAudioSettings(settings);
    }
    recorder->record();
}
/*-------------------------------------槽 动作 录音暂停------------------------------------------------*/
void MainWindow::on_actPause_triggered() { recorder->pause(); }

/*-------------------------------------槽 动作 停止------------------------------------------------*/
void MainWindow::on_actStop_triggered() { recorder->stop(); }
