/*!**************************************************************************
\file    TBIF.h
\author  Michal Havlicek, Jiri Filip, Radomir Vavra
\date    2017/02/27
\version 2.36

BIG data format for multiple image sequences encapsulation:

features:	- fast data loading to memory
			- fast memory access or seeking from HDD (if stored data exceeds memory)
application:	- BTF data
				- dynamic textures

TBIG.h - header file with encoding and decoding procedures

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

using namespace std;
#include <list>
#include <string>

#include "TAlloc.h"
#include "half.h"

// for image data loading
#include "readexr.h"
#include "lodepng.h"

#define halffloatsize 2
#define floatsize 4
#define intsize 4
#define longsize 4
#define longlongsize 8

#define errormessage 1
#define warningmessage 2
#define notemessage 3

class TBIG
{
	private:
		void 
			* data;
		char 
			* map,
			* message, 
			* name, 
			* text,
			* xml;
		int 
			errors,
			notes,
			warnings;
		long long 
			begin,
			datasize,
			height,
			images,
			space,
			spectra, 
			tiles,
			type,
			width,
			* begins;
		float 
			dpi,
			ppmm,
			*angles_device,
			*angles_ideal,
			*angles_real;
		FILE 
			* source;

		void error(char * message, char * position);
		void init();
		void note(char * message, char * position);
        void set_message(char * message, char * position, char type);
		void warning(char * message, char * position);

		int test();
		int write(FILE * file, long long id, long long length, void * data);
		int include_file(char * filename, char * text);

		unsigned short get_data_short(long long idx);
		float get_data_float(long long idx);

	public:
		TBIG();
		~TBIG();


		/*! TBIG:load - the BIG file loading 
			bigname: filename for loading existing file (including path), 
			usemem (0/1): 1 - load to memory or 0 - seek from HDD
			memlimit: user defined mem limit for data loading
		*/
		int load(const char * bigname, bool usemem=true, long long memlimit=-1);

		//! TBIG:report - printing loaded file information
		void report();

		/*! TBIG:get_pixel - a look-up function for a specific pixels
			tile: number of image tile (default 0)
			y: row index
			x: column index
			which: file index (according to the saving sequence)
			RGB: return values
		*/
		void get_pixel(int tile, int y, int x, int which, float RGB[]);

		/*! TBIG:save - the BIG creation
			filename: filename to save a new file (including path), 
			file_list: list of filenames (including path) to be included in a datafile
			sangles1: array of angles in spherical system [theta_i, phi_i, theta_v, phi_v] for all included files (real measured values) 
			sangles2: array of angles in spherical system [theta_i, phi_i, theta_v, phi_v] for all included files (ideal measured values) 
			sangles3: array of angles in a coordinate sytem of measurement device
			stiles: number of image tiles stored for each image
			sspace: color space og the data
			sppmm: spatial resolution of the data - pixels per mm
			sdpi: DPI of the measured data
			stextfile: user notes related to data
			sname: name of the material
			sxmlfile: additional XML info
		*/
		//int save(char * filename, list<string>file_list, list<float>sangles1, list<float>sangles2, list<float>sangles3,
		//long long stiles=1, long long sspace=-1, float sppmm=-1.0, float sdpi=-1.0, char * stextfile=0, char * sname=0, char * sxmlfile=0);

		/*! TBIG:saveTXT - the BIG creation based on a list of files in a TXT file
			filename: filename to save a new file (including path), 
			listfile: filename of TXT file (including path), (see example in script.txt) 
		*/
		//int saveTXT(char * filename, char * listfile);


		//! other functions for querying individual data chunks
		void get_angles(int index, float angles[]);

		char * get_message();
		char * get_name();		
		char * get_space();
		char * get_text();
		char * get_type();
		char * get_xml();

		int get_errors();
		int get_notes();
		int get_warnings();

		long long get_height();
		long long get_images();
		long long get_spectra();
		long long get_tiles();
		long long get_width();

		float get_dpi();
		float get_ppmm();

};//--- TBIG ---


