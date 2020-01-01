//============================================================================
//
//% Student Name 1: Siavash Rezghighomi
//% Student 1 #: 301311417
//% Student 1 userid (email): srezghig@sfu.ca
//
//% Student Name 2: Arshdeep Singh Bhullar
//% Student 2 #: 301326478
//% Student 2 userid (email): stu2 (stu2@sfu.ca)

//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//			We did not have any direct help from someone but we have been discussing
//			the project with Kwok Kiang Lee, Minh Phat and Alikhan Zhansykbyev
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P3_<userid1>_<userid2>" (eg. P3_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : myIO.cpp
// Version     : September, 2019
// Description : Wrapper I/O functions for ENSC-351
// Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include "SocketReadcond.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <sys/ioctl.h>
#include <ScopedMutex.h>
#include "AtomicCOUT.h"
#include <iostream>
using namespace std;

std::mutex globmutex;

class threadObject
{
public:
	int buf;
	int prdSkts;
	mutex mtx;
	condition_variable cv;


	void myTcDrainLock()
	{
		unique_lock<mutex> lk(mtx);
		cv.wait(lk,[this]{return (buf<=0);});
	}

	threadObject(unsigned int temp);

};


threadObject::threadObject(unsigned int temp)
{
	buf = 0;
	prdSkts = temp;
}


static vector<threadObject*> clsVector;


int mySocketpair( int domain, int type, int protocol, int des[2] )
{
	unique_lock <mutex> lk (globmutex);
	int returnValue = socketpair(domain, type, protocol, des);
	if(returnValue != 0)
	{
		cout << "socket pair creation failed" << endl;
	}

	else
	{
		unsigned maxSize = max(des[0],des[1]);
		if (clsVector.size() < maxSize+1)
		{
			clsVector.resize(maxSize+1);
		}



		clsVector[des[0]] = new threadObject(des[1]);
		clsVector[des[1]] = new threadObject(des[0]);

		lk.unlock();
	}

	return returnValue;
}


int myOpen(const char *pathname, int flags, mode_t mode)
{


	lock_guard<mutex> lk(globmutex);

	int des = open(pathname, flags, mode);
	if(des >= clsVector.size())
	{
		clsVector.resize(des+1);
	}
	else
	{
		clsVector[des] = nullptr; // should already be nullptr
	}

	return des;
}


int myCreat(const char *pathname, mode_t mode)
{

	int des = creat(pathname, mode);
	if(des >= clsVector.size())
	{
		clsVector.resize(des+1);
	}

	return des;
}


ssize_t myWrite( int fildes, const void* buf, size_t nbyte )
{
	unique_lock<mutex> lk(globmutex);

	if(clsVector.at(fildes))
	{
		if(clsVector.at(fildes) -> prdSkts == -1)
		{
			return -1;
		}
		else
		{
			int prdSktsNum = clsVector[fildes] -> prdSkts;
			unique_lock<mutex> lk (clsVector[fildes] -> mtx);
			ssize_t writtenBytes = write(fildes, buf, nbyte);
			clsVector[prdSktsNum] -> buf += writtenBytes;
			lk.unlock();

			return writtenBytes;
		}
	}
	else
	{
		return write(fildes, buf, nbyte);
	}






}


int myClose( int fd )
{


	threadObject* addrs = clsVector[fd];
	clsVector[fd] = NULL; // nullptr
	delete addrs;
	return close(fd);



}


int myTcdrain(int des)
{ //is also included for purposes of the course.

	unique_lock <mutex> lk (clsVector[des]->mtx);
	int prdSktsNum = clsVector[des] -> prdSkts;
	clsVector[prdSktsNum] -> myTcDrainLock();
	lk.unlock();
	return 0;
}

/* See:
 *  https://developer.blackberry.com/native/reference/core/com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 *  */
int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{
	int bufBytes;
	ioctl(des, FIONREAD, &bufBytes);

	while (bufBytes < min)
	{
		unique_lock <mutex> lk (clsVector[des] -> mtx);
		clsVector[des] -> buf -= bufBytes;
		clsVector[des] -> cv.notify_all();
		ioctl(des, FIONREAD, &bufBytes);
		lk.unlock();
	}

	unique_lock <mutex> lk (clsVector[des]-> mtx);
	ssize_t readBytes = wcsReadcond(des, buf, n, min, time, timeout);
	clsVector[des]->buf -= readBytes;
	if (clsVector[des]->buf == 0)
	{
		clsVector[des]->cv.notify_all();

	}
	lk.unlock();
	return readBytes;

}


ssize_t myRead( int fildes, void* buf, size_t nbyte )
{
	if(fildes < clsVector.size())
	{
		//while dealing with descriptors
		if(!(clsVector.at(fildes)))
		{
			return read(fildes, buf, nbyte );
		}
	}

	//while dealing with the socketpairs
	return myReadcond(fildes, buf, nbyte, 1, 0, 0);

}







