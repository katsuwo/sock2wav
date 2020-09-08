#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>

  
int main() {

    int sockfd;
    struct sockaddr_in addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != 0){
        perror("Socket");
    }
    bzero((char*)&addr, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = inet_addr("192.168.10.109");
    
    int result = 0;
    result = connect(sockfd,(struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    
    if (result != 0){
        perror("Socket");
    }

    int rsize;
    char buf[100];
    memset(buf, 0, sizeof(buf));

    //set to non blocking
    int val = 1;
    ioctl(sockfd, FIONBIO, &val);

    //wave struct
    typedef struct {
        char riff[4];
        int chunk_size;
        char format[4];
        char fmt_ident[4];
        int fmt_chunk_bytes;
        short audio_format;
        short channels;
        int sampling_freq;
        int bytes_per_sec;
        short block_size;
        short bits_per_sample;
        char subchunk_ident[4];
        int subchunk_size;
    } WAVEFMT;
    WAVEFMT wavefmt;

    //setup chunk data.
    strncpy(wavefmt.riff, "RIFF",4);
    wavefmt.chunk_size = 4;
    strncpy(wavefmt.format, "WAVE",4);
    strncpy(wavefmt.fmt_ident, "fmt ",4);
    wavefmt.fmt_chunk_bytes = 16;
    wavefmt.audio_format = 1;
    wavefmt.channels = 1;           // mono:1 stereo:2
    wavefmt.sampling_freq = 32000;
    wavefmt.block_size = 2;         // mono:2 stereo:4
    wavefmt.bytes_per_sec = wavefmt.block_size *  wavefmt.sampling_freq * wavefmt.channels;
    wavefmt.bits_per_sample = 16;
    strncpy(wavefmt.subchunk_ident, "data", 4);

    FILE* pFile;
    int total = 0;
    int filecount = 0;
    while (filecount < 10){

        //file open
        char filename[50];
        sprintf(filename, "wavetest_%d.wav", filecount++);
        pFile = fopen(filename, "wb");
        fseek(pFile, sizeof(WAVEFMT), SEEK_SET);
        total = 0;

        while( true ){
            rsize = recv(sockfd, buf, sizeof(buf), 0);
            if (rsize > 0){
                fwrite(buf, 1, rsize, pFile);
                total += rsize;
            }
            if (total >= 1000*1000){
                fseek(pFile, 0, SEEK_SET);
                wavefmt.chunk_size = (int)sizeof(WAVEFMT) + total - 8;
                wavefmt.subchunk_size = total - 126;
                fwrite(&wavefmt, sizeof(WAVEFMT), 1, pFile);
                fflush(pFile);
                fclose(pFile);
                break;
            }
        }
    }
}