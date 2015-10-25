#ifndef I2CDEVICE_H
#define I2CDEVICE_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

class Device{
public:
	Device();
	Device(const std::string& sdev);
	~Device();
	void addRef();
	bool open();
	void close();
	int dev() const;
	int set_addr(unsigned char addr);
	int set_param(int param, int value);
	int write(unsigned char addr, unsigned char* data, int len);
	int read(unsigned char addr, unsigned char* data, int len);
	bool is_opened() const;
	std::string name() const;
private:
	int m_ref;
	std::string m_name;
	std::vector< u_char > m_write_data;
	bool m_opened;

	mutable int m_dev;

};


class I2CInstance{
public:
	Device* open(const std::string& sdev);
	void close(const std::string& sdev);

	static I2CInstance& instance();
private:
	static I2CInstance m_instance;

	std::map< std::string, Device > m_devices;
};

class I2CDevice
{
public:
	enum{
		UNKNOWN = 0xff
	};
	I2CDevice(std::string device = "/dev/i2c-1", u_char dev = UNKNOWN, bool tenbits = false);
	~I2CDevice();

	u_char addr() const;

	std::string device() const;

	bool is_opened() const;

	bool open(u_char addr, bool tenbits = false);
	void close();

	int write(u_char addr, u_char data[], int count);
	int read(u_char addr, u_char data[], int count);

private:
	Device* m_i2cdev;
	u_char m_addr;
	std::string m_device;
};

#endif // I2CDEVICE_H
