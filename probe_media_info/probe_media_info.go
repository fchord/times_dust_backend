package main

/*

#cgo LDFLAGS: -g -lstdc++ -lcpp_lib -lffmpeg_probe -lx264 -L./ -L./ffmpeg_module/build/ -L./ffmpeg_module/lib/x264/ -L./ffmpeg_module/lib/ffmpeg/n4.4/

#include "cpp_lib.h"
#include "ffmpeg_module/inc/ffmpeg_probe.h"

*/
import "C"

import (
	"fmt"
	"errors"
	"net"
	"net/rpc"
	"os"
	//"os/signal"
	"syscall"
)


// 定义服务对象
type ProbeMediaService struct{}

// 定义请求结构
type Args struct {
	MediaPath string `json:"media_path"`
}

// 定义响应结构
type MediaInfo struct {
	Size int64 `json:"size"`
	IsMedia bool `json:"is_media"`
	Format string `json:"format"`
	Duration int64 `json:"duration"`
	Width int `json:"width"`
	Height int `json:"height"`
	BitRate int64 `json:"bitrate"`
	//20240515新增
	VideoFormat string `json:"video_format"`
	VideoIndex int `json:"video_index"`
	VideoBitrate int `json:"video_bitrate"`
	AudioFormat string `json:"audio_format"`
	AudioIndex int `json:"audio_index"`
	AudioBitrate int `json:"audio_bitrate"`

}

// 定义服务方法
func (m *ProbeMediaService) MediaInfo(args *Args, info *MediaInfo) error {
	fmt.Println("args.MediaPath: ", args.MediaPath)
	file_info, err := os.Stat(args.MediaPath)
	if err != nil {
		fmt.Println("Failed to get file info")
		return err
	}
	info.Size = file_info.Size()
	if info.Size <= 0 {
		return errors.New("File size is 0.")
	}

	// ss := C.call_cpplib_work(3)
	// fmt.Println("ss: ", ss)

	ptr := C.open_media(C.CString(args.MediaPath));
	if ptr == nil {
		fmt.Println("Couldn't open the file.")
		info.IsMedia = false
		return errors.New("Couldn't open the file.")
	}
	info.IsMedia = true

	info.Format = C.GoString(C.getFormat(ptr))
	fmt.Println("Format: ", info.Format)

	info.Duration = int64(C.getDuration(ptr))
	fmt.Println("Duration: ", info.Duration)
	
	info.BitRate = int64(C.getBitrate(ptr))
	fmt.Println("BitRate: ", info.BitRate)

	info.Width = int(C.getWidth(ptr))
	fmt.Println("Width: ", info.Width)

	info.Height = int(C.getHeight(ptr))
	fmt.Println("Height: ", info.Height)

	info.VideoFormat = C.GoString(C.getVideoFormat(ptr))
	fmt.Println("VideoFormat: ", info.VideoFormat)

	info.VideoIndex = int(C.getVideoIndex(ptr))
	fmt.Println("VideoIndex: ", info.VideoIndex)

	info.VideoBitrate = int(C.getVideoBitrate(ptr))
	fmt.Println("VideoBitrate: ", info.VideoBitrate)

	info.AudioFormat = C.GoString(C.getAudioFormat(ptr))
	fmt.Println("AudioFormat: ", info.AudioFormat)

	info.AudioIndex = int(C.getAudioIndex(ptr))
	fmt.Println("AudioIndex: ", info.AudioIndex)

	info.AudioBitrate = int(C.getAudioBitrate(ptr))
	fmt.Println("AudioBitrate: ", info.AudioBitrate)

	C.close_media(ptr)
	
	return nil
}

func main() {

	go func() {
		// 创建一个信号通道，用于接收信号
		sigCh := make(chan os.Signal, 1)

		sig := <-sigCh
		fmt.Printf("Child: Received signal %v, exiting...\n", sig)
		if(sig == syscall.SIGINT) {
			os.Exit(0)
		}
	}()

	// 注册ProbeMediaService对象
	ProbeMediaService := new(ProbeMediaService)
	rpc.Register(ProbeMediaService)

	// 创建TCP监听
	listener, err := net.Listen("tcp", ":3002")
	if err != nil {
		panic(err)
	}

	C.init_ffmpeg_probe()
	defer C.uninit_ffmpeg_probe()

	for {
		// 等待客户端连接
		conn, err := listener.Accept()
		if err != nil {
			continue
		}

		// 启动goroutine处理连接
		go rpc.ServeConn(conn)
	}
}