
#include "mp4muxer.h"

#include <libavformat/avformat.h>


#define OUTPUT "outputVideo.mp4"

int kk = 0;

AVFormatContext *ofmt_ctx = NULL;
AVStream *out_stream = NULL;
AVFormatContext *ifmt_ctx = NULL;
AVStream *in_stream = NULL;
AVCodecContext *codec_context = NULL;

AVPacket pkt = { 0 };

uint8_t *packet_data = NULL;
int max_size = 0;

void DebugMessageC(const char * message);



void mp4_init(){	
	av_register_all();

	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, OUTPUT);
	if (!ofmt_ctx) {
        DebugMessageC("Could not create output context !");
		return;
    }

	out_stream = avformat_new_stream(ofmt_ctx, NULL);
	
	codec_context = avcodec_alloc_context3(NULL);					
			
	out_stream->codec = codec_context;

	if (avformat_open_input(&ifmt_ctx, "outputVideoH264", 0, 0) < 0) {
		DebugMessageC("Could not open input file ");
	}

	if (avformat_find_stream_info(ifmt_ctx, 0) < 0) {
		DebugMessageC("Failed to retrieve input stream information !!! ");
	}


	if(ifmt_ctx != NULL){
		in_stream = ifmt_ctx->streams[0];

		if(in_stream == NULL){
			DebugMessageC("in_stream ie null !!!");
		}


		if(0 != avcodec_copy_context(out_stream->codec, in_stream->codec)){
			DebugMessageC("avcodec_copy_context went wrong !!!");
		}

	}
	
	if ((ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) && (out_stream->codec != NULL)) {
		out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if(avio_open(&ofmt_ctx->pb, OUTPUT, AVIO_FLAG_WRITE) < 0){
		DebugMessageC("Could not open output file !");
		return;
	}

	if(avformat_write_header(ofmt_ctx, NULL) < 0){
		DebugMessageC("Error occurred when opening output file !");
		return;
	}

	kk = 0;

	av_init_packet(&pkt);

			{
		char message[250];
		sprintf(message,"out_stream->time_base->den: %d out_stream->time_base->num: %d",out_stream->time_base.den,out_stream->time_base.num);
		DebugMessageC(message);
	}
}

void mp4_write_data(unsigned char *data, int size){

	AVRational in_time_base;

	if((packet_data == NULL) || (size > max_size)){
		max_size = size;
		packet_data = (uint8_t *)av_realloc(packet_data, max_size);
	}
	
	memcpy(packet_data, data, size); 

	pkt.duration = 72000 ;
	pkt.pts = kk*pkt.duration;kk++;
	pkt.pos = -1;
	pkt.data = packet_data;
	pkt.size = size;
	pkt.stream_index = 0;

	
	in_time_base.den = 1200000;
	in_time_base.num = 1;

	
	pkt.pts = av_rescale_q_rnd(pkt.pts, in_time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt.duration = av_rescale_q(pkt.duration, in_time_base, out_stream->time_base);

	if(av_interleaved_write_frame(ofmt_ctx, &pkt) < 0){
		DebugMessageC("av_interleaved_write_frame not working");
		return;
	}

}

void mp4_release(){
	if(ofmt_ctx != NULL){
		av_write_trailer(ofmt_ctx);
		avio_closep(&(ofmt_ctx->pb));

		avcodec_close(codec_context);
		
		avformat_free_context(ofmt_ctx);
		ofmt_ctx = NULL;
	}

	if(ifmt_ctx != NULL){

		avformat_free_context(ifmt_ctx);
		ifmt_ctx = NULL;
		in_stream = NULL;
	}

	if(packet_data != NULL){
		av_free(packet_data);
		packet_data = NULL;
		max_size = 0;
	}

}