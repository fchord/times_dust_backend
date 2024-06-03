
// char* ffmpeg_av_errnum_2_char(int err_num);

#include <vector>
#include <memory>

class MyClass;

typedef std::shared_ptr<MyClass> spMc;

class MyClass
{
public:
    MyClass() { return; };
    ~MyClass() { return; };
    std::vector<spMc> v;
};
