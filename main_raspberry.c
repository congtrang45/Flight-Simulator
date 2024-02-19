#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>   
#include <math.h>   

#define Sample_rate 25
#define Config      26
#define Gyro_config 27
#define Acc_config  28 
#define Interrupt   56
#define PWR_managment 107
#define Acc_X       59
#define Acc_Y       61
#define Acc_Z       63
#define Gyro_X		67
#define Gyro_Y		69
#define Gyro_Z		71
#define center_bt   12

int mpu, center;
float data;
float deltaT = 0.1; // Update time
volatile float Gx, Gy, prePitch, preRoll, pitch, roll;
void Init_6050(void){
        // register 25->28,56,107
        // Sample Rate: 500hz
        wiringPiI2CWriteReg8(mpu, Sample_rate, 15);
        //Not use the external Frame Synchronization, turn of DLPF
        wiringPiI2CWriteReg8(mpu, Config, 0);
        //gyro FS: +-500 o/s
        wiringPiI2CWriteReg8(mpu, Gyro_config, 0x08);
        //accel FS: +- 8g
        wiringPiI2CWriteReg8(mpu, Acc_config, 0x10);
        //Open interrupt of Data ready
        wiringPiI2CWriteReg8(mpu, Interrupt, 1);
        //Select Clock Source Gyro X
        wiringPiI2CWriteReg8(mpu, PWR_managment, 0x01);
}

// Read sensor Function with int16_t data type
int16_t read_sensor(unsigned char sensor)
{
    int16_t high,low,data;
    high = wiringPiI2CReadReg8(mpu,sensor);
    low = wiringPiI2CReadReg8(mpu,sensor+1);
    data = (high<<8) | low;
    return data;
}
//Update Gyroscope value Function
float UpdateGyro(unsigned char G)
{
	delay(deltaT*1000);
	switch (G)
	{
		case 'X':
			Gx = (float)read_sensor(Gyro_X)/65.5;
			break;  
		default:
			Gy = (float)read_sensor(Gyro_Y)/65.5;
			break;
	}
}
//*****************************************************************//
/*Pitch = Integral from 0 to t of Gyroscope Y
Because when the sensor moves, the value of Gyroscope Y
is a discrete value, so to calculate the fraction, calculate
the area under the graph by summing the areas of the trapezoid.*/
//****************************************************************//
// Function to calculate new pitch angle based on previous pitch and gyro value
float CalculatePitch(float prePitch, float preGyro, float curGyro, float deltaT)
{
	return prePitch + 0.5*(preGyro + curGyro)*deltaT;
}
// Function to calculate new roll angle based on previous roll and gyro value
float CalculateRoll(float preRoll, float preGyro, float curGyro, float deltaT)
{
	return preRoll + 0.5*(preGyro + curGyro)*deltaT;
}

int main()
{
    //Setup GPIO Physical
    wiringPiSetupPhys();
	//Setup I2C Comunication
    mpu = wiringPiI2CSetup(0x68);
    //Set mode for MPU6050
    Init_6050(); 
    //Set mode for GPIO
    pinMode(center_bt, INPUT);
    //Read measured values
    while (1)
    {
        //Get value of sensor when the sensor is stationary
        // Accel value
        float Ax = (float)read_sensor(Acc_X)/4096.0;
        float Ay = (float)read_sensor(Acc_Y)/4096.0;
        float Az = (float)read_sensor(Acc_Z)/4096.0;
		//Previous Pitch and Roll value
        prePitch = atan2(-Ax,sqrt(pow(Ay,2)+pow(Az,2)))*180/M_PI;
        preRoll = atan2(Ay,sqrt(pow(Ax,2)+pow(Az,2)))*180/M_PI;
        //Get value of previous Gyroscope value
        float preGx = (float)read_sensor(Gyro_X)/65.5;
		float preGy = (float)read_sensor(Gyro_Y)/65.5;
		// Calculate Pitch and Roll when the sensor is move
		pitch = CalculatePitch(prePitch, preGx, UpdateGyro('Y'), deltaT);
		roll = CalculateRoll(preRoll, preGy, UpdateGyro('X'), deltaT);
        // Handle the center button
        if (digitalRead(center_bt) == 1) {
            center = 1;
        }
        else {
            center = 0;
        }
        // Handle the UART
        int fd;
        if((fd = serialOpen ("/dev/ttyS0", 115200)) < 0 ){
            fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno));
            return 1;
		}

        
        char cmd[200];
		sprintf(cmd,"%.2f %.2f %d\n",pitch, roll, center);
        printf("%s\n", cmd);
        serialPrintf(fd, cmd);

        // Close serial port before exiting
        serialClose(fd);
	}   
    return 0;
}
