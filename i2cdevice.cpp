#include "i2cdevice.h"

#include <fstream>
#include <ostream>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

I2CDevice::I2CDevice(std::string device/* = "/dev/i2c-1"*/, u_char dev/* = UNKNOWN*/, bool slave/* = true*/, bool tenbits/* = false*/)
	: m_i2cdev(0)
	, m_addr(0)
	, m_device("")
{
	set_device(device);
	open(dev, slave, tenbits);
}


I2CDevice::~I2CDevice()
{
	close();
}

u_char I2CDevice::addr() const
{
	return m_addr;
}

std::string I2CDevice::device() const
{
	return m_device;
}

void I2CDevice::set_device(std::string value)
{
	if(value == m_device)
		return;
	close();
	m_device = value;
}

bool I2CDevice::is_opened() const
{
	return m_i2cdev > 0;
}

bool I2CDevice::open(u_char addr, bool slave, bool tenbits)
{
	if(addr == UNKNOWN || m_addr == addr)
		return false;

	m_i2cdev = ::open(m_device.c_str(), O_RDWR);

	if(m_i2cdev == -1)
		return false;

	m_addr = addr;

	u_char res = 0;
	res = ioctl(m_i2cdev, I2C_TENBIT, tenbits);
	if(slave)
		res = ioctl(m_i2cdev, I2C_SLAVE, m_addr);

	return true;
}

void I2CDevice::close()
{
	if(!m_i2cdev)
		return;

	::close(m_i2cdev);
	m_i2cdev = 0;
}

int I2CDevice::write(u_char addr, u_char data[], int count)
{
	int res = 0;

	m_write_data.resize(1 + count);

	m_write_data[0] = addr;

	if(count){
		std::copy(data, data + count, &m_write_data[1]);
	}

	res = ::write(m_i2cdev, &m_write_data.front(), m_write_data.size());
	return res;
}

int I2CDevice::read(u_char addr, u_char data[], int count)
{
	int res = 0;
	res = ::write(m_i2cdev, &addr, 1);
	if(res >= 0)
		res = ::read(m_i2cdev, data, count);
	return res;
}
