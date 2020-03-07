/*
 * /home/ms/files/source/libsidtune/RCS/SidTune.cpp,v
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _SidTune_cpp_

#include "config.h"
#include "SidTuneCfg.h"
#include "SidTune.h"
#include "SidTuneTools.h"
#include "sidendian.h"
#include "PP20.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif
#include <iostream>
#include <iomanip>
#include <string.h>
#include <limits.h>

#ifdef HAVE_IOS_OPENMODE
    typedef std::ios::openmode openmode;
#else
    typedef int openmode;
#endif


const char SidTune::txt_songNumberExceed[] = "SIDTUNE WARNING: Selected song number was too high";
const char SidTune::txt_empty[] = "SIDTUNE ERROR: No data to load";
const char SidTune::txt_unrecognizedFormat[] = "SIDTUNE ERROR: Could not determine file format";
const char SidTune::txt_noDataFile[] = "SIDTUNE ERROR: Did not find the corresponding data file";
const char SidTune::txt_notEnoughMemory[] = "SIDTUNE ERROR: Not enough free memory";
const char SidTune::txt_cantLoadFile[] = "SIDTUNE ERROR: Could not load input file";
const char SidTune::txt_cantOpenFile[] = "SIDTUNE ERROR: Could not open file for binary input";
const char SidTune::txt_fileTooLong[] = "SIDTUNE ERROR: Input data too long";
const char SidTune::txt_dataTooLong[] = "SIDTUNE ERROR: Size of music data exceeds C64 memory";
const char SidTune::txt_cantCreateFile[] = "SIDTUNE ERROR: Could not create output file";
const char SidTune::txt_fileIoError[] = "SIDTUNE ERROR: File I/O error";
const char SidTune::txt_VBI[] = "VBI";
const char SidTune::txt_CIA[] = "CIA 1 Timer A";
const char SidTune::txt_noErrors[] = "No errors";
const char SidTune::txt_na[] = "N/A";
const char SidTune::txt_badAddr[] = "SIDTUNE ERROR: Bad address data";
const char SidTune::txt_badReloc[] = "SIDTUNE ERROR: Bad reloc data";
const char SidTune::txt_corrupt[] = "SIDTUNE ERROR: File is incomplete or corrupt";

// Default sidtune file name extensions. This selection can be overriden
// by specifying a custom list in the constructor.
const char* defaultFileNameExt[] =
{
    // Preferred default file extension for single-file sidtunes
    // or sidtune description files in SIDPLAY INFOFILE format.
    ".sid",
    // Common file extension for single-file sidtunes due to SIDPLAY/DOS
    // displaying files *.DAT in its file selector by default.
    // Originally this was intended to be the extension of the raw data file
    // of two-file sidtunes in SIDPLAY INFOFILE format.
    ".dat",
    // Extension of Amiga Workbench tooltype icon info files, which
    // have been cut to MS-DOS file name length (8.3).
    ".inf",
    // No extension for the raw data file of two-file sidtunes in
    // PlaySID Amiga Workbench tooltype icon info format.
    "",
    // Common upper-case file extensions from MS-DOS (unconverted).
    ".DAT", ".SID", ".INF",
    // File extensions used (and created) by various C64 emulators and
    // related utilities. These extensions are recommended to be used as
    // a replacement for ".dat" in conjunction with two-file sidtunes.
    ".c64", ".prg", ".p00", ".C64", ".PRG", ".P00",
    // Uncut extensions from Amiga.
    ".info", ".INFO", ".data", ".DATA",
    // Stereo Sidplayer (.mus/.MUS ought not be included because
    // these must be loaded first; it sometimes contains the first
    // credit lines of a MUS/STR pair).
    ".str", ".STR", ".mus", ".MUS",
    // End.
    0
};

// Petscii to Ascii conversion table
static const char _sidtune_CHRtab[256] =  // CHR$ conversion table (0x01 = no output)
{
   0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0xd, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x21, 0x1,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};

const char** SidTune::fileNameExtensions = defaultFileNameExt;

inline void SidTune::setFileNameExtensions(const char **fileNameExt)
{
    fileNameExtensions = ((fileNameExt!=0)?fileNameExt:defaultFileNameExt);
}

SidTune::SidTune(const char* fileName, const char **fileNameExt,
                 const bool separatorIsSlash)
{
    init();
    isSlashedFileName = separatorIsSlash;
    setFileNameExtensions(fileNameExt);
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    // Filename ``-'' is used as a synonym for standard input.
    if ( fileName!=0 && (strcmp(fileName,"-")==0) )
    {
        getFromStdIn();
    }
    else
#endif
    if (fileName != 0)
    {
        getFromFiles(fileName);
    }
}

SidTune::SidTune(const uint_least8_t* data, const uint_least32_t dataLen)
{
    init();
    getFromBuffer(data,dataLen);
}

SidTune::~SidTune()
{
    cleanup();
}

bool SidTune::load(const char* fileName, const bool separatorIsSlash)
{
    cleanup();
    init();
    isSlashedFileName = separatorIsSlash;
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    if ( strcmp(fileName,"-")==0 )
        getFromStdIn();
    else
#endif
    getFromFiles(fileName);
    return status;
}

bool SidTune::read(const uint_least8_t* data, uint_least32_t dataLen)
{
    cleanup();
    init();
    getFromBuffer(data,dataLen);
    return status;
}

const SidTuneInfo& SidTune::operator[](const uint_least16_t songNum)
{
    selectSong(songNum);
    return m_info;
}

void SidTune::getInfo(SidTuneInfo& outInfo)
{
    outInfo = m_info;  // copy
}

const SidTuneInfo& SidTune::getInfo()
{
    return m_info;
}

// First check, whether a song is valid. Then copy any song-specific
// variable information such a speed/clock setting to the info structure.
uint_least16_t SidTune::selectSong(const uint_least16_t selectedSong)
{
    if ( !status )
        return 0;
    else
        m_info.statusString = SidTune::txt_noErrors;

    uint_least16_t song = selectedSong;
    // Determine and set starting song number.
    if (selectedSong == 0)
        song = m_info.startSong;
    if (selectedSong>m_info.songs || selectedSong>SIDTUNE_MAX_SONGS)
    {
        song = m_info.startSong;
        m_info.statusString = SidTune::txt_songNumberExceed;
    }
    m_info.currentSong = song;
    m_info.songLength = songLength[song-1];
    // Retrieve song speed definition.
    if (m_info.compatibility == SIDTUNE_COMPATIBILITY_R64)
        m_info.songSpeed = SIDTUNE_SPEED_CIA_1A;
    else if (m_info.compatibility == SIDTUNE_COMPATIBILITY_PSID)
    {   // This does not take into account the PlaySID bug upon evaluating the
        // SPEED field. It would most likely break compatibility to lots of
        // sidtunes, which have been converted from .SID format and vice versa.
        // The .SID format does the bit-wise/song-wise evaluation of the SPEED
        // value correctly, like it is described in the PlaySID documentation.
        m_info.songSpeed = songSpeed[(song-1)&31];
    }
    else
        m_info.songSpeed = songSpeed[song-1];
    m_info.clockSpeed = clockSpeed[song-1];
    // Assign song speed description string depending on clock speed.
    // Final speed description is available only after song init.
    if (m_info.songSpeed == SIDTUNE_SPEED_VBI)
        m_info.speedString = txt_VBI;
    else
        m_info.speedString = txt_CIA;
    return m_info.currentSong;
}

void SidTune::fixLoadAddress(bool force, uint_least16_t init, uint_least16_t play)
{
    if (m_info.fixLoad || force)
    {
        m_info.fixLoad = false;
        m_info.loadAddr += 2;
        fileOffset += 2;

        if (force)
        {
            m_info.initAddr = init;
            m_info.playAddr = play;
        }
    }
}

// ------------------------------------------------- private member functions

bool SidTune::placeSidTuneInC64mem(uint_least8_t* c64buf)
{
    if ( status && c64buf )
    {
        // The Basic ROM sets these values on loading a file.
        // Program end address
        uint_least16_t start = m_info.loadAddr;
        uint_least16_t end   = start + m_info.c64dataLen;
        endian_little16 (c64buf + 0x2d, end); // Variables start
        endian_little16 (c64buf + 0x2f, end); // Arrays start
        endian_little16 (c64buf + 0x31, end); // Strings start
        endian_little16 (c64buf + 0xac, start);
        endian_little16 (c64buf + 0xae, end);

        uint_least32_t endPos = m_info.loadAddr + m_info.c64dataLen;
        if (endPos <= SIDTUNE_MAX_MEMORY)
        {
            // Copy data from cache to the correct destination.
            memcpy(c64buf+m_info.loadAddr,cache.get()+fileOffset,m_info.c64dataLen);
            m_info.statusString = SidTune::txt_noErrors;
        }
        else
        {
            // Security - cut data which would exceed the end of the C64
            // memory. Memcpy could not detect this.
            //
            // NOTE: In libsidplay1 the rest gets wrapped to the beginning
            // of the C64 memory. It is an undocumented hack most likely not
            // used by any sidtune. Here we no longer do it like that, set
            // an error message, and hope the modified behaviour will find
            // a few badly ripped sids.
            memcpy(c64buf+m_info.loadAddr,cache.get()+fileOffset,m_info.c64dataLen-(endPos-SIDTUNE_MAX_MEMORY));
            m_info.statusString = SidTune::txt_dataTooLong;
        }
        if (m_info.musPlayer)
        {
            MUS_installPlayer(c64buf);
        }
        return true;
    }
    return false;
}

bool SidTune::loadFile(const char* fileName, Buffer_sidtt<const uint_least8_t>& bufferRef)
{
    Buffer_sidtt<const uint_least8_t> fileBuf;
    uint_least32_t fileLen = 0;

    // This sucks big time
    openmode createAtrr = std::ios::in;
#ifdef HAVE_IOS_NOCREATE
    createAtrr |= std::ios::nocreate;
#endif
    // Open binary input file stream at end of file.
#if defined(HAVE_IOS_BIN)
    createAtrr |= std::ios::bin;
#else
    createAtrr |= std::ios::binary;
#endif

    std::fstream myIn(fileName, createAtrr);
    // As a replacement for !is_open(), bad() and the NOT-operator don't seem
    // to work on all systems.
#if defined(DONT_HAVE_IS_OPEN)
    if ( !myIn )
#else
    if ( !myIn.is_open() )
#endif
    {
        m_info.statusString = SidTune::txt_cantOpenFile;
        return false;
    }
    else
    {
#if defined(HAVE_SEEKG_OFFSET)
        fileLen = (myIn.seekg(0,std::ios::end)).offset();
#else
        myIn.seekg(0,std::ios::end);
        fileLen = (uint_least32_t)myIn.tellg();
#endif
#ifdef HAVE_EXCEPTIONS
        if ( !fileBuf.assign(new(std::nothrow) uint_least8_t[fileLen],fileLen) )
#else
        if ( !fileBuf.assign(new uint_least8_t[fileLen],fileLen) )
#endif
        {
            m_info.statusString = SidTune::txt_notEnoughMemory;
            return false;
        }
        myIn.seekg(0,std::ios::beg);
        uint_least32_t restFileLen = fileLen;
        // 16-bit compatible loading. Is this really necessary?
        while ( restFileLen > INT_MAX )
        {
            myIn.read((char*)fileBuf.get()+(fileLen-restFileLen),INT_MAX);  // !cast!
            restFileLen -= INT_MAX;
        }
        if ( restFileLen > 0 )
        {
            myIn.read((char*)fileBuf.get()+(fileLen-restFileLen),restFileLen);  // !cast!
        }
        if ( myIn.bad() )
        {
            m_info.statusString = SidTune::txt_cantLoadFile;
            return false;
        }
        else
        {
            m_info.statusString = SidTune::txt_noErrors;
        }
    }
    myIn.close();
    if ( fileLen==0 )
    {
        m_info.statusString = SidTune::txt_empty;
        return false;
    }

    if ( decompressPP20(fileBuf) < 0 )
        return false;

    bufferRef.assign(fileBuf.xferPtr(),fileBuf.xferLen());
    return true;
}

void SidTune::deleteFileNameCopies()
{
    // When will it be fully safe to call delete[](0) on every system?
    if ( m_info.dataFileName != 0 )
        delete[] m_info.dataFileName;
    if ( m_info.infoFileName != 0 )
        delete[] m_info.infoFileName;
    if ( m_info.path != 0 )
        delete[] m_info.path;
    m_info.dataFileName = 0;
    m_info.infoFileName = 0;
    m_info.path = 0;
}

void SidTune::init()
{
    // Initialize the object with some safe defaults.
    status = false;

    m_info.statusString = SidTune::txt_na;
    m_info.path = m_info.infoFileName = m_info.dataFileName = 0;
    m_info.dataFileLen = m_info.c64dataLen = 0;
    m_info.formatString = SidTune::txt_na;
    m_info.speedString = SidTune::txt_na;
    m_info.loadAddr = ( m_info.initAddr = ( m_info.playAddr = 0 ));
    m_info.songs = ( m_info.startSong = ( m_info.currentSong = 0 ));
    m_info.sidChipBase1 = 0xd400;
    m_info.sidChipBase2 = 0;
    m_info.musPlayer = false;
    m_info.fixLoad = false;
    m_info.songSpeed = SIDTUNE_SPEED_VBI;
#ifdef SIDTUNE_PSID2NG
    m_info.clockSpeed = SIDTUNE_CLOCK_UNKNOWN;
    m_info.sidModel1 = SIDTUNE_SIDMODEL_UNKNOWN;
#else
    m_info.clockSpeed = SIDTUNE_CLOCK_PAL;
    m_info.sidModel1 = SIDTUNE_SIDMODEL_6581;
#endif
    m_info.sidModel2 = SIDTUNE_SIDMODEL_UNKNOWN;
    m_info.compatibility = SIDTUNE_COMPATIBILITY_C64;
    m_info.songLength = 0;
    m_info.relocStartPage = 0;
    m_info.relocPages = 0;

    for ( uint_least16_t si = 0; si < SIDTUNE_MAX_SONGS; si++ )
    {
        songSpeed[si] = m_info.songSpeed;
        clockSpeed[si] = m_info.clockSpeed;
        songLength[si] = 0;
    }

    fileOffset = 0;
    musDataLen = 0;

    for ( uint_least16_t sNum = 0; sNum < SIDTUNE_MAX_CREDIT_STRINGS; sNum++ )
    {
        for ( uint_least16_t sPos = 0; sPos < SIDTUNE_MAX_CREDIT_STRLEN; sPos++ )
        {
            infoString[sNum][sPos] = 0;
        }
    }
    m_info.numberOfInfoStrings = 0;

    // Not used!!!
    m_info.numberOfCommentStrings = 1;
#ifdef HAVE_EXCEPTIONS
    m_info.commentString = new(std::nothrow) char* [m_info.numberOfCommentStrings];
#else
    m_info.commentString = new char* [info.numberOfCommentStrings];
#endif
    if (m_info.commentString != 0)
        m_info.commentString[0] = SidTuneTools::myStrDup("--- SAVED WITH SIDPLAY ---");
}

void SidTune::cleanup()
{
    // Remove copy of comment field.
    uint_least32_t strNum = 0;
    // Check and remove every available line.
    while (m_info.numberOfCommentStrings-- > 0)
    {
        if (m_info.commentString[strNum] != 0)
        {
            delete[] m_info.commentString[strNum];
            m_info.commentString[strNum] = 0;
        }
        strNum++;  // next string
    };
    delete[] m_info.commentString;  // free the array pointer

    deleteFileNameCopies();

    status = false;
}

#if !defined(SIDTUNE_NO_STDIN_LOADER)

void SidTune::getFromStdIn()
{
    // Assume a failure, so we can simply return.
    status = false;
    // Assume the memory allocation to fail.
    m_info.statusString = SidTune::txt_notEnoughMemory;
    uint_least8_t* fileBuf;
#ifdef HAVE_EXCEPTIONS
    if ( 0 == (fileBuf = new(std::nothrow) uint_least8_t[SIDTUNE_MAX_FILELEN]) )
#else
    if ( 0 == (fileBuf = new uint_least8_t[SIDTUNE_MAX_FILELEN]) )
#endif
    {
        return;
    }
    // We only read as much as fits in the buffer.
    // This way we avoid choking on huge data.
    uint_least32_t i = 0;
    char datb;
    while (std::cin.get(datb) && i<SIDTUNE_MAX_FILELEN)
        fileBuf[i++] = (uint_least8_t) datb;
    m_info.dataFileLen = i;
    getFromBuffer(fileBuf,m_info.dataFileLen);
    delete[] fileBuf;
}

#endif

void SidTune::getFromBuffer(const uint_least8_t* const buffer, const uint_least32_t bufferLen)
{
    // Assume a failure, so we can simply return.
    status = false;

    if (buffer==0 || bufferLen==0)
    {
        m_info.statusString = SidTune::txt_empty;
        return;
    }

    if (bufferLen > SIDTUNE_MAX_FILELEN)
    {
        m_info.statusString = SidTune::txt_fileTooLong;
        return;
    }

#ifdef HAVE_EXCEPTIONS
    uint_least8_t* tmpBuf = new(std::nothrow) uint_least8_t[bufferLen];
#else
    uint_least8_t* tmpBuf = new uint_least8_t[bufferLen];
#endif
    if ( tmpBuf == 0 )
    {
        m_info.statusString = SidTune::txt_notEnoughMemory;
        return;
    }
    memcpy(tmpBuf,buffer,bufferLen);

    Buffer_sidtt<const uint_least8_t> buf1(tmpBuf, bufferLen);

    bool foundFormat = false;
    // Here test for the possible single file formats. --------------
    LoadStatus ret = PSID_fileSupport( buf1 );
    if ( ret != LOAD_NOT_MINE )
    {
        if ( ret == LOAD_ERROR )
            return;
        foundFormat = true;
    }
    else
    {
        Buffer_sidtt<const uint_least8_t> buf2;  // empty
        ret = MUS_fileSupport(buf1,buf2);
        if ( ret != LOAD_NOT_MINE )
        {
            if ( ret == LOAD_ERROR )
                return;
            foundFormat = MUS_mergeParts(buf1,buf2);
        }
        else
        {
            // No further single-file-formats available.
            m_info.statusString = SidTune::txt_unrecognizedFormat;
        }
    }

    if ( foundFormat )
    {
        status = acceptSidTune("-","-",buf1);
    }
}

bool SidTune::acceptSidTune(const char* dataFileName, const char* infoFileName,
                            Buffer_sidtt<const uint_least8_t>& buf)
{
    // @FIXME@ - MUS
    if ( m_info.numberOfInfoStrings == 3 )
    {   // Add <?> (HVSC standard) to missing title, author, release fields
        for (int i = 0; i < 3; i++)
        {
            if (infoString[i][0] == '\0')
            {
                strcpy (&infoString[i][0], "<?>");
                m_info.infoString[i] = &infoString[i][0];
            }
        }
    }

    deleteFileNameCopies();
    // Make a copy of the data file name and path, if available.
    if ( dataFileName != 0 )
    {
        m_info.path = SidTuneTools::myStrDup(dataFileName);
        if (isSlashedFileName)
        {
            m_info.dataFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(m_info.path));
            *SidTuneTools::slashedFileNameWithoutPath(m_info.path) = 0;  // path only
        }
        else
        {
            m_info.dataFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(m_info.path));
            *SidTuneTools::fileNameWithoutPath(m_info.path) = 0;  // path only
        }
        if ((m_info.path==0) || (m_info.dataFileName==0))
        {
            m_info.statusString = SidTune::txt_notEnoughMemory;
            return false;
        }
    }
    else
    {
        // Provide empty strings.
        m_info.path = SidTuneTools::myStrDup("");
        m_info.dataFileName = SidTuneTools::myStrDup("");
    }
    // Make a copy of the info file name, if available.
    if ( infoFileName != 0 )
    {
        char* tmp = SidTuneTools::myStrDup(infoFileName);
        if (isSlashedFileName)
            m_info.infoFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(tmp));
        else
            m_info.infoFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(tmp));
        if ((tmp==0) || (m_info.infoFileName==0))
        {
            m_info.statusString = SidTune::txt_notEnoughMemory;
            return false;
        }
        delete[] tmp;
    }
    else
    {
        // Provide empty string.
        m_info.infoFileName = SidTuneTools::myStrDup("");
    }
    // Fix bad sidtune set up.
    if (m_info.songs > SIDTUNE_MAX_SONGS)
        m_info.songs = SIDTUNE_MAX_SONGS;
    else if (m_info.songs == 0)
        m_info.songs++;
    if (m_info.startSong > m_info.songs)
        m_info.startSong = 1;
    else if (m_info.startSong == 0)
        m_info.startSong++;

    if ( m_info.musPlayer )
        MUS_setPlayerAddress();

    m_info.dataFileLen = buf.len();
    m_info.c64dataLen = buf.len() - fileOffset;

    // Calculate any remaining addresses and then
    // confirm all the file details are correct
    if ( resolveAddrs(m_info, buf.get() + fileOffset) == false )
        return false;
    if ( checkRelocInfo(m_info) == false )
        return false;
    if ( checkCompatibility(m_info) == false )
        return false;

    if (m_info.dataFileLen >= 2)
    {
        // We only detect an offset of two. Some position independent
        // sidtunes contain a load address of 0xE000, but are loaded
        // to 0x0FFE and call player at 0x1000.
        m_info.fixLoad = (endian_little16(buf.get()+fileOffset)==(m_info.loadAddr+2));
    }

    // Check the size of the data.
    if ( m_info.c64dataLen > SIDTUNE_MAX_MEMORY )
    {
        m_info.statusString = SidTune::txt_dataTooLong;
        return false;
    }
    else if ( m_info.c64dataLen == 0 )
    {
        m_info.statusString = SidTune::txt_empty;
        return false;
    }

    cache.assign(buf.xferPtr(),buf.xferLen());

    m_info.statusString = SidTune::txt_noErrors;
    return true;
}

bool SidTune::createNewFileName(Buffer_sidtt<char>& destString,
                                const char* sourceName,
                                const char* sourceExt)
{
    Buffer_sidtt<char> newBuf;
    uint_least32_t newLen = strlen(sourceName)+strlen(sourceExt)+1;
    // Get enough memory, so we can appended the extension.
#ifdef HAVE_EXCEPTIONS
    newBuf.assign(new(std::nothrow) char[newLen],newLen);
#else
    newBuf.assign(new char[newLen],newLen);
#endif
    if ( newBuf.isEmpty() )
    {
        m_info.statusString = SidTune::txt_notEnoughMemory;
        return (status = false);
    }
    strcpy(newBuf.get(),sourceName);
    strcpy(SidTuneTools::fileExtOfPath(newBuf.get()),sourceExt);
    destString.assign(newBuf.xferPtr(),newBuf.xferLen());
    return true;
}

// Initializing the object based upon what we find in the specified file.

void SidTune::getFromFiles(const char* fileName)
{
    // Assume a failure, so we can simply return.
    status = false;

    Buffer_sidtt<const uint_least8_t> fileBuf1, fileBuf2;
    Buffer_sidtt<char> fileName2;

    // Try to load the single specified file.  The original method didn't
    // quite work that well, so instead we now let the support files take
    // ownership of a known file and don't assume we should just
    // continue searching when an error is found.
    if ( loadFile(fileName,fileBuf1) )
    {
        // File loaded. Now check if it is in a valid single-file-format.
        LoadStatus ret = PSID_fileSupport(fileBuf1);
        if (ret != LOAD_NOT_MINE)
        {
            if (ret == LOAD_OK)
                status = acceptSidTune(fileName,0,fileBuf1);
            return;
        }

// -------------------------------------- Support for multiple-files formats.
// We cannot simply try to load additional files, if a description file was
// specified. It would work, but is error-prone. Imagine a filename mismatch
// or more than one description file (in another) format. Any other file
// with an appropriate file name can be the C64 data file.

// First we see if ``fileName'' could be a raw data file. In that case we
// have to find the corresponding description file.

        // Right now we do not have a second file (fileBuf2 empty). This
        // will not hurt the file support procedures.

        // Make sure that ``fileBuf1'' does not contain a description file.
        ret = (LoadStatus)( SID_fileSupport (fileBuf2, fileBuf1) |
                            INFO_fileSupport(fileBuf2, fileBuf1) );
        if ( ret == LOAD_NOT_MINE )
        {
            // Assuming ``fileName'' to hold the name of the raw data file,
            // we now create the name of a description file (=fileName2) by
            // appending various filename extensions.

// ------------------------------------------ Looking for a description file.

            int n = 0;
            while (fileNameExtensions[n] != 0)
            {
                if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
                    return;
                // 1st data file was loaded into ``fileBuf1'',
                // so we load the 2nd one into ``fileBuf2''.
                // Do not load the first file again if names are equal.
                if ( MYSTRICMP(fileName,fileName2.get())!=0 &&
                        loadFile(fileName2.get(),fileBuf2) )
                {
                    if ( (SID_fileSupport (fileBuf1, fileBuf2) == LOAD_OK)
                        || (INFO_fileSupport(fileBuf1, fileBuf2) == LOAD_OK)
                        )
                    {
                        status = acceptSidTune(fileName,fileName2.get(),
                                                fileBuf1);
                        return;
                    }
                }
                n++;
            };

// --------------------------------------- Could not find a description file.

            // Try some native C64 file formats
            ret = MUS_fileSupport(fileBuf1,fileBuf2);
            if (ret != LOAD_NOT_MINE)
            {
                if (ret == LOAD_ERROR)
                    return;

                // Try to find second file.
                int n = 0;
                while (fileNameExtensions[n] != 0)
                {
                    if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
                        return;
                    // 1st data file was loaded into ``fileBuf1'',
                    // so we load the 2nd one into ``fileBuf2''.
                    // Do not load the first file again if names are equal.
                    if ( MYSTRICMP(fileName,fileName2.get())!=0 &&
                        loadFile(fileName2.get(),fileBuf2) )
                    {
                        // Check if tunes in wrong order and therefore swap them here
                        if ( MYSTRICMP (fileNameExtensions[n], ".mus")==0 )
                        {
                            if ( MUS_fileSupport(fileBuf2,fileBuf1) == LOAD_OK )
                            {
                                if ( MUS_mergeParts(fileBuf2,fileBuf1) )
                                    status = acceptSidTune(fileName2.get(),fileName,
                                                        fileBuf2);
                                return;
                            }
                        }
                        else
                        {
                            if ( MUS_fileSupport(fileBuf1,fileBuf2) == LOAD_OK )
                            {
                                if ( MUS_mergeParts(fileBuf1,fileBuf2) )
                                    status = acceptSidTune(fileName,fileName2.get(),
                                                        fileBuf1);
                                return;
                            }
                        }
                        // The first tune loaded ok, so ignore errors on the
                        // second tune, may find an ok one later
                    }
                    n++;
                };
                // No (suitable) second file, so reload first without second
                fileBuf2.erase();
                MUS_fileSupport(fileBuf1,fileBuf2);
                status = acceptSidTune(fileName,0,fileBuf1);
                return;
            }

            // Now directly support x00 (p00, etc)
            ret = X00_fileSupport(fileName,fileBuf1);
            if (ret != LOAD_NOT_MINE)
            {
                if (ret == LOAD_OK)
                    status = acceptSidTune(fileName,0,fileBuf1);
                return;
            }

            // Now directly support prgs and equivalents
            ret = PRG_fileSupport(fileName,fileBuf1);
            if (ret != LOAD_NOT_MINE)
            {
                if (ret == LOAD_OK)
                    status = acceptSidTune(fileName,0,fileBuf1);
                return;
            }

            m_info.statusString = SidTune::txt_unrecognizedFormat;
            return;
        }

// -------------------------------------------------------------------------
// Still unsuccessful ? Probably one put a description file name into
// ``fileName''. Assuming ``fileName'' to hold the name of a description
// file, we now create the name of the data file and swap both used memory
// buffers - fileBuf1 and fileBuf2 - when calling the format support.
// If it works, the second file is the data file ! If it is not, but does
// exist, we are out of luck, since we cannot detect data files.

        // Make sure ``fileBuf1'' contains a description file.
        else if ( ret == LOAD_OK )
        {

// --------------------- Description file found. --- Looking for a data file.

            int n = 0;
            while (fileNameExtensions[n] != 0)
            {
                if ( !createNewFileName(fileName2,fileName,fileNameExtensions[n]) )
                    return;
                // 1st info file was loaded into ``fileBuf'',
                // so we load the 2nd one into ``fileBuf2''.
                // Do not load the first file again if names are equal.
                if ( MYSTRICMP(fileName,fileName2.get())!=0 &&

                    loadFile(fileName2.get(),fileBuf2) )
                {
// -------------- Some data file found, now identifying the description file.

                    if ( (SID_fileSupport (fileBuf2,fileBuf1) == LOAD_OK)
                        || (INFO_fileSupport(fileBuf2,fileBuf1) == LOAD_OK)
                        )
                    {
                        status = acceptSidTune(fileName2.get(),fileName,
                                                fileBuf2);
                        return;
                    }
                }
                n++;
            };

// ---------------------------------------- No corresponding data file found.

            m_info.statusString = SidTune::txt_noDataFile;
            return;
        } // end else if ( = is description file )

// ---------------------------------------------------------- File I/O error.

    } // if loaddatafile
    else
    {
        // returned fileLen was 0 = error. The m_info.statusString is
        // already set then.
        return;
    }
} 

void SidTune::convertOldStyleSpeedToTables(uint_least32_t speed, int clock)
{
    // Create the speed/clock setting tables.
    //
    // This routine implements the PSIDv2NG compliant speed conversion.  All tunes
    // above 32 use the same song speed as tune 32
    int toDo = ((m_info.songs <= SIDTUNE_MAX_SONGS) ? m_info.songs : SIDTUNE_MAX_SONGS);
    for (int s = 0; s < toDo; s++)
    {
        clockSpeed[s] = clock;
        if (speed & 1)
            songSpeed[s] = SIDTUNE_SPEED_CIA_1A;
        else
            songSpeed[s] = SIDTUNE_SPEED_VBI;
        if (s < 31)
            speed >>= 1;
    }
}

//
// File format conversion ---------------------------------------------------
//

bool SidTune::saveToOpenFile(std::ofstream& toFile, const uint_least8_t* buffer,
                             uint_least32_t bufLen )
{
    uint_least32_t lenToWrite = bufLen;
    while ( lenToWrite > INT_MAX )
    {
        toFile.write((char*)buffer+(bufLen-lenToWrite),INT_MAX);
        lenToWrite -= INT_MAX;
    }
    if ( lenToWrite > 0 )
        toFile.write((char*)buffer+(bufLen-lenToWrite),lenToWrite);
    if ( toFile.bad() )
    {
        m_info.statusString = SidTune::txt_fileIoError;
        return false;
    }
    else
    {
        m_info.statusString = SidTune::txt_noErrors;
        return true;
    }
}

bool SidTune::saveC64dataFile( const char* fileName, bool overWriteFlag )
{
    bool success = false;  // assume error
    // This prevents saving from a bad object.
    if ( status )
    {
        // Open binary output file stream.
        openmode createAttr = std::ios::out;
#if defined(HAVE_IOS_BIN)
        createAttr |= std::ios::bin;
#else
        createAttr |= std::ios::binary;
#endif
        if ( overWriteFlag )
            createAttr |= std::ios::trunc;
        else
            createAttr |= std::ios::app;
        std::ofstream fMyOut( fileName, createAttr );
        if ( !fMyOut || fMyOut.tellp()>0 )
        {
            m_info.statusString = SidTune::txt_cantCreateFile;
        }
        else
        {
            if ( !m_info.musPlayer )
            {
                // Save c64 lo/hi load address.
                uint_least8_t saveAddr[2];
                saveAddr[0] = m_info.loadAddr & 255;
                saveAddr[1] = m_info.loadAddr >> 8;
                fMyOut.write((char*)saveAddr,2);
            }

            // Data starts at: bufferaddr + fileOffset
            // Data length: m_info.dataFileLen - fileOffset
            if ( !saveToOpenFile( fMyOut,cache.get()+fileOffset, m_info.dataFileLen - fileOffset ) )
            {
                m_info.statusString = SidTune::txt_fileIoError;
            }
            else
            {
                m_info.statusString = SidTune::txt_noErrors;
                success = true;
            }
            fMyOut.close();
        }
    }
    return success;
}

bool SidTune::saveSIDfile( const char* fileName, bool overWriteFlag )
{
    bool success = false;  // assume error
    // This prevents saving from a bad object.
    if ( status )
    {
        // Open ASCII output file stream.
        openmode createAttr = std::ios::out;
        if ( overWriteFlag )
            createAttr |= std::ios::trunc;
        else
            createAttr |= std::ios::app;
        std::ofstream fMyOut( fileName, createAttr );
        if ( !fMyOut || fMyOut.tellp()>0 )
        {
            m_info.statusString = SidTune::txt_cantCreateFile;
        }
        else
        {
            if ( !SID_fileSupportSave( fMyOut ) )
            {
                m_info.statusString = SidTune::txt_fileIoError;
            }
            else
            {
                m_info.statusString = SidTune::txt_noErrors;
                success = true;
            }
            fMyOut.close();
        }
    }
    return success;
}

bool SidTune::savePSIDfile( const char* fileName, bool overWriteFlag )
{
    bool success = false;  // assume error
    // This prevents saving from a bad object.
    if ( status )
    {
        // Open binary output file stream.
        openmode createAttr = std::ios::out;
#if defined(HAVE_IOS_BIN)
        createAttr |= std::ios::bin;
#else
        createAttr |= std::ios::binary;
#endif
      if ( overWriteFlag )
            createAttr |= std::ios::trunc;
        else
            createAttr |= std::ios::app;
        std::ofstream fMyOut( fileName, createAttr );
        if ( !fMyOut || fMyOut.tellp()>0 )
        {
            m_info.statusString = SidTune::txt_cantCreateFile;
        }
        else
        {
            if ( !PSID_fileSupportSave( fMyOut,cache.get() ) )
            {
                m_info.statusString = SidTune::txt_fileIoError;
            }
            else
            {
                m_info.statusString = SidTune::txt_noErrors;
                success = true;
            }
            fMyOut.close();
        }
    }
    return success;
}

bool SidTune::checkRelocInfo (SidTuneInfo &info)
{
    uint_least8_t startp, endp;

    // Fix relocation information
    if (info.relocStartPage == 0xFF)
    {
        info.relocPages = 0;
        return true;
    }
    else if (info.relocPages == 0)
    {
        info.relocStartPage = 0;
        return true;
    }

    // Calculate start/end page
    startp = info.relocStartPage;
    endp   = (startp + info.relocPages - 1) & 0xff;
    if (endp < startp)
    {
        info.statusString = txt_badReloc;
        return false;
    }

    {    // Check against load range
        uint_least8_t startlp, endlp;
        startlp = (uint_least8_t) (info.loadAddr >> 8);
        endlp   = startlp;
        endlp  += (uint_least8_t) ((info.c64dataLen - 1) >> 8);

        if ( ((startp <= startlp) && (endp >= startlp)) ||
             ((startp <= endlp)   && (endp >= endlp)) )
        {
            info.statusString = txt_badReloc;
            return false;
        }
    }

    // Check that the relocation information does not use the following
    // memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF
    if ((startp < 0x04)
        || ((0xa0 <= startp) && (startp <= 0xbf))
        || (startp >= 0xd0)
        || ((0xa0 <= endp) && (endp <= 0xbf))
        || (endp >= 0xd0))
    {
        info.statusString = txt_badReloc;
        return false;
    }
    return true;
}

bool SidTune::resolveAddrs (SidTuneInfo &info, const uint_least8_t *c64data)
{   // Originally used as a first attempt at an RSID
    // style format. Now reserved for future use
    if ( info.playAddr == 0xffff )
        info.playAddr  = 0;

    // loadAddr = 0 means, the address is stored in front of the C64 data.
    if ( info.loadAddr == 0 )
    {
        if ( info.c64dataLen < 2 )
        {
            info.statusString = txt_corrupt;
            return false;
        }
        info.loadAddr = endian_16( *(c64data+1), *c64data );
        fileOffset += 2;
        c64data += 2;
        info.c64dataLen -= 2;
    }

    if ( info.compatibility == SIDTUNE_COMPATIBILITY_BASIC )
    {
        if ( info.initAddr != 0 )
        {
            info.statusString = txt_badAddr;
            return false;
        }
    }
    else if ( info.initAddr == 0 )
        info.initAddr = info.loadAddr;
    return true;
}

bool SidTune::checkCompatibility (SidTuneInfo &info)
{
    switch ( info.compatibility )
    {
    case SIDTUNE_COMPATIBILITY_R64:
        // Check valid init address
        switch (info.initAddr >> 12)
        {
        case 0x0F:
        case 0x0E:
        case 0x0D:
        case 0x0B:
        case 0x0A:
            info.statusString = txt_badAddr;
            return false;
        default:
            if ( (info.initAddr < info.loadAddr) ||
                 (info.initAddr > (info.loadAddr + info.c64dataLen - 1)) )
            {
                info.statusString = txt_badAddr;
                return false;
            }
        }
        // deliberate run on

    case SIDTUNE_COMPATIBILITY_BASIC:
        // Check tune is loadable on a real C64
        if ( info.loadAddr < SIDTUNE_R64_MIN_LOAD_ADDR )
        {
            info.statusString = txt_badAddr;
            return false;
        }
        break;
    }
    return true;
}

// returns 0 for no decompression (buf unchanged), 1 for decompression and -1 for error
int SidTune::decompressPP20(Buffer_sidtt<const uint_least8_t>& buf)
{
    // Check for PowerPacker compression: load and decompress, if PP20 file.
    PP20 myPP;
    uint_least32_t fileLen;
    if ( myPP.isCompressed(buf.get(),buf.len()) )
    {
        uint_least8_t* destBufRef = 0;
        if ( 0 == (fileLen = myPP.decompress(buf.get(),buf.len(),
                                             &destBufRef)) )
        {
            m_info.statusString = myPP.getStatusString();
            return -1;
        }
        else
        {
            m_info.statusString = myPP.getStatusString();
            // Replace compressed buffer with uncompressed buffer.
            buf.assign(destBufRef,fileLen);
        }
        return 1;
    }
    return 0;
}

int SidTune::convertPetsciiToAscii(SmartPtr_sidtt<const uint8_t>& spPet, char* dest)
{
    int count = 0;
    char c;
    if (dest)
    {
        do
        {
            c = _sidtune_CHRtab[*spPet];  // ASCII CHR$ conversion
            if ((c>=0x20) && (count<=31))
                dest[count++] = c;  // copy to info string

            // If character is 0x9d (left arrow key) then move back.
            if ((*spPet==0x9d) && (count>0))
                count--;
            spPet++;
        }
        while ( !((c==0x0D)||(c==0x00)||spPet.fail()) );
    }
    else
    {   // Just find end of string
        do
        {
            c = _sidtune_CHRtab[*spPet];  // ASCII CHR$ conversion
            spPet++;
        }
        while ( !((c==0x0D)||(c==0x00)||spPet.fail()) );
    }
    return count;
}
