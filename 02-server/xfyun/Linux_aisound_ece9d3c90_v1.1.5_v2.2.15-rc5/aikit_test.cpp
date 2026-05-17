#include <fstream>

#include <iostream>

#include <cstring>

#include <atomic>

#include <unistd.h>

#include "aikit_biz_api.h"

#include "aikit_constant.h"

#include "aikit_biz_config.h"



using namespace std;

using namespace AIKIT;



// WAV 头部结构体定义

struct WAV_HEADER {

    char RIFF[4] = {'R', 'I', 'F', 'F'};

    uint32_t ChunkSize;

    char WAVE[4] = {'W', 'A', 'V', 'E'};

    char fmt[4] = {'f', 'm', 't', ' '};

    uint32_t Subchunk1Size = 16;

    uint16_t AudioFormat = 1; // PCM

    uint16_t NumOfChan = 1;   // 单声道

    uint32_t SamplesPerSec = 16000; // 16K采样率

    uint32_t bytesPerSec = 16000 * 2;

    uint16_t blockAlign = 2;

    uint16_t bitsPerSample = 16;

    char Subchunk2ID[4] = {'d', 'a', 't', 'a'};

    uint32_t Subchunk2Size;

};



FILE *fin = nullptr;

static const char *ABILITY = "ece9d3c90";

static std::atomic_bool ttsFinished(false);



void OnOutput(AIKIT_HANDLE* handle, const AIKIT_OutputData* output){

    if((output->node->value) && (fin != nullptr)) {

        fwrite(output->node->value, sizeof(char), output->node->len, fin);

    }

}



void OnEvent(AIKIT_HANDLE* handle, AIKIT_EVENT eventType, const AIKIT_OutputEvent* eventValue){

    if(eventType == AIKIT_Event_End) ttsFinished = true;

}



void OnError(AIKIT_HANDLE* handle, int32_t err, const char* desc){

    printf("OnError:%d - %s\n", err, desc);

}



void WriteWavHeader(FILE* f, uint32_t pcmSize) {

    WAV_HEADER header;

    header.ChunkSize = 36 + pcmSize;

    header.Subchunk2Size = pcmSize;

    fseek(f, 0, SEEK_SET);

    fwrite(&header, sizeof(WAV_HEADER), 1, f);

}



int RunTTS(string text, string outputPath) {

    AIKIT_ParamBuilder* paramBuilder = nullptr;

    AIKIT_DataBuilder* dataBuilder = nullptr;

    AIKIT_HANDLE* handle = nullptr;

    int ret = 0;

    ttsFinished = false;



    fin = fopen(outputPath.c_str(), "wb+");

    if (fin == nullptr) return -1;



    // 预留 44 字节的头部位置

    uint8_t dummy[44] = {0};

    fwrite(dummy, 1, 44, fin);



    paramBuilder = AIKIT_ParamBuilder::create();

    paramBuilder->param("vcn","xiaoyan",7);

    paramBuilder->param("textEncoding","UTF-8",5);

    

    ret = AIKIT_Start(ABILITY, AIKIT_Builder::build(paramBuilder), nullptr, &handle);

    if(ret != 0) return ret;



    dataBuilder = AIKIT_DataBuilder::create();

    auto aiText_raw = AiText::get("text")->data(text.c_str(), text.length())->valid();

    dataBuilder->payload(aiText_raw);

    

    ret = AIKIT_Write(handle, AIKIT_Builder::build(dataBuilder));

    if(ret != 0) return ret;



    while(!ttsFinished) { usleep(1000); }



    // 计算 PCM 数据大小并回填 WAV 头

    uint32_t fileLen = ftell(fin);

    WriteWavHeader(fin, fileLen - 44);



    AIKIT_End(handle);

    fclose(fin);

    if(paramBuilder) delete paramBuilder;

    if(dataBuilder) delete dataBuilder;

    return 0;

}



int main(int argc, char* argv[]) {

    if(argc < 3) {

        printf("用法: %s <文本内容> <输出路径.wav>\n", argv[0]);

        return -1;

    }



    // 初始化

    AIKIT_Configurator::builder()

        .app().appID("087c391f").apiSecret("YjJkNzIwOGUzNmM1YWYzNTI5YTIwNTEw").apiKey("946de51d93d63d83370087c98097632e").workDir("./")

        .auth().authType(0);

    

    if(AIKIT_Init() != 0) return -1;

    AIKIT_Callbacks cbs = {OnOutput, OnEvent, OnError};

    AIKIT_RegisterAbilityCallback(ABILITY, cbs);



    // 执行

    int res = RunTTS(argv[1], argv[2]);

    

    AIKIT_UnInit();

    return res;

}