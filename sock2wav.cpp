#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <iostream>
#include <chrono>

// usage
// sock2wav -p ./wavedir -s 32000 -S 10K outputfile
// -p : wave_file output path
// -f : wave_file_name
// -s : sampling frequency
// -S : wave file split size(Byte)
// -T : wave file split Time(sec)
// -b : bits per sample

double get_time_msec(void);
using namespace std::chrono;

int main(int argc, char *argv[]) {

	int sockfd;
	struct sockaddr_in addr;

	char outputPath[1024] = "./wave/";
	char baseFileName[1024] = "wavefile";
	char ip_addr[128] = "";
	int port = 0;
	int fs = 48000;
	int splitSize = 1000;
	int splitTime = 0;
	int bps = 16;

	//Command line optioons handling
	int i, opt;
	opterr = 0;
	char lastchar;
	while ((opt = getopt(argc, argv, "p:i:P:s:S:T:b")) != -1) {
		switch (opt) {
			case 'p':
				strcpy(outputPath, optarg);
				lastchar = outputPath[strlen(outputPath)-1];
				if (lastchar !='/') strcat(outputPath, "/");

				struct stat st;
				if (0 ==stat(outputPath, &st)){
					printf("found output path %s\n", outputPath);
				}
				else {
					printf("can't find output path %s\n", outputPath);
					exit(-1);
				}
				break;

			case 'b':
				printf("Bits/sample is %s \n", ip_addr);
				bps = std::atoi(optarg);
				break;

			case 'i':
				printf("Receiver ip address is %s \n", ip_addr);
				strcpy(ip_addr, optarg);
				break;

			case 'P':
				port = std::atoi(optarg);
				printf("Receiver port is %d \n", port);
				break;

			case 's':
				fs = std::atoi(optarg);
				printf("sampling frequency is %d Hz \n",fs);
				break;

			case 'S':
				splitSize = std::atoi(optarg);
				printf("file split size is %d Bytes \n",splitSize);
				break;

			case 'T':
				splitTime = std::atoi(optarg);
				printf("file split time is %d sec \n",splitTime);
				break;

			default:
				printf("Usage: %s [-i receiver IP address] [-P receiver port] [-p output_path] [-s sampling frequency(Hz)] [-S file split size(kByte)] [-T file split time(sec)] output_filename \n", argv[0]);
				exit(-1);
				break;
		}
	}

	// Check missing mandatory options
	if (strlen(ip_addr) == 0){
		printf("ip address is not specified.");
		exit(-1);
	}
	if (port == 0){
		printf("port is not specified.");
		exit(-1);
	}

	if (optind != argc){
		strcpy(baseFileName, argv[optind]);
	}
	printf("output base file name is %s \n", baseFileName);

	//setup socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != 0) {
		perror("Socket");
	}
	bzero((char *) &addr, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip_addr);

	//Connect to Receiver
	int result = 0;
	result = connect(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

	if (result != 0) {
		perror("Socket");
	}

	int rsize;
	char buf[100];
	memset(buf, 0, sizeof(buf));

	// set to non blocking
	int val = 1;
	ioctl(sockfd, FIONBIO, &val);

	// wave struct
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

	// setup chunk data.
	strncpy(wavefmt.riff, "RIFF", 4);
	wavefmt.chunk_size = 4;
	strncpy(wavefmt.format, "WAVE", 4);
	strncpy(wavefmt.fmt_ident, "fmt ", 4);
	wavefmt.fmt_chunk_bytes = 16;
	wavefmt.audio_format = 1;
	wavefmt.channels = 1; // mono:1 stereo:2
	wavefmt.sampling_freq = fs;
	wavefmt.block_size = 2; // mono:2 stereo:4
	wavefmt.bytes_per_sec = wavefmt.block_size * wavefmt.sampling_freq;
	wavefmt.bits_per_sample = bps;
	strncpy(wavefmt.subchunk_ident, "data", 4);

	FILE *pFile;
	int total = 0;
	struct tm *timeptr;
	char timebuf[512];
	time_t t;

	//Receive loop
	while (true) {

		//get current time
		t = time(NULL);
		timeptr = localtime(&t);
		strftime(timebuf, sizeof(timebuf), "__%Y_%m_%d__%H_%M_%S", timeptr);

		// file open
		char filename[1024];
		sprintf(filename, "%s%s_%s.wav", outputPath, baseFileName, timebuf);
		pFile = fopen(filename, "wb");
		fseek(pFile, sizeof(WAVEFMT), SEEK_SET);
		total = 0;

		//write start time
		double startTime = get_time_msec();
		while (true) {
			double duration = get_time_msec() - startTime;
			rsize = recv(sockfd, buf, sizeof(buf), 0);

			if (rsize > 0) {
				fwrite(buf, 1, rsize, pFile);
				total += rsize;
			}

			if (((total >=splitSize * 1024) && (splitTime == 0)) ||
				(( duration / 1000 ) >= splitTime) && (splitTime != 0 )) {
				fseek(pFile, 0, SEEK_SET);
				wavefmt.chunk_size = (int) sizeof(WAVEFMT) + total - 8;
				wavefmt.subchunk_size = total - 126;
				fwrite(&wavefmt, sizeof(WAVEFMT), 1, pFile);
				fflush(pFile);
				fclose(pFile);

				std::cout << "file output:" << filename << std::endl;
				break;
			}
		}
	}
}

double get_time_msec(void){
	return static_cast<double>(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())/1000000;
}
