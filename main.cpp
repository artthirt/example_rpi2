#include <iostream>
#include <time.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <ostream>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <pthread.h>

#include "senddata.h"

using namespace std;

void delay(int ms)
{
	usleep(ms * 1000);
}

////////////////////////////////////

namespace gpio{

string gpio_export("/sys/class/gpio/export");
string gpio_unexport("/sys/class/gpio/unexport");

bool open_pin(int pin)
{
	ofstream file(gpio_export.c_str());

	if(!file.is_open()){
		cout << "pin " << pin << " not initialize" << endl;
		return false;
	}

	file << pin;

	file.close();
	return true;
}

bool close_pin(int pin)
{
	ofstream file(gpio_unexport.c_str());

	if(!file.is_open()){
		cout << "pin " << pin << " not closed" << endl;
		return false;
	}

	file << pin;

	file.close();
	return true;
}

template< typename T >
bool write_to_file(string sfile, T value)
{
	stringstream ssv;

	ofstream file(sfile.c_str());

	if(!file.is_open()){
		return false;
	}

	ssv << value;

	file.write(ssv.str().c_str(), ssv.str().size());

	file.close();

	return true;

}

template < typename T >
bool write_to_gpio_custom(int pin, string key, T value)
{
	stringstream ss;

	ss << "/sys/class/gpio/gpio" << pin << "/" << key;

	if(!write_to_file(ss.str(), value)){
		cout << "not write to gpio pin " << pin << " for " << key << endl;
		return false;
	}

	cout << "write to gpio pin " << pin << " for key " << key << " value " << value << endl;

	return true;
}

bool write_to_pin(int pin, bool value)
{
	return write_to_gpio_custom(pin, "value", value);
}

int read_from_pin(int pin)
{
	stringstream ss;
	stringstream ssv;

	ss << "/sys/class/gpio/gpio" << pin << "/value";

	ifstream file(ss.str().c_str());

	if(!file.is_open()){
		cout <<  "not readed pin" << endl;
		return -1;
	}

	int value;

	file >> value;

	file.close();

	return value;
}

enum DIRECTION{
	D_IN = 0,
	D_OUT = 1
};

bool direction_gpio(int pin, DIRECTION dir)
{
	switch (dir) {
	case D_IN:
		return write_to_gpio_custom(pin, "direction", "in");
	case D_OUT:
		return write_to_gpio_custom(pin, "direction", "out");
	default:
		break;
	}
	return false;
}

}

int example_gpio()
{
	const int pin_fan = 14;

	cout << "Hello World!" << endl;

	if(!gpio::open_pin(pin_fan)){
		return 1;
	}

	if(!gpio::direction_gpio(pin_fan, gpio::D_OUT)){
		return 2;
	}

	if(!gpio::write_to_pin(pin_fan, 1)){
		return 3;
	}

	cout << "start delay" << endl;

//	delay(1000);
	//	for(;;){

	//	}

	bool ff = false;
	int one = 0, zero = 0;
	for(int i = 0; i < 10000; i++){
		int val = gpio::read_from_pin(pin_fan);
		val == 0? ++zero : ++one;
		cout << (double) one / max(1.0, (double)(zero + one)) << "|  v1=" << one << " v0="<< zero << endl;
		delay(1);
	}


	cout << "stop delay" << endl;

	if(!gpio::close_pin(pin_fan)){
		return 4;
	}
}


///////////////////////////

#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

int example_i2c()
{
	int gaddr = 0x68;
	int res = 0;
	int i2ch = open("/dev/i2c-1", O_RDWR);

	std::cout << "handle opened device " << i2ch << endl;

	send_data::SendData* m_sendThread = new send_data::SendData;
	m_sendThread->start();

	char txbuf[32] = {0};
	char rxbuf[32] = {0};

	u_char addr_begin = 0x3b;
	u_char addr_end = 0x49;

	std::vector< u_char > data_addr, data_out;

	std::cout << "begin\n";

	for(int i = addr_begin; i < addr_end; i++){
		data_addr.push_back(i);
	}
	data_out.resize(data_addr.size());

	if(!i2ch){
		return 1;
	}

	res = ioctl(i2ch, I2C_TENBIT, 0);
	std::cout << "result set tenbit " << res << endl;
	res = ioctl(i2ch, I2C_SLAVE, gaddr);
	std::cout << "result set slave bit " << res << endl;

	txbuf[0] = 0x6B;
	write(i2ch, txbuf, 2);

	read(i2ch, rxbuf, 1);
	std::cout << "after write 0: " << (u_int)rxbuf[0] << endl;

//	FILE *fcsv = fopen("/root/data.csv", "w");
//	std::cout << "data file result open: " << fcsv << endl;
	float data[7];

	const float del_gyro = 1.f / 3000.f;
	const float del_accel = 1.f / 20000.f;

	for(int i = 0; i < 1000; i++){
		txbuf[0] = 0x3b;
		res = write(i2ch, txbuf, 1);

		std::stringstream ss;
		for(int i = 0; i < 7/*data_out.size()*/; i++){
			short val = 0;
			txbuf[0] = 0x3b + 2 * i;
			res = write(i2ch, txbuf, 1);
			res = read(i2ch, rxbuf, 1);
			val = (rxbuf[0] << 8);

			txbuf[0] = 0x3b + 2 * i + 1;
			res = write(i2ch, txbuf, 1);
			res = read(i2ch, rxbuf, 1);
			val |= rxbuf[0];
			ss << (int)val << "; ";
			data[i] = val;
		}
		cout << ss.str() << endl;

		m_sendThread->push_data(Vector3f(data[4], data[5], data[6]) * del_gyro,
				Vector3f(data[0], data[1], data[2]) * del_accel,
				data[3] / 340.f * 36.53f);

//		std::string line = ss.str() + "\n";

//		res = fputs(line.c_str(), fcsv);
		//res = fwrite(line.c_str(), line.size(), fcsv);
		std::cout << "result write to file: " << res << endl;

		delay(100);
	}

	close(i2ch);

//	fclose(fcsv);

	std::cout << "end\n";

	return 0;

}

///////////////////////////

int main()
{
	return example_i2c();

	return 0;
}

