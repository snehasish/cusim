#include <iostream>
#include <stdint.h>
#include "evictionrecord.H"
#include <map>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace std;

ifstream hintFile;
ofstream dumpFile;
string fileName;

multimap<uint64_t, EvictionRecord *> regionBin;
map<uint64_t, uint64_t> topBin;

inline uint64_t getIndex(uint64_t addr) { return ( addr << int(log2(4096)) ); }

void setArgs(int argc, char* argv[])
{
    short c;
    while( (c = getopt(argc, argv, "f:")) != -1)
    {
        switch(c)
        {
          case 'f':
            fileName = optarg;
            break;
         }
    }
}

void process(EvictionRecord* er)
{
    uint64_t index = getIndex(er->getBlockAddress());

    regionBin.insert(pair<uint64_t, EvictionRecord*>(index, er));

    if(topBin.count(index) > 0)
        topBin[index]++;
    else
        topBin[index]=1;
}

void dumpCSV()
{
    string fname = "/p/weekly/lbm/reg_", fname2;

    for(int i = 0; i < 10; i++)
    {
        stringstream sstr;
        string numStr;

        sstr << i;
        sstr >> numStr;

        fname2 = fname + numStr + ".csv";

        dumpFile.open(fname2.c_str(),ios::out);

        uint64_t max = 0, maxIndex = 0;

        for(map<uint64_t,uint64_t>::iterator it = topBin.begin(); it != topBin.end(); it++)
        {
            if( it->second > max )
            {
                max = it->second;
                maxIndex = it->first;
            }
        }


        pair<multimap<uint64_t,EvictionRecord*>::iterator,multimap<uint64_t,EvictionRecord*>::iterator> ret;
        ret = regionBin.equal_range(maxIndex);

        for(multimap<uint64_t,EvictionRecord*>::iterator it = ret.first; it != ret.second; ++it)
        {
            EvictionRecord *er = it->second;

            for( int i = 0; i < er->getBlockSize(); i++ )
            {
                if(er->getBitmapValue(i) > 0)
                    dumpFile << "1,";
                else
                    dumpFile << "0,";
            }
            dumpFile << endl;

        }

        topBin.erase(maxIndex);
        dumpFile.close();
    }
}

void cleanup(void)
{
    for(multimap<uint64_t,EvictionRecord*>::iterator it = regionBin.begin(); it != regionBin.end(); it++)
        delete it->second;
}


int main(int argc, char **argv)
{
    setArgs(argc, argv);
    hintFile.open(fileName.c_str(), ios::in | ios::binary);

    while(!hintFile.eof())
    {
        EvictionRecord *er = new EvictionRecord();
        hintFile.read( (char *)er,sizeof(EvictionRecord));
        process(er);

    }
    dumpCSV();
    cleanup();
    hintFile.close();
}
