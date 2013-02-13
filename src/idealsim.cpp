/*
 *
 */

#include "idealsim.H"


uint32_t optGran = 64, optSetCount = 4, optBinSize = 4096;
string optFileName, optHintFilePath;
bool optCSV = false, optHint = false, optAligned = false;
uint64_t optWarmCount = WARM_INS, optSetSize, optSimCount = SIM_COUNT;


/*
 * Function declarations
 */
void setArgs(int, char** );
void *tMain(void *);

/*
 * Main
 */
CacheController *cc;
Predictor *hint;


int main(int argc, char* argv[]){
    setArgs(argc, argv);
    cc = new CacheController( optSetCount, optSetSize , optGran , optAligned, optWarmCount);
    hint = new Predictor(optGran, optAligned, optHintFilePath, optSetCount, optSetSize, optBinSize);
    tMain((void*)0);
    delete cc;
    return 0;
}



/*
 * Set command line arguments
 * -f Filename of the Gzipped Address Trace
 * 
 */

void setArgs(int argc, char** argv)
{
    short c;
    while((c = getopt(argc, argv, "f:c:t:g:e:b:d:w:s:xha?")) != -1){
        switch(c){
          case 'a':
            optAligned = true;
            break;
          case 'b':
            optBinSize = atoi(optarg);
            break;
          case 's':
            optSetCount = atoi(optarg);
            break;
          case 'g':
            optGran = atoi(optarg);
            break;
          case 'c':
            optSetSize = atoi(optarg);
            break;
          case 'e':
            optSimCount = atoll(optarg);
            break;
          case 'w':
            optWarmCount = atoll(optarg);
            break;
          case 'f':
            optFileName = optarg;
            break;
          case 'x':
            optCSV = true;
            break;
          case 'd':
            optHintFilePath = optarg;
            optHint = true;
            break;
          case 'h':
          case '?':
          default:
              cout << "Usage : " << argv[0]
                   << "\n\t-f path/to/Tracefile \n\t-s SetCount \n\t-c SetSize \n\t -g LineSize"
                   << "\n\t-w WarmUpCount -d path/to/HintFile \n\t[-x] CSV Output"
                   << ""
                   << endl;
          exit(0);
        }
    }
}

/*
 * Thread Main - Individual file processing
 */
void *tMain(void * tArgs){
    uint64_t insCount, insPointer, effectiveAddress, firstIns = 0, nodeid;
    uint32_t memoryAccessSize;
    igzstream inFile;
    char rw, tid = '0' + (uint64_t)tArgs;
    string threadLocalFilename = optFileName, ext = ".gz";


    // ext.replace(1,1,1,tid);
    // threadLocalFilename = threadLocalFilename + ext;

    inFile.open(threadLocalFilename.c_str(), ios::in);

    uint64_t maxAddr = 0, counter = 0;

    if(inFile)
    {
        cerr << "Processing " << endl;
        while(inFile >> insCount >> rw >> hex >> insPointer >> hex >> effectiveAddress >> dec >> memoryAccessSize)
        {
            if(counter % 1000000 == 0)
                cerr << ".";

            uint64_t sA = (effectiveAddress >> int(log2(WORD_SIZE))) << int(log2(WORD_SIZE));
            uint32_t size = 0;
            uint64_t eA = effectiveAddress + memoryAccessSize;

            if( eA % WORD_SIZE == 0 )
            {
                size = eA - sA;
            }
            else
            {
                eA = (eA >> int(log2(WORD_SIZE))) << int(log2(WORD_SIZE));
                size  = eA - sA + WORD_SIZE; // Extra word for non- word aligned access
            }

            vector<memblock> blocks = hint->predict(sA, size, insCount);

            for(vector<memblock>::iterator it = blocks.begin(); it != blocks.end(); it++)
            {
                cc->access( *it , effectiveAddress, memoryAccessSize );
            }
            counter++;

            if( firstIns == 0 ) firstIns = insCount;
            if( ( optSimCount != 0 ) && ( firstIns + optSimCount < insCount ) ) break;


            char c = '\0';
            while ((!inFile.eof()) && (c != '\n')) {
                inFile.get(c);
            }
        }

        cc->purge(insCount);
        cc->hub->stats(false);
        if(optHint && optAligned) cc->hub->dumpHint(optHintFilePath);
    }
    else
    {
        cout << "File " << threadLocalFilename << " not found." << endl;
    }
    inFile.close();
}
