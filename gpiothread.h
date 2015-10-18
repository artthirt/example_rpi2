#ifndef GPIOTHREAD_H
#define GPIOTHREAD_H

#include <QRunnable>
#include <QElapsedTimer>
#include <QMap>

#include "gpiowork.h"

class GPIOThread : public QRunnable
{
public:
	GPIOThread();
	~GPIOThread();

	bool open_pin(int pin, float delay_one, float delay_zero);
	void close_pin(int pin);
	/**
	 * @brief set_delay
	 * @param delay - delay in ms
	 */
	void set_delay(float delay);
	void set_frequency(uint freq);
protected:
	virtual void run();

private:
	struct Pin{
		enum CASE{
			ONE = 0,
			ZERO
		};

		Pin(){
			pin = 0;
			delay_one = 0;
			delay_zero = 0;
			cur_case = ONE;
			last_time = 0;
		}
		Pin(uint delay_one, uint delay_zero, int pin){
			this->cur_case = ONE;
			this->last_time = 0;
			this->pin = pin;
			this->delay_one = delay_one;
			this->delay_zero = delay_zero;
		}
		uint delay_one;
		uint delay_zero;
		int pin;

		CASE cur_case;
		qint64 last_time;

		qint64 case_delay(){
			switch (cur_case) {
				case ONE:
					return delay_one;
					break;
				case ZERO:
				default:
					return delay_zero;
					break;
			}
		}
		int cur_value(){
			switch (cur_case) {
				case ONE:
					return 1;
				case ZERO:
				default:
					return 0;
			}
		}

		void swap_case(){
			switch (cur_case) {
				case ONE:
					cur_case = ZERO;
					break;
				case ZERO:
					cur_case = ONE;
					break;
			}
		}
	};

	uint m_delay;				/// us
	QElapsedTimer m_time;
	uint m_frequency;			/// Hz
	uint m_delay_for_freq;
	bool m_done;
	gpio::GPIOWork m_gpio;
	bool m_opened;
	QMap< int, Pin > m_pins_freq;
	qint64 m_max_delay;
	qint64 m_last_time;

	void one_cycle();
	void work();
	qint64 usec_elapsed();
};

#endif // GPIOTHREAD_H
