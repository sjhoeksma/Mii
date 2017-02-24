#ifndef _DB_h
#define _DB_h

// Uncomment the definition of TIMED_EVENTS_DYNAMIC if you want to use
// dynamic memory allocation
//#define DB_DYNAMIC

//SD_FAT16 can be downloaded from http://code.google.com/p/fat16lib/downloads/list
//Please uncomment SD_FAT16 and Save(5.616 bytes) or use default SD_S
//#define SD_SD
#define SD_FAT16

#ifndef DB_DYNAMIC
#define DB_MAX_COLUMNS 10
#endif

//We could save space by;
//  replace sprintf conversions

#include <Arduino.h>

#define DEFAULT_FIELDSIZE 15
#define DB_FORWARD  1
#define DB_REVERSE  2
//Include current record in searches
#define DB_CUR_FORWARD 3
#define DB_CUR_REVERSE 4
#define DB_YES      1


//Check if we use FAT16
#ifdef SD_FAT16
#include <Fat16.h>             //Small foot printed SD Lib
#define FILE_READ O_READ
#define FILE_WRITE (O_READ | O_WRITE | O_CREAT)
extern SdCard SD;                     //Our reference to the FAT16 SDCard
#endif

//Check if we use full SD
#ifdef  SD_SD
#include <SD.h>                //The SD Card software library
#endif

//Check if we use the tinyFat
#ifdef SD_TINY
#include <tinyFAT.h>
#endif

//A special constant for creating a file which only can be appended (log files)
#define FILE_APPEND (O_WRITE | O_CREAT | O_APPEND)

//Wrapper for SDfile
class SDFile : public Stream {
public:
  ~SDFile(void){}; // destructor
  SDFile() {}
  void init();
  unsigned long position();
  boolean seek(unsigned long position);
  unsigned long size();
  int peek();
  boolean open(const char *file,int flags);
  void close(){myFile.close();}
  int read(void){return myFile.read();}
  int read(void* buf, uint16_t nbyte){return myFile.read(buf,nbyte);}
  int write(const void *buf, uint16_t nbyte);
  size_t write(uint8_t b){return myFile.write(b);}
  int write(const char* str){return myFile.write(str);}
  int available();
  void flush();
  operator bool();

  using Print::write;

private:
  #ifdef SD_FAT16
   Fat16 myFile;
  #endif
  #ifdef SD_SD
   File myFile;
  #endif
  #ifdef SD_TINY
    tinyFAT myFile;
  #endif
};

//Wrapper for the SDCard
class SDWrapper {
public:
  boolean begin(int SDSelect);
  boolean exists(const char *file);
  boolean remove(const char *file);
  operator bool(){return _ready;}
private:
  boolean _ready;
  #ifdef SD_FAT16
  Fat16 myFile;
  #endif
};

extern SDWrapper SDCARD;


class DB {
public:
  ~DB(){close();}
  boolean open(const char *file);
  void close();
  boolean create(const char *file,const char *definition,byte fieldSize=DEFAULT_FIELDSIZE);
  byte read(byte column,char *data,int datalen);
  byte read(byte column,char *data);
  byte read(byte column,long *data);
  byte read(byte column,int  *data);
  byte read(byte column,char data);
  byte read(byte column,unsigned long *data);
  byte read(byte column,unsigned int  *data);
  byte write(byte column,char *data,int datalen=-1);
  byte write(byte column,char data);
  byte write(byte column,unsigned long data);
  byte write(byte column,long data);
  byte write(byte column,unsigned int data);
  byte write(byte column,int data);
  byte search(byte column,char *data,byte direction=0,byte includeDeleted=0);
  byte search(byte column,char data,byte direction=0,byte includeDeleted=0);
  byte search(byte column,unsigned long data,byte direction=0,byte includeDeleted=0);
  byte search(byte column,long data,byte direction=0,byte includeDeleted=0);
  byte search(byte column,unsigned int data,byte direction=0,byte includeDeleted=0);
  byte search(byte column,int data,byte direction=0,byte includeDeleted=0);
  byte next(byte showDeleted=0);
  byte prev(byte showDeleted=0);
  byte first(byte showDeleted=0);
  byte last(byte showDeleted=0);
  byte append(byte reuse=0);
  void remove();
  void restore();
  unsigned long count(byte includeDeleted=0);
  byte isEmpty();
  byte isDeleted();
  byte columnSize(byte column);
  byte columnNo(char *column);
  byte columnCount(){return fieldCount;}
  unsigned long position();
  byte seek(unsigned long position);

protected:
  char readColumn(char *readbuf);
  int columnPos(byte column);
  byte fieldCount;
  #ifdef DB_DYNAMIC
  byte *columns;
  #else
  byte columns[DB_MAX_COLUMNS];
  #endif
  unsigned int  rowSize;
  unsigned int  dataStart;
  unsigned long curPos;
  SDFile myDB;

};

#endif