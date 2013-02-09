#include "predictor.H"

Predictor::Predictor(uint32_t mg, bool aA, string path, uint32_t setCount, uint64_t setSize, uint32_t bS):
    maxGran(mg),
    alignedAccess(aA),
    binSize(bS)
{
    if(alignedAccess)
    {
        cerr << "Using Standard aligned mode at " << maxGran << "B" <<endl;
        useHints = false;
    }
    else
    {
        stringstream sstrA, sstrB;
        string sC, sS;
        sstrA << setCount;
        sstrA >> sC;
        sstrB << ( ( setSize - 4*64 ) * setCount )/1024;
        sstrB >> sS;

        path += "hint_"+sC+"_"+sS+".bin";
        hintFile.open( path.c_str(), ios::in | ios::binary);

        /* Open Hint file and load the eviction records */

        if(hintFile)
        {
            useHints = true;
            vector<EvictionRecord> vSplitER;
            vector<int> positions;
            while( !hintFile.eof() )
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
                    vSplitER.push_back(erCopy);
                }

                for(int i = 0; i < vSplitER.size(); i++)
                {
                    process(&(vSplitER[i]));

                }
                vSplitER.clear();
                positions.clear();
                delete er;
            }
            hintFile.close();

            cerr << "Using Region Hints to Predict" << endl;

        }
        else
        {
            cerr << "Using Unaligned Fallback" << endl;
            useHints = false;
        }
    }
}

Predictor::~Predictor()
{

}

vector<memblock> Predictor::predict(uint64_t effectiveAddress, uint32_t memoryAccessSize, uint64_t insCount)
{
    vector<memblock> blocks;

    //  Cache Aligned Standard Implementation
    if (alignedAccess)
    {
        blocks = predictAligned(effectiveAddress, memoryAccessSize, insCount);
    }
    // Unaligned Access - Either perfect access using only the size or using hints
    else
    {
        if(useHints)
        {
            blocks = predictRegion(effectiveAddress, memoryAccessSize, insCount);
        }
        else
        {
            blocks.push_back(memblock(effectiveAddress, effectiveAddress + memoryAccessSize - WORD_SIZE, insCount, 1));
        }
    }

    return blocks;
}

bool Predictor::isSpanningAccess(uint64_t effectiveAddress, uint32_t memoryAccessSize)
{
    uint64_t alignedStartAddress = (effectiveAddress >> int(log2(maxGran))) << int(log2(maxGran));
    /// This is -1 as the smallest access can be of size 1B
    uint64_t alignedEndAddress = ((effectiveAddress + memoryAccessSize - 1) >> int(log2(maxGran))) << int(log2(maxGran));
    return alignedEndAddress != alignedStartAddress;
}

void Predictor::process(EvictionRecord* er)
{
    uint64_t addr = er->getBlockAddress();
    uint64_t index = addr >> int(log2(binSize));
    uint64_t wc = wordCount(er);

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

    if(binIndexCount.count(index) > 0)
        binIndexCount[index]++;
    else
        binIndexCount[index] = 1;

}

int Predictor::wordCount( EvictionRecord* er)
{
    int sum = 0;
    for(int i=0; i < er->getBlockSize(); i++)
    {
        if(er->getBitmapValue(i) > 0)
            sum++;
    }
    return sum;
}

vector<memblock> Predictor::predictAligned(uint64_t effectiveAddress, uint32_t memoryAccessSize, uint64_t insCount)
{
    vector<memblock> blocks;

    if (isSpanningAccess( effectiveAddress, memoryAccessSize))
        {
            uint64_t alignedStartAddress = (effectiveAddress >> int(log2(maxGran))) << int(log2(maxGran));
            uint64_t alignedEndAddress = ((effectiveAddress + memoryAccessSize - 1) >> int(log2(maxGran))) << int(log2(maxGran));

            while(alignedStartAddress <= alignedEndAddress)
            {
                blocks.insert(blocks.begin(),memblock(alignedStartAddress, alignedStartAddress + maxGran - WORD_SIZE, insCount, 1));
                alignedStartAddress += maxGran;
            }
        }
        else
        {
            uint64_t sa = (effectiveAddress >> int(log2(maxGran))) << int(log2(maxGran));
            uint64_t ea = sa + maxGran - WORD_SIZE;
            blocks.push_back(memblock(sa,ea,insCount,1));
        }
    return blocks;

}


vector<memblock> Predictor::predictRegion(uint64_t effectiveAddress, uint32_t memoryAccessSize, uint64_t insCount)
{
    vector<memblock> blocks;

    /* Static Page based predictor logic */

    int gran = 1, max = 0;
    uint64_t sa, ea;
    uint64_t index = effectiveAddress >> int(log2(binSize));


    if ( binIndexCount.count(index) >= REGION_THRESHOLD)
    {

        for(int i = 0; i < EVICT_BITMAP_MAX_SIZE; i++)
        {
            if ( bin[index].count(i) > 0)
            {
                if (bin[index][i] > max )
                {
                    max = bin[index][i];
                    gran = i;
                }
            }
        }

        if(isSpanningAccess(effectiveAddress, memoryAccessSize))
        {
            sa = effectiveAddress;
            ea = effectiveAddress + memoryAccessSize - WORD_SIZE;
        }
        else
        {
            if(isSpanningAccess(effectiveAddress, gran * WORD_SIZE))
            {
                sa = effectiveAddress;
                uint64_t alignedStart = (effectiveAddress >> int(log2(maxGran))) << int(log2(maxGran));
                ea = alignedStart + maxGran - WORD_SIZE;

            }
            else
            {
                sa = effectiveAddress;
                ea = effectiveAddress + ( (gran == 1 ? 1 : gran)- 1 ) * WORD_SIZE;
            }
        }


        /* Logic based single load only */

        blocks.push_back(memblock(sa,ea,insCount,1));
    }
    else
    {
        // Fall back to aligned
        blocks = predictAligned(effectiveAddress,memoryAccessSize,insCount);
    }
    return blocks;
}

