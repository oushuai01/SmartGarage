/*
 * @Author: oushuai 1768621963@qq.com
 * @Date: 2023-10-30 18:27:47
 * @LastEditors: oushuai 1768621963@qq.com
 * @LastEditTime: 2023-10-31 19:59:34
 * @FilePath: \开发板代码\HyperLPR-master\Prj-Linux\hyperlpr\main.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "include/Pipeline.h"
#include <time.h>
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc,char **argv)
{
    if(argc<2)
    {
        printf("请指定一张JPEG格式的包含车牌的照片\n");
        return 0;
    }

    pr::PipelinePR prc("model/cascade.xml",
    "model/HorizonalFinemapping.prototxt",
    "model/HorizonalFinemapping.caffemodel",
    "model/Segmentation.prototxt",
    "model/Segmentation.caffemodel",
    "model/CharacterRecognization.prototxt",
    "model/CharacterRecognization.caffemodel",
    "model/SegmenationFree-Inception.prototxt",
    "model/SegmenationFree-Inception.caffemodel");

    cv::Mat image = cv::imread(argv[1]);
    std::vector<pr::PlateInfo> res = prc.RunPiplineAsImage(image,pr::SEGMENTATION_FREE_METHOD);

    fstream f("license", ios::out);
               
    for(auto st:res) 
    {
        if(st.confidence>0.75)
            f.write(st.getPlateName().c_str(), st.getPlateName().length());
    }
}
