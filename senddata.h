#ifndef SENDDATA_H
#define SENDDATA_H

#include <QThread>
#include <QHostAddress>
#include <QVector>
#include <QMutex>

#include "struct_controls.h"

class QUdpSocket;
class QTimer;

namespace send_data{

const int max_send_buffer_count = 100;
const int delay_timer = 5;

const ushort c_port_receiver_default = 7777;

class SendData : public QThread
{
	Q_OBJECT
public:
	SendData(QObject *parent = NULL);
	virtual ~SendData();

	sc::StructGyroscope config_gyroscope() const;
	sc::StructControls control_params() const;
	void set_config_gyroscope(const sc::StructGyroscope& telem);

	void setDelay(int delay);
	void set_port_receiver(ushort port);
	void set_gyroscope(const vector3_::Vector3i& gyroscope, const vector3_::Vector3i& accelerometer, float temp, qint64 time);
	void set_compass(const vector3_::Vector3i& compass, uchar mode, qint64 time);
	void set_barometer(int data, qint64 time);
	void set_temperature(int temp);

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
	QMutex m_mutex;
	QTimer *m_timer;
	QUdpSocket* m_socket;
	ushort m_port_receiver;
	ushort m_port_sender;
	QHostAddress m_host_sender;
	sc::StructTelemetry m_data_send;
	sc::StructGyroscope m_config_gyroscope;

	sc::StructControls m_control_params;
	sc::StructControls m_last_control_params;
	bool m_send_start;
	bool m_is_available_data;

	void read_ctrl(QDataStream& data);
};

}

#endif // SENDDATA_H
