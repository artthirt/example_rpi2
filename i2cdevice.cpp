#include "i2cdevice.h"

#include <fstream>
#include <ostream>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

/////////////////////////////////////////////

Device::Device()
{
	m_ref = 0;
	m_dev = 0;
	m_opened = false;
}

Device::Device(const std::string& sdev)
{
	m_ref = 0;
	m_dev = 0;
	m_name = sdev;
}


Device::~Device()
{
	close();
}

void Device::addRef()
{
	m_ref++;
}

bool Device::open()
{
	if(m_dev){
		addRef();
		return true;
	}
	m_dev = ::open(m_name.c_str(), O_RDWR);

	if(m_dev != -1){
		addRef();
		std::cout << "device '" << m_name << "' open. ref="<< m_ref << " dev=" << m_dev <<  std::endl;
		m_opened = true;
		return true;
	}
	m_dev = 0;
	m_opened = false;

	return false;
}

void Device::close()
{
	if(!m_dev)
		return;

	m_ref--;
	std::cout << "close. ref=" << m_ref << std::endl;
	if(m_ref <= 0){
		::close(m_dev);
		std::cout << "device '" << m_name << "' closed" << std::endl;
		m_ref = 0;
		m_dev = 0;
		m_opened = false;
	}
}

int Device::dev() const
{
	return m_dev;
}

int Device::set_addr(unsigned char addr)
{
	return ioctl(m_dev, I2C_SLAVE, addr);
}

int Device::set_param(int param, int value)
{
	return ioctl(m_dev, param, value);
}

int Device::write(unsigned char addr, unsigned char *data, int len)
{
	if(!m_dev || !data || !len)
		return -1;
	m_write_data.resize(1 + len);

	m_write_data[0] = addr;
	if(len){
		std::copy(data, data + len, &m_write_data[1]);
	}

	return ::write(m_dev, &m_write_data.front(), m_write_data.size());
}

int Device::read(unsigned char addr, unsigned char *data, int len)
{
	if(!m_dev || !data || !len)
		return -1;
	int res = ::write(m_dev, &addr, 1);
	if(res >= 0)
		res = ::read(m_dev, data, len);
	return res;
}

bool Device::is_opened() const
{
	return m_opened;
}

std::string Device::name() const
{
	return m_name;
}

/////////////////////////////////////////////

I2CInstance I2CInstance::m_instance;

Device *I2CInstance::open(const std::string& sdev)
{
	if(m_devices.find(sdev) != m_devices.end()){
		return &m_devices[sdev];
	}
	m_devices[sdev] = Device(sdev);
	if(m_devices[sdev].open()){
		return &m_devices[sdev];
	}else{
		return 0;
	}
}

void I2CInstance::close(const std::string &sdev)
{
	if(m_devices.find(sdev) != m_devices.end()){
		m_devices[sdev].close();
	}
}

I2CInstance &I2CInstance::instance()
{
	return m_instance;
}

/////////////////////////////////////////////

I2CDevice::I2CDevice(std::string device/* = "/dev/i2c-1"*/, u_char dev/* = UNKNOWN*/, bool tenbits/* = false*/)
	: m_i2cdev(0)
	, m_addr(0)
	, m_device("")
{
	m_device = device;
	open(dev, tenbits);
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

bool I2CDevice::is_opened() const
{
	return m_i2cdev && m_i2cdev->dev() > 0;
}

bool I2CDevice::open(u_char addr, bool tenbits)
{
	if(addr == UNKNOWN || m_addr == addr)
		return false;

	m_i2cdev = I2CInstance::instance().open(m_device);

	if(!m_i2cdev)
		return false;

	m_addr = addr;

	u_char res = 0;
	res = m_i2cdev->set_param(I2C_TENBIT, tenbits);

	return m_i2cdev->is_opened();
}

void I2CDevice::close()
{
	I2CInstance::instance().close(m_device);
	m_i2cdev = 0;
}

int I2CDevice::write(u_char addr, u_char data[], int count)
{
	if(!m_i2cdev)
		return -1;

	int res = 0;

	res = m_i2cdev->set_addr(m_addr);

	res = m_i2cdev->write(addr, data, count);
	//std::cout << "write. addr=" << (int)m_addr << " res=" << res << std::endl;
	return res;
}

int I2CDevice::read(u_char addr, u_char data[], int count)
{
	if(!m_i2cdev)
		return -1;
	int res = 0;
	res = m_i2cdev->set_addr(m_addr);

	res = m_i2cdev->read(addr, data, count);
	//std::cout << "read. addr=" << (int)m_addr << " res=" << res << std::endl;
	return res;
}
