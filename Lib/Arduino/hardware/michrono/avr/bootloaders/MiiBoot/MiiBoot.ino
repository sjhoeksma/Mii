/*
This file contains the boot loader for the MiiChrono devices
It will load from a SDCard (FAT16) the file MIIAP000.HEX (000 is version) and loads it. 
Loading is only done when reset is hit, default bootloader will jump into the program.

It is possible to debug program using Android, but keep it is loaded in to program mem and not boot.

To install boot loader just run make prog for boot loader directory
make prog

*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>  /* filename from eeprom */

//Should we be able to load it into
//#define BOOT_LOADER

//For compatability reasons we allow any version to be installed
#define FLASH_ANY_VERSION

//Should we debug, will be undef when using BOOT_LOADER
#define UART_DEBUG

// These define the Pin, Port and DDR of the Chip Select to the MMC...
// Used to be defined here, but is now in the Makefile 
#define MMC_CS		PB2		//also change MMC_PORT and MMC_DDR acordingly
#define MMC_PORT        PORTB
#define MMC_DDR         DDRB

#define VERSION 2
// if the code cannot be programmed/run then the bootloader sits flashing an LED. The
// following two defines say where that LED is PD6 is digital 6
#if VERSION == 2
#define LED_PORT PORTD
#define LED_DDR  DDRD
#define LED_PIN  PD6
#else
#define LED_PORT PORTC
#define LED_DDR  DDRC
#define LED_PIN  PC3

#endif

#define BAUD_RATE 57600

//The name we will use to match file
char nameMatch[6] = {'M','I','I','A','P','\0'};


#ifdef BOOT_LOADER
/* function prototype */
int main (void) __attribute__ ((naked,section (".init9")));
//Bootloader should not have debug stuff
#undef UART_DEBUG
#endif

#define MMC_CMD0_RETRY	(unsigned char)16


// Just enable the UART Tx and set baud rate for 38400 on 3.6864MHz (STK500)
//http://www.wormfood.net/avrbaudcalc.php
void UART_init(void) {
  #ifdef BOOT_LOADER
   UBRR0L = 12; // 38400 @ 8MHz
  #else
    UBRR0L = (uint8_t)(F_CPU/(BAUD_RATE*16L)-1);
    UBRR0H = (F_CPU/(BAUD_RATE*16L)-1) >> 8;
  #endif  
    //UBRR0L = 12; // 38400 @ 8MHz
    UCSR0B = (1 << TXEN0);
}

// The classic Tx one character routine
void UART_put(uint8_t c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

// classic Tx a C-string routine
// As there is no .data (in the bootloader) it only makes sense for theis to use PSTR()
void UART_puts(const char * str) {
    char c;
    do {
        c = pgm_read_byte(str++);
        if (c) {
            UART_put(c);
        }
    } while (c != 0);
}

// Just outputs "\r\n"
void UART_newline(void){
    UART_put('\r');
    UART_put('\n');
}

// used in printing a 2 digit hex number, outputs one of the two nibbles
// the parameter is expected to be 0..F
void UART_putnibble(uint8_t c) {
    if (c < 10) {
        UART_put('0' + c);
    }
    else {
        UART_put('A' + c - 10);
    }
}

// print both nibbles of an 8 bit hex number
void UART_puthex(uint8_t c) {
    UART_putnibble(c >> 4);
    UART_putnibble(c & 0x0F);
}

// print both bytes of a 16 bit hex number
void UART_puthex16(uint16_t n) {
    UART_puthex(n >> 8);
    UART_puthex(n & 0xFF);
}

// this expect the first parameter to be a string in dlash (that is PSTR())
// and then the second to be a value to print out in hex. typically used in
// the form UART_putsP(PSTR("SecPerClus = "), SecPerClus)
void UART_putsP(const char * str, uint16_t n) {
    UART_puts(str);
    UART_puthex16(n);
    UART_newline();
}

// dump the 512 bytes at the given address in the form:
// CD BF 10 E0 A0 E6 B0 E0  E4 E5 F0 E0 02 C0 05 90   Õø ‡†Ê∞‡‰Â‡ ¿ ê
void UART_dumpsector(uint8_t * Buff) {
    for (uint16_t i=0; i<512; i++) {
        if ((i % 16) == 0) {
            UART_put(' ');
            for(uint16_t j=(i -16); j<=i; j++) {
                UART_put(((Buff[j]>=(uint8_t)32) && (Buff[j]<(uint8_t)127)) ? Buff[j] : '.');
            }
            UART_newline();
        }
        UART_puthex(Buff[i]);
        UART_put(' ');
    }
    UART_newline();
}



/* some variables */
//const void (*app_start)(void) = 0x0000;
void(* app_start) (void)=0; //declare reset function @ address 0
uint8_t reset_reason = 0;


/* page buffer ---------------------------------------------------- */
uint8_t pagebuffer[SPM_PAGESIZE];


/* address buffer */
uint16_t address;

/* access to flash memory------------------------------------------ */

void write_flash_page()
{
	uint16_t i = 0;

	eeprom_busy_wait ();

	boot_page_erase (address);
	boot_spm_busy_wait ();      // Wait until the memory is erased.

	for (i=0; i<SPM_PAGESIZE; i+=2)
	{
		// Set up little-endian word.
		uint16_t w = *((uint16_t*)(pagebuffer + i));
		boot_page_fill (address + i, w);
	}

	boot_page_write(address);     // Store buffer in flash page.
	boot_spm_busy_wait();            // Wait until the memory is written.

	boot_rww_enable ();
}

/* This code has a rather big sector buffer of 512 bytes  */
/* with this export, other code can use it, too           */
extern uint8_t buff[512];

#if defined (__AVR_ATmega644__)
 #define SPCR	SPCR0
 #define SPIE	SPIE0
 #define SPE	SPE0	
 #define DORD	DORD0
 #define MSTR	MSTR0
 #define CPOL	CPOL0
 #define CPHA	CPHA0
 #define SPR1	SPR01
 #define SPR0	SPR00

 #define SPSR	SPSR0
 #define SPIF	SPIF0
 #define WCOL	WCOL0
 #define SPI2X	SPI2X0

 #define SPDR	SPDR0
#endif

//Port & Pin definitions.
//Settings below are recommended for a MEGA168 and MEGA328
#define SPI_PORT PORTB
#define SPI_DDR  DDRB
#define SPI_MISO	PB4		//DataOut of MMC
#define SPI_MOSI	PB3		//DataIn of  MMC
#define SPI_CLK  	PB5		//Clock of MMC
#define SPI_SS          PB2             //SS pin of SPI interface


//Clockrate while initialisation / reading / writing
#define SPI_INIT_CLOCK 1<<SPR1 | 1<<SPR0
#define SPI_READ_CLOCK 0<<SPR1 | 0<<SPR0
#define SPI_WRITE_CLOCK 1<<SPR1 | 0<<SPR0

#define SPI_DOUBLE_SPEED 0 //0: normal speed, 1: double speed


// Result Codes
#define MMC_OK 			0
#define MMC_INIT        1
#define MMC_NOSTARTBYTE	2
#define MMC_CMDERROR	3
#define MMC_TIMEOUT		4


/* ---[ FAT Structs ] --------------------------------------------- */

typedef struct
{
  unsigned char state;               // 0x80 if active
  unsigned char startHead;           // Starting head
  unsigned int  startCylinderSector; // Starting cyinder and sector
                                     // Format:
                                     // Bit 0..5  = Bit 0..5 of sector
                                     // Bit 6..7  = Bit 8..9 of cylinder
                                     // Bit 8..15 = Bit 0..7 of cylinder
  unsigned char typeId;              // Partition type
  unsigned char endHead;             // End head
  unsigned int  endCylinderSector;   // End cylinder and sector
                                     // Format see above
  unsigned long sectorOffset;        // Starting sector (counting from 0)
  unsigned long nSectors;            // Number of sectors in partition
} partition_t;

typedef union
{
   unsigned char buffer[512];

   struct
   {
      unsigned char fill[0x1BE];
      partition_t partition[4];
      unsigned short magic;
   } sector;
} mbr_t;

typedef struct
{
	uint8_t  		bsjmpBoot[3]; 		// 0-2   Jump to bootstrap (E.g. eb 3c 90; on i86: JMP 003E NOP. One finds either eb xx 90, or e9 xx xx. The position of the bootstrap varies.)
	char    		bsOEMName[8]; 		// 3-10  OEM name/version (E.g. "IBM  3.3", "IBM 20.0", "MSDOS5.0", "MSWIN4.0".
	uint16_t   	bsBytesPerSec;		// 11-12 Number of bytes per sector (512). Must be one of 512, 1024, 2048, 4096.
	uint8_t			bsSecPerClus;		// 13    Number of sectors per cluster (1). Must be one of 1, 2, 4, 8, 16, 32, 64, 128.
	uint16_t		bsRsvdSecCnt;   	// 14-15 Number of reserved sectors (1). FAT12 and FAT16 use 1. FAT32 uses 32.
	uint8_t			bsNumFATs;			// 16    Number of FAT copies (2)
	uint16_t		bsRootEntCnt; 		// 17-18 Number of root directory entries (224). 0 for FAT32. 512 is recommended for FAT16.
	uint16_t		bsTotSec16; 		// 19-20 Total number of sectors in the filesystem (2880). (in case the partition is not FAT32 and smaller than 32 MB)
	uint8_t			bsMedia;			// 21    Media descriptor type ->  For hard disks:  Value 0xF8  ,  DOS version 2.0   
	uint16_t		bsNrSeProFAT16;     	// 22-23 Number of sectors per FAT (9). 0 for FAT32.
	uint16_t		bsSecPerTrk;		// 24-25 Number of sectors per track (12)
	uint16_t		bsNumHeads; 		// 26-27 Number of heads (2, for a double-sided diskette)
	uint32_t		bsHiddSec;			// 28-31 Number of hidden sectors (0)
	uint32_t		bsTotSec32; 		// 32-35 Number of total sectors (in case the total was not given in bytes 19-20)
	union
	{
		struct
		{
			uint8_t			bsLogDrvNr;			// 36    Logical Drive Number (for use with INT 13, e.g. 0 or 0x80)
			uint8_t			bsReserved;			// 37    Reserved (Earlier: Current Head, the track containing the Boot Record) Used by Windows NT: bit 0: need disk check; bit 1: need surface scan
			uint8_t			bsExtSign;			// 38    Extended signature (0x29)  Indicates that the three following fields are present. Windows NT recognizes either 0x28 or 0x29.
			uint32_t		bsParSerNr; 		// 39-42 Serial number of partition
		};
		
		struct
		{
			uint32_t		SecPerFAT32;
			uint8_t			reserved[3];
		};
	};
	uint8_t			bsVolLbl[11]; 		// 43-53 Volume label or "NO NAME    "
	uint8_t			bsFileSysType[8]; 	// 54-61 Filesystem type (E.g. "FAT12   ", "FAT16   ", "FAT     ", or all zero.)
	uint8_t			bsBootstrap[448]; 	// Bootstrap
	uint16_t		bsSignature; 		// 510-511 Signature 55 aa
} vbr_t;

typedef struct 
{
	char		name[11];      //8 chars filename
	uint8_t	attr;         //file attributes RSHA, Longname, Drive Label, Directory
	uint8_t reserved;
	uint8_t fcrttime;			//Fine resolution creation time stamp, in tenths of a second
	uint32_t crttime;			//Time of Creation
	uint16_t lactime;			//Last Access Time
	uint16_t eaindex;			//EA-Index (used by OS/2 and NT) in FAT12 and FAT16, high 2 ytes of first clusternumber in FAT32
	uint32_t lmodtime;		//Last Modified Time
	uint16_t fstclust;		//First cluster in FAT12 and FAT16, low 2 bytes of first clusternumber in FAT32
	uint32_t filesize;		//File size
} direntry_t;

typedef struct
{
	uint16_t fat_entry[256]; //0: Cluster unused, 1 - Clustercount: Next clusternum, 0xFFFF0 - 0xFFFF6: Reserved Cluster, 0xFFF7 dead Cluster, 0xFFF8 - 0xFFFF: EOF
} fatsector_t;



static unsigned char cmd[6];

/* ---[ SPI Interface ]---------------------------------------------- */

static void spi_send_byte(unsigned char data)
{
	SPDR=data;
	loop_until_bit_is_set(SPSR, SPIF); // wait for byte transmitted...
}

static unsigned char send_cmd(void)
{
	unsigned char i;
	unsigned char *buf;
	
  	spi_send_byte(0xFF);      //Dummy delay 8 clocks
	MMC_PORT &= ~(1<<MMC_CS); //MMC Chip Select -> Low (activate mmc)

	/* send the 6 cmd bytes */
	i=6;
	buf = cmd;
	while(i) {
		spi_send_byte(*buf++);
		i--;
	}

	unsigned char result;
	
	/* wait for response */
	for(i=0; i<255; i++) {
	
 		spi_send_byte(0xFF);
		result = SPDR;
		
		if ((result & 0x80) == 0)
			break;
	}

	return(result); // TimeOut !?
}

/* ---[ MMC Interface ]---------------------------------------------- */

//all MMC Commandos needed for reading a file from the card
#define MMC_GO_IDLE_STATE 0
#define MMC_SEND_OP_COND 1
#define MMC_READ_SINGLE_BLOCK 17
unsigned long lastAdr=0xFFFFFFFF;
/* the sector buffer */
uint8_t buff[512];


/*			
*		Call mmc_init one time after a card has been connected to the �C's SPI bus!
*	
*		return values:
*			MMC_OK:				MMC initialized successfully
*			MMC_INIT:			Error while trying to reset MMC
*			MMC_TIMEOUT:	Error/Timeout while trying to initialize MMC
*/
static inline unsigned char mmc_init(void)
{
	// the default after reset is already input
	//SPI_DDR &= ~(1<<SPI_MISO);	//SPI Data Out -> Input (default)
	SPI_PORT |= 1<<SPI_SS;   //PB2 output: High (deselect other SPI chips)
	SPI_DDR  |= 1<<SPI_CLK | 1<<SPI_MOSI | 1<<SPI_SS; // SPI Data -> Output
        SPCR = 1<<SPE | 1<<MSTR | SPI_INIT_CLOCK; //SPI Enable, SPI Master Mode

        #if MMC_CS != SPI_SS
          MMC_DDR |= 1<<MMC_CS; 	//MMC Chip Select -> Output
        #endif

	unsigned char i;
	
	i=10;
	while(i) { //Pulse 80+ clocks to reset MMC
		spi_send_byte(0xFF);	
 		i--;
	}

	unsigned char res;

	cmd[0] = 0x40 + MMC_GO_IDLE_STATE;
	cmd[1] = 0x00; cmd[2] = 0x00; cmd[3] = 0x00; cmd[4] = 0x00; cmd[5] = 0x95;
	
	for (i=0; i<MMC_CMD0_RETRY; i++)
	{
		res=send_cmd(); //store result of reset command, should be 0x01

		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate mmc);
      	 	spi_send_byte(0xFF);
		if (res == 0x01)
			break;
	}

	if(i==MMC_CMD0_RETRY) return(MMC_TIMEOUT);

	if (res != 0x01) //Response R1 from MMC (0x01: IDLE, The card is in idle state and running the initializing process.)
		return(MMC_INIT);
	
	cmd[0]=0x40 + MMC_SEND_OP_COND;
		
//May be this becomes an endless loop ?
//Counting i from 0 to 255 and then timeout
//was to SHORT for some of my cards !
	while(send_cmd() != 0) {
		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate);
		spi_send_byte(0xFF);
	}
	
	return(MMC_OK);
}

static inline unsigned char wait_start_byte(void)
{
	unsigned char i;
	
	i=255;
	do {
		spi_send_byte(0xFF);
		if(SPDR == 0xFE) return MMC_OK;
	} while(--i);
	
	return MMC_NOSTARTBYTE;
}

/*
 *		mmc_start_read_sector initializes the reading of a sector
 *
 *		Parameters:
 *			adr: specifies address to be read from
 *
 *		Return values:
 *			MMC_OK:						Command successful
 *			MMC_CMDERROR:			Error while sending read command to mmc
 *			MMC_NOSTARTBYTE:	No start byte received
 */
static unsigned char mmc_start_read_block(unsigned long adr)
{
	if (adr==lastAdr) return (MMC_OK);
        lastAdr = adr;
        adr <<= 1;
	
	cmd[0] = 0x40 + MMC_READ_SINGLE_BLOCK;
	cmd[1] = (adr & 0x00FF0000) >> 0x10;
	cmd[2] = (adr & 0x0000FF00) >> 0x08;
	cmd[3] = (adr & 0x000000FF);
	cmd[4] = 0;

	SPCR = 1<<SPE | 1<<MSTR | SPI_READ_CLOCK; //SPI Enable, SPI Master Mode
	
	if (send_cmd() != 0x00 || wait_start_byte()) {
		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate mmc);
		return(MMC_CMDERROR); //wrong response!
	}
	
	//mmc_read_buffer
	unsigned char *buf;
	unsigned short len;
 
	buf = buff;
	len= 512;
	
	while (len) {
		spi_send_byte(0xFF);
		*buf++ = SPDR;
		len--;
	}
	
	//mmc_stop_read_block
	//read 2 bytes CRC (not used);
	spi_send_byte(0xFF);
	spi_send_byte(0xFF);
	MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate mmc);
	
        #ifdef UART_DEBUG
          UART_dumpsector(buff);
        #endif   
     return(MMC_OK);
}

/* ---[ FAT16 ]------------------------------------------------------ */

static uint16_t  RootDirRegionStartSec;
static uint32_t  DataRegionStartSec;
static uint16_t  RootDirRegionSize;
static uint8_t   SectorsPerCluster;
static uint16_t  FATRegionStartSec;

static inline unsigned char fat16_init(void)
{
	mbr_t *mbr = (mbr_t*) buff;
	vbr_t *vbr = (vbr_t*) buff;
		
	if (mmc_init() != MMC_OK) return 1;
	
    mmc_start_read_block(0);

    // Try sector 0 as a bootsector
	if ((vbr->bsFileSysType[0] == 'F') && (vbr->bsFileSysType[4] == '6'))
	{
		FATRegionStartSec = 0;
	}
	else // Try sector 0 as a MBR	
	{     	 
		FATRegionStartSec = mbr->sector.partition[0].sectorOffset;
          
		mmc_start_read_block(mbr->sector.partition[0].sectorOffset);
	  
        if ((vbr->bsFileSysType[0] != 'F') || (vbr->bsFileSysType[4] != '6'))
		   return 2; // No FAT16 found
     }
    
	SectorsPerCluster  			= vbr->bsSecPerClus; // 4
	
	// Calculation Algorithms
	FATRegionStartSec			+= vbr->bsRsvdSecCnt;						// 6
	RootDirRegionStartSec	 	= FATRegionStartSec + (vbr->bsNumFATs * vbr->bsNrSeProFAT16);		// 496	
	RootDirRegionSize		 	= (vbr->bsRootEntCnt / 16); 						// 32
	DataRegionStartSec 			= RootDirRegionStartSec + RootDirRegionSize;	// 528
	
	return 0;
}

static struct _file_s {
	uint16_t startcluster;
 	uint16_t sector_counter;
 	uint32_t size;
 	uint8_t* next;
} file;

static inline uint8_t fat16_readRootDirEntry(uint16_t entry_num,uint16_t *filever) {
	uint8_t direntry_in_sector;
 	direntry_t *dir;
		
	/* Check for end of root dir region reached! */
	if ((entry_num / 16) >= RootDirRegionSize)
		return 0;

	/* this finds the sector in which the entry will be saved */	
	uint32_t dirsector = RootDirRegionStartSec + entry_num / 16;

	/* this is the offset inside the sector */
	/* there are 16 entries in a sector, each 32 bytes long */
    direntry_in_sector = (unsigned char) entry_num % 16;

	/* get the sector into the buffer */
	mmc_start_read_block(dirsector);
	
	/* pointer to the direntry inside the buffer */
	dir = (direntry_t *) buff + direntry_in_sector;

	if ((dir->name[0] == 0) || (dir->name[0] == 0xE5) || (dir->fstclust == 0))
		return 0;

	/* fill in the file structure */
	file.startcluster = dir->fstclust;
	file.size = dir->filesize;
	file.sector_counter = 0;
	file.next = buff + 512;

        
	/* compare name */
	uint8_t i = 0;
	uint8_t match = 1;
	for (i = 0; nameMatch[i]; i++) { 
	  match &= (nameMatch[i] == dir->name[i]);
	}
	if (!(match && i)) return 0;

        *filever = ((dir->name[i]-'0') << 8) | ((dir->name[i+1]-'0') << 4) | (dir->name[i+2]-'0');
	
	/* match ending, seach for HEX => return 1, or EEP => return 2*/
	if (dir->name[9] != 'E') return 0;
	if (dir->name[8] == 'H' && dir->name[10] == 'X') return 1;
	if (dir->name[8] == 'E' && dir->name[10] == 'P') return 2;
	return 0;
}

static void inline fat16_readfilesector()
{
	uint16_t clusteroffset;
	uint8_t currentfatsector;
	uint8_t temp, secoffset;
	uint32_t templong;
	uint16_t cluster = file.startcluster;
	
	fatsector_t *fatsector = (fatsector_t*) buff;

	/* SectorsPerCluster is always power of 2 ! */
	secoffset = (uint8_t)file.sector_counter & (SectorsPerCluster-1);
	
	clusteroffset = file.sector_counter;
	temp = SectorsPerCluster >> 1;
	while(temp) {
		clusteroffset >>= 1;
        temp >>= 1;
    }

	currentfatsector = 0xFF;
	while (clusteroffset)
	{
		temp = (unsigned char)((cluster & 0xFF00) >>8);
          
		if (currentfatsector != temp)
		{
			mmc_start_read_block(FATRegionStartSec + temp);

			currentfatsector = temp;
		}
		
		cluster = fatsector->fat_entry[cluster % 256];
		clusteroffset--;
	}

	templong = cluster - 2;
	temp = SectorsPerCluster>>1;
	while(temp) {
		templong <<= 1;	
		temp >>= 1;
	}
		
	/* read the sector of the file into the buffer */
	mmc_start_read_block(templong + DataRegionStartSec + secoffset);
	
	/* advance to next sector */
	file.sector_counter++;
}

/* ----[ file ]--------------------------------------------------- */

static uint8_t file_read_byte() {	// read a byte from the open file from the mmc...
	if (file.next >= buff + 512) {
	    fat16_readfilesector();
		file.next = file.next - 512;
	}
	file.size--;
	return *file.next++;
}

static char gethexnib(char a) {
	if(a >= 'a') {
		return (a - 'a' + 0x0a);
	} else if(a >= 'A') {
		return (a - 'A' + 0x0a);
	} else if(a >= '0') {
		return(a - '0');
	}
	return a;
}

static uint8_t file_read_hex(void) {
	return (gethexnib(file_read_byte()) << 4) + gethexnib(file_read_byte());
}

static inline void read_hex_file(void) {
	// read file and convert it from intel hex and flash it
    uint8_t num_flash_words = 0;
	uint8_t* out = pagebuffer;
    address = 0;
	while (file.size)
	{
		if (num_flash_words)
		{
			// read (de-hexify)
			*out++ = file_read_hex();
			num_flash_words--;
		
			// if pagebuffer is full
			if (out - pagebuffer == SPM_PAGESIZE) {
			    // write page
#ifdef BOOT_LOADER
			    write_flash_page();
#endif
			    address += SPM_PAGESIZE;
				out = pagebuffer;
			}
		} 
		else
		{
			// skip bytes until we find another ':'
			if (file_read_byte() == ':') {
				num_flash_words = file_read_hex();
				file.next+=4; /* skip 4 bytes, address */
				if (file_read_hex()==1) break; //01 is EOF
			}
		}
	}
#ifdef BOOT_LOADER
	if (out != pagebuffer) write_flash_page();
#endif
}

static inline void read_eep_file(void) {
	// read file and convert it from intel hex and flash it
    uint8_t num_flash_words = 0;
    address = 0;
    while (file.size) {
	if (num_flash_words) {
             eeprom_update_byte((uint8_t *)address++, file_read_hex());
	     num_flash_words--;
         } else {
	  // skip bytes until we find another ':'
	  if (file_read_byte() == ':') {
            num_flash_words = file_read_hex();
	    address=file_read_hex()<<8 | file_read_hex(); /* Read new address */
            if (file_read_hex()==1) break;
	  }
        }
    }
}

/* main program starts here */
#ifdef BOOT_LOADER
int main(void)
#else
int main(void)
#endif
 {
	/* here we learn how we were reset */
	reset_reason = MCUSR;
	MCUSR = 0;

	/* stop watchdog */
	WDTCSR |= _BV(WDCE) | _BV(WDE);
	WDTCSR = 0;
#ifdef BOOT_LOADER
	/* start app right ahead if this was not an external reset */
	/* this means that all the code below this line is only executed on external reset */
	if (!(reset_reason & _BV(EXTRF))) app_start();

	/* this is needed because of the __attribute__ naked, section .init 9 */
	/* from now, we can call functions :-) */
	asm volatile ( "clr __zero_reg__" );
	SP=RAMEND;
#endif

 uint16_t filever = 0;
 #if defined(UART_DEBUG) || !defined(FLASH_ANY_VERSION)
 uint16_t flashver = eeprom_read_word((const uint16_t *)E2END - 1);
 #endif
 
 
#ifdef UART_DEBUG
    UART_init();
#endif

   LED_DDR |= 1<<LED_PIN; 	//LED -> Output
   LED_PORT &= ~(1<<LED_PIN);   //LED  -> Low (activate)


   /* we have found a board name! 		   */
   /* now go on and see if we find a      */
   /* file on mmc with the same name...   */
		
   /* first, init mmc / fat */
   if (fat16_init() == MMC_OK) {	

     #ifdef UART_DEBUG
       UART_putsP(PSTR("SD="),1);
    #endif
     /* for each file in ROOT... */
     for (uint16_t entrycounter=0; entrycounter<512; entrycounter++){
    	/* skip all unimportant files */
        uint8_t fileType=fat16_readRootDirEntry(entrycounter,&filever);
    	if (fileType== 1
          #ifndef FLASH_ANY_VERSION
           && (flashver==0xFFFF || flashver<filever)
          #endif 
        ) {
            #ifdef UART_DEBUG
              UART_putsP(PSTR("start="),flashver);
            #endif

           //We are in progress write flash, busy bit
           eeprom_update_word((uint16_t *)E2END - 1, 0xFFFF);
           //Read the hex file
           read_hex_file();
            //We are ready write flash, file version
           eeprom_update_word((uint16_t *)E2END - 1, filever);
           #ifdef UART_DEBUG
             UART_putsP(PSTR("finish="),filever);
           #endif
           break;
           
        }
        if (fileType == 2     //EEP
           #ifndef FLASH_ANY_VERSION
           && (flashver==0xFFFF || flashver==filever)
          #endif 
         ){ 
           //Read the eep hex file
           read_eep_file();
           break;
         }
        
     }	              
     
  } else {
    #ifdef UART_DEBUG
       UART_putsP(PSTR("SD="),0);
    #endif
  }

  #ifdef UART_DEBUG
     UART_putsP(PSTR("REBOOT="),filever);
  #endif

#ifdef BOOT_LOADER
	/* we reset via watchdog in order to reset all the registers to their default values */
	WDTCSR = _BV(WDE);
	while (1); // 16 ms
#endif
}
