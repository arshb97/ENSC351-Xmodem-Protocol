//============================================================================
//
    //% Student Name 1: Siavash Rezghighomi
    //% Student 1 #: 301311417
    //% Student 1 userid (email): srezghig (srezghig@sfu.ca)
    //
    //% Student Name 2: Arshdeep Singh Bhullar
    //% Student 2 #: 301326478
    //% Student 2 userid (email): asb29 (asb29@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
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
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : ReceiverX.cpp
// Version     : September 3rd, 2019
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"

//using namespace std;

ReceiverX::
ReceiverX(int d, const char *fname, bool useCrc)
:PeerX(d, fname, useCrc), 
NCGbyte(useCrc ? 'C' : NAK),
goodBlk(false), 
goodBlk1st(false), 
syncLoss(true),
numLastGoodBlk(0)
{
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	transferringFileD = PE2(myCreat(fileName, mode), fileName);

	// ***** improve this member function *****

	// below is just an example template.  You can follow a
	// 	different structure if you want.

	// inform sender that the receiver is ready and that the
	//		sender can send the first block
	sendByte(NCGbyte);

	goodBlk1st = true;

	while(PE_NOT(myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] == SOH))
	{
		getRestBlk();

		if(goodBlk && goodBlk1st)
		{
		    sendByte (ACK);
		    writeChunk();
		    numLastGoodBlk++;
		}
		else if(!goodBlk)
		{
		    sendByte(NAK);
		}
		else if(goodBlk && !goodBlk1st)
		{
		    sendByte(ACK);
		    goodBlk1st = true;
		}
	};

	if(rcvBlk[0] == EOT)
	{
	    sendByte(NAK);

	    PE_NOT(myRead(mediumD, rcvBlk, 1), 1);

	    if(rcvBlk[0] == EOT)
	    {
	        sendByte(ACK);
	        result = "Done";
	    }
	}
	if(rcvBlk[0] == CAN)
	{
	    PE_NOT(myRead(mediumD, rcvBlk, 1), 1);

	    if(rcvBlk[0] == EOT)
	    {
	        sendByte(ACK);
	        result = "Cancel by Sender";
	    }
	}

	if((rcvBlk[0] != SOH) && (rcvBlk[0] != EOT) && (rcvBlk[0] != CAN))
	{
	    can8();
	}

	PE(close(transferringFileD));
}


/* Only called after an SOH character has been received.
The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
*/
void ReceiverX::getRestBlk()
{
	// ********* this function must be improved ***********
	PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
	goodBlk1st = goodBlk = true;
	uint8_t nextBlkNumber = numLastGoodBlk + 1;

	if(NCGbyte == 'C')
	{
	    uint16_t crcTest;
	    uint16_t crcRcv;
	    crcRcv = ((uint16_t)rcvBlk[132] << 8) | rcvBlk[131];
	    crc16ns(&crcTest, &rcvBlk[DATA_POS]);
	    if(crcTest != crcRcv)
	    {
	        goodBlk = false;
	        return;
	    }
	}
	else
	{
	    //useCrc == false
	    uint8_t chksum = 0;
	    for(int i = DATA_POS; i < CHUNK_SZ+DATA_POS; i++)
	    {
	        chksum += rcvBlk[i];
	    }
	    if(chksum != rcvBlk[131])
	    {
	        goodBlk = false;
	        return;
	    }
	}
	if((rcvBlk[1]+rcvBlk[2]) != 255)
	{
	    goodBlk = false;
	    return;
	}
	if(rcvBlk[1] != nextBlkNumber)
	{
	    goodBlk1st = false;
	}
	return;
}
//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
// the canceling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver
	const int canLen = 8; // move to defines.h
	char buffer[canLen];
	memset( buffer, CAN, canLen);
	PE_NOT(myWrite(mediumD, buffer, canLen), canLen);

}

//Write chunk (data) in a received block to disk



