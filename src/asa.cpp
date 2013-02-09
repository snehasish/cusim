#include <iostream>
#include <unistd.h>
#include <fstream>
#include <cstdlib>
#include <string>
#include <map>
#include <cmath>
#include <vector>
#include "evictionrecord.H"

using namespace std;

string fileName;
fstream hintFile;
uint64_t binSize;
int topCount;
bool optCSV;

typedef struct binStat
{
    uint64_t sumSamples;
    uint64_t sumSqSamples;
    uint64_t count;
    binStat(uint64_t, uint64_t, uint64_t);
} binStat;

binStat::binStat(uint64_t a, uint64_t b, uint64_t c ):sumSqSamples(a),sumSamples(b),count(c){}

map<uint64_t, map<uint64_t,uint64_t> > bin;
map<uint64_t, binStat*> binstat;
map<uint64_t, binStat*> topMap;

void setArgs(int argc, char* argv[])
{
    short c;
    while( (c = getopt(argc, argv, "f:b:t:x")) != -1)
    {
        switch(c)
        {
          case 'f':
            fileName = optarg;
            break;
          case 'b':
            binSize = atoi(optarg);
            break;
          case 't':
            topCount = atoi(optarg);
            break;
          case 'x':
            optCSV = true;
            break;
        }
    }
}

int wordCount( EvictionRecord* er)
{
    int sum = 0;
    for(int i=0; i < er->getBlockSize(); i++)
    {
        if(er->getBitmapValue(i) > 0)
            sum++;
    }
    return sum;
}

void setStat(uint64_t index, EvictionRecord* er)
{
    int wc = wordCount(er);

    if(binstat.count(index) > 0)
    {
        binstat[index]->sumSamples += wc;
        binstat[index]->sumSqSamples += wc*wc;
        binstat[index]->count += 1;
    }
    else
    {
        binStat *bS = new binStat(wc*wc, wc, 1);
        binstat.insert(pair<uint64_t,binStat*>(index,bS));
    }
}

void process(EvictionRecord* er)
{
    uint64_t addr = er->getBlockAddress();
    if(true)//addr >> 32 )
    {
        uint64_t index = addr >> int(log2(binSize));
        uint64_t wc = wordCount(er);
        setStat(index, er);
        if( bin.count(index) > 0)
        {
            if ( bin[index].count(wc) > 0)
                bin[index][wc] += 1;
            else
                bin[index][wc] = 1;
        }
        else
        {
            bin[index][wc] = 1;
        }
    }
}

void stat(void)
{
    int end =  binstat.size() * topCount / 100;
    end = end > topCount ? topCount : end;
    for(int i = 0; i < end; i++)
    {
        uint64_t max = 0;
        map<uint64_t, binStat*>::iterator key;

        for(map<uint64_t, binStat*>::iterator it = binstat.begin(); it != binstat.end(); it++)
        {
            if( it->second->count > max)
            {
                key = it;
                max = it->second->count;
            }
        }
        topMap.insert(pair<uint64_t, binStat*>(key->first, key->second));
        binstat.erase(key);
    }

    if(optCSV)
    {
        for(map<uint64_t, binStat*>::iterator i = topMap.begin(); i != topMap.end(); i++ )
        {
            double variance = double(i->second->sumSqSamples - double(i->second->sumSamples * i->second->sumSamples)/i->second->count)/(i->second->count-1);
            double sd = sqrt(variance);
            double mean = double(i->second->sumSamples)/i->second->count;
            cout << i->first << ",";
            cout << i->second->count << ",";
            cout << variance << ",";
            cout << sd << ",";
            cout << mean << ",";
            for(int j = 1; j <= 8; j++)
            {
                if(bin[i->first].count(j) > 0)
                    cout << j << "," << bin[i->first][j];
                else
                    cout << j << ",0";
                cout << ",";
            }
            cout << endl;
        }
    }
    else
    {
        for(map<uint64_t, binStat*>::iterator i = topMap.begin(); i != topMap.end(); i++ )
        {
            double variance = double(i->second->sumSqSamples - double(i->second->sumSamples * i->second->sumSamples)/i->second->count)/(i->second->count-1);
            double sd = sqrt(variance);
            double mean = double(i->second->sumSamples)/i->second->count;
            cout << "-- Bin[" << i->first << "]" << endl;
            cout << "Count: " << i->second->count << endl;
            cout << "Variance: " << variance << endl;
            cout << "Standard Deviation: " << sd << endl;
            cout << "Mean: " << mean << endl;
            for (map<uint64_t,uint64_t>::iterator j = bin[i->first].begin(); j != bin[i->first].end(); j++)
            {
                cout << j->first << " words accessed " << j->second << " times" << endl;;
            }
            cout << endl;
        }
    }
}

int main(int argc, char *argv[])
{
    setArgs(argc, argv);
    hintFile.open(fileName.c_str(), ios::in | ios::binary);
    vector<EvictionRecord> vSplitER;
    vector<int> positions;

    while(!hintFile.eof())
    {
        EvictionRecord *er = new EvictionRecord();
        hintFile.read( (char *)er,sizeof(EvictionRecord));

        bool flag = true;
        for(int i = 0; i < er->getBlockSize(); i++)
        {
            if( er->getBitmapValue(i) > 0 && flag )
            {
                positions.push_back(i);
                flag = false;
            }

            if( er->getBitmapValue(i) == 0 && !flag)
            {
                positions.push_back(i - 1);
                flag = true;
            }
        }
        if(!flag) positions.push_back(er->getBlockSize() - 1);

        for(int i = 0; i < positions.size(); i+=2)
        {
            EvictionRecord erCopy(*er);
            int start = positions[i], end = positions[i+1];
            for(int j = 0; j < erCopy.getBlockSize(); j++)
            {
                if( !(j >= start && j <= end))
                    erCopy.setBitmapValue(j,0);
            }
            /*
            for(int k = 0; k < erCopy.getBlockSize(); k++)
                cout << erCopy.getBitmapValue(k) << " ";
            cout << endl;
            */
            vSplitER.push_back(erCopy);
        }
        /*
        // Checking
        for(int i = 0; i < er->getBlockSize(); i++)
            cout << er->getBitmapValue(i) << " ";
        cout << endl;

        for(int i = 0; i < positions.size(); i++)
            cout << positions[i] << " ";
        cout << endl;

        cout << vSplitER.size() << endl;


        //Checking End
        */
        for(int i = 0; i < vSplitER.size(); i++)
        {
            process(&(vSplitER[i]));
            // vSplitER[i].print();
        }

        // cin.get();
        vSplitER.clear();
        positions.clear();
        delete er;
    }
    hintFile.close();
    // cout << "Bin Count: " << bin.size() << endl;
    stat();

    for(map<uint64_t, binStat*>::iterator it = binstat.begin(); it != binstat.end(); it++)
    {
        delete it->second;
    }
    for(map<uint64_t, binStat*>::iterator it = topMap.begin(); it != topMap.end(); it++)
    {
        delete it->second;
    }
    return 0;
}
