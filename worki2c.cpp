#include "worki2c.h"

#include <sstream>
#include <iostream>

#include <QDebug>
#include <QDateTime>

#include "senddata.h"

using namespace  sc;
using namespace vector3_;

const uchar addr_mpu6050 = 0x68;
const uchar addr_hmp5883 = 0x1E;
const uchar addr_bmp180 = 0x77;

const uchar confRegA_hmc5883 = 0x00;
const uchar confRegB_hmc5883 = 0x01;
const uchar confRegM_hmc5883 = 0x02;

const int freq_gyroAndCompass = 100;
const int period_gyroAndCompass = 1000/freq_gyroAndCompass;

const int freq_baro = 500;
const int period_baro = 1000/freq_baro;

const int default_interval_ms_barometer_read = 30;
const uchar oss_bmp180 = 2;

namespace barotimeset{
	const int temp = 5;
	const int pressure_oss0 = 5;
	const int pressure_oss1 = 8;
	const int pressure_oss2 = 14;
	const int pressure_oss3 = 26;
	int pressure_ossN = pressure_oss0;
}

///////////////////////////////////////////////

WorkI2C::WorkI2C(QObject *parent) : QObject(parent)
  , m_sendData(0)
  , m_regA_hmc5883(0)
  , m_baroState(NULLSTATE),
	m_baro_interval(0)
  , m_baro_time_set(0)
{
	connect(&m_timer_mpu6050, SIGNAL(timeout()), this, SLOT(on_timeout_mpu6050()));
	m_timer_mpu6050.start(period_gyroAndCompass);

	connect(&m_timer_hmc5883, SIGNAL(timeout()), this, SLOT(on_timeout_hmc5883()));
	m_timer_hmc5883.start(period_gyroAndCompass);

	connect(&m_timer_bmp180, SIGNAL(timeout()), this, SLOT(on_timeout_bmp180()));
	m_timer_bmp180.start(period_baro);

	init();
}

WorkI2C::~WorkI2C()
{

}

void WorkI2C::set_senser(send_data::SendData *sender)
{
	m_sendData = sender;
}

void WorkI2C::init()
{
	init_mpu6050();
	init_hmc5883();
	init_bmp180();
}

void WorkI2C::on_timeout_mpu6050()
{
	if(!m_i2cMpu6050.is_opened())
		return;

	uchar txbuf[10] = {0};

	int res;

	u_short data_out[7];
	short data[7];

	txbuf[0] = 0x3b;
	res = m_i2cMpu6050.read(0x3b, reinterpret_cast<u_char *>(data_out), 14);

	qint64 tick = QDateTime::currentMSecsSinceEpoch();

	for(int i = 0; i < 7/*data_out.size()*/; i++){
		data[i] = static_cast<short>((data_out[i] << 8) | (data_out[i] >> 8));
	}

	if(m_sendData){
		m_sendData->set_gyroscope(Vector3i(data[4], data[5], data[6]),
				Vector3i(data[0], data[1], data[2]),
				data[3] / 340.f + 36.53f, tick);
	}
}

void WorkI2C::on_timeout_hmc5883()
{
	if(!m_i2cHmc5883.is_opened())
		return;
	int res = 0;
	uchar data_out[6] = { 0 };
	short data[3] = { 0 };
	uchar addr_data_hmc5883 = 0x03;

	res = m_i2cHmc5883.read(addr_data_hmc5883, data_out, sizeof(data_out));

//	qDebug() << "compass raw" << (int)m_i2cHmc5883.addr() << data_out[0] << data_out[1] << data_out[2] << res;

	qint64 tick = QDateTime::currentMSecsSinceEpoch();

	if(res <= 0)
		return;

	for(int i = 0; i < 3; i++){
		data[i] = (data_out[(i << 1)] << 8) | (data_out[(i << 1) + 1]);
	}

	Vector3i out(data[0], data[2], data[1]);

	double d = out.x() * out.x() + out.y() * out.y();

	qDebug() << out << sqrt(d);

	if(m_sendData){
		m_sendData->set_compass(out, m_regA_hmc5883, tick);
	}
}

void WorkI2C::on_timeout_bmp180()
{
	if(!m_i2cBmp180.is_opened())
		return;

	if(m_baroTime.elapsed() > m_baro_time_set + m_baro_interval){
		switch (m_baroState) {
			case NULLSTATE:
			case SETTEMP:
				write_baro_get_temp();
				m_baroState = GETTEMP;
				m_baro_interval = barotimeset::temp;
				break;
			case GETTEMP:
				read_baro_get_temp();
				m_baroState = SETPRESSURE;
				m_baro_interval = 0;
			case SETPRESSURE:
				write_baro_get_pressure();
				m_baroState = GETPRESSURE;
				m_baro_interval = barotimeset::pressure_ossN;
				break;
			case GETPRESSURE:
				read_baro_get_pressure();
				m_baroState = SETTEMP;
				m_baro_interval = 0;
				break;
			default:
				break;
		}
		//qDebug() << m_baroState << m_baro_interval;
		m_baro_time_set = m_baroTime.elapsed();
	}

}

void WorkI2C::write_baro_get_temp()
{
	uchar addr_reg = 0xf4;
	uchar val_reg = 0x2e;
	m_i2cBmp180.write(addr_reg, &val_reg, 1);

}

void WorkI2C::read_baro_get_temp()
{
	uchar data[2];
	uchar addr_reg = 0xf6;
	m_i2cBmp180.read(addr_reg, data, 2);
	int utemp = (data[0] << 8) | data[1];

	//qDebug() << utemp;

	if(m_sendData){
		m_sendData->set_temperature(utemp);
	}

}

void WorkI2C::write_baro_get_pressure()
{
	uchar addr_reg = 0xf4;

	uchar val_reg = 0x34 + (oss_bmp180 << 6);

	m_i2cBmp180.write(addr_reg, &val_reg, 1);
}

void WorkI2C::read_baro_get_pressure()
{
	uchar addr_read = 0xf6;

	uchar data[3];

	int res = m_i2cBmp180.read(addr_read, data, sizeof(data));

	qint64 tick = QDateTime::currentMSecsSinceEpoch();

	if(res <= 0)
		return;

	int pressure = ((data[0] << 16) + (data[1] << 8) + data[2]) >> (8 - oss_bmp180);

	if(m_sendData){
		m_sendData->set_barometer(pressure, tick);
	}
}

void WorkI2C::init_mpu6050()
{
	if(!m_i2cMpu6050.open(addr_mpu6050)){
		std::cout << "mpu6050: device not open\n";
		return;
	}
	std::cout << "mpu6050: device opened\n";
	u_char data[2] = { 0 };
	m_i2cMpu6050.write(0x6B, data, 1);

	if(m_sendData){
		StructGyroscope gyroscope;

		m_i2cMpu6050.read(0x0d, gyroscope.raw, raw_count);
		gyroscope.afs_sel = (gyroscope.raw[28 - 0x0d] >> 3) & 0x03;
		gyroscope.fs_sel = (gyroscope.raw[27 - 0x0d] >> 3) & 0x03;

		uchar dlpf_cfg = gyroscope.raw[26 - 0x0d] & 0x3;
		int smplrt_div = gyroscope.raw[25 - 0x0d];
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

		gyroscope.freq =  sample_rate_div;

		m_sendData->set_config_gyroscope(gyroscope);
	}
}

void WorkI2C::init_hmc5883()
{
	if(!m_i2cHmc5883.open(addr_hmp5883)){
		std::cout << "hmp5883l: device not open\n";
		return;
	}
	std::cout << "hmc5883: device opened\n";

	const uchar ma10 = 0x3;		/// 4 samples averages per measurement output;
	const uchar do210 = 0x6;	/// mode of output rate (0x6 === 75Hz)
	const uchar ms10 = 0;		/// measurement mode (0 === normal)

	m_regA_hmc5883 =  (ma10 << 5) | (do210 << 2) | (ms10);

	m_i2cHmc5883.write(confRegA_hmc5883, &m_regA_hmc5883, 1);

	uchar config_regB = 0x1 << 5;//	/// 1.3 Ga (default)

	m_i2cHmc5883.write(confRegB_hmc5883, &config_regB, 1);

	uchar mode = 0x0;			/// continuous mode

	m_i2cHmc5883.write(confRegM_hmc5883, &mode, 1);
}

void WorkI2C::init_bmp180()
{
	if(!m_i2cBmp180.open(addr_bmp180)){
		std::cout << "bmp180: device not open\n";
		return;
	}
	std::cout << "bmp180: device opened\n";

	switch (oss_bmp180) {
		case 0:
		default:
			barotimeset::pressure_ossN = barotimeset::pressure_oss0;
			break;
		case 1:
			barotimeset::pressure_ossN = barotimeset::pressure_oss1;
			break;
		case 2:
			barotimeset::pressure_ossN = barotimeset::pressure_oss2;
			break;
		case 3:
			barotimeset::pressure_ossN = barotimeset::pressure_oss3;
			break;
	}
	m_timer_bmp180.setInterval(barotimeset::pressure_ossN);

	m_baroState = SETTEMP;
	m_baroTime.start();
	m_baro_time_set = m_baroTime.elapsed();
}

