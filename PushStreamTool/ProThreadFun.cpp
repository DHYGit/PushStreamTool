#include "stdafx.h"
#include <conio.h>
#include "ProThreadFun.h"
#include "FFmpegTool.h"
#include "Resource.h"
#include "PushStreamToolDlg.h"
#include "librtmpTool.h"

#include "rtmp_sys.h"
#include "log.h"

int PushStream_LibRtmp(LPVOID pM)
{
	CPushStreamToolDlg *pDlg = (CPushStreamToolDlg*)pM;
	RTMP *rtmp=NULL;
	RTMPPacket *packet=NULL;
	uint32_t start_time=0;
	uint32_t now_time=0;
	//the timestamp of the previous frame
	long pre_frame_time=0; 
	long lasttime=0;
	int bNextIsKey=1;
	uint32_t preTagsize=0;
	//packet attributes
	uint32_t type=0;			
	uint32_t datalength=0;		
	uint32_t timestamp=0;		
	uint32_t streamid=0;
	FILE *fp = NULL;
	fp=fopen((LPSTR)(LPCTSTR)pDlg->m_SourceFileName,"rb");
	if (!fp){
		RTMP_LogPrintf("Open File Error.\n");
		cprintf("Open file error \n");
		CleanupSockets();
		return -1;
	}
	if (!InitSockets()){
		RTMP_LogPrintf("Init Socket Err\n");
		cprintf("Init socket error \n");
		return -1;
	}
	// 创建一个RTMP会话的句柄
	rtmp=RTMP_Alloc();
	// 初始化RTMP句柄
	RTMP_Init(rtmp);
	//set connection timeout,default 30s
	rtmp->Link.timeout=5;
	// 设置URL
	if(!RTMP_SetupURL(rtmp,(LPSTR)(LPCTSTR)pDlg->m_URL))
	{
		RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();
		return -1;
	}
	//if unable,the AMF command would be 'play' instead of 'publish'
	RTMP_EnableWrite(rtmp);	
	// RTMP_Connect分为2步：RTMP_Connect0和RTMP_Connect1
	// 0负责建立TCP底层连接
	// 1负责RTMP握手操作
	if (!RTMP_Connect(rtmp,NULL)){
		RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();
		return -1;
	}
	if (!RTMP_ConnectStream(rtmp,0)){
		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		CleanupSockets();
		return -1;
	}
	packet=(RTMPPacket*)malloc(sizeof(RTMPPacket));
	RTMPPacket_Alloc(packet,1024*64);
	RTMPPacket_Reset(packet);
	packet->m_hasAbsTimestamp = 0;	
	packet->m_nChannel = 0x04;	
	packet->m_nInfoField2 = rtmp->m_stream_id;
	RTMP_LogPrintf("Start to send data ...\n");
	// FLV格式的header为9个字节
	fseek(fp,9,SEEK_SET);	
	// 跳过表征前一段Tag大小的4个字节
	fseek(fp,4,SEEK_CUR);	
	start_time=RTMP_GetTime();
	int frame_index = 0;
	while(1){
		if((((now_time=RTMP_GetTime())-start_time)
			<(pre_frame_time)) && bNextIsKey){	
				//wait for 1 sec if the send process is too fast
				//this mechanism is not very good,need some improvement
				if(pre_frame_time>lasttime){
					RTMP_LogPrintf("TimeStamp:%8lu ms\n",pre_frame_time);
					lasttime=pre_frame_time;
				}
				Sleep(1000);
				continue;
		}
		//not quite the same as FLV spec
		// 读取当前Tag的类型（1个字节）
		if(!ReadU8(&type,fp))	
			break;
		// 读取当前Tag data部分的大小（3个字节）
		if(!ReadU24(&datalength,fp))
			break;
		// 读取时间戳（4个字节）
		if(!ReadTime(&timestamp,fp))
			break;
		// 读取stream id（3个字节），一般为0
		if(!ReadU24(&streamid,fp))
			break;
		// 跳过既非视频也非音频的Tag
		if (type!=0x08&&type!=0x09){
			//jump over non_audio and non_video frame，
			//jump over next previousTagSizen at the same time
			fseek(fp,datalength+4,SEEK_CUR);
			continue;
		}
		// 读取当前音视频Tag的数据到packet
		if(fread(packet->m_body,1,datalength,fp)!=datalength)
			break;
		packet->m_headerType = RTMP_PACKET_SIZE_LARGE; 
		packet->m_nTimeStamp = timestamp; 
		packet->m_packetType = type;
		packet->m_nBodySize  = datalength;
		pre_frame_time=timestamp;

		if (!RTMP_IsConnected(rtmp)){
			RTMP_Log(RTMP_LOGERROR,"rtmp is not connect\n");
			break;
		}
		// 这样看下来是一个FLV的Tag发送一个RTMPPacket
		if (!RTMP_SendPacket(rtmp,packet,0)){
			RTMP_Log(RTMP_LOGERROR,"Send Error\n");
			break;
		}
		cprintf("Send %d video frames to %s\n", frame_index++, (LPSTR)(LPCTSTR)pDlg->m_URL);
		// 读取前一个Tag的size
		if(!ReadU32(&preTagsize,fp))
		{
			break;
		}
		// 读取当前Tag的type
		if(!PeekU8(&type,fp))
		{
			break;
		}
		if(type==0x09){
			if(fseek(fp,11,SEEK_CUR)!=0)
				break;
			if(!PeekU8(&type,fp)){
				break;
			}
			if(type==0x17)
			{
				bNextIsKey=1;
			}
			else
			{
				bNextIsKey=0;
			}
			fseek(fp,-11,SEEK_CUR);
		}
	}
	RTMP_LogPrintf("\nSend Data Over\n");
	if(fp)
	{
		fclose(fp);
	}
	if (rtmp!=NULL){
		RTMP_Close(rtmp);	
		RTMP_Free(rtmp);	
		rtmp=NULL;
	}
	if (packet!=NULL){
		RTMPPacket_Free(packet);	
		free(packet);
		packet=NULL;
	}
	CleanupSockets();
	return 0;
}

int EncodeFun(LPVOID pM){
	CPushStreamToolDlg *pDlg = (CPushStreamToolDlg*)pM;

	int n = pDlg->ffmpeg->FFmpeg_openFile((LPSTR)(LPCTSTR)pDlg->m_SourceFileName);
	if (n != 0){
		cprintf("Open source file failed\n");
		return n;
	}
	cprintf("SPS PPS: \n");
	for(int i = 0; i < pDlg->ffmpeg->pCodecCtx_In->extradata_size; i++){
		cprintf("%02X  ", pDlg->ffmpeg->pCodecCtx_In->extradata[i]);
	}
	cprintf("\n");
	pDlg->ffmpeg->bmp = SDL_CreateYUVOverlay(pDlg->ffmpeg->pCodecCtx_In->width, pDlg->ffmpeg->pCodecCtx_In->height, SDL_YV12_OVERLAY, pDlg->ffmpeg->screen);
	cprintf("Open source file success \n");
	int ret = pDlg->ffmpeg->FFmpeg_openOutPutFile((LPSTR)(LPCTSTR)pDlg->m_URL, "hls");
	if(ret != 0){
		cprintf("Open output file failed \n");
		return ret;
	}
	cprintf("Open output file success \n");
	pDlg->ffmpeg->Encoder_Init();
	cprintf("Encoder SPS PPS: \n");
	for(int i = 0; i < pDlg->ffmpeg->pCodecCtx_Enc->extradata_size; i++){
		cprintf("%02X  ", pDlg->ffmpeg->pCodecCtx_Enc->extradata[i]);
	}

	pDlg->ffmpeg->start_time = av_gettime();
	int frame_index = 0;
	AVPacket pkt;
	AVPacket *pkt_Out = (AVPacket *)malloc(sizeof(AVPacket));
	av_init_packet(pkt_Out);
	av_new_packet(pkt_Out, pDlg->ffmpeg->y_size);
	while(1){
		if(pDlg->encodeStatus == 1){
			AVStream *in_stream, *out_stream;
			ret = av_read_frame(pDlg->ffmpeg->pFormatCtx_In, &pkt);
			if (ret < 0)
			{
				pDlg->MessageBox(_T("编码完成"));
				//cprintf("Read file over and restart yet\n");
				break;
			}
			if(pkt.stream_index == pDlg->ffmpeg->videoindex){
				//预览
				pDlg->ffmpeg->FFmeg_PlayPacket(&pkt);


				//FIX：No PTS (Example: Raw H.264)
				//Simple Write PTS
				if (pkt.pts == AV_NOPTS_VALUE)
				{
					//Write PTS
					AVRational time_base1 = pDlg->ffmpeg->pFormatCtx_In->streams[pDlg->ffmpeg->videoindex]->time_base;
					//Duration between 2 frames (us)
					int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(pDlg->ffmpeg->pFormatCtx_In->streams[pDlg->ffmpeg->videoindex]->r_frame_rate);
					//Parameters
					pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
					pkt.dts = pkt.pts;
					pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);

					pkt_Out->pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
					pkt_Out->dts = pkt_Out->pts;
					pkt_Out->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
				}
				//Important:Delay

				if (pkt.stream_index == pDlg->ffmpeg->videoindex)
				{
					AVRational time_base = pDlg->ffmpeg->pFormatCtx_In->streams[pDlg->ffmpeg->videoindex]->time_base;
					AVRational time_base_q = {1, AV_TIME_BASE};
					int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
					int64_t now_time = av_gettime() - pDlg->ffmpeg->start_time;
					if (pts_time > now_time)
					{
						av_usleep(pts_time - now_time);
					}
				}
				//Sleep(100);
				in_stream  = pDlg->ffmpeg->pFormatCtx_In->streams[pkt.stream_index];
				out_stream = pDlg->ffmpeg->pFormatCtx_Out->streams[pkt_Out->stream_index];
				/* copy packet */
				//转换PTS/DTS(Convert PTS/DTS)
				pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
				pkt.pos = -1;
				pkt_Out->pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				pkt_Out->dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				pkt_Out->duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
				pkt_Out->pos = -1;
				if (pkt.stream_index == pDlg->ffmpeg->videoindex)
				{
					//printf("Send %8d video frames to output URL\n", frame_index);
					frame_index++;
				}
				//编码
				pDlg->ffmpeg->pFrameYUV->pts = pkt.pts;
				ret = avcodec_encode_video2(pDlg->ffmpeg->pCodecCtx_Enc, pkt_Out,  pDlg->ffmpeg->pFrameYUV, &pDlg->ffmpeg->got_packet);
				if(ret == 0){//编码成功
					cprintf("avcodec_encode_video2 success  frame num is  %d \n", frame_index);
				}else{//编码失败
					cprintf("avcodec_encode_video2 failed frame num is  %d \n", frame_index);
				}
				ret = av_interleaved_write_frame(pDlg->ffmpeg->pFormatCtx_Out, pkt_Out);
				if (ret < 0)
				{
					cprintf("Error muxing packet err = %d\n", ret);
					//break;
				}else{
					if(pkt.stream_index == pDlg->ffmpeg->videoindex){
						cprintf("Send %d video frames to %s\n", frame_index, (LPSTR)(LPCTSTR)pDlg->m_URL);
					}
				}
			}
			av_packet_unref(&pkt);
		}else{
			break;
		}
	}
	return 0;
}
int PushStream_FFmpeg2(LPVOID pM)
{
	CPushStreamToolDlg *pDlg = (CPushStreamToolDlg*)pM;

	int n = pDlg->ffmpeg->FFmpeg_openFile((LPSTR)(LPCTSTR)pDlg->m_SourceFileName);
	if (n != 0){
		cprintf("Open source file failed\n");
		return n;
	}
	cprintf("Key SPS PPS len %d: \n", pDlg->ffmpeg->pCodecCtx_In->extradata_size);
	for(int i = 0; i < pDlg->ffmpeg->pCodecCtx_In->extradata_size; i++){
		cprintf("%02X  ", pDlg->ffmpeg->pCodecCtx_In->extradata[i]);
	}
	cprintf("\n");
	pDlg->ffmpeg->bmp = SDL_CreateYUVOverlay(pDlg->ffmpeg->pCodecCtx_In->width, pDlg->ffmpeg->pCodecCtx_In->height, SDL_YV12_OVERLAY, pDlg->ffmpeg->screen);
	cprintf("Open source file success \n");
	int ret = -1;
	if(pDlg->m_ratio_rtmp.GetCheck()){
		ret = pDlg->ffmpeg->FFmpeg_openOutPutFile((LPSTR)(LPCTSTR)pDlg->m_URL, "flv");
	}else if(pDlg->m_radio_hls.GetCheck()){
		ret = pDlg->ffmpeg->FFmpeg_openOutPutFile((LPSTR)(LPCTSTR)pDlg->m_URL, "hls");
	}
	if(ret != 0){
		cprintf("Open output file failed \n");
		return ret;
	}
	cprintf("\n");
	cprintf("Open output file success \n");
	pDlg->ffmpeg->Encoder_Init();
	cprintf("Encoder SPS PPS: \n");
	for(int i = 0; i < pDlg->ffmpeg->pCodecCtx_Enc->extradata_size; i++){
		cprintf("%02X  ", pDlg->ffmpeg->pCodecCtx_Enc->extradata[i]);
	}
	pDlg->ffmpeg->start_time = av_gettime();
	int video_index = 0;
	int audio_index = 0;
	AVPacket pkt;
	av_init_packet(&pkt);
	sleep(5);
	while(1){
		if(pDlg->pushStatus == 1){
			AVStream *in_stream, *out_stream;
			ret = av_read_frame(pDlg->ffmpeg->pFormatCtx_In, &pkt);
			if (ret < 0)
			{
				ret = av_seek_frame(pDlg->ffmpeg->pFormatCtx_In, pDlg->ffmpeg->videoindex, pDlg->ffmpeg->pFormatCtx_In->start_time, AVSEEK_FLAG_BACKWARD);
				if(ret >= 0){
					cprintf("seek to file start_time success \n");
				}
				pDlg->ffmpeg->start_time = av_gettime();
				video_index = 0;
				av_packet_unref(&pkt);
				//cprintf("Read file over and restart yet\n");
				continue;
			}
			if(pkt.stream_index == pDlg->ffmpeg->videoindex){
				//预览
				cprintf("Preview %d video pkt.pts %ld, pkt.dts %ld, pkt.duration %d, pkt.size %d\n", video_index, pkt.pts, pkt.dts, pkt.duration, pkt.size);
				/*for(int i = 0; i < pDlg->ffmpeg->pCodecCtx_In->extradata_size + 10; i++){
					cprintf("%02X  ", pkt.data[i]);
				}*/
				pDlg->ffmpeg->FFmeg_PlayPacket(&pkt);
			}
			//FIX：No PTS (Example: Raw H.264)
			//Simple Write PTS
			if (pkt.pts == AV_NOPTS_VALUE)
			{
				//Write PTS
				AVRational time_base1 = pDlg->ffmpeg->pFormatCtx_In->streams[pDlg->ffmpeg->videoindex]->time_base;
				//Duration between 2 frames (us)
				int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(pDlg->ffmpeg->pFormatCtx_In->streams[pDlg->ffmpeg->videoindex]->r_frame_rate);
				//Parameters
				pkt.pts = (double)(video_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
				pkt.dts = pkt.pts;
				pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			}
			//Important:Delay
			if (pkt.stream_index == pDlg->ffmpeg->videoindex)
			{
				AVRational time_base = pDlg->ffmpeg->pFormatCtx_In->streams[pDlg->ffmpeg->videoindex]->time_base;
				AVRational time_base_q = {1, AV_TIME_BASE};
				int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
				int64_t now_time = av_gettime() - pDlg->ffmpeg->start_time;
				if (pts_time > now_time)
				{
					av_usleep(pts_time - now_time);
				}
			}
			//Sleep(100);
			in_stream  = pDlg->ffmpeg->pFormatCtx_In->streams[pkt.stream_index];
			out_stream = pDlg->ffmpeg->pFormatCtx_Out->streams[pkt.stream_index];
			/* copy packet */
			//转换PTS/DTS（Convert PTS/DTS）
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;

			if (pkt.stream_index == pDlg->ffmpeg->videoindex)
			{
				//printf("Send %8d video frames to output URL\n", frame_index);
				video_index++;
				cprintf("Send %d video pkt.pts %ld, pkt.dts %ld, pkt.duration %d, pkt.size %d\n", video_index, pkt.pts, pkt.dts, pkt.duration, pkt.size);
				
			}else if(pkt.stream_index == pDlg->ffmpeg->audioindex){
				audio_index++;
				cprintf("Send %d audio pkt->pts %ld, pkt->dts %ld, pkt->duration %d, pkt->size %d\n", audio_index, pkt.pts, pkt.dts, pkt.duration, pkt.size);
			}
			ret = av_write_frame(pDlg->ffmpeg->pFormatCtx_Out, &pkt);
			if (ret < 0)
			{
				cprintf("Error muxing packet err = %d\n", ret);
				//break;
			}
		}
		
		av_packet_unref(&pkt);

	}
	return 0;
}

int PushStream_FFmpeg(LPVOID pM)
{
	CPushStreamToolDlg *pDlg = (CPushStreamToolDlg*)pM;
	//open file
	AVOutputFormat *ofmt = NULL;
	//输入对应一个AVFormatContext，输出对应一个AVFormatContext
	//（Input AVFormatContext and Output AVFormatContext）
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret;
	unsigned int i;
	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;
	av_register_all();
	//Network
	avformat_network_init();
	//输入（Input）
	if ((ret = avformat_open_input(&ifmt_ctx, (LPSTR)(LPCTSTR)pDlg->m_SourceFileName, 0, 0)) < 0) {
		cprintf("Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		cprintf("Failed to retrieve input stream information");
		goto end;
	}
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}
	if(videoindex == -1){
		cprintf("No video streaming was found \n");
		goto end;
	}
	av_dump_format(ifmt_ctx, 0, (LPSTR)(LPCTSTR)pDlg->m_SourceFileName, 0);

	//输出（Output）
	avformat_alloc_output_context2(&ofmt_ctx, ofmt, "flv", (LPSTR)(LPCTSTR)pDlg->m_URL); //rtmp
	if (!ofmt_ctx) {
		cprintf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		//根据输入流创建输出流（Create output AVStream according to input AVStream）
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream)
		{
			cprintf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//复制AVCodecContext的设置（Copy the settings of AVCodecContext）
		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		if (ret < 0)
		{
			cprintf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}  
		//out_stream->codecpar->codec_tag = 0;
		if(ofmt->flags & AVFMT_GLOBALHEADER){
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			if(in_stream->codec->extradata_size > 0){
				int extra_size = (uint64_t)in_stream->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE;
				out_stream->codec->extradata = (uint8_t*)malloc(extra_size);
				memcpy(out_stream->codec->extradata, in_stream->codec->extradata, in_stream->codec->extradata_size);
				out_stream->codec->extradata_size = in_stream->codec->extradata_size;
			}
		}
	}
	av_dump_format(ofmt_ctx, 0, (LPSTR)(LPCTSTR)pDlg->m_URL, 1);
	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx->pb, (LPSTR)(LPCTSTR)pDlg->m_URL, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			cprintf("Could not open output URL '%s'", (LPSTR)(LPCTSTR)pDlg->m_URL);
			goto end;
		}
	}
	//写文件头（Write file header）
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0)
	{
		cprintf("Error occurred when opening output URL\n");
		goto end;
	}
	start_time = av_gettime();
	while(1){
		if(pDlg->pushStatus == 1){
			AVStream *in_stream, *out_stream;
			//获取一个AVPacket（Get an AVPacket）
			ret = av_read_frame(ifmt_ctx, &pkt);
			if (ret < 0)
			{
				ret = av_seek_frame(ifmt_ctx, videoindex, ifmt_ctx->start_time, AVSEEK_FLAG_BACKWARD);
				if(ret >= 0){
					cprintf("seek to file start_time success \n");
				}
				av_packet_unref(&pkt);
				//cprintf("Read file over and restart yet\n");
				continue;
			}
			//FIX：No PTS (Example: Raw H.264)
			//Simple Write PTS
			if (pkt.pts == AV_NOPTS_VALUE)
			{
				//Write PTS
				AVRational time_base1 = ifmt_ctx->streams[videoindex]->time_base;
				//Duration between 2 frames (us)
				int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
				//Parameters
				pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
				pkt.dts = pkt.pts;
				pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			}
			//Important:Delay
			if (pkt.stream_index == videoindex)
			{
				AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
				AVRational time_base_q = {1, AV_TIME_BASE};
				int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
				int64_t now_time = av_gettime() - start_time;
				if (pts_time > now_time)
				{
					av_usleep(pts_time - now_time);
				}
			}
			in_stream  = ifmt_ctx->streams[pkt.stream_index];
			out_stream = ofmt_ctx->streams[pkt.stream_index];
			/* copy packet */
			//转换PTS/DTS（Convert PTS/DTS）
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;

			if (pkt.stream_index == videoindex)
			{
				//printf("Send %8d video frames to output URL\n", frame_index);
				frame_index++;
			}
			ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
			if (ret < 0)
			{
				cprintf("Error muxing packet err = %d\n", ret);
				//break;
			}else{
				cprintf("Send %d video frames to %s\n", frame_index, (LPSTR)(LPCTSTR)pDlg->m_URL);
			}
			av_packet_unref(&pkt);
		}else{
			break;
		}
	}
	//写文件尾（Write file trailer）
	av_write_trailer(ofmt_ctx);
end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) {
		avio_close(ofmt_ctx->pb);
	}
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
	}
	return 0;
}

