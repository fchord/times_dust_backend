#ifndef FFMPEG_PROBE_H_
#define FFMPEG_PROBE_H_
#ifdef __cplusplus
#include <iostream>

class FfmpegProbe
{
public:
    FfmpegProbe();
    ~FfmpegProbe();
    int Open(std::string media_file);

    int open_ret_ = 0;
    long long bitrate_;
    long long duration_;
    int width_, height_;
    std::string format_;

    //video
    std::string video_format_;
    int video_index_;
    int video_bitrate_;
    //audio
    std::string audio_format_;
    int audio_index_;
    int audio_bitrate_;

private:
    int get_info_from_format_ctx(
        AVFormatContext *format_ctx,
        int index,
        AVStream **stream,
        AVCodecContext **codec_ctx);
    int get_index_from_format_ctx(AVFormatContext *format_ctx, int media_type);

    CDemuxer *demuxer_;
    std::string src_url_;


};

extern "C"
{
#endif
    void init_ffmpeg_probe();
    void uninit_ffmpeg_probe();
    void *open_media(char *media_file);
    void close_media(void *fm);
    const char* getFormat(void *fm);
    long long getBitrate(void *fm);
    long long getDuration(void *fm);
    int getWidth(void *fm);
    int getHeight(void *fm);

    const char *getVideoFormat(void *fm);
    int getVideoIndex(void *fm);
    int getVideoBitrate(void *fm);
    const char *getAudioFormat(void *fm);
    int getAudioIndex(void *fm);
    int getAudioBitrate(void *fm);

#ifdef __cplusplus
}
#endif
#endif /* FFMPEG_PROBE_H_ */
