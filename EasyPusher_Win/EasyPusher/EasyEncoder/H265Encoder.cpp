// H265EncoderManager.cpp: implementation of the H264Encoder class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "H265Encoder.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CH265Encoder::CH265Encoder()
{
	m_hx265=NULL;
	m_bIsworking=false;
	m_nEncoderIndex=0;
	m_x265_picin=NULL;
	m_x265_param = NULL;
}

CH265Encoder::~CH265Encoder()
{
	Clean();
}

//品质
int CH265Encoder::Init(int width,int height,int fps,int keyframe,int bitrate,int level,int qp,int nUseQP)
{
	if (keyframe<5)
	{
		keyframe=10;
	}
	if (bitrate<100)
	{
		bitrate=1000;
	}
	if(qp<=0||qp>51)
		qp=26;
	if (fps<10)
	{
		fps=15;
	}
	Clean();

	m_nwidth=width;
	m_nheight=height;
	m_ncheckyuvsize=m_nwidth*m_nheight*3/2;

	m_x265_param = x265_param_alloc();
	x265_param_default(m_x265_param);
	CH265Encoder::X265_CONFIG_SET(m_x265_param,level);

	//手动配置X265编码器
	//m_x265_param->bRepeatHeaders = 1;//write vps sps,pps before keyframe
	m_x265_param->bRepeatHeaders = 0;// do not write vps sps,pps before keyframe

	//默认I420，不写没事
	int csp = X265_CSP_I420;
	m_x265_param->internalCsp = csp;
	m_x265_param->sourceWidth = width;
	m_x265_param->sourceHeight = height;
	m_x265_param->fpsNum = 25;//fps
	m_x265_param->fpsDenom = 1;

	m_x265_param->keyframeMin = 0; // X265_KEYFRAME_MIN_AUTO;
	m_x265_param->keyframeMax = keyframe;//gop
 
 	m_x265_param->bAnnexB = 0;//there is no B frame
 	 	
	//  X265_RC_ABR,
	//	X265_RC_CQP,
	//	X265_RC_CRF
	if(nUseQP==1)
	{
		if(qp>0&&qp<=51)
		{
			m_x265_param->rc.rateControlMode = X265_RC_CQP;//恒定质量	
			m_x265_param->rc.qp = qp;
		}
	}
	else
	{
		if (bitrate>0)
		{
			if(nUseQP==0)
				m_x265_param->rc.rateControlMode =X265_RC_ABR;////平均码率//恒定码率
			else if(nUseQP==2)
				m_x265_param->rc.rateControlMode =X265_RC_CRF;//码率动态

			m_x265_param->rc.bitrate = bitrate;//单位kbps
			m_x265_param->rc.vbvMaxBitrate = bitrate;
		}
	}

 ////////////////////////////////////
//     m_x265_param.i_threads = 1;
//     m_x265_param.i_lookahead_threads = 1;
//     m_x265_param.b_deterministic = 1;
//     m_x265_param.i_sync_lookahead = 1;

//	m_x265_param.analyse.intra = X265_ANALYSE_I4x4 ;//| X265_ANALYSE_I8x8 | X265_ANALYSE_PSUB8x8;
//	m_x265_param.analyse.inter = X265_ANALYSE_I4x4 ;//| X265_ANALYSE_I8x8 | X265_ANALYSE_PSUB8x8;
////	m_x265_param.analyse.i_me_range = 24;//越大越好
//	m_x265_param.analyse.b_transform_8x8 = 0;
//    m_x265_param.i_cqm_preset = X265_CQM_FLAT;
//	m_x265_param.analyse.i_me_method = X265_ME_UMH;
//	m_x265_param.b_cabac=0;
//	m_x265_param.rc.b_mb_tree = 0;//实时编码为0
//	m_x265_param.rc.b_stat_write=0;   /* Enable stat writing in psz_stat_out */
//	m_x265_param.rc.b_stat_read=0;    /* Read stat from psz_stat_in and use it */

/////////////////////////////////////////////

	//Create a x265 Encoder
	m_hx265=x265_encoder_open(m_x265_param);
	m_x265_picin = x265_picture_alloc();
	x265_picture_init(m_x265_param, m_x265_picin);

	int y_size = m_x265_param->sourceWidth * m_x265_param->sourceHeight;
	char* buff = NULL;
	switch (csp)
	{
	case X265_CSP_I444: 
		{
			buff = (char *)malloc(y_size * 3);
			m_x265_picin->planes[0] = buff;
			m_x265_picin->planes[1] = buff + y_size;
			m_x265_picin->planes[2] = buff + y_size * 2;
			m_x265_picin->stride[0] = width;
			m_x265_picin->stride[1] = width;
			m_x265_picin->stride[2] = width;
			break;
		}
	case X265_CSP_I420: 
		{
			buff = (char *)malloc(y_size * 3 / 2);
			m_x265_picin->planes[0] = buff;
			m_x265_picin->planes[1] = buff + y_size;
			m_x265_picin->planes[2] = buff + y_size * 5 / 4;
			m_x265_picin->stride[0] = width;
			m_x265_picin->stride[1] = width / 2;
			m_x265_picin->stride[2] = width / 2;
			break;
		}
	default: 
		{
			//printf("Colorspace Not Support.\n");
			return -1;
		}
	}

	m_bIsworking=true;
	return 0;
}

unsigned char* CH265Encoder::Encoder(unsigned char *indata, int inlen, int &outlen, bool &bIsKeyFrame)
{
	int nRet = 0;
	uint32_t m_inal=0;
	if (m_bIsworking&&(inlen==m_ncheckyuvsize))
	{	
		memcpy(m_x265_picin->planes[0],indata,inlen);
		nRet = x265_encoder_encode(m_hx265,&m_x265_nal,&m_inal,m_x265_picin,&m_x265_picout);
		m_x265_picin->pts++;
	}else
	{
		outlen=-1;
	}
	if (nRet>0)
	{
		outlen = m_x265_nal[0].sizeBytes;
		bIsKeyFrame=m_x265_picout.sliceType==1;

		TRACE("CH265Encoder::Encoder  m_inal=%d; sliceType=%d\n", m_inal, m_x265_picout.sliceType);
		return m_x265_nal[0].payload;
	}else
	{
		return NULL;
	}
}

int CH265Encoder::Clean()
{
	if (m_hx265 != NULL)
	{
		m_bIsworking = false;
		uint32_t m_inal = 0;
		x265_encoder_encode(m_hx265, &m_x265_nal, &m_inal, NULL, &m_x265_picout);
		x265_encoder_close(m_hx265);
		m_hx265 = NULL;
		if (m_x265_picin != NULL)
		{
			//free(m_x265_picin);
			//m_x265_picin=NULL;
			x265_picture_free(m_x265_picin);
			m_x265_picin = NULL;
		}
		if (m_x265_param)
		{
			x265_param_free(m_x265_param);
			m_x265_param = NULL;
		}
	}
	return 0;
}

void CH265Encoder::X265_CONFIG_SET(x265_param* param,int mode)
{
#if 0
	switch (mode)
	{
	case ZH265_DEFAULT:
	case ZH265_MEDIUM:
	case ZH265_ULTRAFAST:
		{
			param->i_frame_reference = 1;
			param->i_scenecut_threshold = 0;
			param->b_deblocking_filter = 0;
			param->b_cabac = 0;
			param->i_bframe = 0;
			param->analyse.intra = 0;
			param->analyse.inter = 0;
			param->analyse.b_transform_8x8 = 0;
			param->analyse.i_me_method = X265_ME_DIA;
			param->analyse.i_subpel_refine = 0;
			param->rc.i_aq_mode = 0;
			param->analyse.b_mixed_references = 0;
			param->analyse.i_trellis = 0;
			param->i_bframe_adaptive = X265_B_ADAPT_NONE;
			param->rc.b_mb_tree = 0;
			param->analyse.i_weighted_pred = X265_WEIGHTP_NONE;
			param->analyse.b_weighted_bipred = 0;
			param->rc.i_lookahead = 0;
		}
		break;
	case ZH265_SUPERFAST:
		{
			param->analyse.inter = X265_ANALYSE_I8x8|X265_ANALYSE_I4x4;
			param->analyse.i_me_method = X265_ME_DIA;
			param->analyse.i_subpel_refine = 1;
			param->i_frame_reference = 1;
			param->analyse.b_mixed_references = 0;
			param->analyse.i_trellis = 0;
			param->rc.b_mb_tree = 0;
			param->analyse.i_weighted_pred = X265_WEIGHTP_SIMPLE;
			param->rc.i_lookahead = 0;
		}
		break;
	case ZH265_VERYFAST:
		{
			param->analyse.i_me_method = X265_ME_HEX;
			param->analyse.i_subpel_refine = 2;
			param->i_frame_reference = 1;
			param->analyse.b_mixed_references = 0;
			param->analyse.i_trellis = 0;
			param->analyse.i_weighted_pred = X265_WEIGHTP_SIMPLE;
			param->rc.i_lookahead = 10;
		}
		break;
	case ZH265_FASTER:
		{
			param->analyse.b_mixed_references = 0;
			param->i_frame_reference = 2;
			param->analyse.i_subpel_refine = 4;
			param->analyse.i_weighted_pred = X265_WEIGHTP_SIMPLE;
			param->rc.i_lookahead = 20;			
		}
		break;
	case ZH265_FAST:
		{
			param->i_frame_reference = 2;
			param->analyse.i_subpel_refine = 6;
			param->analyse.i_weighted_pred = X265_WEIGHTP_SIMPLE;
			param->rc.i_lookahead = 30;
		}
		break;
	case ZH265_SLOW:
		{
			param->analyse.i_me_method = X265_ME_UMH;
			param->analyse.i_subpel_refine = 8;
			param->i_frame_reference = 5;
			param->i_bframe_adaptive = X265_B_ADAPT_TRELLIS;
			param->analyse.i_direct_mv_pred = X265_DIRECT_PRED_AUTO;
			param->rc.i_lookahead = 50;
		}
		break;
	case ZH265_SLOWER:
		{
			param->analyse.i_me_method = X265_ME_UMH;
			param->analyse.i_subpel_refine = 9;
			param->i_frame_reference = 8;
			param->i_bframe_adaptive = X265_B_ADAPT_TRELLIS;
			param->analyse.i_direct_mv_pred = X265_DIRECT_PRED_AUTO;
			param->analyse.inter |= X265_ANALYSE_PSUB8x8;
			param->analyse.i_trellis = 2;
			param->rc.i_lookahead = 60;
		}
		break;
	case ZH265_VERYSLOW:
		{
			param->analyse.i_me_method = X265_ME_UMH;
			param->analyse.i_subpel_refine = 10;
			param->analyse.i_me_range = 24;
			param->i_frame_reference = 16;
			param->i_bframe_adaptive = X265_B_ADAPT_TRELLIS;
			param->analyse.i_direct_mv_pred = X265_DIRECT_PRED_AUTO;
			param->analyse.inter |= X265_ANALYSE_PSUB8x8;
			param->analyse.i_trellis = 2;
			param->i_bframe = 8;
			param->rc.i_lookahead = 60;
		}
		break;
	}
#endif
}

bool CH265Encoder::IsWorking(void)
{
	return m_hx265!=NULL;
}
void CH265Encoder::GetVPSSPSAndPPS(unsigned char*vps, long&vpslen, unsigned char*sps,long&spslen,unsigned char*pps,long&ppslen)
{
	if (m_hx265!=NULL)
	{
		uint32_t i_nal=0;
		x265_nal* tnal=NULL;
		x265_encoder_headers(m_hx265,&tnal,&i_nal);
		vpslen=tnal[0].sizeBytes-4;
		memcpy(vps,tnal[0].payload+4,vpslen);
		spslen=tnal[1].sizeBytes-4;
		memcpy(sps,tnal[1].payload+4,spslen);
		ppslen = tnal[2].sizeBytes - 4;
		memcpy(pps, tnal[2].payload + 4, ppslen);
	}
	else
	{
		vpslen = 0;
		spslen = 0;
		ppslen = 0;
	}
}

void CH265Encoder::SetEncoderIndex(int nIndex)
{
	m_nEncoderIndex=nIndex;
}