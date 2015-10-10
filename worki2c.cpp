#include "worki2c.h"

#include <sstream>
#include <iostream>

#include <QDebug>
#include <QDateTime>

const int gaddr = 0x68;

using namespace  sc;
using namespace vector3_;

WorkI2C::WorkI2C(QObject *parent) : QObject(parent)
{
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
	m_timer.start(5);

	m_sendData = new send_data::SendData;
	m_sendData->start();
	m_sendData->moveToThread(m_sendData);

	init();
}

WorkI2C::~WorkI2C()
{
	if(m_sendData){
		delete m_sendData;
	}
}

void WorkI2C::init()
{
	if(!m_i2cdev.open(0x68)){
		std::cout << "device not open\n";
		return;
	}
	std::cout << "device opened\n";
	u_char data[2] = { 0 };
	m_i2cdev.write(0x6B, data, 1);

	if(m_sendData){
		StructTelemetry telem;

		m_i2cdev.read(0x0d, telem.raw, raw_count);
		telem.afs_sel = (telem.raw[28 - 0x0d] >> 3) & 0x03;
		telem.fs_sel = (telem.raw[27 - 0x0d] >> 3) & 0x03;

		uchar dlpf_cfg = telem.raw[26 - 0x0d] & 0x3;
		int smplrt_div = telem.raw[25 - 0x0d];
		int gyr_out_rate;

		switch (dlpf_cfg) {
			case 0:
			case 7:
				gyr_out_rate = 8000;
				break;
			default:
				gyr_out_rate = 1000;
				break;
		}

		double sample_rate_div = gyr_out_rate / (1 + smplrt_div);

		telem.freq =  sample_rate_div;

		m_sendData->set_config_params(telem);
	}
}

void WorkI2C::on_timeout()
{
	if(!m_i2cdev.is_opened())
		return;

	uchar txbuf[10] = {0};

	int res;

	u_short data_in[7];
	short data[7];

	txbuf[0] = 0x3b;
	res = m_i2cdev.read(0x3b, reinterpret_cast<u_char *>(data_in), 14);

	qint64 tick = QDateTime::currentMSecsSinceEpoch();

//	QString str;
	for(int i = 0; i < 7/*data_out.size()*/; i++){
		data[i] = static_cast<short>((data_in[i] << 8) | (data_in[i] >> 8));
//		str += QString::number(data[i]) + "; ";
	}
//	for(int i = 0; i < 7/*data_out.size()*/; i++){
//		short val = 0;
//		txbuf[0] = 0x3b + 2 * i;
//		res = write(m_i2cdev, txbuf, 1);
//		res = read(m_i2cdev, rxbuf, 1);
//		val = (rxbuf[0] << 8);

//		txbuf[0] = 0x3b + 2 * i + 1;
//		res = write(m_i2cdev, txbuf, 1);
//		res = read(m_i2cdev, rxbuf, 1);
//		val |= rxbuf[0];
//		ss << (int)val << "; ";
//		data[i] = val;
//	}

//	qDebug() << str;
//	std::cout << ss.str() << std::endl;

	if(m_sendData){
		m_sendData->push_data(Vector3i(data[4], data[5], data[6]),
				Vector3i(data[0], data[1], data[2]),
				data[3] / 340.f + 36.53f, tick);
	}
}

