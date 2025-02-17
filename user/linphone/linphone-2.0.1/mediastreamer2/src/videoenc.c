/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <ffmpeg/avcodec.h>
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>			/* ntohl(3) */
#endif

#include "rfc2429.h"

static bool_t avcodec_initialized=FALSE;

void ms_ffmpeg_check_init(){
	if(!avcodec_initialized){
		avcodec_init();
		avcodec_register_all();
		avcodec_initialized=TRUE;
	}
}

typedef struct EncState{
	AVCodecContext av_context;
	AVCodec *av_codec;
	enum CodecID codec;
	mblk_t *comp_buf;
	MSVideoSize vsize;
	int mtu;	/* network maximum transmission unit in bytes */
	int profile;
	float fps;
	int maxbr;
	int qmin;
	bool_t req_vfu;
}EncState;

static int enc_set_fps(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	s->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	*(float*)arg=s->fps;
	return 0;
}

static int enc_set_vsize(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	s->vsize=*(MSVideoSize*)arg;
	return 0;
}

static int enc_get_vsize(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	*(MSVideoSize*)arg=s->vsize;
	return 0;
}

static int enc_set_mtu(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	s->mtu=*(int*)arg;
	return 0;
}

static int enc_set_br(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	bool_t snow=s->codec==CODEC_ID_SNOW;
	s->maxbr=*(int*)arg;
	if (s->maxbr>=512000){
		s->vsize.width=MS_VIDEO_SIZE_CIF_W;
		s->vsize.height=MS_VIDEO_SIZE_CIF_H;
		s->fps=17;
	}else if (s->maxbr>=256000){
		s->vsize.width=MS_VIDEO_SIZE_CIF_W;
		s->vsize.height=MS_VIDEO_SIZE_CIF_H;
		s->fps=10;
		s->qmin=3;
	}else if (s->maxbr>=128000){
		s->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		s->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->fps=10;
		s->qmin=3;
	}else if (s->maxbr>=64000){
		s->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		s->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->fps=snow ? 7 : 5;
		s->qmin=snow ? 4 : 5;
	}else{
		s->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		s->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->fps=5;
		s->qmin=5;
	}
	return 0;
}

static bool_t parse_video_fmtp(const char *fmtp, float *fps, MSVideoSize *vsize){
	char *tmp=ms_strdup(fmtp);
	char *semicolon;
	char *equal;
	bool_t ret=TRUE;

	ms_message("parsing %s",fmtp);
	/*extract fisrt pair */
	if ((semicolon=strchr(tmp,';'))!=NULL){
		*semicolon='\0';
	}
	if ((equal=strchr(tmp,'='))!=NULL){
		int divider;
		*equal='\0';
		if (strcasecmp(tmp,"CIF")==0){
			if (vsize->width>=MS_VIDEO_SIZE_CIF_W){
				vsize->width=MS_VIDEO_SIZE_CIF_W;
				vsize->height=MS_VIDEO_SIZE_CIF_H;
			}
		}else if (strcasecmp(tmp,"QCIF")==0){
			vsize->width=MS_VIDEO_SIZE_QCIF_W;
			vsize->height=MS_VIDEO_SIZE_QCIF_H;
		}else{
			ms_warning("unsupported video size %s",tmp);
			ret=FALSE;
		}
		divider=atoi(equal+1);
		if (divider!=0){
			float newfps=29.97/divider;
			if (*fps>newfps) *fps=newfps;
		}else{
			ms_warning("Could not find video fps");
			ret=FALSE;
		}
	}else ret=FALSE;
	ms_free(tmp);
	return ret;
}

static int enc_add_fmtp(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	const char *fmtp=(const char*)arg;
	char val[10];
	if (fmtp_get_value(fmtp,"profile",val,sizeof(val))){
		s->profile=atoi(val);
	}else parse_video_fmtp(fmtp,&s->fps,&s->vsize);
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *unused){
	EncState *s=(EncState*)f->data;
	s->req_vfu=TRUE;
	return 0;
}

static void enc_init(MSFilter *f, enum CodecID codec)
{
	EncState *s=(EncState *)ms_new(EncState,1);
	f->data=s;
	ms_ffmpeg_check_init();
	s->profile=0;/*always default to profile 0*/
	s->comp_buf=allocb(32000,0);
	s->fps=15;
	s->mtu=1400;
	s->maxbr=500000;
	s->codec=codec;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->qmin=2;
	s->req_vfu=FALSE;
}

static void enc_h263_init(MSFilter *f){
	enc_init(f,CODEC_ID_H263P);
}

static void enc_mpeg4_init(MSFilter *f){
	enc_init(f,CODEC_ID_MPEG4);
}

static void enc_snow_init(MSFilter *f){
	enc_init(f,CODEC_ID_SNOW);
}

static void prepare(EncState *s){
	AVCodecContext *c=&s->av_context;
	avcodec_get_context_defaults(c);
	/* put codec parameters */
	c->bit_rate=(float)s->maxbr*0.9;
	c->bit_rate_tolerance=s->maxbr/5;
	if (s->codec!=CODEC_ID_SNOW && s->maxbr<256000){
		/*snow does not like 1st pass rate control*/
		/*and rate control eats too much cpu with CIF high fps pictures*/
		c->rc_max_rate=s->maxbr;
		c->rc_min_rate=0;
		c->rc_buffer_size=100000;
	}else{
		/*use qmin instead*/
		c->qmin=s->qmin;
	}

	ms_message("Codec bitrate set to %i",c->bit_rate);
	c->width = s->vsize.width;  
	c->height = s->vsize.height;
	c->time_base.num = 1;
	c->time_base.den = (int)s->fps;
	c->gop_size=(int)s->fps*5; /*emit I frame every 5 seconds*/
	c->pix_fmt=PIX_FMT_YUV420P;
	
	if (s->codec==CODEC_ID_SNOW){
		c->strict_std_compliance=-2;
	}
	
}

static void prepare_h263(EncState *s){
	AVCodecContext *c=&s->av_context;
	/* we don't use the rtp_callback but use rtp_mode that forces ffmpeg to insert
	Start Codes as much as possible in the bitstream */
	c->rtp_mode = 1;
	c->rtp_payload_size = s->mtu/2;
	if (s->profile==0){
		s->codec=CODEC_ID_H263;
	}else{
		c->flags|=CODEC_FLAG_H263P_UMV;
		c->flags|=CODEC_FLAG_AC_PRED;
		c->flags|=CODEC_FLAG_H263P_SLICE_STRUCT;
		/*
		c->flags|=CODEC_FLAG_OBMC;
		c->flags|=CODEC_FLAG_AC_PRED;
		*/
		s->codec=CODEC_ID_H263P;
	}
}

static void prepare_mpeg4(EncState *s){
	AVCodecContext *c=&s->av_context;
	c->max_b_frames=0; /*don't use b frames*/
	c->flags|=CODEC_FLAG_AC_PRED;
	c->flags|=CODEC_FLAG_H263P_UMV;
	/*c->flags|=CODEC_FLAG_QPEL;*/ /*don't enable this one: this forces profile_level to advanced simple profile */
	c->flags|=CODEC_FLAG_4MV;
	c->flags|=CODEC_FLAG_GMC;
	c->flags|=CODEC_FLAG_LOOP_FILTER;
	c->flags|=CODEC_FLAG_H263P_SLICE_STRUCT;
}

static void enc_uninit(MSFilter  *f){
	EncState *s=(EncState*)f->data;
	if (s->comp_buf!=NULL)	freemsg(s->comp_buf);
	ms_free(s);
}
#if 0
static void enc_set_rc(EncState *s, AVCodecContext *c){
	int factor=c->width/MS_VIDEO_SIZE_QCIF_W;
	c->rc_min_rate=0;
	c->bit_rate=400; /* this value makes around 100kbit/s at QCIF=2 */
	c->rc_max_rate=c->bit_rate+1;
	c->rc_buffer_size=20000*factor;	/* food recipe */
}
#endif

static void enc_preprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	int error;
	prepare(s);
	if (s->codec==CODEC_ID_H263P || s->codec==CODEC_ID_H263)
		prepare_h263(s);
	else if (s->codec==CODEC_ID_MPEG4)
		prepare_mpeg4(s);
	else if (s->codec==CODEC_ID_SNOW){
		/**/
	}else {
		ms_error("Unsupported codec id %i",s->codec);
		return;
	}
	s->av_codec=avcodec_find_encoder(s->codec);
	if (s->av_codec==NULL){
		ms_error("could not find encoder for codec id %i",s->codec);
		return;
	}
	error=avcodec_open(&s->av_context, s->av_codec);
	if (error!=0) {
		ms_error("avcodec_open() failed: %i",error);
		return;
	}
	ms_debug("image format is %i.",s->av_context.pix_fmt);
	ms_message("qmin=%i qmax=%i",s->av_context.qmin,s->av_context.qmax);
}

static void enc_postprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	if (s->av_context.codec!=NULL){
		avcodec_close(&s->av_context);
		s->av_context.codec=NULL;
	}
}

static void mpeg4_fragment_and_send(MSFilter *f,EncState *s,mblk_t *frame, uint32_t timestamp){
	uint8_t *rptr;
	mblk_t *packet=NULL;
	int len;
	for (rptr=frame->b_rptr;rptr<frame->b_wptr;){
		len=MIN(s->mtu,(frame->b_wptr-rptr));
		packet=dupb(frame);
		packet->b_rptr=rptr;
		packet->b_wptr=rptr+len;
		mblk_set_timestamp_info(packet,timestamp);
		ms_queue_put(f->outputs[0],packet);
		rptr+=len;
	}
	/*set marker bit on last packet*/
	mblk_set_marker_info(packet,TRUE);
}

static void generate_packets(MSFilter *f, EncState *s, mblk_t *frame, uint32_t timestamp, uint8_t *psc, uint8_t *end, bool_t last_packet){
	mblk_t *packet;
	int len=end-psc;
	
	packet=dupb(frame);	
	packet->b_rptr=psc;
	packet->b_wptr=end;
	/*ms_message("generating packet of size %i",end-psc);*/
	rfc2429_set_P(psc,1);
	mblk_set_timestamp_info(packet,timestamp);

	
	if (len>s->mtu){
		/*need to slit the packet using "follow-on" packets */
		/*compute the number of packets need (rounded up)*/
		int num=(len+s->mtu-1)/s->mtu;
		int i;
		uint8_t *pos;
		/*adjust the first packet generated*/
		pos=packet->b_wptr=packet->b_rptr+s->mtu;
		ms_queue_put(f->outputs[0],packet);
		ms_debug("generating %i follow-on packets",num);
		for (i=1;i<num;++i){
			mblk_t *header;
			packet=dupb(frame);
			packet->b_rptr=pos;
			pos=packet->b_wptr=MIN(pos+s->mtu,end);
			header=allocb(2,0);
			header->b_wptr[0]=0;
			header->b_wptr[1]=0;
			header->b_wptr+=2;
			/*no P bit is set */
			header->b_cont=packet;
			packet=header;
			mblk_set_timestamp_info(packet,timestamp);
			ms_queue_put(f->outputs[0],packet);
		}
	}else ms_queue_put(f->outputs[0],packet);
	/* the marker bit is set on the last packet, if any.*/
	mblk_set_marker_info(packet,last_packet);
}

/* returns the last psc position just below packet_size */
static uint8_t *get_psc(uint8_t *begin,uint8_t *end, int packet_size){
	int i;
	uint8_t *ret=NULL;
	uint8_t *p;
	if (begin==end) return NULL;
	for(i=1,p=begin+1;p<end && i<packet_size;++i,++p){
		if (p[-1]==0 && p[0]==0){
			ret=p-1;
		}
		p++;/* to skip possible 0 after the PSC that would make a double detection */
	}
	return ret;
}

static void split_and_send(MSFilter *f, EncState *s, mblk_t *frame){
	uint8_t *lastpsc;
	uint8_t *psc;
	uint32_t timestamp=f->ticker->time*90LL;
	
	if (s->codec==CODEC_ID_MPEG4 || s->codec==CODEC_ID_SNOW)
	{
		mpeg4_fragment_and_send(f,s,frame,timestamp);
		return;
	}

	ms_debug("processing frame of size %i",frame->b_wptr-frame->b_rptr);
	lastpsc=frame->b_rptr;
	while(1){
		psc=get_psc(lastpsc+2,frame->b_wptr,s->mtu);
		if (psc!=NULL){
			generate_packets(f,s,frame,timestamp,lastpsc,psc,FALSE);
			lastpsc=psc;
		}else break;
	}
	
	/* send the end of frame */
	generate_packets(f,s,frame, timestamp,lastpsc,frame->b_wptr,TRUE);
}

static void process_frame(MSFilter *f, mblk_t *inm){
	EncState *s=(EncState*)f->data;
	AVFrame pict;
	AVCodecContext *c=&s->av_context;
	int error;
	mblk_t *comp_buf=s->comp_buf;
	int comp_buf_sz=comp_buf->b_datap->db_lim-comp_buf->b_datap->db_base;
	
	/* convert image if necessary */
	avcodec_get_frame_defaults(&pict);
	avpicture_fill((AVPicture*)&pict,(uint8_t*)inm->b_rptr,c->pix_fmt,c->width,c->height);
	
	/* timestamp used by ffmpeg, unset here */
	pict.pts=AV_NOPTS_VALUE;
	if (s->req_vfu){
		pict.pict_type=FF_I_TYPE;
		s->req_vfu=FALSE;
	}
	comp_buf->b_rptr=comp_buf->b_wptr=comp_buf->b_datap->db_base;
	if (s->codec==CODEC_ID_SNOW){
		//prepend picture size
		uint32_t header=((s->vsize.width&0xffff)<<16) | (s->vsize.height&0xffff);
		*(uint32_t*)comp_buf->b_wptr=htonl(header);
		comp_buf->b_wptr+=4;
		comp_buf_sz-=4;
	}
	error=avcodec_encode_video(c, (uint8_t*)comp_buf->b_wptr,comp_buf_sz, &pict);
	if (error<=0) ms_warning("ms_AVencoder_process: error %i.",error);
	else{
		if (c->coded_frame->pict_type==FF_I_TYPE){
			ms_message("Emitting I-frame");
		}
		comp_buf->b_wptr+=error;
		split_and_send(f,s,comp_buf);
	}
	freemsg(inm);
}

static void enc_process(MSFilter *f){
	mblk_t *inm;
	EncState *s=(EncState*)f->data;
	if (s->av_context.codec==NULL) {
		ms_queue_flush(f->inputs[0]);
		return;
	}
	while((inm=ms_queue_get(f->inputs[0]))!=0){
		process_frame(f,inm);
	}
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_SET_VIDEO_SIZE ,	enc_set_vsize },
	{	MS_FILTER_GET_VIDEO_SIZE ,	enc_get_vsize },
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp },
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_SET_MTU	,	enc_set_mtu	},
	{	MS_FILTER_REQ_VFU	,	enc_req_vfu	},
	{	0			,	NULL	}
};

#ifdef _MSC_VER

MSFilterDesc ms_h263_enc_desc={
	MS_H263_ENC_ID,
	"MSH263Enc",
	"A video H.263 encoder using ffmpeg library.",
	MS_FILTER_ENCODER,
	"H263-1998",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_h263_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

MSFilterDesc ms_mpeg4_enc_desc={
	MS_MPEG4_ENC_ID,
	"MSMpeg4Enc",
	"A video MPEG4 encoder using ffmpeg library.",
	MS_FILTER_ENCODER,
	"MP4V-ES",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_mpeg4_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

MSFilterDesc ms_snow_enc_desc={
	MS_SNOW_ENC_ID,
	"MSSnowEnc",
	"A video snow encoder using ffmpeg library.",
	MS_FILTER_ENCODER,
	"x-snow",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_snow_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

#else

MSFilterDesc ms_h263_enc_desc={
	.id=MS_H263_ENC_ID,
	.name="MSH263Enc",
	.text="A video H.263 encoder using ffmpeg library.",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="H263-1998",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_h263_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

MSFilterDesc ms_mpeg4_enc_desc={
	.id=MS_MPEG4_ENC_ID,
	.name="MSMpeg4Enc",
	.text="A video MPEG4 encoder using ffmpeg library.",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="MP4V-ES",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_mpeg4_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

MSFilterDesc ms_snow_enc_desc={
	.id=MS_SNOW_ENC_ID,
	.name="MSSnowEnc",
	.text="The snow codec is royalty-free and is open-source. \n"
		"It uses innovative techniques that makes it one of the best video "
		"codec. It is implemented within the ffmpeg project.\n"
		"However it is under development and compatibility with other versions "
		"cannot be guaranteed.",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="x-snow",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_snow_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

#endif

#if defined(WIN32) || defined(_WIN32_WCE)
__declspec(dllexport) PayloadType payload_type_x_snow
#else
PayloadType payload_type_x_snow
#endif
={
	PAYLOAD_VIDEO,
	90000,
	0,
	NULL,
	0,
	256000,
	"x-snow"
};


MS_FILTER_DESC_EXPORT(ms_mpeg4_enc_desc)
MS_FILTER_DESC_EXPORT(ms_h263_enc_desc)
MS_FILTER_DESC_EXPORT(ms_snow_enc_desc)

