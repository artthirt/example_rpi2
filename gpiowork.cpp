#include "gpiowork.h"
#include <sstream>
#include <ostream>
#include <unistd.h>
#include <fstream>
#include <iostream>

using namespace gpio;
using namespace std;

GPIOWork::GPIOWork()
{

}

GPIOWork::~GPIOWork()
{
	for(map<int, string>::iterator it = m_open_pins.begin(); it != m_open_pins.end(); it++){
		close_pin(it->first);
	}
}

bool GPIOWork::open_pin(int pin)
{
	if(contains(pin)){
		return true;
	}

	ofstream file(gpio_export.c_str());

	if(!file.is_open()){
		cout << "pin " << pin << " not initialize" << endl;
		return false;
	}

	file << pin;

	file.close();

	m_open_pins[pin] = path_pin(pin);

	return true;
}

bool GPIOWork::close_pin(int pin)
{
	if(!contains(pin)){
		return true;
	}

	ofstream file(gpio_unexport.c_str());

	if(!file.is_open()){
		cout << "pin " << pin << " not closed" << endl;
		return false;
	}

	file << pin;

	file.close();

	m_open_pins.erase(pin);

	return true;
}

bool GPIOWork::write_to_file(const string &sfile, const string &value)
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

bool GPIOWork::write_to_gpio_custom(int pin, const string &key, const string &value)
{
	if(!contains(pin))
		return false;

	if(!write_to_file(pin_key_file(pin, key), value)){
		cout << "not write to gpio pin " << pin << " for " << key << endl;
		return false;
	}

	//cout << "write to gpio pin " << pin << " for key " << key << " value " << value << endl;

	return true;
}

bool GPIOWork::write_to_gpio_custom(int pin, const string &key, const int value)
{
	stringstream ss;
	ss << value;
	return write_to_gpio_custom(pin, key, ss.str());
}

bool GPIOWork::write_to_pin(int pin, bool value)
{
	return write_to_gpio_custom(pin, "value", value);
}

int GPIOWork::read_from_pin(int pin)
{

	string path = path_pin(pin) + "/value";

	ifstream file(path);

	if(!file.is_open()){
		cout <<  "not readed pin" << endl;
		return -1;
	}

	int value;

	file >> value;

	file.close();

	return value;
}

bool GPIOWork::set_direction_gpio(int pin, DIRECTION dir)
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

bool GPIOWork::contains(int key)
{
	return m_open_pins.find(key) != m_open_pins.end();
}

string GPIOWork::path_pin(int pin) const
{
	stringstream ss;

	ss << "/sys/class/gpio/gpio" << pin;
	return ss.str();
}

string GPIOWork::pin_key_file(int pin, const string &key)
{
	if(!contains(pin))
		return "";
	return  path_pin(pin) + "/" + key;

}
