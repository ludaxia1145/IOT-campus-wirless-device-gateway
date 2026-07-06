#ifndef LIBRARYPAGE_H
#define LIBRARYPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "networkclient.h"
#include "audioplayer.h"
class LibraryPage : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryPage(NetworkClient *client, QWidget *parent = nullptr);
    void loadSeats();

signals:
    void backToMain();

private slots:
    void onSeatClicked();
    void onLibrarySeatsReceived(const QJsonObject &data);
    void onResponseReceived(const QString &type, const QJsonObject &data);

private:
    NetworkClient *networkClient;
    QComboBox *timeSlotCombo;
    QLineEdit *studentIdEdit;
    QScrollArea *scrollArea;
    QWidget *seatContainer;
    QGridLayout *seatLayout;
    QPushButton *btnBack;

    QMap<int, QPushButton*> seatButtons;
    QMap<int, QString> seatOwners;  // 座位号 -> 学号
    int selectedSeat;
    QString currentTimeSlot;
};

#endif
