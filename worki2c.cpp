#include "worki2c.h"

#include <sstream>
#include <iostream>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

const int gaddr = 0x68;
const float del_gyro = 1.f / 200.f;
const float del_accel = 1.f / 2000.f;

WorkI2C::WorkI2C(QObject *parent) : QObject(parent)
  , m_i2cdev(0)
{
	init();

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
	m_timer.start(100);

	m_sendData = new send_data::SendData;
	m_sendData->start();
}

WorkI2C::~WorkI2C()
{
	if(m_i2cdev){
		close(m_i2cdev);
	}
}

void WorkI2C::init()
{
	m_i2cdev = open("/dev/i2c-1", O_RDWR);
	int res;

	res = ioctl(m_i2cdev, I2C_TENBIT, 0);
	std::cout << "result set tenbit " << res << std::endl;
	res = ioctl(m_i2cdev, I2C_SLAVE, gaddr);
	std::cout << "result set slave bit " << res << std::endl;

	uchar txbuf[2] = {0x6B, 0};
	write(m_i2cdev, txbuf, 2);

	uchar rxbuf[2] = {0};
	read(m_i2cdev, rxbuf, 1);
	std::cout << "after write 0: " << (u_int)rxbuf << std::endl;

}

void WorkI2C::on_timeout()
{
	if(!m_i2cdev)
		return;

	uchar rxbuf[10] = {0}, txbuf[10] = {0};

	int res;

	int data[7];

	//std::stringstream ss;
	for(int i = 0; i < 7/*data_out.size()*/; i++){
		short val = 0;
		txbuf[0] = 0x3b + 2 * i;
		res = write(m_i2cdev, txbuf, 1);
		res = read(m_i2cdev, rxbuf, 1);
		val = (rxbuf[0] << 8);

		txbuf[0] = 0x3b + 2 * i + 1;
		res = write(m_i2cdev, txbuf, 1);
		res = read(m_i2cdev, rxbuf, 1);
		val |= rxbuf[0];
		//ss << (int)val << "; ";
		data[i] = val;
	}

	FOREACH(i, 3, data[i] >>= 4);
	FOREACH(i, 3, data[4 + i] >>= 6);

	//std::cout << ss.str() << std::endl;

	if(m_sendData){
		m_sendData->push_data(Vector3f(data[4], data[5], data[6]) * del_gyro,
				Vector3f(data[0], data[1], data[2]) * del_accel,
				data[3] / 340.f * 36.53f);
	}
}

