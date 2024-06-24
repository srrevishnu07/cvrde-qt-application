#include "mainwindow.h"
#include "ui_MMWS.h"
#include <QMessageBox>
#include<QLabel>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->setDegreeButton, SIGNAL(clicked()), this, SLOT(on_setDegreeButton_clicked()));
    connect(ui->radarDisplayWidget, SIGNAL(dataSent(QByteArray)), this, SLOT(displayData(QByteArray)));
    connect(ui->verticalSlider, &QSlider::valueChanged, ui->radarDisplayWidget, &RadarWidget::setSliderDegree);
    connect(ui->DTVbutton, &QPushButton::clicked, ui->radarDisplayWidget, &RadarWidget::onDTVButtonClick);
    connect(ui->Zoominbutton, &QPushButton::clicked, ui->radarDisplayWidget, &RadarWidget::onZoomInButtonClick);
    connect(ui->Zoomoutbutton, &QPushButton::clicked, ui->radarDisplayWidget, &RadarWidget::onZoomOutButtonClick);
    connect(ui->Fplusbutton, &QPushButton::clicked, ui->radarDisplayWidget, &RadarWidget::onFPlusButtonClick);
    connect(ui->Fminusbutton, &QPushButton::clicked, ui->radarDisplayWidget, &RadarWidget::onFMinusButtonClick);
    connect(ui->radarDisplayWidget, &RadarWidget::degreeChanged, this, &MainWindow::updateDegreeLabel);
    connect(ui->radarDisplayWidget, &RadarWidget::dtvStatusChanged, this, &MainWindow::updateDTVStatusLabel);
    connect(ui->radarDisplayWidget, &RadarWidget::tiStatusChanged, this, &MainWindow::updateTIStatusLabel);
    connect(ui->radarDisplayWidget, &RadarWidget::fovStatusChanged, this, &MainWindow::updateFOVStatusLabel);

}



void MainWindow::on_setDegreeButton_clicked() {
    bool ok;
    int degree = ui->degreeInput->text().toInt(&ok);
    if (ok) {
        ui->radarDisplayWidget->setDegree(degree);
    } else {
        QMessageBox::warning(this, "Input Error", "Please enter a valid degree value.");
    }
}

void MainWindow::displayData(const QByteArray &data) {
// for displaying the serial data in the table.
    ui->bytesTable->insertRow(ui->bytesTable->rowCount());
    int lastRow = ui->bytesTable->rowCount() - 1;
    for (int i = 0; i < data.size(); ++i) {
        ui->bytesTable->setItem(lastRow, i, new QTableWidgetItem(QString::number(static_cast<unsigned char>(data[i]), 16).toUpper()));
    }

}


void MainWindow::on_verticalSlider_valueChanged(int value) {
    ui->textvalue->setText(QString::number(value) + "°");
}


void MainWindow::updateDegreeLabel(int degree) {
    ui->degreeval->setText(QString::number(degree) + "°");
}

void MainWindow::updateDTVStatusLabel(const QString& status) {
    ui->DTVStatusLabel->setText(status);
}

void MainWindow::updateTIStatusLabel(const QString& status) {
    ui->TIStatusLabel->setText(status);
}

void MainWindow::updateFOVStatusLabel(const QString& status) {
    ui->FOVStatusLabel->setText("FOV     " +    status);
}

MainWindow::~MainWindow()
{
    delete ui;
}



