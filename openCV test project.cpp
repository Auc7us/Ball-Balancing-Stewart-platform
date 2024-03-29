#include "stdafx.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include "SerialPort.h"

using namespace cv;
using namespace std;
int offset[2] = { 0,0 };
float ratio[2] = { 0.7,1 };
vector <float> setPointX;
vector <float> setPointY;
int currentStep;

//arduino_output reader
char output[MAX_DATA_LENGTH];

//change port number accordingly
char port_name[] = "\\\\.\\COM7";

//data_in string
char incomingData[MAX_DATA_LENGTH];

void load(string fileName) 
{
	fileName += ".AbNAr";
	ifstream file;
	file.open(fileName.c_str(), ios::in|ios::binary);
	setPointX.resize(0);
	setPointY.resize(0);
	currentStep = 0;
	int size;
	file.read((char *)&size, sizeof(size));
	for(int i = 0 ; i < size/2 ; ++i) {
		float data;
		file.read((char*)&data, sizeof(data));
		setPointX.push_back(data);
		file.read((char*)&data, sizeof(data));
		setPointY.push_back(data);
	}
}

bool getOVal(char &x, char &y, int height , int width , float pointX , float pointY , int cValX , int cValY) {
	cValX -= width / 2;
	cValX -= offset[0];
	cValY -= height / 2;
	cValY -= offset[1];
	pointX *= width * ratio[0] / 2;
	pointY *= height * ratio[1] / 2;
	int error[2];
	error[0] = cValX - pointX;
	error[1] = cValY - pointY;
	if ((error[0] > width * ratio[0] / 2) || (error[0] < -width * ratio[0] / 2))goto end;
	if ((error[1] > height * ratio[1] / 2) || (error[1] < -height * ratio[1] / 2))goto end;
	x = int(float(error[0])*256 / (width*ratio[0]));
	y = int(float(error[1])*256 / (height*ratio[1]));
	return 0;
	end:;
	return 1;
}


int main()
{
	start:
	{
		int temp;
		SerialPort arduino(port_name);
		if (arduino.isConnected()) cout << "Connection Established" << endl;
		else cout << "ERROR, check port name", cin >> temp;

		VideoCapture capWebcam(0);
		
		Mat imgOriginal;
		Mat imgHSV;
		Mat imgThresh;
		Mat imgThresh2;

		std::vector<Vec3f> v3fCircles;

		char checkKey = 0;
		load("sqrure");
		char dataX = 0;
		char dataY = 0;
		while (checkKey != 27 && capWebcam.isOpened()) {
			capWebcam.set(CAP_PROP_BRIGHTNESS, 500);
			capWebcam.read(imgOriginal);
			cvtColor(imgOriginal, imgHSV, CV_BGR2HSV);
			inRange(imgHSV, Scalar(0, 128, 200), Scalar(18, 255, 255), imgThresh);
			inRange(imgHSV, Scalar(165, 128, 200), Scalar(180, 255, 255), imgThresh2);
			imgThresh = imgThresh + imgThresh2;
			GaussianBlur(imgThresh, imgThresh, cv::Size(3, 3), 0);
			Mat structuringElement = getStructuringElement(MORPH_RECT, Size(3, 3));
			dilate(imgThresh, imgThresh, structuringElement);
			erode(imgThresh, imgThresh, structuringElement);
			HoughCircles(imgThresh, v3fCircles, CV_HOUGH_GRADIENT, 2, imgThresh.rows / 4, 100, 50, 10, 70);
			}

			int  X = 0, Y = 0;
			cv::Size Sz = imgThresh.size();
			for (int i = 0; i < v3fCircles.size(); ++i) {
				X = v3fCircles[i][0];
				Y = v3fCircles[i][1];
				if (!getOVal(dataX, dataY, Sz.height, Sz.width, 0, 0, v3fCircles[i][0], v3fCircles[i][1])) {
					circle(imgOriginal, Point((int)v3fCircles[i][0], (int)v3fCircles[i][1]), 3, Scalar(0, 255, 0), CV_FILLED);
					circle(imgOriginal, Point((int)v3fCircles[i][0], (int)v3fCircles[i][1]), (int)v3fCircles[i][2], Scalar(255, 0, 0), 4);
					break;
				}
				
			}
			//draw all the points
			for (int i = 0; i < setPointX.size(); ++i) {
				int displayX, displayY;
				displayX = setPointX[i] * Sz.width*ratio[0] / 2 + Sz.width / 2 + offset[0];
				displayY = setPointY[i] * Sz.height*ratio[1] / 2 + Sz.height / 2 + offset[1];
				circle(imgOriginal, Point((int)displayX, (int)displayY), 2, Scalar(0, 100, 255), CV_FILLED);
			}
			
			if (currentStep < setPointX.size()) {
				int displayX, displayY;
				displayX = setPointX[currentStep] * Sz.width*ratio[0] / 2 + Sz.width / 2 + offset[0];
				displayY = setPointY[currentStep] * Sz.height*ratio[1] / 2 + Sz.height / 2 + offset[1];
				circle(imgOriginal, Point((int)displayX, (int)displayY), 3, Scalar(0, 255, 255), CV_FILLED);
			}
			else {
				currentStep = 0;
			}
			

			arduino.writeSerialPort(&dataX, 1/*MAX_DATA_LENGTH*/);
			arduino.writeSerialPort(&dataY, 1/*MAX_DATA_LENGTH*/);
			line(imgOriginal, Point(0, (Sz.height / 2) + offset[1] - Sz.height*ratio[1] / 2), Point(Sz.width, (Sz.height / 2) + offset[1] - Sz.height*ratio[1] / 2), Scalar(0, 0, 255));
			line(imgOriginal, Point(0, (Sz.height / 2) + offset[1] + Sz.height*ratio[1] / 2), Point(Sz.width, (Sz.height / 2) + offset[1] + Sz.height*ratio[1] / 2), Scalar(0, 0, 255));
			line(imgOriginal, Point((Sz.width / 2) + offset[0] - Sz.width*ratio[0] / 2, 0), Point((Sz.width / 2) + offset[0] - Sz.width*ratio[0] / 2, Sz.height), Scalar(255, 100, 0));
			line(imgOriginal, Point((Sz.width / 2) + offset[0] + Sz.width*ratio[0] / 2, 0), Point((Sz.width / 2) + offset[0] + Sz.width*ratio[0] / 2, Sz.height), Scalar(255, 100, 0));
			

			namedWindow("imgOriginal", CV_WINDOW_AUTOSIZE);
			namedWindow("imgThresh", CV_WINDOW_AUTOSIZE);
			imshow("imgOriginal", imgOriginal);
			imshow("imgThresh", imgThresh);
			char output;
			arduino.readSerialPort(&output, 1/*MAX_DATA_LENGTH*/);
			arduino.readSerialPort(&output, 1/*MAX_DATA_LENGTH*/);
			v3fCircles.resize(0);
			checkKey = waitKey(1);
			switch (checkKey)
			{
			case 'a':
				ratio[0] -= 0.01;
				break;
			case 'd':
				ratio[0] += 0.01;
				break;
			case 'A':
				offset[0] -= 5;
				break;
			case 'D':
				offset[0] += 5;
				break;
			case 'w':
				ratio[1] -= 0.01;
				break;
			case 's':
				ratio[1] += 0.01;
				break;
			case 'W':
				offset[1] -= 5;
				break;
			case 'S':
				offset[1] += 5;
				break;
			case 'N':
				currentStep++;
				break;
			case 'R':
				goto start;
				break;
			}
		}
	}
	return 0;
}