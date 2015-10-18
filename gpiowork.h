#ifndef GPIOWORK_H
#define GPIOWORK_H

#include <string>
#include <vector>
#include <map>

namespace gpio{

const std::string gpio_export("/sys/class/gpio/export");
const std::string gpio_unexport("/sys/class/gpio/unexport");

class GPIOWork
{
public:
	enum DIRECTION{
		D_IN = 0,
		D_OUT = 1
	};

	GPIOWork();
	~GPIOWork();

	bool open_pin(int pin);

	bool close_pin(int pin);

	bool write_to_file(const std::string& sfile, const std::string& value);

	bool write_to_gpio_custom(int pin, const std::string& key, const std::string& value);
	bool write_to_gpio_custom(int pin, const std::string& key, const int value);

	bool write_to_pin(int pin, bool value);

	int read_from_pin(int pin);

	bool set_direction_gpio(int pin, DIRECTION dir);

private:
	std::map< int, std::string > m_open_pins;

	bool contains(int key);
	std::string path_pin(int pin) const;
	std::string pin_key_file(int pin, const std::string& key);
};

}

#endif // GPIOWORK_H
