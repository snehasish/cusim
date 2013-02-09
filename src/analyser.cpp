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

int main(int argc, char **argv)
{
    setArgs(argc, argv);
    hintFile.open(fileName.c_str(), ios::in | ios::binary);

    while(!hintFile.eof())
    {
        EvictionRecord *er = new EvictionRecord();
        hintFile.read( (char *)er,sizeof(EvictionRecord));
        er->print();
    }
    hintFile.close();
}
