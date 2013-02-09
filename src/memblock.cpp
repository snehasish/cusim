#include "memblock.H"

memblock::memblock(uint64_t sa, uint64_t ea, uint64_t ic, uint32_t mc):
    startAddress(sa),
    endAddress(ea),
    insCount(ic),
    modCount(mc),
    size( (ea - sa + WORD_SIZE))
{

}

void memblock::print(void)
{
    cout << "Start Addr: " << hex << startAddress << endl;
    cout << "End Addr: " << hex << endAddress << endl;
    cout << "Size: " << dec << size << endl;
}
