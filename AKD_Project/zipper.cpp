#include "zipper.h"
#include <zlib.h>
#include <algorithm>
#include <sstream>
#include <time.h>

namespace akdzlib
{
	const unsigned int BUFSIZE = 2048;

	zipper::zipper() :
		zipFile_( 0 ), 
		entryOpen_( false )
	{
	}

	zipper::~zipper(void)
	{
		close();
	}

	// Create a new zip file.
	// param:
	//		filename	path and the filename of the zip file to open
	//		append		set true to append the zip file
	// return: true if open, false otherwise
	bool zipper::open( const char* filename, bool append ) 
	{
		close();
		zipFile_ = zipOpen64( filename, append ? APPEND_STATUS_ADDINZIP:0 );

		return isOpen();
	}

	// Close the zip file
	void zipper::close()
	{
		if ( zipFile_ )
		{
			closeEntry();
			zipClose( zipFile_, 0 );
			zipFile_ = 0;
		}
	}

	// Check if a zipfile is open.
	// return: true if open, false otherwise
	bool zipper::isOpen()
	{
		return zipFile_ != 0;
	}

	// Create a zip entry; either file or folder. Folder has to 
	// end with a slash or backslash.
	// return: true if open, false otherwise
	bool zipper::addEntry( const char* filename, bool bz2Compression, int compressionLevel)
	{
		if ( isOpen() )
		{
			closeEntry();

			while( filename[0] == '\\' || filename[0] == '/' )
			{
				filename++;
			}
			
			zip_fileinfo zi = {0};
			getTime( zi.tmz_date );

			int err = 0;
			if(!bz2Compression)
			{
				if (compressionLevel != Z_NO_COMPRESSION &&
					compressionLevel != Z_BEST_SPEED &&
					compressionLevel != Z_BEST_COMPRESSION &&
					compressionLevel != Z_DEFAULT_COMPRESSION ) compressionLevel = Z_DEFAULT_COMPRESSION;

				err = zipOpenNewFileInZip( zipFile_, filename, &zi,
				NULL, 0, NULL, 0, NULL, Z_DEFLATED, compressionLevel);
			}
			else
			{
				if (compressionLevel > 9) compressionLevel = 9;
				if (compressionLevel < 1) compressionLevel = 1;
				 err = zipOpenNewFileInZip(zipFile_, filename, &zi,
					NULL, 0, NULL, 0, NULL, Z_BZIP2ED, compressionLevel);
			}

			entryOpen_ = (err == ZIP_OK);
		}
		return entryOpen_;
	}

	// Close the currently open zip entry.
	void zipper::closeEntry()
	{
		if ( entryOpen_ )
		{
			zipCloseFileInZip( zipFile_ );
			entryOpen_ = false;
		}
	}

	// Check if there is a currently open file zip entry.
	// return: true if open, false otherwise
	bool zipper::isOpenEntry()
	{
		return entryOpen_;
	}

	bool zipper::addRawEntry(const char * filename, bool bz2Compression)
	{
		if (isOpen())
		{
			closeEntry();

			while (filename[0] == '\\' || filename[0] == '/')
			{
				filename++;
			}

			zip_fileinfo zi = { 0 };
			getTime(zi.tmz_date);

			//zipRemoveExtraInfoBlock(pLocalHeaderExtraFieldData, &nLocalHeaderExtraFieldDataLen, 0x0001);
			int err = 0;

			err = zipOpenNewFileInZip4_64(zipFile_, filename, &zi,
				NULL, 0, NULL, 0, NULL, bz2Compression ? Z_BZIP2ED : Z_DEFLATED, NULL, 1, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, 0, 0);

			

			entryOpen_ = (err == ZIP_OK);
		}
		return entryOpen_;
		
	}
	
	// Stream operator for dumping data from an input stream to the 
	// currently open zip entry.
	zipper& zipper::operator<<( std::istream& is )
	{
		int err = ZIP_OK;
		char buf[BUFSIZE];
		unsigned long nRead = 0;

		if ( isOpenEntry() )
		{
			while (err == ZIP_OK && is.good() )
			{
				is.read( buf, BUFSIZE );
				unsigned int nRead = (unsigned int)is.gcount();

				if ( nRead )
				{
					err = zipWriteInFileInZip( zipFile_, buf, nRead );
				}
				else
				{
					break;
				}
			}
		}
		return *this;
	}

	void zipper::WriteRawData(char* data, int length)
	{
		int err = ZIP_OK;

		err = zipWriteInFileInZip(zipFile_, data, length);
		zipCloseFileInZipRaw(zipFile_, 0, 0);

		return ;
	}

	// Fill the zip time structure
	// param: tmZip	time structure to be filled
	void zipper::getTime(tm_zip& tmZip)
	{
		time_t rawtime;
		time (&rawtime);
		auto timeinfo = localtime(&rawtime);
		tmZip.tm_sec = timeinfo->tm_sec;
		tmZip.tm_min = timeinfo->tm_min;
		tmZip.tm_hour = timeinfo->tm_hour;
		tmZip.tm_mday = timeinfo->tm_mday;
		tmZip.tm_mon = timeinfo->tm_mon;
		tmZip.tm_year = timeinfo->tm_year;
	}
};
