#ifndef I2CDEVICE_H
#define I2CDEVICE_H

#include <iostream>
#include <string>
#include <vector>

class I2CDevice
{
public:
	enum{
		UNKNOWN = 0xff
	};
	I2CDevice(std::string device = "/dev/i2c-1", u_char dev = UNKNOWN, bool slave = true, bool tenbits = false);
	~I2CDevice();

	u_char addr() const;

	std::string device() const;
	void set_device(std::string value);

	bool is_opened() const;

	bool open(u_char addr, bool slave = true, bool tenbits = false);
	void close();

	int write(u_char addr, u_char data[], int count);
	int read(u_char addr, u_char data[], int count);

private:
	int m_i2cdev;
	u_char m_addr;
	std::string m_device;
	std::vector< u_char > m_write_data;
};

#endif // I2CDEVICE_H
