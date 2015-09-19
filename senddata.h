#ifndef SENDDATA_H
#define SENDDATA_H

#include <QThread>
#include <QHostAddress>
#include <QVector>

#include "struct_controls.h"

class QUdpSocket;
class QTimer;

namespace send_data{

const int max_send_buffer_count = 100;
const int delay_timer = 10;

const ushort c_port_receiver_default = 7777;

class SendData : public QThread
{
	Q_OBJECT
public:
	SendData(QObject *parent = NULL);
	virtual ~SendData();

	void setDelay(int delay);
	void set_port_receiver(ushort port);
	void push_data(const Vector3f& gyroscope, const Vector3f& accelerometer, float temp);

signals:
	void send_set_interval(int);

public slots:
	void on_readyRead();
	void on_timeout();
	void on_send_set_delay(int value);
protected:
	virtual void run();
	void tryParseData(const QByteArray& data, const QHostAddress& host, ushort port);

private:
	QTimer *m_timer;
	QUdpSocket* m_socket;
	ushort m_port_receiver;
	ushort m_port_sender;
	QHostAddress m_host_sender;
	QVector< StructTelemetry > m_data_send;
	bool m_send_start;
};

}

#endif // SENDDATA_H