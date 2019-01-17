// H265EncoderManager.h: interface for the CH265EncoderManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(H265ENCODERMANAGER_H)
#define H265ENCODERMANAGER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stdint.h"
extern "C"{
#include "x265.h"
};
#pragma comment(lib,"libx265.lib")

#define ZH265_DEFAULT 0
#define ZH265_ULTRAFAST 1
#define ZH265_SUPERFAST 2
#define ZH265_VERYFAST 3
#define ZH265_FASTER 4
#define ZH265_FAST 5
#define ZH265_MEDIUM 6
#define ZH265_SLOW 7
#define ZH265_SLOWER 8
#define ZH265_VERYSLOW 9


class CH265Encoder  
{
private:
	x265_encoder* m_hx265;
	x265_param* m_x265_param;
	x265_picture *m_x265_picin;
	x265_picture m_x265_picout;
	x265_nal*	m_x265_nal;

	int m_nwidth;
	int m_nheight;
	bool m_bIsworking;
	int m_nEncoderIndex;
	int m_ncheckyuvsize;

public:
	CH265Encoder();
	virtual ~CH265Encoder();
	static	void X265_CONFIG_SET(x265_param* param,int mode);
	unsigned char*	Encoder(unsigned char *indata,int inlen,int &outlen, bool& bIsKeyFrame);
	int Clean();
	int Init(int width,int height,int fps,int keyframe,int bitrate,int level,int qp,int nUseQP);
	bool IsWorking(void);
	void GetVPSSPSAndPPS(unsigned char*vps, long&vpslen, unsigned char*sps,long&spslen,unsigned char*pps,long&ppslen);
	void SetEncoderIndex(int nIndex);
};

#endif // !defined(AFX_H265ENCODERMANAGER_H__1E331664_81DA_41D7_B8B8_524FF359B7DE__INCLUDED_)
