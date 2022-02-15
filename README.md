# light

#### 介绍
- 一个命令行音乐播放器

#### 编译
- `g++ main.cpp -lmad -lpulse -lpulse-simple -lcurl -pthread -o light -std=c++17 -O3`

#### 使用说明
- `./light -h`

#### 依赖
- curl
- MAD
- PulseAudio

#### 参考
- [MyMinimad ── Linux下用libmad写的mp3解码播放程序(四)](https://my.oschina.net/guzhou/blog/3132065)
- [The libcurl API](https://curl.se/libcurl/c/)
- [PulseAudio Documentation](https://www.freedesktop.org/software/pulseaudio/doxygen/index.html)
