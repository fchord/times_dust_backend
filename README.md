# times_dust_backend

nginx配置转发到后端：
    location /times_dust_api {
            proxy_pass http://localhost:3001; # 将请求转发到golang后端
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection 'upgrade';
            proxy_set_header Host $host;
            proxy_cache_bypass $http_upgrade;
    }

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/probe_media_info/ffmpeg_module/lib/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/probe_media_info/ffmpeg_module/lib/x264
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/probe_media_info/ffmpeg_module/ffmpeg/n4.4/
./times_dust_backend