#ifndef WORKI2C_H
#define WORKI2C_H

#include <QObject>
#include <QTimer>

#include "senddata.h"
#include "struct_controls.h"
#include "i2cdevice.h"

class WorkI2C : public QObject
{
	Q_OBJECT
public:
	explicit WorkI2C(QObject *parent = 0);
	virtual ~WorkI2C();

	void init();

signals:

public slots:
	void on_timeout();

private:
	QTimer m_timer;
	send_data::SendData* m_sendData;

	I2CDevice m_i2cdev;

};

#endif // WORKI2C_H
