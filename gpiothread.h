#ifndef GPIOTHREAD_H
#define GPIOTHREAD_H

#include <QRunnable>
#include <QElapsedTimer>
#include <QTime>
#include <QMap>

#include "gpiowork.h"
#include "struct_controls.h"

namespace send_data{
class SendData;
}

//////////////////////////////////////////

struct Pin{
	enum CASE{
		ONE = 0,
		ZERO
	};

	Pin();
	Pin(uint impulse, uint period, int pin);

	qint64 case_delay();
	int cur_value();
	void swap_case();
	bool set_new_impulse(uint impulse);
	float current_angle() const;
	float desired_angle() const;
	void set_desired_impulse(float angle);
	void calculate_params();
	int get_impulse_from_time();
	bool is_desired_made();

	uint impulse;
	uint desired_impulse;
	uint past_impulse;
	uint period;
	uint speed_of_change;
	uint timework_ms;
	uint time_start_ms;
	int pin;
	bool opened;

	QTime time_watch;

	CASE cur_case;
	qint64 last_time;
	int past_time;
	int delta_time;
};

//////////////////////////////////////////

class GPIOThread : public QRunnable
{
public:
	GPIOThread();
	~GPIOThread();

	bool open_pin(int pin, float impulse, float freq_meandr);
	void close_pin(int pin);
	/**
	 * @brief set_delay
	 * @param delay - delay in ms
	 */
	void set_sender(send_data::SendData* sender);
protected:
	virtual void run();

private:
	QElapsedTimer m_time;
	QElapsedTimer m_global_time;
	uint m_frequency;			/// Hz
	bool m_done;
	gpio::GPIOWork m_gpio;
	bool m_opened;
	QMap< int, Pin > m_pins_freq;
	qint64 m_max_delay;
	qint64 m_last_time;

	float m_timework_ms;
	float m_freq_meandr;

	send_data::SendData* m_sender;

	sc::StructServo m_last_state;

	void work();
	qint64 usec_elapsed();

	void check_controls();
};

#endif // GPIOTHREAD_H
