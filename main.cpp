#include<bits/stdc++.h>
#include<readHeader.h>
#include<pwOutput.h>
using namespace std;

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        cerr<<"Incorrect number of argyuments. Usage: "<<argv[0]<<" <wav_file>\n";
        return 1;
    }
    wavHeader header =readWavHeader(argv[1]);
    playWav(header);

    return 0;
}