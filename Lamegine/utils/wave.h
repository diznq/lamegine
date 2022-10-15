#pragma once
#include <vector>
#include <stdio.h>
class Wave {
private:
    std::vector<float> data;
    int sampleRate;
public:
    Wave(const char *file){
        FILE *f = fopen(file, "rb");
        if(!f) return;
        struct WAV_HEADER {
            char                RIFF[4];        // RIFF Header      Magic header
            unsigned long       ChunkSize;      // RIFF Chunk Size  
            char                WAVE[4];        // WAVE Header      
            char                fmt[4];         // FMT header       
            unsigned long       Subchunk1Size;  // Size of the fmt chunk                                
            unsigned short      AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
            unsigned short      NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
            unsigned long       SamplesPerSec;  // Sampling Frequency in Hz                             
            unsigned long       bytesPerSec;    // bytes per second 
            unsigned short      blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
            unsigned short      bitsPerSample;  // Number of bits per sample  
        };

        struct WAV_CHUNK {
            char                ID[4];
            unsigned long       size;
        };

        WAV_HEADER hdr;
        fread(&hdr, sizeof(hdr), 1, f);
        sampleRate = hdr.SamplesPerSec;
        WAV_CHUNK chunk;

        while(true){
            fread(&chunk, sizeof(WAV_CHUNK), 1, f);
            if(chunk.ID[0] == 'd' && chunk.ID[1] == 'a' && chunk.ID[2] == 't' && chunk.ID[3] == 'a'){
                break;
            }
            fseek(f, chunk.size, SEEK_CUR);
        }

        if(hdr.AudioFormat == 3){
            data.resize(chunk.size / 4);
            fread(&data[0], 4, data.size(), f);
        } else if(hdr.AudioFormat == 1){
            int n = chunk.size / (hdr.bitsPerSample / 8);
            data.resize(n);
            if(hdr.bitsPerSample == 32){
                int *samples = new int[n];
                fread(samples, chunk.size, 1, f);
                for(int i=0; i<n; i++){
                    data[i] = samples[i] / 2147483648.0f;
                }
                delete[] samples;
            } else if(hdr.bitsPerSample == 16){
                short *samples = new short[n];
                fread(samples, chunk.size, 1, f);
                for(int i=0; i<n; i++){
                    data[i] = samples[i] / 32768.0f;
                }
                delete[] samples;
            } else if(hdr.bitsPerSample == 8){
                char *samples = new char[n];
                fread(samples, chunk.size, 1, f);
                for(int i=0; i<n; i++){
                    data[i] = samples[i] / 128.0f;
                }
                delete[] samples;
            }
        }
        fclose(f);
    }
    Wave(std::vector<float>& samples, int rate) : data(samples), sampleRate(rate) {

    }
    void write(const char *file){
        FILE* out = fopen(file, "wb");
        if(!out) return;
        char RIFF[] = "RIFF----WAVEfmt ____----____----____data---";
        auto put = [&](unsigned offset, unsigned value) { for(unsigned n=0; n<32; n+=8) RIFF[offset + n/8] = value >> n; };
        put(4, 36);
        put(16, 16);
        put(20, 1*65536+3);
        put(24, sampleRate);
        put(28, 4*sampleRate);
        put(32, (4*8)*65536+4);
        put(40, 4*data.size());
        fwrite(RIFF, 1, sizeof(RIFF), out);
        fwrite(&data[0], 4, data.size(), out);
        fclose(out);
    }
    float& operator[](size_t idx) {
        return data[idx];
    }
    const float operator[](size_t idx) const {
        return data[idx];
    }
    size_t size() const { return data.size(); }
    int rate() const { return sampleRate; }
};