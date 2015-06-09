
struct __attribute__ ((__packed__)) RLASWindow {
	short left;
	short right;
	short bottom;
	short top;
	};

const unsigned short RLA_MAGIC_OLD = 0xFFFE;
const unsigned short RLA_MAGIC  = 0xFFFD; // started using this with R3.1 
const int RLA_Y_PAGE_SIZE = 32;

struct __attribute__ ((__packed__)) RLAHeader {
	RLASWindow window;
	RLASWindow active_window;
	short          frame;
	short          storage_type;
	short          num_chan;
	short          num_matte;
	short          num_aux;
	short          revision;
	char           gamma[16];
	char           red_pri[24];
	char           green_pri[24];
	char           blue_pri[24];
	char           white_pt[24];
	long           job_num;
	char           name[128];
	char           desc[128];
	char           program[64];
	char           machine[32];
	char           user[32];
	char           date[20];
	char           aspect[24];
	char           aspect_ratio[8];
	char           chan[32];
	short          field;
	char           time[12];
	char           filter[32];
	short          chan_bits;
	short          matte_type;
	short          matte_bits;
	short          aux_type;
	short          aux_bits;
	char           aux[32];
	char           space[36];
	long           next;
	};


#ifndef _RLAFMTSPEC_
#define _RLAFMTSPEC_

//-----------------------------------------------------------------------------
//-- RLA data Structure -------------------------------------------------------
//
//	Channels are defined in gbuf.h:
//
//	BMM_CHAN_Z
//	BMM_CHAN_MTL_ID
//	BMM_CHAN_NODE_ID
//	BMM_CHAN_UV
//	BMM_CHAN_NORMAL
//	BMM_CHAN_REALPIX
//    etc..

#define RLAVERSION 301

struct RLAUSERDATA {
	DWORD	version;		//-- Reserved
	DWORD	channels;		//-- Bitmap with channels to save
	BOOL	usealpha;		//-- Save Alpha (if present)
	BYTE 	rgb;			//-- 0=8b, 1=16b, 2=32b
	BOOL 	defaultcfg;		//-- Reserved
	char 	desc[128];		//-- Description (ASCII)
	char 	user[32];		//-- User Name (ASCII)
	BOOL	premultAlpha;   //-- Premultiply Alpha
};

#endif


#define NUMGBCHAN 14

// GBuffer channels (number of bytes in parenthesis)
#define GB_Z       			0  	// (4)  Z-Buffer depth, float
#define GB_MTL_ID  			1  	// (1)  ID assigned to mtl via mtl editor
#define GB_NODE_ID 			2  	// (2)  ID assigned to node via properties
#define GB_UV       		3 	// (8)  UV coordinates - Point2 
#define GB_NORMAL   		4 	// (4)  Normal vector in view space, compressed 
#define GB_REALPIX  		5 	// (4)  Non clamped colors in "RealPixel" format 
#define GB_COVERAGE 		6 	// (1)  Pixel coverage  
#define GB_BG 	     		7 	// (3)  RGB color of what's behind layer 
#define GB_NODE_RENDER_ID 	8 	// (2)  Node render index, word
#define GB_COLOR		 	9 	// (3)  Color (RGB)
#define GB_TRANSP		 	10 	// (3)  Transparency (RGB)
#define GB_VELOC		 	11 	// (8)  Velocity (Point2)
#define GB_WEIGHT		 	12 	// (3)  Weight of layers contribution to pixel color
#define GB_MASK			 	13 	// (2)  Sub pixal coverage mask


// Recognized channel bits 

#define BMM_CHAN_NONE     0
#define BMM_CHAN_Z        (1<<GB_Z) 		//  Z-buffer depth, float 
#define BMM_CHAN_MTL_ID   (1<<GB_MTL_ID) 	//  ID assigned to mtl via mtl editor 
#define BMM_CHAN_NODE_ID  (1<<GB_NODE_ID) 	//  ID assigned to node via properties 
#define BMM_CHAN_UV       (1<<GB_UV) 		//  UV coordinates - Point2 
#define BMM_CHAN_NORMAL   (1<<GB_NORMAL) 	//  Normal vector in view space, compressed 
#define BMM_CHAN_REALPIX  (1<<GB_REALPIX) 	//  Non clamped colors in "RealPixel" format 
#define BMM_CHAN_COVERAGE (1<<GB_COVERAGE) 	//  Pixel coverage of front surface 
#define BMM_CHAN_BG 	  (1<<GB_BG) 		//  RGB color of what's behind front object 
#define BMM_CHAN_NODE_RENDER_ID (1<<GB_NODE_RENDER_ID) //  node render index 
#define BMM_CHAN_COLOR    (1<<GB_COLOR) 	//  Color (Color24) 
#define BMM_CHAN_TRANSP   (1<<GB_TRANSP) 	//  Transparency (Color24) 
#define BMM_CHAN_VELOC    (1<<GB_VELOC) 	//  Velocity ( Point2 ) 
#define BMM_CHAN_WEIGHT   (1<<GB_WEIGHT) 	//  Weight ( Color24 ) 
#define BMM_CHAN_MASK	  (1<<GB_MASK)   	//  Subpixel mask ( word ) 

// Recognized types of channels
#define BMM_CHAN_TYPE_UNKNOWN 0 
#define BMM_CHAN_TYPE_8   2 // 1 byte per pixel
#define BMM_CHAN_TYPE_16  3 // 1 word per pixel
#define BMM_CHAN_TYPE_24  8 // 3 bytes per pixel
#define BMM_CHAN_TYPE_32  4 // 2 words per pixel
#define BMM_CHAN_TYPE_48  5 // 3 words per pixel
#define BMM_CHAN_TYPE_64  6 // 4 words per pixel
#define BMM_CHAN_TYPE_96  7 // 6 words per pixel

//-- EOF: fmtspec.h -----------------------------------------------------------

//-----------------------------------------------------------------------------
//-- Class Definition ---------------------------------------------------------
//

class BitmapIO_RLA : public BitmapIO {
     public:
		BOOL 				isRPF;
		RLAHeader			hdr;     
		RLAUSERDATA			UserData;
		FILE*				inStream;
		float 				gamma;
		float 				aspect;
		ULONG 				gbChannels;
		RenderInfo 			*rendInfo;
		void				GetCfgFilename 	( TCHAR *filename );
		BOOL 				ReadCfg		   	( );
		void 				WriteCfg	   	( );
		int  				ReadHeader	   	( );
		void 				InitHeader		(RLAHeader &h, int width, int height, float aspect, 
								BOOL doAlpha, ULONG gbChannels, RenderInfo *ri, BOOL saveLayerData, BOOL saveNameTab);
		BOOL				GetHDRData(BMM_Color_fl* in, BMM_Color_fl* out, int y, int width);


     public:

        //-- Constructors/Destructors

        BitmapIO_RLA       (BOOL rlf=0);
        ~BitmapIO_RLA       ( );

		INT_PTR			Control				 ( HWND ,UINT ,WPARAM ,LPARAM );

        //-- Number of extemsions supported

        int            ExtCount           ( )       { return 1;}
    
        //-- Extension #n (i.e. "3DS")
        
        const TCHAR   *Ext                ( int n ) {return isRPF ? _T("rpf"):_T("rla"); }
        
        //-- Descriptions
        
        const TCHAR   *LongDesc           ( );
        const TCHAR   *ShortDesc          ( );

        //-- Miscelaneous Messages
        
        const TCHAR   *AuthorName         ( )       { return _T("Gus J Grubba");}
        const TCHAR   *CopyrightMessage   ( )       { return _T("Copyright 1995, Yost Group");}
        const TCHAR   *OtherMessage1      ( )       { return _T("");}
        const TCHAR   *OtherMessage2      ( )       { return _T("");}
        
        unsigned int   Version            ( )       { return (100);}

        //-- Driver capabilities
        
        int            Capability         ( )       { return BMMIO_READER    | 
                                                             BMMIO_WRITER    | 
                                                             BMMIO_INFODLG   |
                                                             BMMIO_EXTENSION |
                                                             BMMIO_CONTROLWRITE; }
        
        //-- Driver Configuration
        
        BOOL           LoadConfigure      ( void *ptr );
        BOOL           SaveConfigure      ( void *ptr );
        DWORD          EvaluateConfigure  ( );
        
        //-- Show DLL's "About..." box
        
        void           ShowAbout          ( HWND hWnd );  

        //-- Show DLL's Control Panel
        
        BOOL           ShowControl        ( HWND hWnd, DWORD flag );

        //-- Return info about image
        
        BMMRES   		GetImageInfo	( BitmapInfo *fbi );        
		INT_PTR 			ImageInfoDlg	(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
		BMMRES 			GetImageInfoDlg	(HWND hWnd, BitmapInfo *fbi, const TCHAR *filename);

        //-- Image Input
        
        BitmapStorage *Load               ( BitmapInfo *fbi, Bitmap *map, BMMRES *status);

        //-- Image Output
        
        BMMRES         OpenOutput         ( BitmapInfo *fbi, Bitmap *map );
        BMMRES         Write              ( int frame );
        int            Close              ( int flag );
        
        //-- Channels
        
        DWORD          ChannelsRequired   ( );

};


#endif

//-- EOF: rla.h ---------------------------------------------------------------

#include "rlarc.h"
#include "gamma.h"
#include <tab.h>

#define BIGFLOAT 1.0e30f
#define ALPHA_TRESHOLD (1.0f/65536.0f) 

static int encode(unsigned char * input, unsigned char* output, int xSize, int stride);
static BYTE* decode(BYTE *input, BYTE  *output,int xFile, int xImage, int stride);

// Characters to indicate presence of gbuffer channels in the 'program' string
static char gbufChars[] =   "ZEOUNRCBIGTVWM";
//GB_Z-----------------------||||||||||||||
//GB_MTL_ID-------------------|||||||||||||
//GB_NODE_ID-------------------||||||||||||
//GB_UV-------------------------|||||||||||
//GB_NORMAL----------------------||||||||||
//GB_REALPIX----------------------|||||||||
//GB_COVERAGE----------------------||||||||
//GB_BG-----------------------------|||||||
//GB_NODE_RENDER_ID -----------------||||||
//GB_COLOR----------------------------|||||
//GB_TRANSP----------------------------||||
//GB_VELOC------------------------------|||
//GB_WEIGHT------------------------------||
//GB_MASK---------------------------------|

// Other letters in "program" string
//  L  Layer data present
//  P  RenderInfo present
//  D  Node name table is present.
//  A  Non-premultiplied alpha

//-----------------------------------------------------------------------------



static void MakeProgString(char *s, ULONG chan, BOOL rendInfo, BOOL layerData, BOOL saveNameTab, BOOL premultAlpha) {
	//chan &= ~BMM_CHAN_NODE_RENDER_ID; // don't save this one;
#ifdef DESIGN_VER
	strcpy(s, "Autodesk VIZ : (");
#else
	strcpy(s, "3ds max : (");
#endif // DESIGN_VER
	char d[3];
	strcpy(d," X");
	for (int i=0; i<NUMGBCHAN; i++) {
		if (chan&(1<<i)) {
			d[1] = gbufChars[i];
			strcat(s,d);
			}
		}
	if (chan&&layerData) strcat(s," L");   // indicates layer data is present
	if (rendInfo)	
		strcat(s," P");
	if (saveNameTab)
		strcat(s, " D");
	if (!premultAlpha)
		strcat(s, " A");
	strcat(s, " )");
	}

static void MakeInfoString(TCHAR *s1, TCHAR *s2, ULONG chan, BOOL rendInfo) {
	chan &= ~BMM_CHAN_NODE_RENDER_ID; // don't save this one;
	_tcscpy(s1, _T(" "));
	_tcscpy(s2, _T(" "));
	BOOL first = 1;
	int nchar= 0;
	TCHAR* s = s1;
	for (int i=0; i<NUMGBCHAN; i++) {
		if (chan&(1<<i)) {
			if (!first)
				_tcscat(s,_T(", "));
			_tcscat(s,GBChannelName(i));
			first = 0;
			nchar += static_cast<int>(_tcslen(GBChannelName(i)));	// SR DCAST64: Downcast to 2G limit.
			if (nchar>45&&s==s1) {
				_tcscat(s1,_T(", "));
				s = s2;
				first = TRUE;
				}
			}
		}
	if (rendInfo) {
		if (!first)
			_tcscat(s,_T(", "));
		_tcscat(s, _T("Projection"));
		}
	}

static int findChar(char *s, char c) {
	for (int i=0; s[i]!=0; i++)
		if (c==s[i])
			return i;
	return -1;
	}

static ULONG ChannelsFromString(char *s, BOOL &gotRendInfo, BOOL &gotLayerData, BOOL &gotNodeNames, BOOL &gotPremultAlpha) {
	ULONG chan = 0;
	gotRendInfo = FALSE;
	gotPremultAlpha = TRUE;
	int i = findChar(s,'(');
	if (i<0)  
		return 0;
	for (i++; s[i]!=0; i++) {
		char c = s[i];
		int n = findChar(gbufChars, c);
		if (n>=0) 
			chan |= (1<<n);
		else switch(c) {
			case 'P': gotRendInfo = TRUE; break;
			case 'L': gotLayerData = TRUE; break;
			case 'D': gotNodeNames = TRUE; break;
			case 'A': gotPremultAlpha = FALSE; break;
			default: break;
			}
		}
	return chan;
	}

//-----------------------------------------------------------------------------
long lswap(long x) {
	return ((x >> 24) & 0x000000ff) |
         ((x >>  8) & 0x0000ff00) |
         ((x <<  8) & 0x00ff0000) |
         ((x << 24) & 0xff000000);
	}

//-----------------------------------------------------------------------------
short sswap(short x) {
	return ((x >> 8) & 0x00ff) |
         ((x << 8) & 0xff00);
	}

inline void SSW(short &s) { s = sswap(s); }
inline void LSW(long &l) { l = lswap(l); }

//-----------------------------------------------------------------------------
//-- File Class

class File {
     public:
        FILE  *stream;
        File  ( const TCHAR *name, const TCHAR *mode) { stream = _tfopen(name,mode); }
        ~File ( ) { Close(); 	}
        void Close() { if(stream) fclose(stream); stream = NULL; }
	};

class FloatBuf {
	public:
		float *p;
		FloatBuf(int n) { p = new float[n]; }
		~FloatBuf() { delete [] p; }
	};

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::BitmapIO_RLA()

BitmapIO_RLA::BitmapIO_RLA  ( int rlf ) { 
	isRPF = rlf;
	UserData.version		= RLAVERSION;
	UserData.channels		= BMM_CHAN_NONE;
	UserData.usealpha		= TRUE;
	UserData.premultAlpha		= TRUE;
	UserData.defaultcfg	= TRUE;
	UserData.rgb = 0;
	UserData.desc[0] = 0;
	UserData.user[0] = 0;
}

BitmapIO_RLA::~BitmapIO_RLA ( ) { }

//-----------------------------------------------------------------------------
// #> BitmapIO_JPEG::LoadConfigure()

BOOL BitmapIO_RLA::LoadConfigure ( void *ptr ) {
     RLAUSERDATA *buf = (RLAUSERDATA *)ptr;
     if (buf->version == RLAVERSION) {
        memcpy((void *)&UserData,ptr,sizeof(RLAUSERDATA));
		UserData.defaultcfg = FALSE;
        return (TRUE);
     } else
        return (FALSE);
}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::SaveConfigure()

BOOL BitmapIO_RLA::SaveConfigure ( void *ptr ) {
     if (ptr) {
		UserData.defaultcfg = FALSE;
        memcpy(ptr,(void *)&UserData,sizeof(RLAUSERDATA));
        return (TRUE);
     } else
        return (FALSE);
}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::EvaluateConfigure()

DWORD BitmapIO_RLA::EvaluateConfigure ( ) {
      return (sizeof(RLAUSERDATA));
}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::ChannelsRequired()

DWORD BitmapIO_RLA::ChannelsRequired() {
	if (UserData.defaultcfg)
		ReadCfg();
	if(UserData.rgb==2)
	{
		DWORD channels = UserData.channels |BMM_CHAN_REALPIX | BMM_CHAN_WEIGHT | BMM_CHAN_COVERAGE |BMM_CHAN_Z;
		return channels;
	}
	else return(UserData.channels);
}
     
//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::LongDesc()

const TCHAR *BitmapIO_RLA::LongDesc() {
     return GetString(isRPF?IDS_RPF_FILE:IDS_RLA_FILE);
}
     
//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::ShortDesc()


//-----------------------------------------------------------------------------
// *> BitmapIO_RLA::ReadHeader()
//
//    Read a .RLA file header

int BitmapIO_RLA::ReadHeader() {
	if(fread(&hdr,sizeof(RLAHeader),1,inStream)!=1)
		return 0;
	return 1;
}

static void SwapHdrBytes(RLAHeader& h) {
	SSW(h.window.left);
	SSW(h.window.right);
	SSW(h.window.top);
	SSW(h.window.bottom);
	SSW(h.active_window.left);
	SSW(h.active_window.right);
	SSW(h.active_window.top);
	SSW(h.active_window.bottom);
	SSW(h.frame);
	SSW(h.storage_type);
	SSW(h.num_chan);
	SSW(h.num_matte);
	SSW(h.num_aux);
	SSW(h.revision);
	LSW(h.job_num);
	SSW(h.field);
	SSW(h.chan_bits);
	SSW(h.matte_type);
	SSW(h.matte_bits);
	SSW(h.aux_bits);
	LSW(h.next);
}	

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::GetImageInfo()

BMMRES BitmapIO_RLA::GetImageInfo ( BitmapInfo *fbi ) {
     //-- Open RLA File -----------------------------------
     
	File file(fbi->Name(), _T("rb"));
	inStream = file.stream;
	if(inStream == NULL)
		return (ProcessImageIOError(fbi));

     //-- Read File Header --------------------------------
     
	if (!ReadHeader())
		return (ProcessImageIOError(fbi,BMMRES_BADFILEHEADER));

	SwapHdrBytes(hdr);
	fbi->SetWidth(hdr.active_window.right-hdr.active_window.left + 1);
	fbi->SetHeight(hdr.active_window.top-hdr.active_window.bottom + 1);
	sscanf( hdr.gamma, "%f", &gamma);
	fbi->SetGamma (gamma);
	sscanf( hdr.aspect_ratio, "%f", &aspect);
	fbi->SetAspect (aspect);
	fbi->SetFirstFrame(0);
	fbi->SetLastFrame(0);

	if (hdr.chan_bits==16) {
		if (hdr.num_matte>0)
			fbi->SetType(BMM_TRUE_64); 
		else
			fbi->SetType(BMM_TRUE_48); 
	} 
	else if (hdr.chan_bits==32) {
			fbi->SetType(BMM_FLOAT_RGBA_32); 
	} 
	else {
		if (hdr.num_matte>0)
			fbi->SetType(BMM_TRUE_32); 
		else
			fbi->SetType(BMM_TRUE_24); 
	}

	if(hdr.num_matte>0) {
		fbi->SetFlags(MAP_HAS_ALPHA);
	}

    return BMMRES_SUCCESS;

	}

BitmapInfo *infoBI;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// RLA Reader
//-----------------------------------------------------------------------------

class RLAReader {
	FILE* fd;
	long *offsets;
	BYTE *inp,*curInp;
	BYTE *out;
	int width, height;
	int maxLength, inited;
	RLAHeader* hdr;
	public:
		RLAReader(RLAHeader *hd,FILE *f, int w, int h);
		int Init();
		int ReadRendInfo( RenderInfo *ri );
		int ReadNameTab(NameTab &nt);
		int BeginLine( int y );
		int ReadNextChannel( BYTE *out, int stride,int w = -1, BOOL longLength=0 );	
		int ReadLowByte( BYTE *out, int stride );	
		int ReadFloatChannel(float *ptr, int stride, BOOL twoBytes);
		int ReadNChannels( BYTE *out, int n, int w=-1, BOOL longLength=0);	
		int ReadRGBA(BMM_Color_fl *pix);
		int ReadNumLRecs();
		~RLAReader();
	};

RLAReader::RLAReader(RLAHeader *hd, FILE *f, int w, int h) {
	hdr = hd;
	fd = f; 
	width = w; 
	height = h; 
//	maxLength = 6 * ((width * 280) / 256); 
	maxLength = 30 * width;  // more space for layers 
	offsets = NULL;
	inp = NULL;
	}

int RLAReader::Init() {
	offsets = new long[height];
	inp = new BYTE[maxLength];
	out = new BYTE[width];
	if (offsets==NULL||inp==NULL||out==NULL) return 0;
	// Read in table of offsets.
	size_t count = fread(offsets, sizeof(long), height, fd);
	if (count != height) return 0;
	for (int i=0; i<height; i++) offsets[i] = lswap(offsets[i]);
	return 1;
	}


#define RENDINFO_VERS1 1000

int RLAReader::ReadRendInfo(RenderInfo *ri) {
	short vers;
	if (fread(&vers,sizeof(short),1,fd)!=1) return 0;
	int size = sizeof(RenderInfo);
	if (vers != RENDINFO_VERS1) {
		// the old version didn't start with a version word, but
		// with the projType which is 0 or 1.
		size -= sizeof(Rect); // the old record didn't have the region Rect.
		//SS 7/11/2002: The sizeof operator returns an unsigned value; it needs
		// to be cast to a signed value before using the unary minus operator.
		// fseek expects a long as its second arg.
	    fseek(fd,-(long)(sizeof(short)),SEEK_CUR); // undo the version read
		}
	if (ri) {
		if (fread(ri,size,1,fd)!=1) return 0;
		}
	else 
		fseek(fd, size, SEEK_CUR);
	return 1;
	}

int RLAReader::ReadNameTab(NameTab &nt) {
	DWORD n;
	DWORD nchars;
	if (fread(&n,sizeof(DWORD),1,fd)!=1) return 0;
	if (fread(&nchars,sizeof(DWORD),1,fd)!=1) return 0;
	TCHAR *buf = (TCHAR *)malloc(nchars*sizeof(TCHAR));
	if (fread(buf,sizeof(TCHAR)*nchars,1,fd)!=1) {
		free(buf);
		return 0;
		}
	TCHAR *pb = buf;
	nt.SetSize(n); // allocate n
	nt.Shrink();
	nt.SetSize(0); // set count to 0;
	for (DWORD i=0; i<n; i++) {
		nt.AddName(pb);
		pb+=_tcslen(pb)+1;
		}
	free(buf);
	return 1;
	}

RLAReader::~RLAReader() {
	delete [] offsets;
	delete [] inp;
	delete [] out;
	}

int RLAReader::BeginLine(int y) {
	int yy = (height - 1) - y;
	// position at beginning of line;
	fseek(fd, offsets[yy], SEEK_SET);
	return 1;
	}

int RLAReader::ReadNextChannel( BYTE *out, int stride, int w, BOOL longLength) {	
	WORD length = 0;
	if (w<0) w = width;
	if (longLength) {
		int l = 0;
		if (fread(&l, 4, 1, fd)!=1)
			return 0;
		l = lswap(l);
		if (l>=maxLength)
			return 0;
		if (fread(inp, 1, l, fd) <= 0)
			return 0;
		}
	else {
		if (fread(&length, 2, 1, fd)!=1)
			return 0;
		length = sswap(length);
		if (length>=maxLength)
			return 0;
		if (fread(inp, 1, length, fd) <= 0)
			return 0;
		}
	curInp = decode(inp, out, w, w, stride);             
	return 1;
	}

int RLAReader::ReadLowByte( BYTE *out, int stride) {	
	curInp = decode(curInp, out, width, width, stride);             
	return 1;
	}

int RLAReader::ReadFloatChannel(float *ptr, int stride, BOOL twoBytes) {
	float *p = ptr;
	
	if(hdr->chan_bits==32)
	{

		// 32 bit float channels are not encoded,
		WORD length = 0;
		float *floatbuf;
		 {
			if (fread(&length, 2, 1, fd)!=1)
				return 0;
			length = sswap(length);
			if (length>=maxLength)
				return 0;
			if (fread(inp, 1, length, fd) <= 0)
				return 0;
			}
		floatbuf = (float*)inp;
		LONG *longbuf = (LONG*)inp;

		// lets swap  and store the bytes
		for (int i=0; i<width; i++) 
		{ 	longbuf[i] = lswap(longbuf[i]);
			*p = floatbuf[i];
			p += stride; 	
		}
	}
	else
	{
		if (!ReadNextChannel(out, 1)) return 0;
		for (int i=0; i<width; i++) 
		{ 	
			*p = out[i]/(256.0);	 
		    p += stride; 	
		}
		if (twoBytes) {
			if (!ReadLowByte(out, 1)) return 0;
			p = ptr;
			for (int i=0; i<width; i++) { 
				*p += (out[i]/65536.0);	
				p += stride; 
			} 
		}
	}
	return 1;
}

int RLAReader::ReadRGBA( BMM_Color_fl *pix) {
	BOOL do2 = (hdr->chan_bits==16)?1:0;
	if (!ReadFloatChannel(&pix->r, 4, do2)) return 0;
	if (!ReadFloatChannel(&pix->g, 4, do2)) return 0;
	if (!ReadFloatChannel(&pix->b, 4, do2)) return 0;
	if (hdr->num_matte>0)
		if (!ReadFloatChannel(&pix->a, 4, hdr->matte_bits==16?1:0)) return 0;
	return 1;
	}

int RLAReader::ReadNChannels( BYTE *out, int n, int w, BOOL longLength) {	
	// read n byte channels starting with high byte
	for (int i=n-1; i>=0; i--) 
		if (!ReadNextChannel( out+i, n, w, longLength)) 
			return 0;
	return 1;		
	}

int RLAReader::ReadNumLRecs() {
	long n;
	if (fread(&n, 4, 1, fd)!=1)
		return -1;
	return n;
	}

static int MaxGBSize(ULONG gbChannels) {
	int sz = 0;
	for (int i=0; i<NUMGBCHAN; i++) {
		if (gbChannels&(1<<i)) {
			int s = GBDataSize(i);
			if (s>sz) sz = s;
			}
		}
	if (sz<2) sz = 2;
	return sz;
	}


//-----------------------------------------------------------------------------
//-- BitmapIO_RLA::Load()
//

BitmapStorage *BitmapIO_RLA::Load(BitmapInfo *fbi, Bitmap *map, BMMRES *status) {
	BitmapStorage *s = NULL;

	//-- Initialize Status Optimistically

	*status = BMMRES_SUCCESS;

	//-- Make sure nothing weird is going on

	if(openMode != BMM_NOT_OPEN) {
		*status = ProcessImageIOError(fbi,BMMRES_INTERNALERROR);
		return NULL;
		}
     //-- Open RLA File -----------------------------------
     
     File file(fbi->Name(), _T("rb"));

	 inStream = file.stream;
     if (inStream == NULL) {
		*status = ProcessImageIOError(fbi);
        return NULL;
     	}

	if(fread(&hdr,sizeof(RLAHeader),1,inStream)!=1){
		*status = ProcessImageIOError(fbi);
		return NULL;
		}

	SwapHdrBytes(hdr);
	BOOL oldver = (hdr.revision == (short)RLA_MAGIC_OLD)?1:0;
		
	int w = hdr.active_window.right-hdr.active_window.left + 1;
	fbi->SetWidth(w);
	int h = hdr.active_window.top-hdr.active_window.bottom + 1;
	fbi->SetHeight(h);
	sscanf( hdr.gamma, "%f", &gamma);
	fbi->SetGamma (gamma);
	sscanf( hdr.aspect_ratio, "%f", &aspect);
	fbi->SetAspect (aspect);
	fbi->SetFirstFrame(0);
	fbi->SetLastFrame(0);

	if (hdr.chan_bits == 16) {
		s = BMMCreateStorage(map->Manager(),BMM_TRUE_64);
		fbi->SetType(BMM_TRUE_64);
		} 
	else if (hdr.chan_bits == 32){
		s = BMMCreateStorage(map->Manager(),BMM_FLOAT_RGBA_32);
		fbi->SetType(BMM_FLOAT_RGBA_32); 
		}
	else {
		s = BMMCreateStorage(map->Manager(),BMM_TRUE_32);
		fbi->SetType(BMM_TRUE_32); 
		}
	if(!s)
		goto bailout;

	if(hdr.num_matte > 0)
		fbi->SetFlags(MAP_HAS_ALPHA);
	else
		fbi->ResetFlags(MAP_HAS_ALPHA);

	if (s->Allocate(fbi,map->Manager(),BMM_OPEN_R)==0) {
		bailout:
		*status = ProcessImageIOError(fbi,BMMRES_INTERNALERROR);
		if(s) {	 delete s;	s = NULL; }
		return NULL;
		}

	// Do we have GBuffer channels to load?
	BOOL gotRendInfo = FALSE;
	BOOL gotLayerData = FALSE;
	BOOL gotNodeNames = FALSE;
	BOOL gotPremultAlpha = FALSE;
	BYTE *gbChan[NUMGBCHAN];
	int maxgbsz;

	gbChannels = ChannelsFromString(hdr.program, gotRendInfo, gotLayerData, gotNodeNames, gotPremultAlpha);

	GBuffer *gb = NULL;
	if (gbChannels) {
		s->CreateChannels(gbChannels);
		if (s->ChannelsPresent()==gbChannels) {
			ULONG ctype;
			for (int i=0; i<NUMGBCHAN; i++) 
				gbChan[i] = (BYTE *)s->GetChannel(1<<i,ctype);
			}
		gb = s->GetGBuffer();
		maxgbsz = MaxGBSize(gbChannels);
		gb->InitBuffer();
		}

	if( !gotPremultAlpha )
		s->ClearFlags( MAP_ALPHA_PREMULTIPLIED );

	RLAReader rla(&hdr,inStream,w,h);

	// This reads in the offsets table and positions read head after it.
	if (!rla.Init()) 
		goto bailout;

	if (gotRendInfo) {
		RenderInfo* ri = s->AllocRenderInfo();
		if (!rla.ReadRendInfo(ri)) goto bailout;
		}

	if (gotNodeNames) {
		assert(gb);
		if (!rla.ReadNameTab(gb->NodeRenderIDNameTab())) goto bailout;
		}

//	PixelBuf  line64(w);
	Tab <BMM_Color_fl> line;
	line.SetCount(w);


	for (int y=0; y<h; y++) {
		rla.BeginLine(y);
		if (!rla.ReadRGBA(line.Addr(0)))
			goto bailout;
		if(!s->PutPixels(0,y,w,line.Addr(0))) 
			goto bailout;
		if (gbChannels) {
			for (int i=0; i<NUMGBCHAN; i++) {
				if (gbChan[i]) {
					int sz = GBDataSize(i);
					if (!rla.ReadNChannels(&gbChan[i][w*y*sz], sz))	
						goto bailout;
					}
				}
			if (gotLayerData) {
				int nlrecs = rla.ReadNumLRecs();
				if (nlrecs<0) 
					goto bailout;
				if (nlrecs>0) {
					gb->CreateLayerRecords(y,nlrecs);
					char *lbuf = (char *)malloc((nlrecs+4)*maxgbsz);
					if (!rla.ReadNChannels((UBYTE*)lbuf, 2, nlrecs, TRUE))	 //Read X values
						goto bailout;
					gb->SetLayerChannel(y,-1,lbuf);  // set array of X values
					for (int i=0; i<NUMGBCHAN; i++) {
						// DS 10/1/99: R3 didn't write out the weight channel correctly for the layers:
						// oldver indicates R3 wrote the file.
						if (i==GB_WEIGHT&&oldver) 
							if (!gbChan[i]) continue;
						if (gbChannels&(1<<i)) {
						//if (gbChan[i]) {
							int sz = GBDataSize(i);
							if (!rla.ReadNChannels((UBYTE*)lbuf, sz, nlrecs, TRUE))	 //AAAA
								goto bailout;
							gb->SetLayerChannel(y,i,lbuf);
							}
						}
					free(lbuf);
					}
				}
			}
		}
	if (gb)
		gb->UpdateChannelMinMax();
	return s;
	}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::OpenOutput()
//

BMMRES BitmapIO_RLA::OpenOutput(BitmapInfo *fbi, Bitmap *map) {

	if (openMode != BMM_NOT_OPEN)
		return (ProcessImageIOError(fbi,BMMRES_INTERNALERROR));
		
	if (!map)
		return (ProcessImageIOError(fbi,BMMRES_INTERNALERROR));
		
	//-- Check for Default Configuration -----------------
	
	if( UserData.defaultcfg )
		ReadCfg();

    //-- Save Image Info Data

    bi.CopyImageInfo(fbi);    
    bi.SetUpdateWindow(fbi->GetUpdateWindow());

    this->map   = map;
    openMode    = BMM_OPEN_W;

    return BMMRES_SUCCESS;

	}

//-----------------------------------------------------------------------------
//--   write out RLA  ---------------------------------------------------------
//-----------------------------------------------------------------------------

void BitmapIO_RLA::InitHeader(RLAHeader &h, int width, int height, float aspect,
        BOOL doAlpha, ULONG gbChannels, RenderInfo *ri, BOOL saveLayerData, BOOL saveNameTab) {
	int status = 0;
	int ni = 3;              // The number of color channels in the data.
	int nm = doAlpha ? 1 : 0;  // The number of alpha channels in the data.
	//int na = aux ? 1 : 0;    // The number of aux (Z buffer) channels in the data.
	int na = 0;
	char *u = "3D Studio";

	// The overall size of the image.
	h.window.left = 0;
	h.window.right = width - 1;
	h.window.bottom = 0;
	h.window.top = height - 1;

	// The size of the active (non-zero) portion of the image.
	// <Assume> entire window is non black.
	h.active_window.left = 0;
	h.active_window.right = width - 1;
	h.active_window.bottom = 0;
	h.active_window.top = height - 1;

	// Animation frame number (optional).
	h.frame = 1;

	// Number of image channels.
	h.num_chan = ni;

	// Number of matte channels (>1 means multispectral mattes).
	h.num_matte = nm;

	// Number of auxiliary data channels (maybe Z-depth or surface normals?).
	h.num_aux = na;

	// Version number.
	h.revision = RLA_MAGIC;

// DS: 4/23/97 enabled gamma correction on output for RLA files
	float g = OutputGamma(); 
//	float g = UserData.rgb16?1.0f : OutputGamma(); 
	sprintf(h.gamma, "%15f", g); 

	// Chromaticities of red, green and blue primaries and the white point.
	// What the hell is this used for? Put in the suggested, NTSC, defaults.
	sprintf(h.red_pri,   "%7.4f %7.4f", 0.670, 0.080);	// [24]
	sprintf(h.green_pri, "%7.4f %7.4f", 0.210, 0.710);	// [24]
	sprintf(h.blue_pri,  "%7.4f %7.4f", 0.140, 0.330);	// [24]
	sprintf(h.white_pt,  "%7.4f %7.4f", 0.310, 0.316);	// [24]

	// User-specified job number (optional).
	h.job_num = 12340;

	// Filename used to open the output file (optional).
	strncpy(h.name, "", 127);			// [128]

	// Description of file contents (optional).
	strncpy(h.desc, UserData.desc, 127);				// [128]

	// Program creating file (optional).
	MakeProgString(h.program, gbChannels, ri?1:0, saveLayerData, saveNameTab, UserData.premultAlpha);

	// Machine on which file was created (optional).
	strncpy(h.machine, "", 31);				// [32]

	// User name of creator (optional).
	strncpy(h.user, UserData.user, 31);				// [32]

	// Date  of creation (optional).
	char date[16];
	char time[16];
	_strdate(date);
	_strtime(time);
	sprintf(h.date,"%s  %s",date,time);

	// Name of aspect ratio.
	strncpy(h.aspect, "", 23);				// [24]

	// Aspect ratio of image - we assume square pixels. (Not any more we don't)
	sprintf(h.aspect_ratio, "%.5f", ((float)width * aspect) / (float)height);// [8]

	// Color space Can be one of rgb/xyz/sampled/raw.
	strncpy(h.chan, "rgb", 31);				// [32]

	// Flag for if image was rendered on fields.
	h.field = 0;
	if (GetCOREInterface()->GetRendFieldRender())
		h.field = 1;

	// (Rendering?) time taken to create image (optional).?
	strncpy(h.time, "0", 11);				// [12]

	// Filter used to post-process the image (optional).
	strncpy(h.filter, "", 31);				// [32]

	// Bit precision of data.
	if (UserData.rgb==1)
		h.chan_bits = h.matte_bits = 16;
	else if(UserData.rgb==2)
		h.chan_bits = h.matte_bits = 32;
	else 
		h.chan_bits = h.matte_bits = 8;


	h.aux_bits = 8;

	// Type of data (0=integer, 4=float).
	h.storage_type = 0;
	h.matte_type = 0;
	h.aux_type = 0;

	// Kind of auxiliary data. Can be one of range/depth.
	strncpy(h.aux, "", 31);				// [32]

	// Unused - must be zeros.
	memset(h.space, 0, 36);			// [36]

	// Offset of next image in file.
	h.next = 0;
	}


//------------------------------------------------------------------
//------------------------------------------------------------------

class RLAWriter {
	FILE* fd;
	long *offsets;
	BYTE *out, *outptr;
	int width, height;
	int chanLength;
	int maxLength, cury, curpos;
	RLAHeader* hdr;
	int bufSizeW;
	public:
		RLAWriter(RLAHeader *hd,FILE *f, int w, int h);
		int Init();
		int ResizeOutBuffer(int w);
		void BeginLine(int y);
		void BeginChannel();	
		int EndChannel(BOOL longLength=FALSE);	
		void EncodeByteArray( BYTE *inp, int stride, int w);	
		int WriteNumLRecs(int nl);
		int WriteRenderInfo(RenderInfo &ri);
		int WriteOffsets();
		int WriteFloatChannel(float*, int stride=1);
		int WriteChannel(BYTE *bp, int stride=1, int w=-1, BOOL longLength = FALSE);
		int WriteNChannels(UBYTE *p, int n, int w=-1, BOOL longLength = FALSE);
		int WriteRGBAChannels(BMM_Color_fl *pix);
		int WriteFloatChannel(float *bf);
		int WriteNameTab(NameTab &nt);
		~RLAWriter();
	};


RLAWriter::RLAWriter(RLAHeader *hd, FILE *f, int w, int h) {
	hdr = hd;
	fd = f; 
	width = w; 
	height = h; 
	//maxLength = 6 * ((width * 280) / 256); 
	// DS 7/7/99: got over flow with 6 * ((width * 280) / 256) -- try doubling it; 
	maxLength = 6 * ((width * 280) / 256); 
	offsets = NULL;
	curpos = 0;
	bufSizeW = 0;
	}

int RLAWriter::Init() {
	SwapHdrBytes(*hdr);
	if(fwrite(hdr,sizeof(RLAHeader),1,fd)!=1)
		return 0;
	SwapHdrBytes(*hdr);
	offsets = new long[height];
	out = new BYTE[maxLength];
	bufSizeW = width;
	if (offsets==NULL||out==NULL) 
		return 0;
	curpos = sizeof(RLAHeader)+ height*sizeof(LONG);
	// seek past the offsets table
	fseek(fd, curpos, SEEK_SET);
	return 1;
	}

int RLAWriter::ResizeOutBuffer(int w) {
	if (w>bufSizeW) {
		if (out)
			delete out;
		maxLength = 6 * ((w * 280) / 256); 
		out = new BYTE[maxLength];
		bufSizeW = w;
		}
	return out?1:0;
	}


int RLAWriter::WriteNumLRecs(int nl) {
	long n = nl;
	if (fwrite(&n,4,1,fd)!=1) 
		return 0;
	curpos += 4;
	return 1;
	}

RLAWriter::~RLAWriter() {
	delete [] offsets;
	delete [] out;
	}

void RLAWriter::BeginLine(int y) {
	cury = (height - 1) - y;
	offsets[cury] = curpos;
	}

void RLAWriter::BeginChannel() {	
	outptr = out;
	chanLength = 0;	
	}
	 	
void RLAWriter::EncodeByteArray( BYTE *inp, int stride, int w) {	
	int ln = encode(inp, outptr, w, stride);             
	chanLength += ln;
	outptr += ln;
	assert(outptr-out<maxLength);
	}

int RLAWriter::EndChannel(BOOL longLength) {	
	// write out the channel
	// chanLength is the number of bytes in the channel.
	if (longLength) {
		long len = lswap(chanLength);
		if (fwrite(&len, 4, 1, fd) != 1) return 0;
		}
	else {
		WORD len = sswap(chanLength);
		if (fwrite(&len, 2, 1, fd) != 1) return 0;
		}
	if (fwrite(out, sizeof(BYTE), chanLength, fd) != (size_t)chanLength) return 0;
	curpos += chanLength + (longLength?4:2);
	return 1;
	}

int RLAWriter::WriteOffsets() {
	for (int i=0; i<height; i++) offsets[i] = lswap(offsets[i]);
	fseek(fd, sizeof(RLAHeader), SEEK_SET);
	if (fwrite(offsets, sizeof(long), height, fd) != (size_t)height)
		return 0;
	return 1;
	}	

int RLAWriter::WriteFloatChannel(float *bp, int stride) {
	if(hdr->chan_bits==32)
	{
		BeginChannel();
		
		float *floatbuf = (float*)out;
		LONG *longbuf = (LONG*)out;
		
		chanLength = width*4;
		for(int i=0;i<width;i++)
		{
			floatbuf[i] = bp[i*stride];
			longbuf[i] = lswap(longbuf[i]);
		}
		return EndChannel();
	}
	else
	{
		//Let's create a byte array out out the channel
		BOOL doTwo = (hdr->chan_bits==16)?1:0;
		Tab <WORD> data;
		data.SetCount(width);
		for(int i = 0; i <width; i++)
		{
			data[i] = bp[i*stride] * 65535.0;
		}
		BeginChannel();
		EncodeByteArray(((BYTE*)data.Addr(0))+1,2,width); // point at high byte, skip along in strides
		if (doTwo) 
			EncodeByteArray((BYTE*)data.Addr(0),2,width); // point at low byte, skip along in strides
		return EndChannel();
	}
}

int RLAWriter::WriteChannel(BYTE *bp, int stride, int w, BOOL longLength) {
	if (w<0) w = width;
	BeginChannel();
	EncodeByteArray(bp,stride,w); // point at low byte, skip along in strides
	return EndChannel(longLength);
	}

int RLAWriter::WriteRGBAChannels( BMM_Color_fl *pix) {
	if (!WriteFloatChannel((&pix->r), 4)) return 0;
	if (!WriteFloatChannel((&pix->g), 4)) return 0;
	if (!WriteFloatChannel((&pix->b), 4)) return 0;
	if(hdr->num_matte>0) {
		if (!WriteFloatChannel((&pix->a),4))
			return 0;
		}
	return 1;
	}

int RLAWriter::WriteNChannels(UBYTE *p, int n, int w, BOOL longLength) {
	// write n byte channels starting with high byte
	if (w<0) w = width;
	for (int i=n-1; i>=0; i--) 
		if (!WriteChannel(p+i, n, w, longLength)) return 0;
	return 1;
	}

int RLAWriter::WriteFloatChannel(float *bf) {
	WORD len = sswap(width*4); //???
	if (fwrite(&len, sizeof(WORD), 1, fd) != 1) return 0;
	if (fwrite(bf, sizeof(float), width, fd) != 1) return 0;
	return 1;
	}

int RLAWriter::WriteRenderInfo(RenderInfo &ri) {
	short vers = RENDINFO_VERS1;
	if (fwrite(&vers,sizeof(short),1,fd)!=1)
 		return 0;
	curpos += sizeof(short);
	if (fwrite(&ri,sizeof(RenderInfo),1,fd)!=1)
 		return 0;
	curpos += sizeof(RenderInfo);
	return 1;
	}

int RLAWriter::WriteNameTab(NameTab &nt) {
	DWORD n = nt.Count();
	DWORD tot=0;
	for (DWORD i=0; i<n; i++) {
		tot += static_cast<DWORD>(_tcslen(nt[i])+1);			// SR DCAST64: Downcast to 4G limit.
		}

	TCHAR *buf = (TCHAR *)malloc(tot*sizeof(TCHAR));
	TCHAR *pb = buf;
	for (DWORD i=0; i<n; i++) {
		_tcscpy(pb,nt[i]);
		pb += _tcslen(nt[i])+1;		
		}
	if (fwrite(&n, sizeof(DWORD), 1, fd) != 1)  { free(buf); return 0; }
	if (fwrite(&tot, sizeof(DWORD), 1, fd) != 1) { free(buf); return 0; }
	if (fwrite(buf, tot*sizeof(TCHAR), 1, fd) != 1) { free(buf); return 0; }
	curpos += 2*sizeof(DWORD) + tot*sizeof(TCHAR);
	free(buf);
	return 1;
	}

BOOL BitmapIO_RLA::GetHDRData(BMM_Color_fl* in, BMM_Color_fl* out, int y, int width)
{
	//Get GBuffer
	GBuffer *gbuf = map->GetGBuffer();
	if(!gbuf) return FALSE;

	//check if we have necessary channels
	ULONG chans = gbuf->ChannelsPresent();
	if( (chans & ChannelsRequired()) != ChannelsRequired()) return FALSE;

	GBufReader *rdr = gbuf->CreateReader();
	if(!rdr) return FALSE;

	int res=rdr->StartLine(y);

	for(int x=0;x<width;x++)
	{
		res = rdr->StartPixel(x);	// -1 = eol,; 0 = empty; 1= has data  ( Automatically starts first layer)
		assert(res==1);
		int layercount=0;
				
		int		cov_sum = 0;			//sum of all coverages in all layers
		Color	col_sum(0,0,0);
		float z;

		//read first layer
		RealPixel	l0_rp;
		BYTE		l0_cov;
		Color24		l0_wg;

		rdr->ReadChannelData(GB_COVERAGE	,&l0_cov);
		rdr->ReadChannelData(GB_WEIGHT	,&l0_wg);
		rdr->ReadChannelData(GB_REALPIX	,&l0_rp);
		rdr->ReadChannelData(GB_Z,&z);

		cov_sum = l0_cov;
		col_sum = Color(l0_rp) * float(l0_cov);
							
		//Now process the other layers
		while ((res=rdr->StartNextLayer()) != 0) 
		{
			RealPixel	rp;
			//float		z;
			Color24		wg;
			BYTE		cov;
			
			rdr->ReadChannelData(GB_COVERAGE	,&cov);
			rdr->ReadChannelData(GB_WEIGHT	,&wg);
			rdr->ReadChannelData(GB_REALPIX	,&rp);

			Color c(rp);
			col_sum.r+=c.r*wg.r;
			col_sum.g+=c.g*wg.g;
			col_sum.b+=c.b*wg.b;
			cov_sum+=cov;
			layercount++;
		}

		//now apply layer 0 using the remaining coverage 
		if(cov_sum<255)
		{
			col_sum+=Color(l0_rp)*float(255-cov_sum);
		}
		if(layercount==0 && z==-BIGFLOAT)
		{
			out[x] = in[x];
		}
		else
		{

			out[x].r = col_sum.r/255.0f;
			out[x].g = col_sum.g/255.0f;
			out[x].b = col_sum.b/255.0f;
			out[x].a = in[x].a;
		}
	}

	gbuf->DestroyReader(rdr);

	return TRUE;
}


//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::Write()
//
//

BMMRES BitmapIO_RLA::Write(int frame) {
     
	//-- If we haven't gone through an OpenOutput(), leave

	if (openMode != BMM_OPEN_W)
		return (ProcessImageIOError(&bi,BMMRES_INTERNALERROR));

	//-- Resolve Filename --------------------------------

	TCHAR filename[MAX_PATH];

	if (frame == BMM_SINGLEFRAME) {
		_tcscpy(filename,bi.Name());
	} else {
		if (!BMMCreateNumberedFilename(bi.Name(),frame,filename))
			return (ProcessImageIOError(&bi,BMMRES_NUMBEREDFILENAMEERROR));
		}
	
	//-- Create Image File (self closing)-------------------------------
     
	File file(filename, _T("wb"));
	if (!file.stream) {
		return (ProcessImageIOError(&bi));
		}

	inStream = file.stream;

	// Find out what kind of output file we're dealing with
	BitmapStorage *saveStorage = map->Storage();
	if(!saveStorage)
		return (ProcessImageIOError(&bi,BMMRES_INTERNALERROR));

	BOOL hasAlpha = saveStorage->HasAlpha();
	bool storageIsHDR = (saveStorage->IsHighDynamicRange() != 0);
	int w = map->Width();
	int h = map->Height();

	// Do we have GBuffer channels to save?
	ULONG ctype;
	BYTE* gbChan[NUMGBCHAN];
	RenderInfo *rendInfo = NULL;
	gbChannels = saveStorage->ChannelsPresent();

	// Need to keep only the channels the user requested.
	// We can't just use "ChannelsRequested" since we sometimes add channels that the user has not requested
	// for the purpose of 32 bit floating point support
	gbChannels &= UserData.channels;

	GBuffer *gb = NULL;
	UBYTE *layerBuffer=NULL;
		
	if (gbChannels) {
		for (int i=0; i<NUMGBCHAN; i++) 
		{
			// only request the storage for the channels the user has requested
			if(gbChannels & (1<<i))
				gbChan[i] = (BYTE*)saveStorage->GetChannel(1<<i,ctype);
			else
				gbChan[i] = NULL;
		}
		//gbChan[GB_NODE_RENDER_ID] = NULL; // dont save this one
		rendInfo = saveStorage->GetRenderInfo();
		gb = saveStorage->GetGBuffer();
		assert(gb);
		}

	RLAHeader hd;

	BOOL saveLayerData = TRUE;
	BOOL saveNameTab = FALSE;

	// Write out the node name table.
	if (gb&&gbChan[GB_NODE_RENDER_ID]) {
		NameTab &names = gb->NodeRenderIDNameTab();
		if (names.Count()>0)
			saveNameTab = TRUE;
		}

	InitHeader(hd, w, h, map->Aspect(), UserData.usealpha&&hasAlpha, 
		gbChannels, rendInfo, saveLayerData, saveNameTab);
	
	RLAWriter rla(&hd,inStream, w, h);

	// this writes the header, and positions after the offsets table.
	if (!rla.Init()) {
		bailout:
		return (ProcessImageIOError(&bi,BMMRES_INTERNALERROR));
		}	

	Tab <BMM_Color_fl> line;
	line.SetCount(w);
	Tab <BMM_Color_fl> linehdr;
	linehdr.SetCount(w);

	if (rendInfo) {
		if (!rla.WriteRenderInfo(*rendInfo)) goto bailout;
		}

	if (saveNameTab) {
		if (!rla.WriteNameTab(gb->NodeRenderIDNameTab())) goto bailout;
		}

	int maxlrecs = 0;
	char *lbuf = NULL;
	int maxgbsize;
	bool hdrPixels = UserData.rgb==2;

	if (gbChannels) {
		for (int y=0; y<h; y++) {
			int nlrecs = gb->NumberLayerRecords(y);
			if (nlrecs>maxlrecs) maxlrecs = nlrecs;
			}
		if (saveLayerData) {
			maxgbsize = MaxGBSize(gbChannels);
			lbuf = (char *)malloc(maxgbsize*(maxlrecs+1));
			}
		}

	if (!rla.ResizeOutBuffer(maxlrecs+1))
		goto bailout;

	for (int y=0; y<h; y++) {

		rla.BeginLine(y);

		GetOutputPixels(0,y,w,line.Addr(0),hdrPixels? TRUE:UserData.premultAlpha);

		// Write the current line
		int writeRGBAResult = 0;
		if(hdrPixels) {
			// We need to output HDR values.

			if(!storageIsHDR && GetHDRData(line.Addr(0),linehdr.Addr(0),y,w)) {
				// Main frame buffer is not HDR, so we need to get HDR data from the RealPix GBuffer
				// channel.
				writeRGBAResult = rla.WriteRGBAChannels(linehdr.Addr(0));
			}
			else {
				// Main channel is HDR, or the RealPix channel is absent: we need to save
				// the main frame buffer
				writeRGBAResult = rla.WriteRGBAChannels(line.Addr(0));
			}
		}
		else
		{
			// We don't need to output HDR values: just get whatever pixels have been rendered
			// in the main frame buffer (it could be 32bit-floating point, or not...)
			writeRGBAResult = rla.WriteRGBAChannels(line.Addr(0));
		}

		if(!writeRGBAResult) {
			// If writing failed, just bail out.
			goto bailout;
		}

		if (gbChannels) {
			for (int i=0; i<NUMGBCHAN; i++) {
				if (gbChan[i]) {
					int sz = GBDataSize(i);
					if (!rla.WriteNChannels(&gbChan[i][w*y*sz], sz))
						goto bailout;
					}
				}
			if (lbuf) {
				int nlrecs = gb->NumberLayerRecords(y);
				if (!rla.WriteNumLRecs(nlrecs))
					goto bailout;
				if (nlrecs>0) {
					gb->GetLayerChannel(y,-1,lbuf);	 // get array of X values
					if (!rla.WriteNChannels((UBYTE *)lbuf, 2, nlrecs, TRUE)) // Write X values
						goto bailout;
					for (int i=0; i<NUMGBCHAN; i++) {
						if (gbChannels&(1<<i)) {
//						if (gbChan[i]) {
							int sz = GBDataSize(i);
							gb->GetLayerChannel(y,i,lbuf);
							if (!rla.WriteNChannels((UBYTE *)lbuf, sz, nlrecs, TRUE)) //AAAA
								goto bailout;
							}
						}
					}
				}
			}
		}
						
	if (lbuf)
		free(lbuf);
	rla.WriteOffsets();
    return (BMMRES_SUCCESS);
	}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::Close()
//

int  BitmapIO_RLA::Close( int flag ) {
	if(openMode != BMM_OPEN_W)
		return 0;
	return 1;
	}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::GetCfgFilename()
//

void BitmapIO_RLA::GetCfgFilename( TCHAR *filename ) {
     _tcscpy(filename,TheManager->GetDir(APP_PLUGCFG_DIR));
     int len = static_cast<int>(_tcslen(filename));
     if (len) {
        if (_tcscmp(&filename[len-1],_T("\\")))
           _tcscat(filename,_T("\\"));
    	 }   
     _tcscat(filename,isRPF?rpfCONFIGNAME:rlaCONFIGNAME);   
	}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::ReadCfg()
//

BOOL BitmapIO_RLA::ReadCfg() {
    TCHAR filename[MAX_PATH];
    TCHAR tmpstr[MAX_PATH];
    GetCfgFilename(filename);

    wsprintf(tmpstr,"%d",UserData.channels);
    GetPrivateProfileString(
        rlaSECTION,rlaCHANNELS,tmpstr,tmpstr,	sizeof(tmpstr),filename);
    UserData.channels = (DWORD)atoi(tmpstr);

    wsprintf(tmpstr,"%d",UserData.usealpha);
    GetPrivateProfileString(
       rlaSECTION,rlaUSEALPHA,tmpstr,tmpstr,	sizeof(tmpstr),filename);
    UserData.usealpha = atoi(tmpstr);

    wsprintf(tmpstr,"%d",UserData.premultAlpha);
    GetPrivateProfileString(
       rlaSECTION,rlaPREMULTALPHA,tmpstr,tmpstr,	sizeof(tmpstr),filename);
    UserData.premultAlpha = atoi(tmpstr);

    wsprintf(tmpstr,"%d",UserData.rgb);
    GetPrivateProfileString(
        rlaSECTION,rlaRGB,tmpstr,tmpstr,	sizeof(tmpstr),filename);
    UserData.rgb = atoi(tmpstr);

    GetPrivateProfileString(
		rlaSECTION,rlaUSER, UserData.user, UserData.user, 31,filename);

    GetPrivateProfileString(
		rlaSECTION,rlaDESC, UserData.desc, UserData.desc,127,filename);

	return TRUE;
	}

//-----------------------------------------------------------------------------
// #> BitmapIO_RLA::WriteCfg()
//

void BitmapIO_RLA::WriteCfg() {
    TCHAR filename[MAX_PATH];
    TCHAR tmpstr[MAX_PATH];
    GetCfgFilename(filename);
    wsprintf(tmpstr,"%d",UserData.channels);
    WritePrivateProfileString(rlaSECTION,rlaCHANNELS,tmpstr,filename);
    wsprintf(tmpstr,"%d",UserData.usealpha);
    WritePrivateProfileString(rlaSECTION,rlaUSEALPHA,tmpstr,filename);
    wsprintf(tmpstr,"%d",UserData.rgb);
    WritePrivateProfileString(rlaSECTION,rlaRGB,tmpstr,filename);
    WritePrivateProfileString(rlaSECTION,rlaDESC,UserData.desc,filename);
    WritePrivateProfileString(rlaSECTION,rlaUSER,UserData.user,filename);
    wsprintf(tmpstr,"%d",UserData.premultAlpha);
    WritePrivateProfileString(rlaSECTION,rlaPREMULTALPHA,tmpstr,filename);
	}


//==================================================================
// Encodes one byte channel from input buffer.
//==================================================================

static int encode(unsigned char * input, unsigned char* output, int xSize, int stride) {
	unsigned char * in = input;
	unsigned char* out = output;
	unsigned char * inend = in+(xSize*stride);
	signed char* outid = (signed char*)out;
	unsigned char lastval = ! *in;
	int runcnt = 0;
	int cnt;

	out++;

	while (in < inend) {
		unsigned char val = *in;
		*out++ = val;
		in += stride;
		if (val == lastval) {
			if (++runcnt == 3) {
				cnt = (signed char*)out-outid;
				if (cnt > 4) {
					*outid = -(cnt-4);
					outid = (signed char*)out-3;
				}
				while (in < inend) {
					val = *in;
					if (val == lastval) {
						runcnt++;
						in += stride;
					} else {
						break;
					}
				}
				out = (unsigned char*)outid+1;
				while (runcnt) {
					int chunk = runcnt;
					if (chunk > 128) chunk = 128;
					*outid = chunk-1;
					outid += 2;
					*out = lastval;
	 				out += 2;
	 				runcnt -= chunk;
				}
                if (in < inend) {
	 				*out++ = val;
					in += stride;
				}
				lastval = val;
				runcnt = 1;
			} else if ((cnt = (signed char*)out-outid) == 129) {
				*outid = -(cnt-1);
				outid = (signed char*)out;
				lastval = ! *in;
				out++;
				runcnt = 0;
			}
		} else {
			cnt = ((signed char*)out-outid);
			if (cnt == 129) {
				*outid = -(cnt-1);
				outid = (signed char*)out;
				lastval = ! *in;
				out++;
				runcnt = 0;
			} else {
				lastval = val;
				runcnt = 1;
			}
		}
	}

	if ((signed char*)out-outid > 1) {
		*outid = -((signed char*)out - outid - 1);
	} else {
		out = (unsigned char*)outid;
	}

	return (out - output);
}

//==================================================================
// Decodes one run-encoded channel from input buffer.
//==================================================================
static BYTE* decode(BYTE *input, BYTE  *output,
           int xFile, int xImage, int stride) {
	int count, x = xFile;
	int useX  = 0;

	BYTE  *out = (BYTE  *)output;
	while (x > 0) {
		count = *(signed char *)input++;
		if (count >= 0) {
			// Repeat pixel value (count + 1) times.
		  	while (count-- >= 0) {
				if (useX < xImage) {
					*out = *input;
					out += stride;
				}
				--x;
				useX++;
			}
			++input;
		} else {
			// Copy (-count) unencoded values.
			for (count = -count; count > 0; --count) {
				if (useX < xImage) {
					*out = *input;
					out += stride;
				}
				input++;
				--x;
				useX++;
			}
		}
	}
	return input;
}


//-----------------------------------------------------------------------------
// Interface for the RLA I/O plug-in
//-----------------------------------------------------------------------------

class BitmapIO_RLA_Imp : public IBitmapIO_RLA {
public:
	// 8, 16, 32
	int		GetColorDepth();
	void	SetColorDepth(int bpp);

	BOOL	GetStoreAlpha();
	void	SetStoreAlpha(BOOL storeAlpha);
	BOOL	GetPremultAlpha();
	void	SetPremultAlpha(BOOL premult);

	TSTR	GetDescription();
	void	SetDescription(TSTR description);
	TSTR	GetAuthor();
	void	SetAuthor(TSTR author);

	BOOL	GetZChannel() {return GetChannel(BMM_CHAN_Z);}
	void	SetZChannel(BOOL b) {SetChannel(BMM_CHAN_Z,b);}
	BOOL	GetMtlIDChannel() {return GetChannel(BMM_CHAN_MTL_ID);}
	void	SetMtlIDChannel(BOOL b) {SetChannel(BMM_CHAN_MTL_ID,b);}
	BOOL	GetNodeIDChannel() {return GetChannel(BMM_CHAN_NODE_ID);}
	void	SetNodeIDChannel(BOOL b) {SetChannel(BMM_CHAN_NODE_ID,b);}
	BOOL	GetUVChannel() {return GetChannel(BMM_CHAN_UV);}
	void	SetUVChannel(BOOL b) {SetChannel(BMM_CHAN_UV,b);}
	BOOL	GetNormalChannel() {return GetChannel(BMM_CHAN_NORMAL);}
	void	SetNormalChannel(BOOL b) {SetChannel(BMM_CHAN_NORMAL,b);}
	BOOL	GetRealpixChannel() {return GetChannel(BMM_CHAN_REALPIX);}
	void	SetRealpixChannel(BOOL b) {SetChannel(BMM_CHAN_REALPIX,b);}
	BOOL	GetCoverageChannel() {return GetChannel(BMM_CHAN_COVERAGE);}
	void	SetCoverageChannel(BOOL b) {SetChannel(BMM_CHAN_COVERAGE,b);}

	// RPF-specific functions
	BOOL	GetNodeRenderIDChannel() {return GetChannel(BMM_CHAN_NODE_RENDER_ID);}
	void	SetNodeRenderIDChannel(BOOL b) {SetChannel(BMM_CHAN_NODE_RENDER_ID,b);}
	BOOL	GetColorChannel() {return GetChannel(BMM_CHAN_COLOR);}
	void	SetColorChannel(BOOL b) {SetChannel(BMM_CHAN_COLOR,b);}
	BOOL	GetTranspChannel() {return GetChannel(BMM_CHAN_TRANSP);}
	void	SetTranspChannel(BOOL b) {SetChannel(BMM_CHAN_TRANSP,b);}
	BOOL	GetVelocChannel() {return GetChannel(BMM_CHAN_VELOC);}
	void	SetVelocChannel(BOOL b) {SetChannel(BMM_CHAN_VELOC,b);}
	BOOL	GetWeightChannel() {return GetChannel(BMM_CHAN_WEIGHT);}
	void	SetWeightChannel(BOOL b) {SetChannel(BMM_CHAN_WEIGHT,b);}
	BOOL	GetMaskChannel() {return GetChannel(BMM_CHAN_MASK);}
	void	SetMaskChannel(BOOL b) {SetChannel(BMM_CHAN_MASK,b);}


	//internal helpers
	BOOL	GetChannel( int chanBit );
	void	SetChannel( int chanBit, BOOL b );
	virtual int IsRPF() {return 0;} //the RPF version of this interface returns 1 here

	enum {
		fnIdGetColorDepth, fnIdSetColorDepth,

		fnIdGetStoreAlpha, fnIdSetStoreAlpha,
		fnIdGetPremultAlpha, fnIdSetPremultAlpha,

		fnIdGetDescription, fnIdSetDescription,
		fnIdGetAuthor, fnIdSetAuthor,

		fnIdGetZChannel, fnIdSetZChannel,
		fnIdGetMtlIDChannel, fnIdSetMtlIDChannel,
		fnIdGetNodeIDChannel, fnIdSetNodeIDChannel,
		fnIdGetUVChannel, fnIdSetUVChannel,
		fnIdGetNormalChannel, fnIdSetNormalChannel,
		fnIdGetRealpixChannel, fnIdSetRealpixChannel,
		fnIdGetCoverageChannel, fnIdSetCoverageChannel,

		// RPF-specific functions
		fnIdGetNodeRenderIDChannel, fnIdSetNodeRenderIDChannel,
		fnIdGetColorChannel, fnIdSetColorChannel,
		fnIdGetTranspChannel, fnIdSetTranspChannel,
		fnIdGetVelocChannel, fnIdSetVelocChannel,
		fnIdGetWeightChannel, fnIdSetWeightChannel,
		fnIdGetMaskChannel, fnIdSetMaskChannel,

		enumIdColorDepth,
	};

	enum {
		RLA_8_BITS=0, RLA_16_BITS=1, RLA_32_BITS=2, //for enumIdColorDepth
	};

	DECLARE_DESCRIPTOR(BitmapIO_RLA_Imp)

	BEGIN_FUNCTION_MAP
		PROP_FNS( fnIdGetColorDepth, GetColorDepth, fnIdSetColorDepth, SetColorDepth, TYPE_INT );

		PROP_FNS( fnIdGetStoreAlpha, GetStoreAlpha, fnIdSetStoreAlpha, SetStoreAlpha, TYPE_BOOL );
		PROP_FNS( fnIdGetPremultAlpha, GetPremultAlpha, fnIdSetPremultAlpha, SetPremultAlpha, TYPE_BOOL );

		PROP_FNS( fnIdGetDescription, GetDescription, fnIdSetDescription, SetDescription, TYPE_TSTR_BV );
		PROP_FNS( fnIdGetAuthor, GetAuthor, fnIdSetAuthor, SetAuthor, TYPE_TSTR_BV );

		PROP_FNS( fnIdGetZChannel, GetZChannel, fnIdSetZChannel, SetZChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetMtlIDChannel, GetMtlIDChannel, fnIdSetMtlIDChannel, SetMtlIDChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetNodeIDChannel, GetNodeIDChannel, fnIdSetNodeIDChannel, SetNodeIDChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetUVChannel, GetUVChannel, fnIdSetUVChannel, SetUVChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetNormalChannel, GetNormalChannel, fnIdSetNormalChannel, SetNormalChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetRealpixChannel, GetRealpixChannel, fnIdSetRealpixChannel, SetRealpixChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetCoverageChannel, GetCoverageChannel, fnIdSetCoverageChannel, SetCoverageChannel, TYPE_BOOL );

		// RPF-specific functions
		PROP_FNS( fnIdGetNodeRenderIDChannel, GetNodeRenderIDChannel, fnIdSetNodeRenderIDChannel, SetNodeRenderIDChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetColorChannel, GetColorChannel, fnIdSetColorChannel, SetColorChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetTranspChannel, GetTranspChannel, fnIdSetTranspChannel, SetTranspChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetVelocChannel, GetVelocChannel, fnIdSetVelocChannel, SetVelocChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetWeightChannel, GetWeightChannel, fnIdSetWeightChannel, SetWeightChannel, TYPE_BOOL );
		PROP_FNS( fnIdGetMaskChannel, GetMaskChannel, fnIdSetMaskChannel, SetMaskChannel, TYPE_BOOL );
	END_FUNCTION_MAP
};

class BitmapIO_RPF_Imp : public BitmapIO_RLA_Imp {
public:
	BitmapIO_RPF_Imp();
	virtual int IsRPF() {return 1;} //the RLA version of this interface returns 0 here
};


int BitmapIO_RLA_Imp::GetColorDepth()
{
	int depth=RLA_8_BITS;

	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		depth = p->UserData.rgb;
		delete p;
	}

	int retVal = 8;
	switch (depth) {
		case RLA_8_BITS: retVal = 8; break;
		case RLA_16_BITS: retVal = 16; break;
		case RLA_32_BITS: retVal = 32; break;
	}
	return retVal;
}

void BitmapIO_RLA_Imp::SetColorDepth(int depth)
{
	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();

		switch (depth) {
			case 8 : p->UserData.rgb = RLA_8_BITS; break;
			case 16 : p->UserData.rgb = RLA_16_BITS; break;
			case 32 : p->UserData.rgb = RLA_32_BITS; break;
		}

		p->WriteCfg();

		delete p;
	}
}

BOOL BitmapIO_RLA_Imp::GetStoreAlpha()
{
	BOOL storeAlpha = TRUE;

	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		storeAlpha = p->UserData.usealpha;
		delete p;
	}
	return storeAlpha;
}

void BitmapIO_RLA_Imp::SetStoreAlpha(BOOL storeAlpha)
{
	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		p->UserData.usealpha = storeAlpha;
		p->WriteCfg();
		delete p;
	}
}

BOOL BitmapIO_RLA_Imp::GetPremultAlpha()
{
	BOOL premultAlpha = TRUE;

	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		premultAlpha = p->UserData.premultAlpha;
		delete p;
	}
	return premultAlpha;
}

void BitmapIO_RLA_Imp::SetPremultAlpha(BOOL premultAlpha)
{
	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		p->UserData.premultAlpha = premultAlpha;
		p->WriteCfg();
		delete p;
	}
}

TSTR BitmapIO_RLA_Imp::GetDescription()
{
	TSTR description;

	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		description = p->UserData.desc;
		delete p;
	}
	return description;
}

void BitmapIO_RLA_Imp::SetDescription(TSTR description)
{
	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		description.remove(128);
		p->ReadCfg();
		_tcscpy(p->UserData.desc,description.data());
		p->WriteCfg();
		delete p;
	}
}

TSTR BitmapIO_RLA_Imp::GetAuthor()
{
	TSTR author;

	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		author = p->UserData.user;
		delete p;
	}
	return author;
}

void BitmapIO_RLA_Imp::SetAuthor(TSTR author)
{
	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		author.remove(32);
		p->ReadCfg();
		_tcscpy(p->UserData.user,author.data());
		p->WriteCfg();
		delete p;
	}
}

BOOL BitmapIO_RLA_Imp::GetChannel( int chanBit )
{
	BOOL b = FALSE;

	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		b = ((p->UserData.channels & chanBit) ? TRUE:FALSE);
		delete p;
	}
	return b;
}

void BitmapIO_RLA_Imp::SetChannel( int chanBit, BOOL b)
{
	BitmapIO_RLA* p = new BitmapIO_RLA( IsRPF() );
	if (p) {
		p->ReadCfg();
		if( b ) p->UserData.channels |= chanBit;
		else p->UserData.channels &= (~chanBit);
		p->WriteCfg();

		delete p;
	}
}

//-- EOF: rla.cpp -------------------------------------------------------------
