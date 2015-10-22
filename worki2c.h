#ifndef WORKI2C_H
#define WORKI2C_H

#include <QObject>
#include <QTimer>
#include <QMutex>

#include "struct_controls.h"
#include "i2cdevice.h"

namespace send_data{
class SendData;
}

class WorkI2C : public QObject
{
	Q_OBJECT
public:
	explicit WorkI2C(QObject *parent = 0);
	virtual ~WorkI2C();

	void set_senser(send_data::SendData* sender);

	void init();

signals:

public slots:
	void on_timeout_mpu6050();
	void on_timeout_hmc5883();
	void on_timeout_bmp180();
	void on_timeout_bmp180_read();

private:
	QTimer m_timer_mpu6050;
	QTimer m_timer_hmc5883;
	QTimer m_timer_bmp180;
	QTimer m_timer_bmp180_read;
	QMutex m_mutex;

	send_data::SendData* m_sendData;

	I2CDevice m_i2cMpu6050;
	I2CDevice m_i2cHmc5883;
	I2CDevice m_i2cBmp180;

	uchar m_regA_hmc5883;

	bool m_pressure_received;

	void init_mpu6050();
	void init_hmc5883();
	void init_bmp180();
};

#endif // WORKI2C_H
