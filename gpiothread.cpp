#include "gpiothread.h"

#include "unistd.h"
#include <QDebug>
#include <QTime>
#include <QThread>

const uint default_freq = 50;
const uint usec = 1000000;
const float usec_in_msec = 1000;
const float nsec_in_usec = 1000;
//void delay(int ms)
//{
//	usleep(ms * 1000);
//}

GPIOThread::GPIOThread()
	: QRunnable()
	, m_delay(1 * usec_in_msec)
	, m_frequency(default_freq)
	, m_delay_for_freq(usec/default_freq)
	, m_done(false)
	, m_opened(false)
	, m_max_delay(0)
{

}

GPIOThread::~GPIOThread()
{
	foreach (Pin pin, m_pins_freq) {
		m_gpio.close_pin(pin.pin);
	}
	m_pins_freq.clear();
}

bool GPIOThread::open_pin(int pin, float delay_one, float delay_zero)
{
	if(pin < 0 || !delay_one)
		return false;
	m_gpio.close_pin(pin);
	m_pins_freq[pin] = Pin(delay_one * usec_in_msec, delay_zero * usec_in_msec, pin);

	uint smax = 0;
	foreach (Pin pin, m_pins_freq) {
		uint val = pin.delay_one + pin.delay_zero;
		smax = qMax(smax, val);
	}
	m_max_delay = smax;
	qDebug() << "open pin: " << pin << m_pins_freq[pin].delay_one << m_pins_freq[pin].delay_zero << smax;

	if(m_gpio.open_pin(pin)){
		m_opened = true;
		m_gpio.set_direction_gpio(pin, gpio::GPIOWork::D_OUT);
	}
}

void GPIOThread::close_pin(int pin)
{
	if(!m_pins_freq.contains(pin))
		return;
	m_opened = false;
	m_gpio.close_pin(pin);
	m_pins_freq.remove(pin);
}

void GPIOThread::set_delay(float delay)
{
	m_delay = delay * usec_in_msec;
}

void GPIOThread::set_frequency(uint freq)
{
	if(!freq)
		freq = 1;

	m_frequency = freq;
	m_delay_for_freq = usec / freq;
	if(!m_delay_for_freq)
		m_delay_for_freq = 1;
}

void GPIOThread::run()
{
	QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);

	while(!m_done){
		if(!m_opened){
			work();
		}else{
			usleep(100 * usec_in_msec);
		}
	}
}

void GPIOThread::one_cycle()
{

}

void GPIOThread::work()
{
	if(!m_max_delay){
		m_opened = false;
		return;
	}

	m_time.restart();
	QTime tcnt;
	tcnt.start();
	uint cnt = 0;
	while(!m_done && m_opened){
		for(QMap< int, Pin >::iterator it = m_pins_freq.begin(); it != m_pins_freq.end(); it++){

			Pin& pin = it.value();

			if(usec_elapsed() >= pin.last_time + pin.case_delay()){

				if(pin.cur_case == Pin::ONE){
					cnt++;
				}

				qint64 sb = usec_elapsed() - pin.last_time;
				//qDebug() << QString("[%1]: %2, time=%3").arg(pin.pin).arg(pin.cur_value()).arg(sb);
				pin.swap_case();
				m_gpio.write_to_pin(pin.pin, pin.cur_value());
				pin.last_time = usec_elapsed();
			}
		}
		//usleep(1);

		if(tcnt.elapsed() > 1000){
			tcnt.restart();
			qDebug() << "count one:" << cnt;
			cnt = 0;
		}
	}
}

qint64 GPIOThread::usec_elapsed()
{
	return m_time.nsecsElapsed() / nsec_in_usec;
}

