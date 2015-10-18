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

#include <QCoreApplication>

#include <QThreadPool>
#include <gpiothread.h>

#include "worki2c.h"
#include "gpiowork.h"

using namespace std;

void delay(int ms)
{
	usleep(ms * 1000);
}

////////////////////////////////////

namespace gpio{

}

int example_gpio()
{
	const int pin_fan = 14;

	gpio::GPIOWork work;

	cout << "Hello World!" << endl;

	if(!work.open_pin(pin_fan)){
		return 1;
	}

	if(!work.set_direction_gpio(pin_fan, gpio::GPIOWork::D_OUT)){
		return 2;
	}

	if(!work.write_to_pin(pin_fan, 1)){
		return 3;
	}

	cout << "start delay" << endl;

//	delay(1000);
	//	for(;;){

	//	}

	bool ff = false;
	int one = 0, zero = 0;
	for(int i = 0; i < 10000; i++){
		int val = work.read_from_pin(pin_fan);
		val == 0? ++zero : ++one;
		cout << (double) one / max(1.0, (double)(zero + one)) << "|  v1=" << one << " v0="<< zero << endl;
		delay(1);
	}


	cout << "stop delay" << endl;

	if(!work.close_pin(pin_fan)){
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

//	send_data::SendData* m_sendThread = new send_data::SendData;
//	m_sendThread->start();

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

//	const float del_gyro = 1.f / 3000.f;
//	const float del_accel = 1.f / 20000.f;

	cout << "data begin\n";

	for(;;){
		txbuf[0] = 0x3b;
		res = write(i2ch, txbuf, 1);

		//std::stringstream ss;
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
			//ss << (int)val << "; ";
			data[i] = val;
		}
		//cout << ss.str() << endl;

//		m_sendThread->push_data(Vector3f(data[4], data[5], data[6]) * del_gyro,
//				Vector3f(data[0], data[1], data[2]) * del_accel,
//				data[3] / 340.f * 36.53f);

//		std::string line = ss.str() + "\n";

//		res = fputs(line.c_str(), fcsv);
		//res = fwrite(line.c_str(), line.size(), fcsv);
		//std::cout << "result read from i2c: " << res << endl;

		delay(100);
	}

	close(i2ch);

//	fclose(fcsv);

	std::cout << "data end\n";

//	delete m_sendThread;

	return 0;

}

///////////////////////////

int main(int argc, char *argv[])
{
//	return example_i2c();
	QCoreApplication app(argc, argv);

	WorkI2C work;

	GPIOThread *thread = new GPIOThread;

	QThreadPool::globalInstance()->start(thread);


	QString sval = "1.5";
	QString sfreq = "50";
	if(argc > 1){
		sval= argv[1];
	}
	if(argc > 2){
		sfreq = argv[2];
	}
	std::cout << "impulse = " << sval.toStdString() << "; freq = " << sfreq.toStdString() << "\n";

	float freq = sfreq.toFloat();
	if(!freq)
		freq = 50;

	float delay_one = sval.toFloat();
	float delay_hz = 1000./freq;
	thread->open_pin(15, delay_one, delay_hz - delay_one);

	app.exec();

	return 0;
}

