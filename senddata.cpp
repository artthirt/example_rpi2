#include "senddata.h"

#include <QUdpSocket>
#include <QDataStream>
#include <QTimer>

using namespace send_data;

SendData::SendData(QObject *parent)
	:QThread(parent)
	, m_socket(0)
	, m_timer(0)
	, m_send_start(false)
{
	m_port_receiver = c_port_receiver_default;
	m_port_sender = 0;
}

SendData::~SendData()
{
	quit();
	wait();

	if(m_socket){
		m_socket->abort();

		delete m_socket;
	}

	if(m_timer){
		delete m_timer;
	}
}

void SendData::setDelay(int delay)
{
	emit send_set_interval(delay);
}

void SendData::set_port_receiver(ushort port)
{

}

void SendData::push_data(const Vector3f &gyroscope, const Vector3f &accelerometer, float temp)
{
	if(!m_send_start)
		return;

	while(m_data_send.size() >= max_send_buffer_count){
		m_data_send.pop_back();
	}

	StructTelemetry st;
	st.gyro = gyroscope;
	st.accel = accelerometer;
	st.temp = temp;

	m_data_send.push_back(st);
}

void SendData::on_readyRead()
{
	QByteArray data;
	while(m_socket->hasPendingDatagrams()){
		qint64 size = m_socket->pendingDatagramSize();
		data.resize(size);
		QHostAddress host;
		ushort port;
		m_socket->readDatagram(data.data(), data.size(), &host, &port);
		tryParseData(data, host, port);
	}
}

void SendData::on_timeout()
{
	if(!m_send_start)
		return;

	if(m_data_send.size()){

		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		StructTelemetry& st = m_data_send[0];
		stream << st.power_on;
		FOREACH(i, cnt_engines, stream << st.power);
		stream << st.tangaj;
		stream << st.bank;
		stream << st.course;
		stream << st.temp;
		stream << st.height;
		stream << st.gyro.x();
		stream << st.gyro.y();
		stream << st.gyro.z();
		stream << st.accel.x();
		stream << st.accel.y();
		stream << st.accel.z();

		m_data_send.pop_front();
	}
}

void SendData::on_send_set_delay(int value)
{
	if(m_timer){
		m_timer->setInterval(value);
	}
}

void SendData::run()
{
	m_socket = new QUdpSocket;
	m_socket->bind(m_port_receiver);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(on_readyRead()));

	m_timer = new QTimer;
	connect(m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
	connect(this, SIGNAL(send_set_interval(int)), this, SLOT(on_send_set_delay(int)), Qt::QueuedConnection);
	m_timer->start(delay_timer);

	exec();
}

void SendData::tryParseData(const QByteArray &data, const QHostAddress &host, ushort port)
{
	if(data == "START"){
		m_host_sender = host;
		m_port_sender = port;
		m_send_start = true;
	}
	if(data == "STOP"){
		m_send_start = false;
		m_data_send.clear();
	}
}

