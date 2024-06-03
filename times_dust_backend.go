package main

import (
	"fmt"
	"io"
	"net/http"
	"mime/multipart"
	"os"
	"os/exec"
	"os/signal"
	"syscall"
	"path/filepath"
	"path"
	"strings"
	"time"
	"log"
	"sync"
	"net/rpc"
	"crypto/md5"
	"container/list"
	"strconv"
	"encoding/json"
	"database/sql"
	_ "github.com/mattn/go-sqlite3"
	"github.com/gorilla/mux"
)

var myPrefixPath = "/times_dust_api"

var transcodeTaskList list.List
var tranTaskMutex sync.Mutex
var transcodeTaskCond *sync.Cond

type TranscodeTask struct {
	video_id string
	status string // pending, executing, completed
	path string // 既是存放临时视频的路径，也是目标路径
	various_streams int // 是否转多分辨率，多分辨率流的数量
	src_name string // 源视频文件名
	src_resolution_w int // 源视频流分辨率
	src_resolution_h int // 源视频流分辨率
	src_video_format string // 源视频流编码格式
	src_video_index int // 源视频流序号
	src_video_bitrate int // 源视频流码率
	src_audio_format string // 源音频流编码格式
	src_audio_index int // 源音频流序号
	src_audio_bitrate int //源音频流码率	
	video_item VideoItem // 写库的信息
}

// var uploadTaskList list.List
var uploadTaskMap map[string]UploadTask
var uploadTaskMutex  sync.Mutex  // 算了，要加锁的地方太多了，估计不会多个视频同时上传。

type UploadTask struct {
	VideoId string `json:"video_id"`
	Size int64 `json:"size"`
	Progress float32 `json:"progress"`
	StoreDir string `json:"store_dir"`
	SegmentInfo []SegmentInfoSt `json:"segment_info"`
}

type SegmentInfoSt struct {
	Index int `json:"index"`
	Start int64 `json:"start"`
	Size int64 `json:"size"`
	Path string `json:"path"`
	Accepted bool `json:"accepted"`
}

type VideoItem struct {
	video_id string
	title string
	original_name string
	local_path string
	local_name string
	description string
	label string
	category string
	upload_time time.Time
	political_sensitivity int
	erotic_sensitivity int 
	width int
	height int
	size int64
	bitrate int
	duration int64	
}

type UploadResultResponse struct {
	Status 			string  `json:"status"`
    VideoId			string	`json:"video_id"`
    UploadTime		string  `json:"upload_time"`
    Size			int64	`json:"size"`
}

type SelectVideoResult struct {
	VideoId		string `json:"video_id"`
	Title 		string `json:"title"`
	NetPathName	string `json:"net_path_name"`
	Category 	string `json:"category"`
}

type RequestVideoResult struct {
	Results 		[]SelectVideoResult `json:"result"`
}

// 定义请求结构
type Args struct {
	MediaPath string
}

// 定义响应结构
type MediaInfo struct {
	Size int64 `json:"size"`
	IsMedia bool `json:"is_media"`
	Format string `json:"format"`
	Duration int64 `json:"duration"`
	Width int `json:"width"`
	Height int `json:"height"`
	BitRate int `json:"bitrate"`
	//20240515新增
	VideoFormat string `json:"video_format"`
	VideoIndex int `json:"video_index"`
	VideoBitrate int `json:"video_bitrate"`
	AudioFormat string `json:"audio_format"`
	AudioIndex int `json:"audio_index"`
	AudioBitrate int `json:"audio_bitrate"`
}


var g_sub_dirctionary = "media/mp4/uncategorized"
var g_nginx_root_prefix = "/data"
var g_file_path_prefix = "/data" + "/" + g_sub_dirctionary

func hashString(input string) string {
	hash := md5.New()                   //创建散列值
	n, err := hash.Write([]byte(input)) //写入待处理字节
	if err != nil {
		fmt.Println(err, n)
		return ""
	}
	bytes := hash.Sum(nil) //获取最终散列值的字符切片
	return fmt.Sprintf("%x", bytes)
}

func currentTimeByString() (time.Time, string) {
	this_time := time.Now()
	t1 := this_time.Year()        //年
	t2 := this_time.Month()       //月
    t3 := this_time.Day()         //日
    t4 := this_time.Hour()        //小时
    t5 := this_time.Minute()      //分钟
    t6 := this_time.Second()      //秒
    t7 := this_time.Nanosecond()  //纳秒 
	current := fmt.Sprintf("%04d-%02d-%02d_%02d-%02d-%02d_%09d", t1, t2, t3, t4, t5, t6, t7)	
	return this_time, current
}

func makeVideoId(original_name, current string) string {
	return hashString(original_name + current)[0:10]
}

func makeStoreDir(current string) string {
	local_path := g_nginx_root_prefix + "/" + g_sub_dirctionary + "/" + current + "/"
	os.Mkdir(local_path, 0755)
	return local_path
}
/* 
	1、检查文件是否为mp4格式 (todo...)
	2、存储文件到目标路径下
	返回值：http访问路径，本地路径，文件名，文件大小
*/
func storeFile(video_id, current string, file_reader io.Reader) (string, string, string, int64) {
	local_name := video_id + ".tmp"
	local_path := g_nginx_root_prefix + "/" + g_sub_dirctionary + "/" + current + "/"
	//local_path := g_file_path_prefix + "/" + current + "/"
	os.Mkdir(local_path, 0755)

	// 创建一个本地文件保存上传的内容
	out, err := os.Create(local_path + local_name)
	if err != nil {
		//http.Error(w, "Failed to create file", http.StatusInternalServerError)
		fmt.Println("Failed to create file")
		return "", "", "", 0
	}
	defer out.Close()	

	// 将上传的文件内容拷贝到本地文件
	_, err = io.Copy(out, file_reader)
	if err != nil {
		//http.Error(w, "Failed to copy file", http.StatusInternalServerError)
		fmt.Println("Failed to copy file")
		return "", "", "", 0
	}
	file_info, err := os.Stat(local_path + local_name)
	if err != nil {
		fmt.Println("Failed to get file info")
		return "", "", "", 0
	}
	http_access_path := "/" + g_sub_dirctionary + "/" + current + "/"
	return http_access_path, local_path, local_name, file_info.Size()
}

/*
	视频信息写数据库
*/
func writeDatabase(item VideoItem) bool {

	// 连接SQLite数据库
	db, err := sql.Open("sqlite3", "./times_dust.db")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// 插入数据
	insertData := `
	INSERT INTO users (
			video_id, title, original_name, local_path, local_name, description, 
			label, category, upload_time, political_sensitivity, erotic_sensitivity,
			width, height, size, bitrate,
			duration) 
	VALUES (?, ?, ?, ?, ?, ?,
			?, ?, ?, ?, ?,
			?, ?, ?, ?, 
			?);
	`
	_, err = db.Exec(
		insertData, item.video_id, item.title, item.original_name, item.local_path, item.local_name, item.description,
		item.label, item.category, item.upload_time, item.political_sensitivity, item.erotic_sensitivity,
		item.width, item.height, item.size, item.bitrate,
		item.duration)
	if err != nil {
		log.Fatal(err)
	}

	return true
}

func uploadHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
	if r.Method != http.MethodPost {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	}

	var local_path = ""
	var local_name = ""
	var video_id = ""
	var local_size int64 = 0

	// fmt.Println("r: ", r)
	add_method := r.FormValue("add_method") // 为什么有时候读出来是空的？网络包里data不是空的
	fmt.Println("add_method: ", add_method)	

	this_time, current := currentTimeByString()

	// multipart.File, *multipart.FileHeader, error
	// var file multipart.File
	// var header *multipart.FileHeader
	file_name := ""

	// Step 1 ：拿到文件
	if ("local" == add_method) {
		// var err error 
		// 解析请求体，获取上传的文件
		/* file, header, err = r.FormFile("file")
		defer func(f multipart.File) {
			if f != nil {
				f.Close()
			}
		}(file)
		if err != nil {
			fmt.Println("Get FormFile file err: ", err)
			http.Error(w, "Failed to parse form", http.StatusBadRequest)
			return
		}
		fmt.Println("header: ", header.Filename)
		file_name = header.Filename */

		video_id = r.FormValue("video_id")
		relative_info_str := r.FormValue("relative_info")
		relative_info, err := strconv.ParseBool(relative_info_str)
		if err != nil {
			relative_info = false
		}
		// 查询本地上传的进度
		if r.Method == http.MethodGet {
			var u UploadTask
			if video_id != "" {
				
				// 1. 查询正在上传的
				/* for e := uploadTaskList.Front(); e != nil; e = e.Next() {
					// do something with e.Value
					var ok bool
					u, ok = e.Value.( UploadTask ) //断言是否为 UploadTask 类型
					if !ok {
						// fmt.Println("It 's not ok")
						continue;
					} else {
						fmt.Println("u: ", u)
					}
					if u.VideoId == video_id {
						upload_task_json , _ :=json.Marshal(u)
						_, _ = w.Write(upload_task_json)
						return 
					}
				} */
				task, exists := uploadTaskMap[video_id]
				if exists {
					upload_task_json , _ :=json.Marshal(task)
					_, _ = w.Write(upload_task_json)
					return 
				}

				// 2. 查询已经入库的
				_, path , _, _ := selectByVideoId(video_id)
				// title, local_path[len(g_nginx_root_prefix):] + local_name, width, height
				if path != "" {
					/* u := &UploadTask{
						VideoId : video_id
						Size : 0
						Progress : 1.0
					} */
					u.VideoId = video_id
					u.Size = 0  // 待查文件大小
					u.Progress = 1.0
					upload_task_json , _ :=json.Marshal(u)
					_, _ = w.Write(upload_task_json)
					return 
				} else {
					// 数据库里也没有
					w.WriteHeader(http.StatusNotFound)
					return 
				}

			} else {
				w.WriteHeader(http.StatusBadRequest)
				return
			}
		} else if r.Method == http.MethodPost {
			// 使用POST来上传
			// video_id 为空被认为是一次新的上传请求
			if video_id == "" {
				file_size_str := r.FormValue("file_size")
				fmt.Println("file_size_str: ", file_size_str)
				file_size, err := strconv.ParseInt(file_size_str, 10, 64)
				if (err != nil) {
					fmt.Printf("Atoi err: %v\n", err)
					file_size = 0
				}				
				fmt.Println("file_size: ", file_size)
				if file_size == 0 {
					// 文件size 为0不接受
					fmt.Println("file_size is 0")
					w.WriteHeader(http.StatusNotAcceptable)
					return 					
				}
				file_name = r.FormValue("file_name")
				if file_name == "" {
					file_name = "Unknown.mp4"
				}
				video_id = makeVideoId(file_name, current)
				store_dir := makeStoreDir(current)
				var u UploadTask
				u.VideoId = video_id
				u.Size = file_size
				u.Progress = 0.0
				u.StoreDir = store_dir
				var si SegmentInfoSt
				DefaultSegmentSize := int64(50 * 1024 * 1024)
				temp_accumulate := int64(0)
				for index := 0; ; index++ {
					if temp_accumulate >= file_size {
						break;
					}
					si.Index = index
					si.Start = temp_accumulate
					if file_size - temp_accumulate >= DefaultSegmentSize {
						si.Size = DefaultSegmentSize
					} else {
						si.Size = file_size - int64(index) * DefaultSegmentSize
						
					}
					temp_accumulate += si.Size
					si.Accepted = false
					u.SegmentInfo = append(u.SegmentInfo, si)
				}
				fmt.Println("u: ", u)
				uploadTaskMap[video_id] = u
				upload_task_json , _ :=json.Marshal(u)
				_, _ = w.Write(upload_task_json)				
				return
			} else if video_id != "" && relative_info == false {
				// video_id 不为空，表示为正在分片上传
				// var u UploadTask
				// 1. 查询正在上传的
				/* for e := uploadTaskList.Front(); e != nil; e = e.Next() {
					var ok bool
					u, ok = e.Value.( UploadTask ) //断言是否为 UploadTask 类型
					if !ok {
						// fmt.Println("It 's not ok")
						continue;
					} else {
						fmt.Println("u: ", u)
					}
					if u.VideoId == video_id {
						upload_task_json , _ :=json.Marshal(u)
						_, _ = w.Write(upload_task_json)
						return 
					}
				} */
				_, exists := uploadTaskMap[video_id]
				if !exists {
					// 上传了一个分片，但并不存在正在上传的文件
					fmt.Println("No the video id is uploading: ", video_id)
					w.WriteHeader(http.StatusNotAcceptable)
					return
				}
				file, header, err := r.FormFile("file")
				defer func(f multipart.File) {
					if f != nil {
						f.Close()
					}
				}(file)
				if err != nil {
					fmt.Println("Get FormFile file err: ", err)
					http.Error(w, "Failed to parse form", http.StatusBadRequest)
					return
				}
				fmt.Println("header: ", header.Filename)
				//file_name = header.Filename

				seg_index_str := r.FormValue("seg_index")
				var seg_index int
				seg_index, err = strconv.Atoi(seg_index_str)
				if err != nil {
					fmt.Println("seg_index_str err: ", seg_index_str)
					w.WriteHeader(http.StatusNotAcceptable)
					return
				}
				if seg_index < 0 || seg_index > len(uploadTaskMap[video_id].SegmentInfo) - 1 {
					fmt.Println("seg_index err: ", seg_index)
					w.WriteHeader(http.StatusNotAcceptable)
					return
				}
				if uploadTaskMap[video_id].SegmentInfo[seg_index].Accepted {
					fmt.Println("Segment already is true")
					w.WriteHeader(http.StatusNotAcceptable)
					return
				}

				// 创建一个本地文件保存上传的内容
				seg_path := uploadTaskMap[video_id].StoreDir + "segment_" + strconv.Itoa(seg_index) + ".tmp"
				fmt.Println("seg_path: ", seg_path)
				seg_file, err := os.Create(seg_path)
				if err != nil {
					fmt.Println("Failed to create file")
					w.WriteHeader(http.StatusInternalServerError)
					return 
				}
				defer func(f *os.File) {
					if f != nil { f.Close() }
				}(seg_file)	

				// 将上传的文件内容拷贝到本地文件
				_, err = io.Copy(seg_file, file)
				if err != nil {
					fmt.Println("Failed to copy file")
					w.WriteHeader(http.StatusInternalServerError)
					return
				}
				temp_file_info, err := os.Stat(seg_path)
				if err != nil {
					fmt.Println("Failed to get file info")
					w.WriteHeader(http.StatusInternalServerError)
					return
				}
				if temp_file_info.Size() != uploadTaskMap[video_id].SegmentInfo[seg_index].Size {
					fmt.Printf("Segment size is not expected. Expected size: %d. Received size: %d\n", 
						uploadTaskMap[video_id].SegmentInfo[seg_index].Size, temp_file_info.Size())
					w.WriteHeader(http.StatusNotAcceptable)
					return
				}
				uploadTaskMap[video_id].SegmentInfo[seg_index].Path = seg_path
				uploadTaskMap[video_id].SegmentInfo[seg_index].Accepted = true
				// 更新 Progress
				byte_accepted := int64(0)
				for i := 0; i < len(uploadTaskMap[video_id].SegmentInfo); i++ {
					if uploadTaskMap[video_id].SegmentInfo[i].Accepted == true {
						byte_accepted += uploadTaskMap[video_id].SegmentInfo[i].Size
					}
				}
				// map value的结构体成员不能直接赋值，因为不可取址，待研究...
				task := uploadTaskMap[video_id]
				task.Progress = float32(float64(byte_accepted) / float64(uploadTaskMap[video_id].Size))
				fmt.Println("task.Progress: ", task.Progress)
				uploadTaskMap[video_id] = task
				upload_task_json , _ :=json.Marshal(uploadTaskMap[video_id])
				_, _ = w.Write(upload_task_json)
				return 

			} else if video_id != "" && relative_info == true {

				// 视频文件分片上传完成，合并分片，上传视频相关信息
				all_accepted := true
				for _, si:= range uploadTaskMap[video_id].SegmentInfo {
					if si.Accepted == false {
						all_accepted = false
					}
				}
				if all_accepted == true {
					task := uploadTaskMap[video_id]
					delete(uploadTaskMap, video_id)
					// 合并全部分片，并且送入转码List
					temp_name := task.VideoId + ".tmp"
					temp_file, err := os.Create(task.StoreDir + temp_name)
					if err != nil {
						fmt.Println("Failed to create file")
						w.WriteHeader(http.StatusInternalServerError)
						return 
					}
					defer func(f *os.File) {
						if f != nil { f.Close() }
					}(temp_file)	

					for _, si:= range task.SegmentInfo {
						fmt.Println("Copy segment. si.Path: ", si.Path)
						seg_file, err := os.Open(si.Path)
						if err != nil {
							fmt.Println("Open file error: ", err)
						}
						// 将分片文件内容拷贝到tmp文件
						_, err = io.Copy(temp_file, seg_file)
						if err != nil {
							fmt.Println("Failed to copy file")
						}
						if seg_file != nil {
							seg_file.Close()
						}
					}
					temp_file.Close()

					temp_file_info, err := os.Stat(task.StoreDir + temp_name)
					if err != nil {
						fmt.Println("Failed to get file info: ", task.StoreDir + temp_name)
					}
					fmt.Println("temp_file_info, Size: ", temp_file_info.Size())
					if temp_file_info.Size() != task.Size {
						fmt.Println("temp_file_info, Size error!")
						return
					}

					for _, si:= range task.SegmentInfo {
						os.Remove(si.Path)
					}

					// local_path, local_name, local_size
					local_path = task.StoreDir
					local_name = temp_name
					local_size = temp_file_info.Size()
					file_name = r.FormValue("file_name")
					if file_name == "" {
						file_name = "Unknown"
					}
					// 后续的流程下面继续走完：检测是否为媒体文件等。
				} else {
					// 分片没收完  handler函数最后怎么处理还要整理一下。
					fmt.Println("Segments upload not finish!")
					w.WriteHeader(http.StatusNotAcceptable)
					return 
				}

			}

		}
		
		// _, local_path, local_name, local_size = storeFile(video_id, current, file)
	
	} else if ("server" == add_method) {
		server_path := r.FormValue("server_path")
		fmt.Println("server_path: ", server_path)
		if(!filepath.IsAbs(server_path)) {
			fmt.Println("No absolutely path.")
			w.WriteHeader(http.StatusNotFound)	
			return 
		}
		file_info, err := os.Stat(server_path)
		if err != nil {
			fmt.Println("os.Stat err: ", err)
			w.WriteHeader(http.StatusNotFound)	
			return		
		} else if(0 == file_info.Size()) {
			fmt.Println("file_info.Size: 0")
			w.WriteHeader(http.StatusNoContent)	
			return		
		} else if(file_info.IsDir()) {
			fmt.Println("file_info.IsDir: true")
			w.WriteHeader(http.StatusNoContent)	
			return		
		} else {
			file_path, file_name := filepath.Split(server_path)
			fmt.Println("Split: ", file_path, file_name)
			src_file, err := os.Open(server_path)
			if(err != nil){
				fmt.Println("os.Open err: ", err)
				w.WriteHeader(http.StatusInternalServerError)	
				return 
			}
			defer src_file.Close()
			video_id := makeVideoId(file_name, current)
			_, local_path, local_name, local_size = storeFile(video_id, current, src_file)
		}
	} else if ("network" == add_method) {
		network_url := r.FormValue("network_url")
		fmt.Println("network_url: ", network_url)
	} else {
		fmt.Println("add_method error.")	
		w.WriteHeader(http.StatusBadRequest)	
		return 
	}

	// Step 2 ： 检测是否为视频文件
	// 连接RPC服务
	client, err := rpc.Dial("tcp", "localhost:3002")
	if err != nil {
		panic(err)
	}
	defer client.Close()

	// 准备请求参数
	args := &Args{MediaPath: local_path + local_name}
	var media_info MediaInfo

	// 调用远程服务方法
	err = client.Call("ProbeMediaService.MediaInfo", args, &media_info)
	if err != nil {
		panic(err)
	}	

	if !media_info.IsMedia  {
		fmt.Println("It 's not a media file.")
		w.WriteHeader(http.StatusNotAcceptable)
		return 
	}
	// {142482024 true mov,mp4,m4a,3gp,3g2,mj2 309000 2560 1440 3688854 vp9 0 3532531 opus 1 129844}
	fmt.Println("media_info: %v", media_info)

	// Step 3 : 准备转码task相关信息
	description := r.FormValue("description")
	title := r.FormValue("title")
	label1 := r.FormValue("label1")
	label2 := r.FormValue("label2")
	label3 := r.FormValue("label3")
	label4 := r.FormValue("label4")
	label5 := r.FormValue("label5")
	label6 := r.FormValue("label6")
	label := ""
	if (label1 != "") {
		label = label1
	}
	if (label2 != "") {
		label = label + ";" + label2
	}
	if (label3 != "") {
		label = label + ";" + label3
	}
	if (label4 != "") {
		label = label + ";" + label4
	}	
	if (label5 != "") {
		label = label + ";" + label5
	}	
	if (label6 != "") {
		label = label + ";" + label6
	}

	category := r.FormValue("category")

	poli_sens_stri := r.FormValue("political_sensitivity")
	poli_sens, err := strconv.Atoi(poli_sens_stri)
	if (err != nil) {
		fmt.Printf("Atoi err: %v\n", err)
		poli_sens = 0
	}

	erot_sens_stri := r.FormValue("erotic_sensitivity")
	erot_sens, err := strconv.Atoi(erot_sens_stri)
	if (err != nil) {
		fmt.Printf("Atoi err: %v\n", err)
		erot_sens = 0
	}

	var item VideoItem
	item.video_id = video_id
	item.title = title
	item.original_name = file_name
	item.local_path = local_path
	item.local_name = local_name
	item.description = description
	item.label = label 
	item.category = category
	item.upload_time = this_time
	item.political_sensitivity = poli_sens
	item.erotic_sensitivity = erot_sens
	item.width = media_info.Width
	item.height = media_info.Height
	item.size = local_size
	item.bitrate = media_info.BitRate
	item.duration = media_info.Duration

	// Step 4 : Push into transcode list，等待异步转码，以及写库
	var task TranscodeTask
	task.video_id = video_id
	task.status = "pending"
	task.path = local_path
	task.various_streams = 0
	task.src_name = local_name
	task.src_resolution_w = media_info.Width
	task.src_resolution_h = media_info.Height
	task.src_video_format = media_info.VideoFormat
	task.src_video_index = media_info.VideoIndex
	task.src_video_bitrate = media_info.VideoBitrate
	task.src_audio_format = media_info.AudioFormat
	task.src_audio_index = media_info.AudioIndex
	task.src_audio_bitrate = media_info.AudioBitrate
	task.video_item = item
	{
		tranTaskMutex.Lock()
		defer tranTaskMutex.Unlock()
		transcodeTaskList.PushBack(task)		
		fmt.Println("Broadcast")
		transcodeTaskCond.Broadcast() // 通知转码线程有新的转码任务
	}

/* 
	if ( media_info.Format == "mov,mp4,m4a,3gp,3g2,mj2") {
		// 太大就转码为m3u8
		if ( media_info.Size >= 1 * 1024 * 1024 ) {

			//  	ffmpeg -i rtmp://127.0.0.1:1935/stream/example -hls_time 2 -hls_list_size 3 -hls_segment_type mpegts -f hls  /data/media/hls/live/a.m3u8
			local_suffix := path.Ext(local_name)
			local_prefix := local_name[0 : len(local_name) - len(local_suffix)] // temp name

			src := local_path + local_name
			m3u8_suffix := "_.m3u8"
			dst := local_path + "vs%v/" + local_prefix + m3u8_suffix
			//cmd_str := "ffmpeg -i " + src + " -c copy -hls_list_size 0 -hls_segment_type mpegts -f hls -y " + dst
			//fmt.Println("cmd_str: ", cmd_str)

			// 启动子进程
			// 进程名和参数一定要一个对应一个字符串，不可拼接！
			// cmd := exec.Command("ffmpeg", "-i", src, "-c", "copy", "-hls_list_size", "0", "-hls_segment_type", "mpegts", "-f", "hls", "-y", dst)
			cmd := exec.Command("ffmpeg", "-i", src, "-vcodec", "h264_nvenc", "-b:v:0", "4000k", "-b:v:1", "2000k", "-b:v:2", "1000k", "-b:a:0", "128k", "-b:a:1", "64k", "-b:a:2", "32k", "-map", "0:v", "-map", "0:a", "-map", "0:v", "-map", "0:a", "-map", "0:v", "-map", "0:a", "-hls_list_size", "0", "-hls_segment_type", "mpegts", "-f", "hls", "-master_pl_name", "master.m3u8", "-var_stream_map", "v:0,a:0 v:1,a:1 v:2,a:2", "-y", dst)
			cmd.Stdout = os.Stdout
			cmd.Stderr = os.Stderr
			fmt.Println("cmd.Path: ", cmd.Path)
			fmt.Println("cmd.Args: ", cmd.Args)

			// 设置子进程的环境变量
			//cmd.Env = []string{"LD_LIBRARY_PATH=" + pwd + "/probe_media_info/ffmpeg_module/lib/ffmpeg:"}

			// 启动子进程
			// 应该改成异步，先返回，再在转码完了之后写库，不然HTTP会超时504 ！！！
			err := cmd.Start()
			if err != nil {
				fmt.Println("Error starting child process:", err)
			}
			// 等待子进程退出
			err = cmd.Wait()
			if err != nil {
				fmt.Println("FFmpeg Error waiting for child process:", err)
				//os.Exit(0)
				//return
			}
			fmt.Println("FFmpeg Child process exited successfully.")
			// os.Remove(local_path + local_name)
			//local_name = local_prefix + m3u8_suffix
			local_name = "master.m3u8"

		} else {
			// 不很大就改个名
			// 是mp4就把后缀.tmp改为mp4
			fmt.Println("It is mp4 file.")
			origin_suffix := path.Ext(file_name) // original suffix
			fmt.Println("origin_suffix: %s", origin_suffix)

			local_suffix := path.Ext(local_name)
			fmt.Println("local_suffix: %s", local_suffix)
			local_prefix := local_name[0 : len(local_name) - len(local_suffix)] // temp name
			fmt.Println("local_prefix: %s", local_prefix)
			tar_suffix := ""
			origin_suffix_lower := strings.ToLower(origin_suffix)
			if( origin_suffix_lower == ".mp4" ||
				origin_suffix_lower == ".mov" ||
				origin_suffix_lower == ".m4a" ||
				origin_suffix_lower == ".3gp" ||
				origin_suffix_lower == ".3g2" ||
				origin_suffix_lower == ".mj2"){
				tar_suffix = origin_suffix_lower
			} else {
				tar_suffix = ".mp4"
			}
			tar_name := local_prefix + tar_suffix
			fmt.Println("tar_name: %s", tar_name)
			// 重命名
			os.Rename(local_path + local_name, local_path + tar_name)
			local_name = tar_name
		}
	} else if ( media_info.Format == "matroska,webm") {
		
	} */

	result_resp := UploadResultResponse {
		Status : "Created",
		VideoId : video_id,
		UploadTime : this_time.String(),
		Size : local_size,
	}
	fmt.Printf("result_resp: %v", result_resp)
	result, err := json.Marshal(result_resp)
	//fmt.Printf("result: %v", result)
	if(err != nil){
		fmt.Println("err: ", err)
	}
	_, err = w.Write(result)
	// 返回成功响应. 写了 w.Write(result)，默认Status就是200OK，不需要再设状态。设了非200OK状态就不能写result。二选一。
	// w.WriteHeader(http.StatusOK)
}

func liveHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
	if r.Method != http.MethodPost {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	}

	// 解析请求体，获取上传的文件
/* 	file, header, err := r.FormFile("file")
	if err != nil {
		http.Error(w, "Failed to parse form", http.StatusBadRequest)
		return
	}
	defer file.Close()

	fmt.Println("header: ", header) */

	description := r.FormValue("description")
	fmt.Println("description: ", description)
	title := r.FormValue("title")
	fmt.Println("title: ", title)
	src_url := r.FormValue("src_url")
	fmt.Println("src_url: ", src_url)
	
	this_time, _ := currentTimeByString()
	result_resp := UploadResultResponse {
		Status : "Created",
		VideoId : "asdfdf",
		UploadTime : this_time.String(),
		Size : 1,
	}
	result, _ := json.Marshal(result_resp)
	//fmt.Printf("result: %v", result)
	_, _ = w.Write(result)

	w.WriteHeader(http.StatusOK)
}

func selectByVideoId(video_id string) (string, string, int, int) {
	// 连接SQLite数据库
	db, err := sql.Open("sqlite3", "./times_dust.db")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()
	
	// 查询数据
	rows, err := db.Query("SELECT title, local_path, local_name, width, height FROM users where video_id = \"" + video_id + "\"")
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()

	title := ""
	local_path := ""
	local_name := ""
	width := 0
	height := 0
	// 遍历结果集
	for rows.Next() {
		err := rows.Scan(&title, &local_path, &local_name, &width, &height)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("title: %s, local_path: %s, local_name: %s\n", 
			title, local_path, local_name)
		break
	}
	if local_name == "" {
		return "", "", 0, 0
	} else {
		return title, local_path[len(g_nginx_root_prefix):] + local_name, width, height
	}
}

func requestHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
/* 	if r.Method != http.MethodGet {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	} */
	fmt.Printf("requestHandler called.\n")
	video_id_requ := r.FormValue("video_id")
	title, video_path, width, height := selectByVideoId(video_id_requ)
	if video_path == "" {
		//resp_json := "{\"title\" : \"\", \"video_path\" : \"\"}"
		//w.Write([]byte(resp_json))
		w.WriteHeader(http.StatusNotFound)
	} else {
		resp_json := "{\"title\" : \"" + title + "\", \"video_path\" : \"" + video_path + 
					"\", \"width\" : " + strconv.Itoa(width) + ", \"height\" : " + strconv.Itoa(height)  + "}"
		fmt.Println("resp_json: %v", resp_json)

		w.Write([]byte(resp_json))
		//w.WriteHeader(http.StatusOK)
	}
}

func selectAllVideos() ([]SelectVideoResult) {
	// 连接SQLite数据库
	db, err := sql.Open("sqlite3", "./times_dust.db")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()
	
	// 查询数据
	rows, err := db.Query("SELECT video_id, title, local_path, local_name, category FROM users")
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()
	
	// var video_ids []string
	// var titles []string
	// var video_paths []string
	var results []SelectVideoResult
	var result SelectVideoResult
	video_id := ""
	title := ""
	local_path := ""
	local_name := ""
	category := ""
	// 遍历结果集
	for rows.Next() {
		err := rows.Scan(&video_id, &title, &local_path, &local_name, &category)
		if err != nil {
			log.Fatal(err)
			break
		}
		// fmt.Printf("video_id: %s, title: %s, local_path: %s, local_name: %s\n", video_id, title, local_path, local_name)
		// video_ids.append(video_id)
		// titles.append(title)
		// video_paths.append(local_path[len(g_nginx_root_prefix):] + local_name)
		if video_id == "" {
			continue;
		}
			
		result.VideoId = video_id
		result.Title = title
		if len(local_path) > len(g_nginx_root_prefix) {
			result.NetPathName = local_path[len(g_nginx_root_prefix):] + local_name
		}
		result.Category = category
		results = append(results, result)
	}
	return results
}

func videoHandler(w http.ResponseWriter, r *http.Request) {
	results := selectAllVideos()
	res := RequestVideoResult{
		Results: results,
	}
	m, _ := json.Marshal(res)
	// fmt.Println(string(m))
	w.Write([]byte(string(m)))
	// w.WriteHeader(http.StatusOK)
}

func deleteVideo(video_id string) bool {
	// 连接SQLite数据库
	db, err := sql.Open("sqlite3", "./times_dust.db")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()
	
	// 查询数据
	row := db.QueryRow("SELECT title, local_path, local_name FROM users where video_id = \"" + video_id + "\"")

	title := ""
	local_path := ""
	local_name := ""
	// 遍历结果集
	err = row.Scan(&title, &local_path, &local_name)
	if err != nil {
		log.Fatal(err)
		fmt.Printf("deleteVideo. No video: %s\n", video_id)
		return false
	}
	fmt.Printf("deleteVideo. title: %s, local_path: %s, local_name: %s\n", 
		title, local_path, local_name)
	db.Exec("DELETE FROM users WHERE video_id = \"" + video_id + "\"")

	err = os.RemoveAll(local_path)
	if err != nil{
		log.Fatalf("remove file failed err=%s\n", err)
		return false
	}
	return true
}

func deleteHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
/* 	if r.Method != http.MethodGet {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	} */
	fmt.Printf("deleteHandler called. r:\n", r)
	video_id_requ := r.FormValue("video_id")
	fmt.Printf("deleteHandler. video_id_requ: %s\n", video_id_requ)
	b := deleteVideo(video_id_requ)
	if b {
		w.WriteHeader(http.StatusOK)
	} else {
		w.WriteHeader(http.StatusNoContent)
	}
	return
}

//defaultHandler
func defaultHandler(w http.ResponseWriter, r *http.Request) {
	fmt.Printf("defaultHandler called. r:\n", r)
	w.WriteHeader(http.StatusOK)
	return
}

func runProbe() {

}

func checkNvidiaGPU() bool {
	// 执行命令并获取输出
	cmd := exec.Command("nvidia-smi", "-L")
	output, err := cmd.Output()
	if err != nil {
		fmt.Println("Failed to execute command:", err)
		return false
	}

	// 将命令输出转换为字符串
	outputStr := string(output)

	// 检查输出中是否包含 "NVIDIA"
	if strings.Contains(outputStr, "NVIDIA") {
		fmt.Println("Nvidia GPU is installed.")
		return true
	} else {
		fmt.Println("Nvidia GPU is not installed.")
		return false
	}	
}


// execTranscode executes a transcoding task using FFmpeg.
//
// The function takes a TranscodeTask as input and performs the following steps:
//   1. Updates the status of the task to "executing".
//   2. Extracts the local path and name from the task.
//   3. Determines the local suffix and prefix based on the local name.
//   4. Constructs the source and destination paths for the transcoding process.
//   5. Sets the FFmpeg command with the appropriate parameters based on the task's source resolution and preset resolutions.
//   6. Starts the FFmpeg process and waits for it to complete.
//   7. Removes the source file.
//   8. Updates the local name of the video item in the task.
//   9. Writes the video item to the database.
//
// The function does not return any value.
func execTranscode(task TranscodeTask) {
	task.status = "executing"

	local_path := task.path // 存储路径 "/data/media/mp4/uncategorized/2024-05-17_17-50-27_610695148/"
	local_name := task.src_name // "7179bff8f9.tmp"

	local_suffix := path.Ext(local_name) // 例如，src_name = "7179bff8f9.tmp", 获取到后缀 ".tmp"
	local_prefix := local_name[0 : len(local_name) - len(local_suffix)] // 获取到前缀 "7179bff8f9"

	src := local_path + local_name
	m3u8_suffix := "_.m3u8"
	dst := local_path + "vs%v/" + local_prefix + m3u8_suffix


	// 设置转码参数。 确定分辨率
/* 	resolutions_preset :=   []int    {  1440,  1080,   720,   640,    480,    320,    240,    144}
	video_bitrate_preset := []string { "10M",  "4M",   "1M",  "800k", "600k", "300k", "200k", "100k"}
	audio_bitrate_preset := []string { "256k", "128k", "96k", "64k",  "48k",  "32k",  "24k",  "24k"} */
	resolutions_preset :=   []int    {  1080,   720,    480,     240}
	video_bitrate_preset := []string { "4M",   "1M",   "700k",  "400k"}
	audio_bitrate_preset := []string { "128k", "96k",  "64k",   "64k"}	
	fmt.Println("resolutions_preset: ", resolutions_preset)
	short := task.src_resolution_h
	short_side_is_height := true
	if task.src_resolution_w < task.src_resolution_h {
		short = task.src_resolution_w
		short_side_is_height = false
	}
	fmt.Println("short: ", short)

	var ffmpeg_cmd []string
	ffmpeg_cmd = append(ffmpeg_cmd, "-i")
	ffmpeg_cmd = append(ffmpeg_cmd, src)

	// Hardware 
	if checkNvidiaGPU() == true {
		ffmpeg_cmd = append(ffmpeg_cmd, "-vcodec")
		ffmpeg_cmd = append(ffmpeg_cmd, "h264_nvenc")
	}
	temp_cmd := ""
	i := 0
	j := 0
	for ; i < len(resolutions_preset); i++ {
		fmt.Println("i: ", resolutions_preset[i])
		if resolutions_preset[i] <= short {
			ffmpeg_cmd = append(ffmpeg_cmd, "-b:v:" + strconv.Itoa(j))
			ffmpeg_cmd = append(ffmpeg_cmd, video_bitrate_preset[i])
			ffmpeg_cmd = append(ffmpeg_cmd, "-s:v:" + strconv.Itoa(j))
			if short_side_is_height == true {
				long_side := resolutions_preset[i] * task.src_resolution_w / task.src_resolution_h
				if long_side % 2 != 0 {
					long_side += 1
				}
				temp_cmd = strconv.Itoa(long_side) + "x" + strconv.Itoa(resolutions_preset[i])
			} else {
				long_side := resolutions_preset[i] * task.src_resolution_h / task.src_resolution_w
				if long_side % 2 != 0 {
					long_side += 1
				}
				temp_cmd =  strconv.Itoa(resolutions_preset[i]) + "x" +  strconv.Itoa(long_side)
			}
			ffmpeg_cmd = append(ffmpeg_cmd, temp_cmd)
			ffmpeg_cmd = append(ffmpeg_cmd, "-b:a:" + strconv.Itoa(j))
			ffmpeg_cmd = append(ffmpeg_cmd, audio_bitrate_preset[i])
			j += 1
		}
	}
	ffmpeg_cmd = append(ffmpeg_cmd, "-ac")
	ffmpeg_cmd = append(ffmpeg_cmd, "2")	
	//"-map", "0:v", "-map", "0:a", 
	k := 0
	for ; k < j; k++ {
		ffmpeg_cmd = append(ffmpeg_cmd, "-map")
		ffmpeg_cmd = append(ffmpeg_cmd, "0:" + strconv.Itoa(task.src_video_index))
		ffmpeg_cmd = append(ffmpeg_cmd, "-map")
		ffmpeg_cmd = append(ffmpeg_cmd, "0:" + strconv.Itoa(task.src_audio_index))
	}
	// "-hls_list_size", "0" "-hls_segment_type", "mpegts", "-f", "hls"
	ffmpeg_cmd = append(ffmpeg_cmd, "-hls_list_size")
	ffmpeg_cmd = append(ffmpeg_cmd, "0")
	ffmpeg_cmd = append(ffmpeg_cmd, "-hls_segment_type")
	ffmpeg_cmd = append(ffmpeg_cmd, "mpegts")
	ffmpeg_cmd = append(ffmpeg_cmd, "-f")
	ffmpeg_cmd = append(ffmpeg_cmd, "hls")
	// "-master_pl_name", "master.m3u8"
	ffmpeg_cmd = append(ffmpeg_cmd, "-master_pl_name")
	ffmpeg_cmd = append(ffmpeg_cmd, "master.m3u8")
	// "-var_stream_map", "v:0,a:0 v:1,a:1 v:2,a:2", "-y", dst
	ffmpeg_cmd = append(ffmpeg_cmd, "-var_stream_map")
	temp_cmd = ""
	for k = 0; k < j; k++ {
		temp_cmd += "v:" + strconv.Itoa(k) + ",a:" + strconv.Itoa(k)
		if k != j - 1 {
			temp_cmd += " "
		}
	}
	ffmpeg_cmd = append(ffmpeg_cmd, temp_cmd)
	ffmpeg_cmd = append(ffmpeg_cmd, "-y")
	ffmpeg_cmd = append(ffmpeg_cmd, dst)
	fmt.Println("ffmpeg_cmd: ", ffmpeg_cmd)

	//cmd_str := "ffmpeg -i " + src + " -c copy -hls_list_size 0 -hls_segment_type mpegts -f hls -y " + dst
	//fmt.Println("cmd_str: ", cmd_str)

	// 启动子进程
	// 进程名和参数一定要一个对应一个字符串，不可拼接！
	// cmd := exec.Command("ffmpeg", "-i", src, "-c", "copy", "-hls_list_size", "0", "-hls_segment_type", "mpegts", "-f", "hls", "-y", dst)
	//cmd := exec.Command("ffmpeg", "-i", src,  "-b:v:0", "4000k", "-b:v:1", "2000k", "-b:v:2", "1000k", "-b:a:0", "128k", "-b:a:1", "64k", "-b:a:2", "32k", "-map", "0:v", "-map", "0:a", "-map", "0:v", "-map", "0:a", "-map", "0:v", "-map", "0:a", "-hls_list_size", "0", "-hls_segment_type", "mpegts", "-f", "hls", "-master_pl_name", "master.m3u8", "-var_stream_map", "v:0,a:0 v:1,a:1 v:2,a:2", "-y", dst)
	cmd := exec.Command("ffmpeg", ffmpeg_cmd...)
	// "-vcodec", "h264_nvenc",
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	fmt.Println("cmd.Path: ", cmd.Path)
	fmt.Println("cmd.Args: ", cmd.Args)

	// 设置子进程的环境变量
	//cmd.Env = []string{"LD_LIBRARY_PATH=" + pwd + "/probe_media_info/ffmpeg_module/lib/ffmpeg:"}

	// 启动子进程
	// 应该改成异步，先返回，再在转码完了之后写库，不然HTTP会超时504 ！！！
	err := cmd.Start()
	if err != nil {
		fmt.Println("Error starting child process:", err)
	}
	// 等待子进程退出
	err = cmd.Wait()
	if err != nil {
		fmt.Println("FFmpeg Error waiting for child process:", err)
		//os.Exit(0)
		//return
	}
	fmt.Println("FFmpeg Child process exited successfully.")
	os.Remove(local_path + local_name)
	//local_name = local_prefix + m3u8_suffix
	local_name = "master.m3u8"
	task.video_item.local_name = local_name
	
	is_inserted := writeDatabase(task.video_item)
	if (!is_inserted) {
		fmt.Printf("Insert database failed.")
	}
	
	// 处理文本描述，这里简单打印到控制台
	// fmt.Printf("File %s uploaded successfully! Title: %s, Description: %s\n", file_name, title, description)
	return
}

func checkTableExists(db *sql.DB, tableName string) bool {
	query := fmt.Sprintf("SELECT name FROM sqlite_master WHERE type='table' AND name='%s'", tableName)
	rows, err := db.Query(query)
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()

	if rows.Next() {
		return true
	}

	return false
}


// init initializes the database and creates the "users" table if it doesn't exist.
//
// No parameters.
// No return value.
func global_init() {

	transcodeTaskCond = sync.NewCond(&tranTaskMutex)

	createTable := `
	CREATE TABLE IF NOT EXISTS users (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		video_id TEXT,
		title TEXT,
		original_name TEXT,
		local_path TEXT,
		local_name TEXT,
		description TEXT,
		label TEXT,
		category TEXT,
		upload_time TIMESTAMP,
		political_sensitivity INTEGER,
		erotic_sensitivity INTEGER,
		width INTEGER,
		height INTEGER,
		size BIGINT,
		bitrate INTEGER,
		duration BIGINT
	);
	`

	// 检查数据库文件是否存在
	databasePath := "./times_dust.db"
	if _, err := os.Stat(databasePath); os.IsNotExist(err) {
		// 创建数据库
		db, err := sql.Open("sqlite3", databasePath)
		if err != nil {
			log.Fatal(err)
		}
		defer db.Close()

		// 创建表
		_, err = db.Exec(createTable)
		if err != nil {
			log.Fatal(err)
		}

		fmt.Println("数据库和表创建成功")
	} else {
		// 打开数据库连接
		db, err := sql.Open("sqlite3", databasePath)
		if err != nil {
			log.Fatal(err)
		}
		defer db.Close()

		// 检查表是否存在
		tableExists := checkTableExists(db, "users")
		if !tableExists {
			_, err = db.Exec(createTable)
			if err != nil {
				log.Fatal(err)
			}

			fmt.Println("表创建成功")
		} else {
			fmt.Println("表已存在")
		}
	}
}


func videoIdGetHandler(w http.ResponseWriter, r *http.Request) {
	// 获取 URL 参数
	vars := mux.Vars(r)
	videoID := vars["video_id"]
	//videoID := "Dynamic Video ID For Test."

	// 在这里处理 video_id
	// fmt.Fprintf(w, "Video ID: %s\n", videoID)

	// 限制请求方法为 POST
	if r.Method != http.MethodGet {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	}
	fmt.Printf("requestHandler called.\n")
	//video_id_requ := r.FormValue("video_id")
	title, video_path, width, height := selectByVideoId(videoID)
	if video_path == "" {
		//resp_json := "{\"title\" : \"\", \"video_path\" : \"\"}"
		//w.Write([]byte(resp_json))
		w.WriteHeader(http.StatusNotFound)
	} else {
		resp_json := "{\"title\" : \"" + title + "\", \"video_path\" : \"" + video_path + 
					"\", \"width\" : " + strconv.Itoa(width) + ", \"height\" : " + strconv.Itoa(height)  + "}"
		fmt.Println("resp_json: ", resp_json)
		// w.WriteHeader(http.StatusOK)
		//m, _ := json.Marshal(resp_json)
		// fmt.Println(string(m))
		//w.Write([]byte(string(m)))

		w.Write([]byte(resp_json))
		//w.WriteHeader(http.StatusOK)
	}	
}

func videoIdDeleteHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
/* 	if r.Method != http.MethodGet {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	} */
	fmt.Printf("videoIdDeleteHandler called. r:\n", r)
	vars := mux.Vars(r)
	videoID := vars["video_id"]
	// video_id_requ := r.FormValue("video_id")
	video_id_requ := videoID
	fmt.Printf("videoIdDeleteHandler. video_id_requ: %s\n", video_id_requ)
	b := deleteVideo(video_id_requ)
	if b {
		w.WriteHeader(http.StatusOK)
	} else {
		w.WriteHeader(http.StatusNoContent)
	}
	return
}


// 客户端提出上传请求，服务端决策分片方案
func videoPostHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
	if r.Method != http.MethodPost {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	}
	//var local_path = ""
	//var local_name = ""
	var video_id = ""
	//var local_size int64 = 0

	add_method := r.FormValue("add_method") // 为什么有时候读出来是空的？网络包里data不是空的
	fmt.Println("add_method: ", add_method)	

	_, current := currentTimeByString()

	file_name := ""

	file_size_str := r.FormValue("file_size")
	fmt.Println("file_size_str: ", file_size_str)
	file_size, err := strconv.ParseInt(file_size_str, 10, 64)
	if (err != nil) {
		fmt.Printf("Atoi err: %v\n", err)
		file_size = 0
	}				
	fmt.Println("file_size: ", file_size)
	if file_size == 0 {
		// 文件size 为0不接受
		fmt.Println("file_size is 0")
		w.WriteHeader(http.StatusNotAcceptable)
		return 					
	}
	file_name = r.FormValue("file_name")
	if file_name == "" {
		file_name = "Unknown.mp4"
	}
	video_id = makeVideoId(file_name, current)
	store_dir := makeStoreDir(current)
	var u UploadTask
	u.VideoId = video_id
	u.Size = file_size
	u.Progress = 0.0
	u.StoreDir = store_dir
	var si SegmentInfoSt
	DefaultSegmentSize := int64(50 * 1024 * 1024)
	temp_accumulate := int64(0)
	for index := 0; ; index++ {
		if temp_accumulate >= file_size {
			break;
		}
		si.Index = index
		si.Start = temp_accumulate
		if file_size - temp_accumulate >= DefaultSegmentSize {
			si.Size = DefaultSegmentSize
		} else {
			si.Size = file_size - int64(index) * DefaultSegmentSize
			
		}
		temp_accumulate += si.Size
		si.Accepted = false
		u.SegmentInfo = append(u.SegmentInfo, si)
	}
	fmt.Println("u: ", u)
	uploadTaskMap[video_id] = u
	upload_task_json , _ :=json.Marshal(u)
	_, _ = w.Write(upload_task_json)				
	return

}

func segmentPost(w http.ResponseWriter, r *http.Request, video_id string) {
	_, exists := uploadTaskMap[video_id]
	if !exists {
		// 上传了一个分片，但并不存在正在上传的文件
		fmt.Println("No the video id is uploading: ", video_id)
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}
	file, header, err := r.FormFile("file")
	defer func(f multipart.File) {
		if f != nil {
			f.Close()
		}
	}(file)
	if err != nil {
		fmt.Println("Get FormFile file err: ", err)
		http.Error(w, "Failed to parse form", http.StatusBadRequest)
		return
	}
	fmt.Println("header: ", header.Filename)
	//file_name = header.Filename

	seg_index_str := r.FormValue("seg_index")
	var seg_index int
	seg_index, err = strconv.Atoi(seg_index_str)
	if err != nil {
		fmt.Println("seg_index_str err: ", seg_index_str)
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}
	if seg_index < 0 || seg_index > len(uploadTaskMap[video_id].SegmentInfo) - 1 {
		fmt.Println("seg_index err: ", seg_index)
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}
	if uploadTaskMap[video_id].SegmentInfo[seg_index].Accepted {
		fmt.Println("Segment already is true")
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}

	// 创建一个本地文件保存上传的内容
	seg_path := uploadTaskMap[video_id].StoreDir + "segment_" + strconv.Itoa(seg_index) + ".tmp"
	fmt.Println("seg_path: ", seg_path)
	seg_file, err := os.Create(seg_path)
	if err != nil {
		fmt.Println("Failed to create file")
		w.WriteHeader(http.StatusInternalServerError)
		return 
	}
	defer func(f *os.File) {
		if f != nil { f.Close() }
	}(seg_file)	

	// 将上传的文件内容拷贝到本地文件
	_, err = io.Copy(seg_file, file)
	if err != nil {
		fmt.Println("Failed to copy file")
		w.WriteHeader(http.StatusInternalServerError)
		return
	}
	temp_file_info, err := os.Stat(seg_path)
	if err != nil {
		fmt.Println("Failed to get file info")
		w.WriteHeader(http.StatusInternalServerError)
		return
	}
	if temp_file_info.Size() != uploadTaskMap[video_id].SegmentInfo[seg_index].Size {
		fmt.Printf("Segment size is not expected. Expected size: %d. Received size: %d\n", 
			uploadTaskMap[video_id].SegmentInfo[seg_index].Size, temp_file_info.Size())
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}
	uploadTaskMap[video_id].SegmentInfo[seg_index].Path = seg_path
	uploadTaskMap[video_id].SegmentInfo[seg_index].Accepted = true
	// 更新 Progress
	byte_accepted := int64(0)
	for i := 0; i < len(uploadTaskMap[video_id].SegmentInfo); i++ {
		if uploadTaskMap[video_id].SegmentInfo[i].Accepted == true {
			byte_accepted += uploadTaskMap[video_id].SegmentInfo[i].Size
		}
	}
	// map value的结构体成员不能直接赋值，因为不可取址，待研究...
	task := uploadTaskMap[video_id]
	task.Progress = float32(float64(byte_accepted) / float64(uploadTaskMap[video_id].Size))
	fmt.Println("task.Progress: ", task.Progress)
	uploadTaskMap[video_id] = task
	upload_task_json , _ :=json.Marshal(uploadTaskMap[video_id])
	_, _ = w.Write(upload_task_json)
	return 

}

func checkMediaInfo(args *Args) MediaInfo {
	// 连接RPC服务
	client, err := rpc.Dial("tcp", "localhost:3002")
	if err != nil {
		panic(err)
	}
	defer client.Close()

	var media_info MediaInfo

	// 调用远程服务方法
	err = client.Call("ProbeMediaService.MediaInfo", args, &media_info)
	if err != nil {
		panic(err)
	}	
	return media_info
}

// 客户端上传分片，以及提出合并请求，服务端检测是否为视频文件
func videoIdPostHandler(w http.ResponseWriter, r *http.Request) {
	// 限制请求方法为 POST
	if r.Method != http.MethodPost {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	}
		vars := mux.Vars(r)
	videoID := vars["video_id"]
	video_id := videoID

	var local_path = ""
	var local_name = ""
	var local_size int64 = 0
	var file_name = ""

	relative_info_str := r.FormValue("relative_info")
	relative_info, err := strconv.ParseBool(relative_info_str)
	if err != nil {
		relative_info = false
	}

	if relative_info == false {
		// 上传分片
		segmentPost(w, r, video_id)
		return 
	} else {
		// relative_info is true
		// 视频文件分片上传完成，合并分片，上传视频相关信息
		all_accepted := true
		for _, si:= range uploadTaskMap[video_id].SegmentInfo {
			if si.Accepted == false {
				all_accepted = false
			}
		}
		if all_accepted == true {
			task := uploadTaskMap[video_id]
			delete(uploadTaskMap, video_id)
			// 合并全部分片，并且送入转码List
			temp_name := task.VideoId + ".tmp"
			temp_file, err := os.Create(task.StoreDir + temp_name)
			if err != nil {
				fmt.Println("Failed to create file")
				w.WriteHeader(http.StatusInternalServerError)
				return 
			}
			defer func(f *os.File) {
				if f != nil { f.Close() }
			}(temp_file)	

			for _, si:= range task.SegmentInfo {
				fmt.Println("Copy segment. si.Path: ", si.Path)
				seg_file, err := os.Open(si.Path)
				if err != nil {
					fmt.Println("Open file error: ", err)
				}
				// 将分片文件内容拷贝到tmp文件
				_, err = io.Copy(temp_file, seg_file)
				if err != nil {
					fmt.Println("Failed to copy file")
				}
				if seg_file != nil {
					seg_file.Close()
				}
			}
			temp_file.Close()

			temp_file_info, err := os.Stat(task.StoreDir + temp_name)
			if err != nil {
				fmt.Println("Failed to get file info: ", task.StoreDir + temp_name)
			}
			fmt.Println("temp_file_info, Size: ", temp_file_info.Size())
			if temp_file_info.Size() != task.Size {
				fmt.Println("temp_file_info, Size error!")
				return
			}

			for _, si:= range task.SegmentInfo {
				os.Remove(si.Path)
			}

			// local_path, local_name, local_size
			local_path = task.StoreDir
			local_name = temp_name
			local_size = temp_file_info.Size()
			file_name = r.FormValue("file_name")
			if file_name == "" {
				file_name = "Unknown"
			}
			// 后续的流程下面继续走完：检测是否为媒体文件等。
		} else {
			// 分片没收完  handler函数最后怎么处理还要整理一下。
			fmt.Println("Segments upload not finish!")
			w.WriteHeader(http.StatusNotAcceptable)
			return 
		}

	}


	// Step 2 ： 检测是否为视频文件

	// 准备请求参数
	args := &Args{MediaPath: local_path + local_name}
	media_info := checkMediaInfo(args)
	if !media_info.IsMedia  {
		fmt.Println("It 's not a media file.")
		w.WriteHeader(http.StatusNotAcceptable)
		return 
	}
	// {142482024 true mov,mp4,m4a,3gp,3g2,mj2 309000 2560 1440 3688854 vp9 0 3532531 opus 1 129844}
	fmt.Println("media_info: ", media_info)

	// Step 3 : 准备转码task相关信息
	description := r.FormValue("description")
	title := r.FormValue("title")
	label1 := r.FormValue("label1")
	label2 := r.FormValue("label2")
	label3 := r.FormValue("label3")
	label4 := r.FormValue("label4")
	label5 := r.FormValue("label5")
	label6 := r.FormValue("label6")
	label := ""
	if (label1 != "") {
		label = label1
	}
	if (label2 != "") {
		label = label + ";" + label2
	}
	if (label3 != "") {
		label = label + ";" + label3
	}
	if (label4 != "") {
		label = label + ";" + label4
	}	
	if (label5 != "") {
		label = label + ";" + label5
	}	
	if (label6 != "") {
		label = label + ";" + label6
	}

	category := r.FormValue("category")

	poli_sens_stri := r.FormValue("political_sensitivity")
	poli_sens, err := strconv.Atoi(poli_sens_stri)
	if (err != nil) {
		fmt.Printf("Atoi err: %v\n", err)
		poli_sens = 0
	}

	erot_sens_stri := r.FormValue("erotic_sensitivity")
	erot_sens, err := strconv.Atoi(erot_sens_stri)
	if (err != nil) {
		fmt.Printf("Atoi err: %v\n", err)
		erot_sens = 0
	}
	this_time, _ := currentTimeByString()

	var item VideoItem
	item.video_id = video_id
	item.title = title
	item.original_name = file_name
	item.local_path = local_path
	item.local_name = local_name
	item.description = description
	item.label = label 
	item.category = category
	item.upload_time = this_time
	item.political_sensitivity = poli_sens
	item.erotic_sensitivity = erot_sens
	item.width = media_info.Width
	item.height = media_info.Height
	item.size = local_size
	item.bitrate = media_info.BitRate
	item.duration = media_info.Duration

	// Step 4 : Push into transcode list，等待异步转码，以及写库
	var task TranscodeTask
	task.video_id = video_id
	task.status = "pending"
	task.path = local_path
	task.various_streams = 0
	task.src_name = local_name
	task.src_resolution_w = media_info.Width
	task.src_resolution_h = media_info.Height
	task.src_video_format = media_info.VideoFormat
	task.src_video_index = media_info.VideoIndex
	task.src_video_bitrate = media_info.VideoBitrate
	task.src_audio_format = media_info.AudioFormat
	task.src_audio_index = media_info.AudioIndex
	task.src_audio_bitrate = media_info.AudioBitrate
	task.video_item = item
	{
		tranTaskMutex.Lock()
		defer tranTaskMutex.Unlock()
		transcodeTaskList.PushBack(task)		
		fmt.Println("Broadcast")
		transcodeTaskCond.Broadcast() // 通知转码线程有新的转码任务
	}
	
	result_resp := UploadResultResponse {
		Status : "Created",
		VideoId : video_id,
		UploadTime : this_time.String(),
		Size : local_size,
	}
	result, _ := json.Marshal(result_resp)
	//fmt.Printf("result: %v", result)
	_, _ = w.Write(result)

	w.WriteHeader(http.StatusOK)

}
func main() {
	uploadTaskMap = make(map[string]UploadTask)

	global_init()
	
	go func() {
		//return;
		pwd, _ := os.Getwd()
		fmt.Println("pwd: ", pwd)
		/* probe_cmd := exec.Cmd {
			Path : pwd + "/probe_media_info/probe_media_info",

		} */
		// func Command(name string, arg ...string) *Cmd
		//probe_cmd := exec.Command(pwd + "/probe_media_info/probe_media_info", )


		// 创建一个信号通道，用于接收信号
		sigCh := make(chan os.Signal, 1)
		signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

		// 启动子进程
		cmd := exec.Command(pwd + "/probe_media_info/probe_media_info")
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		// 设置子进程的环境变量
		//cmd.Env = []string{"LD_LIBRARY_PATH=" + pwd + "/probe_media_info/ffmpeg_module/lib/ffmpeg:"}

		// 启动子进程
		err := cmd.Start()
		if err != nil {
			fmt.Println("Error starting child process:", err)
			return
		}

		// 启动一个 goroutine，监听信号并处理
		go func() {
			sig := <-sigCh
			fmt.Printf("Received signal %v, exiting...\n", sig)

			// 发送信号给子进程
			cmd.Process.Signal(sig)
		}()

		// 等待子进程退出
		err = cmd.Wait()
		if err != nil {
			fmt.Println("Error waiting for child process:", err)
			os.Exit(0)
			return
		}
		fmt.Println("Child process exited successfully.")
	}()

	go func() {
		for true { 
			tranTaskMutex.Lock()
			for transcodeTaskList.Len() == 0 {
				fmt.Println("Enter waiting")
				transcodeTaskCond.Wait() // 等待有新的转码任务
				fmt.Println("Outer waiting")
			}
			e := transcodeTaskList.Front()
			if(e != nil) {
				fmt.Println("Transcode task list front: ", e.Value)
				var u TranscodeTask
				var ok bool
				u, ok = e.Value.( TranscodeTask ) //断言是否为 TranscodeTask 类型
				transcodeTaskList.Remove(e)
				tranTaskMutex.Unlock()				
				if !ok {
					fmt.Println("It 's not ok")
				} else {
					fmt.Println("u: ", u)
					execTranscode(u)
				}
			} else {
				tranTaskMutex.Unlock()
				// time.Sleep(1 * time.Second)
			}
		}
	}()

	r := mux.NewRouter()
	/* r.HandleFunc(myPrefixPath + "/video", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintf(w, "Static video handler")
	}).Methods("GET") */	
	// 动态路径
	r.HandleFunc(myPrefixPath + "/video", videoPostHandler).Methods("POST")
	r.HandleFunc(myPrefixPath + "/video", videoHandler).Methods("GET")
	r.HandleFunc(myPrefixPath + "/video/{video_id}", videoIdPostHandler).Methods("POST")
	r.HandleFunc(myPrefixPath + "/video/{video_id}", videoIdGetHandler).Methods("GET")
	r.HandleFunc(myPrefixPath + "/video/{video_id}", videoIdDeleteHandler).Methods("DELETE")
	// 设置路由处理函数
	http.HandleFunc(myPrefixPath, defaultHandler)
	// http.HandleFunc(myPrefixPath + "/upload", uploadHandler)
	http.HandleFunc(myPrefixPath + "/live", liveHandler)
	// http.HandleFunc(myPrefixPath + "/request", requestHandler)
	// http.HandleFunc(myPrefixPath + "/video", videoHandler)
	// http.HandleFunc(myPrefixPath + "/delete", deleteHandler)
	http.Handle("/", r)
	/*
		GET /videos/{video_id} : 获取某个视频的信息，包括播放地址等
		GET /videos : 获取全部视频的信息
		POST /videos/{md5} : 新增一个视频，分片上传怎么弄？
	*/

	// 启动服务器，监听端口 3001
	fmt.Println("Server is running on :3001")
	http.ListenAndServe(":3001", nil)
}
