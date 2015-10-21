#include "gpiothread.h"

#include "unistd.h"
#include <QDebug>
#include <QTime>
#include <QThread>

#include "senddata.h"

const uint default_freq				= 50;
const uint usec						= 1000000;
const uint msec						= 1000;
const float usec_in_msec			= 1000;
const float nsec_in_usec			= 1000;

const float minus180_in_msec		= 1.;
const float plus180_in_msec			= 2.;
const uint default_timework_msec	= 5;
//void delay(int ms)
//{
//	usleep(ms * 1000);
//}

////////////////////////////////////////////////////////

float crop_angle(float angle)
{
	return fmod(angle, 180);
}

float cnv_to_impulse(float angle, float minus180, float plus180)
{
	angle = crop_angle(angle);
	return minus180 + (plus180 - minus180) * angle / 360.;
}

////////////////////////////////////////////////////////

Pin::Pin()
{
	pin = 0;
	impulse = 0;
	period = 0;
	cur_case = ONE;
	last_time = 0;
	opened = false;
	desired_impulse = 0;
	speed_of_change = 0;
	timework_ms = default_timework_msec * usec_in_msec;
	time_start_ms = 0;

	time_watch.start();
}

Pin::Pin(uint impulse, uint period, int pin)
{
	this->cur_case = ONE;
	this->last_time = 0;
	this->opened = false;
	this->desired_impulse = 0;
	this->speed_of_change = 0;
	this->timework_ms = default_timework_msec * usec_in_msec;
	this->time_start_ms = 0;

	this->pin = pin;
	this->impulse = impulse;
	this->period = period;

	time_watch.start();
}

qint64 Pin::case_delay()
{
	switch (cur_case) {
		case ONE:
			return impulse;
			break;
		case ZERO:
		default:
			return period - impulse;
			break;
	}
}

int Pin::cur_value(){
	switch (cur_case) {
		case ONE:
			return 1;
		case ZERO:
		default:
			return 0;
	}
}

void Pin::swap_case()
{
	switch (cur_case) {
		case ONE:
			cur_case = ZERO;
			break;
		case ZERO:
			cur_case = ONE;
			break;
	}
}

bool Pin::set_new_impulse(uint impulse)
{
	if(impulse > period)
		return false;
	this->impulse = impulse;
	return true;
}

float Pin::current_angle() const
{
	float angle = -180. + 360. * (impulse / usec_in_msec - minus180_in_msec);
	return angle;
}

float Pin::desired_angle() const
{
	float angle = -180. + 360. * (desired_impulse / usec_in_msec - minus180_in_msec);
	return angle;
}

void Pin::set_desired_impulse(float angle)
{
	desired_impulse = cnv_to_impulse(angle, minus180_in_msec * usec_in_msec, plus180_in_msec * usec_in_msec);
}

void Pin::calculate_params()
{
	if(!speed_of_change){
		impulse = desired_impulse;
		return;
	}
	float a1 = current_angle();
	float a2 = desired_angle();
	float t = (a2 - a1) / speed_of_change;

	past_impulse = impulse;
	past_time = time_watch.elapsed();
	delta_time = t * msec;
}

int Pin::get_impulse_from_time()
{
	int elapsed = time_watch.elapsed() - past_time;
	return past_impulse + (elapsed * (desired_impulse - past_impulse)) / delta_time;
}

bool Pin::is_desired_made()
{
	return impulse == desired_impulse || speed_of_change == 0 || time_watch.elapsed() >= past_time + delta_time;
}

////////////////////////////////////////////////////////

GPIOThread::GPIOThread()
	: QRunnable()
	, m_frequency(default_freq)
	, m_done(false)
	, m_opened(false)
	, m_max_delay(0)
	, m_sender(0)
{
	m_global_time.start();
}

GPIOThread::~GPIOThread()
{
	foreach (Pin pin, m_pins_freq) {
		m_gpio.close_pin(pin.pin);
	}
	m_pins_freq.clear();
}

bool GPIOThread::open_pin(int pin, float impulse, float freq_meandr)
{
	float period = 1000. / freq_meandr;
	if(period < impulse){
		qDebug() << "the too big frequency for the current impulse length";
		return false;
	}

	if(pin < 0 || !period)
		return false;
	m_gpio.close_pin(pin);


	m_pins_freq[pin] = Pin(impulse * usec_in_msec, period * usec_in_msec, pin);

	uint smax = 0;
	foreach (Pin pin, m_pins_freq) {
		smax = qMax(smax, pin.period);
	}
	m_max_delay = smax;
	qDebug() << "open pin: " << pin << m_pins_freq[pin].impulse << m_pins_freq[pin].period << smax;

	if(m_gpio.open_pin(pin)){
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

void GPIOThread::set_sender(send_data::SendData *sender)
{
	m_sender = sender;
}

void GPIOThread::run()
{
	QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);

	while(!m_done){
		if(m_opened){
			work();
		}else{
			usleep(100 * usec_in_msec);

			check_controls();
		}
	}
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

		bool exists_opened_pin = true;

		for(QMap< int, Pin >::iterator it = m_pins_freq.begin(); it != m_pins_freq.end(); it++){

			Pin& pin = it.value();

			exists_opened_pin &= pin.opened;

			if(!pin.opened)
				continue;

			if(usec_elapsed() >= pin.last_time + pin.case_delay()){

				/// counter for test freq
				if(pin.cur_case == Pin::ONE){
					cnt++;
				}

				//qint64 sb = usec_elapsed() - pin.last_time;
				//qDebug() << QString("[%1]: %2, time=%3").arg(pin.pin).arg(pin.cur_value()).arg(sb);
				pin.swap_case();
				m_gpio.write_to_pin(pin.pin, pin.cur_value());
				pin.last_time = usec_elapsed();
			}

			if(pin.is_desired_made()){
				if(m_global_time.elapsed() > pin.time_start_ms + pin.timework_ms){
					pin.opened = false;
				}
			}else{
				pin.impulse = pin.get_impulse_from_time();
			}
		}

		if(!exists_opened_pin){
			m_opened = false;
			break;
		}

		/// for reduction cpu load. jitter
		usleep(1);

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

void GPIOThread::check_controls()
{
	if(!m_sender)
		return;

	const sc::StructServo& servo = m_sender->control_params().servo_ctrl;

	if(!servo.flag_start || !servo.pin || servo.angle == m_pins_freq[servo.pin].current_angle() == servo.angle)
		return;

	int pin = servo.pin;

	if(m_opened && m_pins_freq[pin].opened && m_pins_freq[pin].desired_angle() == servo.angle &&
			m_pins_freq[pin].speed_of_change == servo.speed_of_change)
		return;

	m_opened = true;

	m_pins_freq[pin].opened = true;
	m_pins_freq[pin].set_desired_impulse(servo.angle);

	m_pins_freq[pin].speed_of_change = servo.speed_of_change;

	if(servo.speed_of_change == 0){
		m_pins_freq[pin].set_desired_impulse(servo.angle);
	}

	m_pins_freq[pin].calculate_params();

	if(servo.freq_meandr){
		float period = 1000. / servo.freq_meandr * usec_in_msec;
		m_pins_freq[pin].period = period;
	}
	if(servo.timework_ms){
		m_pins_freq[pin].timework_ms = servo.timework_ms;
	}
	m_pins_freq[pin].time_start_ms = m_global_time.elapsed();
}

