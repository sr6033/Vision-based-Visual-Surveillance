#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
  
#include <iostream>
#include <cstdio>
#include "Eserial.h"

using namespace std;
using namespace cv;
 
/** Function Headers */ 
void detectAndDisplay (Mat frame, int threshold);
vector<Rect> storeLeftEyePos(Mat rightFaceImage);
vector<Rect> storeLeftEyePos_open(Mat rightFaceImage);
 
/** Global variables */ 
//-- Note, either copy these two files from opencv/data/haarscascades to your current folder, or change these locations
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
String leftEyeCascade = "haarcascade_lefteye_2splits.xml";
String rightEyeCascade = "haarcascade_righteye_2splits.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
CascadeClassifier leftEyeDetector;
CascadeClassifier rightEyeDetector;
string window_name = "Captcha";
 
// Serial to Arduino global declarations
int arduino_command;
Eserial * arduino_com;
short MSBLSB = 0;
unsigned char MSB = 0;
unsigned char LSB = 0;

int open_eye = 0, close_eye = 0;

// Serial to Arduino global declarations
int main (int argc, const char **argv) 
{
	VideoCapture capture(0);
	Mat frame;
 	char c = argv[2][0];
 	cout << c << endl;
	/*
	if (argc != 2)
	{
		cerr << "Usage: techbitarFaceDetection <serial device>" << endl;
		exit(1);
	}
	*/
	// serial to Arduino setup 
	arduino_com = new Eserial ();
	if (arduino_com != 0)
	{
		//Standard Arduino uses /dev/ttyACM0 typically, Shrimping.it
		//seems to use /dev/ttyUSB0 or /dev/ttyUSB1.
		arduino_com->connect(argv[1], 57600, spNONE);
	}
  
	// serial to Arduino setup 
	
	//-- 1. Load the cascades
	if (!face_cascade.load (face_cascade_name))
	{
		printf("--(!)Error loading\n");
		return -1;
	};
	if (!eyes_cascade.load (eyes_cascade_name))
	{
		printf("--(!)Error loading\n");
		return -1;
	};
   	
   	if (!leftEyeDetector.load (leftEyeCascade))
	{
		printf("--(!)Error loading\n");
		return -1;
	};
	if (!rightEyeDetector.load (rightEyeCascade))
	{
		printf("--(!)Error loading\n");
		return -1;
	};
	//-- 2. Read the video stream
	//capture = cvCaptureFromCAM (-1);

	if (capture.isOpened())
	{
		while (true)
		{
			capture.read(frame);
			//-- 3. Apply the classifier to the frame
			if(!frame.empty ())
			{
				detectAndDisplay (frame, c-'0');
			}
			else
			{
				printf (" --(!) No captured frame -- Break!");
				break;
			}
			int c = waitKey (10);
			if((char) c == 'c')
			{
				break;
			}
		}
	}
  
	// Serial to Arduino - shutdown
	arduino_com->disconnect ();
	delete arduino_com;
	arduino_com = 0;
  
	// Serial to Arduino - shutdown
	return 0;
}

 
/**
 * @function detectAndDisplay
 */ 
void detectAndDisplay (Mat frame, int threshold) 
{
	std::vector <Rect> faces;
	Mat frame_gray;
	cvtColor (frame, frame_gray, CV_BGR2GRAY);
	equalizeHist (frame_gray, frame_gray);
  
	//-- Detect faces
	face_cascade.detectMultiScale (frame_gray, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size (30, 30));
	for (int i = 0; i < faces.size (); i++)
	{
		Point center (faces[i].x + faces[i].width * 0.5, faces[i].y + faces[i].height * 0.5);
		ellipse (frame, center, Size (faces[i].width * 0.5, faces[i].height * 0.5), 0, 0, 360, Scalar (255, 0, 255), 2, 8, 0);
		
		Mat faceROI = frame_gray (faces[i]);
		std::vector <Rect> eyes;
		std::vector <Rect> eyes1;

		//-- In each face, detect eyes
		//eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CASCADE_SCALE_IMAGE, Size(30, 30) );
		//cout << eyes.size() << endl;
		leftEyeDetector.detectMultiScale(faceROI, eyes, 1.1, 2, CASCADE_FIND_BIGGEST_OBJECT, Size(0, 0));
		rightEyeDetector.detectMultiScale(faceROI, eyes1, 1.1, 2, CASCADE_FIND_BIGGEST_OBJECT, Size(0, 0));

		for ( size_t j = 0; j < eyes.size(); j++ )
		{
			Point eye_center( faces[i].x + eyes[j].x + eyes[j].width/2, faces[i].y + eyes[j].y + eyes[j].height/2 );
			int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
			circle( frame, eye_center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
		}
		for ( size_t j = 0; j < eyes1.size(); j++ )
		{
			Point eye_center( faces[i].x + eyes1[j].x + eyes1[j].width/2, faces[i].y + eyes1[j].y + eyes1[j].height/2 );
			int radius = cvRound( (eyes1[j].width + eyes1[j].height)*0.25 );
			circle( frame, eye_center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
		}
		
		if (eyes.size() > 0)
		{
		    // Now look for open eyes only
		    vector<Rect> eyesRightNew = storeLeftEyePos_open(faceROI);

		    if(open_eye > threshold && close_eye > threshold)
			{
				//  cout << "X:" << faces[i].x  <<  "  y:" << faces[i].y  << endl;
					  
				// send X,Y of face center to serial com port      
				// send X axis
				// read least significant byte
							
				LSB = faces[i].x & 0xff;
						  
				// read next significant byte 
				MSB = (faces[i].x >> 8) & 0xff;
				arduino_com->sendChar (MSB);
				arduino_com->sendChar (LSB);
				//cout << "X MSB: " << hex << (int) MSB << ", X LSB: " << (int) LSB << endl;
						   
				// Send Y axis
				LSB = faces[i].y & 0xff;
				MSB = (faces[i].y >> 8) & 0xff;
				arduino_com->sendChar (MSB);
				arduino_com->sendChar (LSB);
				//cout << "Y MSB: " << hex << (int) MSB << ", Y LSB: " << (int) LSB << endl;
						  
				// serial com port send
			}
			else
			if (eyesRightNew.size() > 0) //Eye is open
		    {              
		   		cout << "OPEN" << endl;
		   		open_eye++;
		    }
		    else //Eye is closed
		    {              
		       	cout << "close" << endl;
		    	close_eye++;
		    }
		} 		
	}  
	//-- Show what you got
	imshow (window_name, frame);
}

// Method for detecting open eyes in right half of face
vector<Rect> storeLeftEyePos_open(Mat rightFaceImage)
{
	std::vector<Rect> eyes;
	eyes_cascade.detectMultiScale(rightFaceImage, eyes, 1.1, 2, CASCADE_FIND_BIGGEST_OBJECT, Size(0, 0));
	return eyes;
}