#ifndef _CRC32_h
#define _CRC32_h

#include <WinSock2.h>
#include <Windows.h>

class CRC32
{
public:

    //=========================================
    //  ctors
    inline CRC32()                                  { Reset();                  }
    inline CRC32(const void* buf, size_t siz)        { Reset(); Hash(buf,siz);   }

    //=========================================
    // implicit cast, so that you can do something like foo = CRC(dat,siz);
    inline operator DWORD () const                    { return Get();             }

    //=========================================
    // getting the crc
    inline DWORD          Get() const                 { return ~mCrc;             }

    //=========================================
    // HashBase stuff
    virtual void        Reset()                     { mCrc = ~0;                }
    virtual void        Hash(const void* buf,size_t siz);

private:
    DWORD         mCrc;
    static bool mTableBuilt;
    static DWORD  mTable[0x100];

    static const DWORD        POLYNOMIAL = 0x04C11DB7;

private:
    //=========================================
    // internal support
    static void         BuildTable();
    static DWORD          Reflect( DWORD v,int bits);
};

#endif
