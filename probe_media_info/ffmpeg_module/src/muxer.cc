#include <spdlog/spdlog.h>
#include "muxer.h"

CMuxer::CMuxer(IListener *listener)
{
	listener_ = listener;
	return;
}

CMuxer::~CMuxer()
{
	return;
}
/*
	key: 输入的avpacket.stream_index
	MuxerParameter: 用于初始化avstream
	out_url: 输出url
	考虑增加 AVOutputFormat**, 以支持内存输出？
*/
int CMuxer::Start(std::unordered_map<int, MuxerParameter> muxer_parameter, std::string out_url)
{
	int ret, i;
	AVStream *stream = NULL;

	muxer_parameter_ = muxer_parameter;
	out_url_ = out_url;

	SPDLOG_INFO("CMuxer Start");

	avformat_alloc_output_context2(&ofmt_ctx, NULL, "mp4", out_url_.c_str());
	if (!ofmt_ctx)
	{
		SPDLOG_WARN("Could not create output context");
		ret = AVERROR_UNKNOWN;
		return -1;
	}

	ofmt_ = ofmt_ctx->oformat;
	// 按照媒体文件中stream的优先顺序依次创建
	for (auto &para : muxer_parameter_)
	{
		create_av_stream(para, AVMEDIA_TYPE_VIDEO);
	}
	for (auto &para : muxer_parameter_)
	{
		create_av_stream(para, AVMEDIA_TYPE_AUDIO);
	}
	for (auto &para : muxer_parameter_)
	{
		create_av_stream(para, AVMEDIA_TYPE_SUBTITLE);
	}

	for (auto &para : muxer_parameter_)
	{
#if 0
        //if(AVMEDIA_TYPE_VIDEO == c->codec_type) {
			//AVCodecParameters* codec_par = avcodec_parameters_alloc();
			//codec_par->codec_id;
			SPDLOG_INFO("para. index: {}, stream time base: {}/{}", para.first, 
				para.second.stream_time_base.num, para.second.stream_time_base.den);
            stream = avformat_new_stream(ofmt_ctx, NULL);
			if(!stream) {
				SPDLOG_WARN("avformat_new_stream failed");
			}
			SPDLOG_INFO("stream index: {}", stream->index);
			/* ret = avcodec_copy_context(video_stream_->codec, c);
			if(ret < 0) {
				SPDLOG_WARN("avcodec_copy_context: {}", ret);
			} */
			ret = avcodec_parameters_copy(stream->codecpar, para.second.codec_par);
			if(ret < 0) {
				SPDLOG_WARN("avcodec_parameters_copy failed: {}", ret);
			}
			ret = avcodec_parameters_to_context(stream->codec, stream->codecpar);
			if(ret < 0) {
				SPDLOG_WARN("avcodec_parameters_to_context failed: {}", ret);
			}
			/* AVRational rational{1, 16000};
			stream->time_base = rational; */
			//stream->time_base = para.second.stream_time_base;
			/* if(AVMEDIA_TYPE_VIDEO == para.second.codec_par->codec_type) {
				stream->time_base = AVRational{1, 16000};
			} else if(AVMEDIA_TYPE_AUDIO == para.second.codec_par->codec_type) {
				stream->time_base = AVRational{1, 44100};
			} */
			muxer_stream_[para.first] = stream;
			AVMEDIA_TYPE_VIDEO;

			//video_stream_->codec->codec_tag = 0;
			//if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			//	video_stream_->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	// AV_CODEC_FLAG_*
#endif
#if 0
        } else if(AVMEDIA_TYPE_AUDIO == c->codec_type) {
            //audio_stream_ = avformat_new_stream(ofmt_ctx, c->codec);
			audio_stream_ = avformat_new_stream(ofmt_ctx, NULL);
			if(!audio_stream_) {
				SPDLOG_WARN("avformat_new_stream failed");
			}
			/* ret = avcodec_copy_context(audio_stream_->codec, c);
			if(ret < 0) {
				SPDLOG_WARN("avcodec_copy_context: {}", ret);
			} */
			ret = avcodec_parameters_copy(audio_stream_->codecpar, c);
			if(ret < 0) {
				SPDLOG_WARN("avcodec_parameters_copy failed: {}", ret);
			}
			ret = avcodec_parameters_to_context(audio_stream_->codec, audio_stream_->codecpar);
			if(ret < 0) {
				SPDLOG_WARN("avcodec_parameters_to_context failed: {}", ret);
			}
			AVRational rational{1, 44100};
			audio_stream_->time_base = rational;
			//audio_stream_->codec->codec_tag = 0;
			//if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			//	audio_stream_->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	// AV_CODEC_FLAG_*			
        }
#endif
	}

	// Open output file
	if (!(ofmt_->flags & AVFMT_NOFILE))
	{
		if (avio_open(&ofmt_ctx->pb, out_url_.c_str(), AVIO_FLAG_WRITE) < 0)
		{
			SPDLOG_WARN("avio_open: failed");
		}
	}
	// Write file header
	if (avformat_write_header(ofmt_ctx, NULL) < 0)
	{
		SPDLOG_WARN("avformat_write_header: failed");
	}

	for (int i = 0; i < ofmt_ctx->nb_streams; i++)
	{
		AVStream *s = ofmt_ctx->streams[i];
		SPDLOG_INFO("muxer stream time_base: {}/{}", s->time_base.num, s->time_base.den);
	}
	for (auto &a : muxer_stream_)
	{
		SPDLOG_INFO("muxer_stream_ time_base: {}/{}", a.second->time_base.num, a.second->time_base.den);
	}

	av_dump_format(ofmt_ctx, 0, out_url.c_str(), 1);

	return 0;
}

int CMuxer::Stop()
{

	int ret = av_write_trailer(ofmt_ctx);
	SPDLOG_INFO("av_write_trailer: {}", ret);
	/* close output */
	if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_close(ofmt_ctx->pb);
	}
	avformat_free_context(ofmt_ctx);
	/* if (ret < 0 && ret != AVERROR_EOF) {
		printf( "Error occurred.\n");
		return -1;
	} */
	return 0;
}

int CMuxer::Process(CPacketWrapper &packet)
{

	int ret;
	AVPacket *pPacket = packet.av_packet;

	if (pPacket)
	{
		if (muxer_stream_.find(pPacket->stream_index) != muxer_stream_.end())
		{
			if (av_cmp_q(muxer_parameter_[pPacket->stream_index].stream_time_base,
						 muxer_stream_[pPacket->stream_index]->time_base))
			{
				pPacket->pts = av_rescale_q(pPacket->pts,
											muxer_parameter_[pPacket->stream_index].stream_time_base, muxer_stream_[pPacket->stream_index]->time_base);
				pPacket->dts = av_rescale_q(pPacket->dts,
											muxer_parameter_[pPacket->stream_index].stream_time_base, muxer_stream_[pPacket->stream_index]->time_base);
				pPacket->duration = av_rescale_q(pPacket->duration,
												 muxer_parameter_[pPacket->stream_index].stream_time_base, muxer_stream_[pPacket->stream_index]->time_base);
			}
			pPacket->stream_index = muxer_stream_[pPacket->stream_index]->index;
		}
		else
		{
			// 未设置到muxer的stream_index包均舍弃
			return -1;
		}
		SPDLOG_INFO("pPacket index: {}, pts: {}, dts: {}, duration: {}", pPacket->stream_index, pPacket->pts, pPacket->dts, pPacket->duration);
	}

	ret = av_interleaved_write_frame(ofmt_ctx, pPacket);
	if (ret < 0)
	{
		SPDLOG_WARN("Error muxing packet. {}", ret);
		return ret;
	}
	listener_->OnFrameReport(NULL);
	return 0;
}

void CMuxer::create_av_stream(std::pair<const int, MuxerParameter> &para, AVMediaType type)
{
	int ret;
	AVMediaType para_type;

	if (para.second.codec_par)
	{
		para_type = para.second.codec_par->codec_type;
	}
	else if (para.second.codec_ctx)
	{
		para_type = para.second.codec_ctx->codec_type;
	}
	else
	{
		para_type = AVMEDIA_TYPE_UNKNOWN;
	}
	SPDLOG_INFO("create_av_stream. type: {}, para_type: {}", type, para_type);
	if (type == para_type)
	{
		SPDLOG_INFO("para. index: {}, stream time base: {}/{}", para.first,
					para.second.stream_time_base.num, para.second.stream_time_base.den);
		AVStream *stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!stream)
		{
			SPDLOG_WARN("avformat_new_stream failed");
		}
		// avstream.index的默认值为，按照avstream创建顺序从0开始递增
		SPDLOG_INFO("stream index: {}", stream->index);
		if (para.second.codec_par)
		{
			ret = avcodec_parameters_copy(stream->codecpar, para.second.codec_par);
		}
		else if (para.second.codec_ctx)
		{
			ret = avcodec_parameters_from_context(stream->codecpar, para.second.codec_ctx);
		}
		else
		{
			return;
		}
		if (ret < 0)
		{
			SPDLOG_WARN("avcodec_parameters_copy failed: {}", ret);
		}
		ret = avcodec_parameters_to_context(stream->codec, stream->codecpar);
		if (ret < 0)
		{
			SPDLOG_WARN("avcodec_parameters_to_context failed: {}", ret);
		}
		stream->time_base = para.second.stream_time_base;
		muxer_stream_[para.first] = stream;
	}
	return;
}

// int avcodec_parameters_from_context(AVCodecParameters *par,
//                                     const AVCodecContext *codec)