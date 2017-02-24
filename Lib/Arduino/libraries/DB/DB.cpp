#include <DB.h>
#include <Time.h>

#define MAX_COLUMN_NAME   10
#define DELIMITER ','
#define DELETED '#'

//TODO add more read and write functions for other types
//3) Check if we need to speed up the append loop

#ifdef SD_FAT16
SdCard SD;                     //Our reference to the SD Card
#endif

//Comment SD_TIME definition if you don't need actual times on your Files
//Saves around 300 bytes in normal code or up to 1k if you don't use time at all
#define SD_TIME

// call back for file timestamps
void SDdateTime(uint16_t* date, uint16_t* time) {
  unsigned long _now = now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year(_now), month(_now), day(_now));
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour(_now), minute(_now), second(_now));
}

//From here the SDFile wrapper class
void SDFile::init(){
  #ifdef SD_FAT16
    myFile.init(&SD);
  #endif
}

unsigned long SDFile::position(){
  #ifdef SD_FAT16
    return myFile.curPosition();
  #endif
  #ifdef SD_SD
    return myFile.position();
  #endif
}

boolean SDFile::seek(unsigned long position){
  #ifdef SD_FAT16
    return myFile.seekSet(position);
  #endif
  #ifdef SD_SD
    return myFile.seek(position);
  #endif
}

int SDFile::available(){
  //Availale buffer is max 128
  return min(size()-1-position(),128);
}
void SDFile::flush(){
 #ifdef SD_FAT16
    myFile.sync();
  #endif
  #ifdef SD_SD
    myFile.flush();
  #endif
}

unsigned long SDFile::size(){
  #ifdef SD_FAT16
    return myFile.fileSize();
  #endif
  #ifdef SD_SD
    return myFile.size();
  #endif
}

int SDFile::peek(){
  #ifdef SD_FAT16
    int c =myFile.read();
    if (c>=0) myFile.seekCur(-1);
    return c;
  #endif
  #ifdef SD_SD
    return myFile.peek();
  #endif
}

boolean SDFile::open(const char *file,int flags){
  if (!SDCARD) return false;
  #ifdef SD_FAT16
   return myFile.open(file,flags);
  #endif
  #ifdef SD_SD
   myFile = SD.open(file,flags);
   return myFile;
  #endif
}


SDFile::operator bool() {
  #ifdef SD_FAT16
    return myFile.isOpen();
  #endif
  #ifdef SD_SD
    return myFile;
  #endif
}

int SDFile::write(const void *buf, uint16_t nbyte){
  #ifdef SD_FAT16
    return myFile.write(buf,nbyte);
  #endif
  #ifdef SD_SD
    return myFile.write((uint8_t*)buf,nbyte);
  #endif
}

//From here the SDWrapper class
boolean SDWrapper::begin(int SDSelect){
  pinMode(SDSelect,OUTPUT);
  #ifdef SD_FAT16
   #ifdef SD_TIME
   Fat16::dateTimeCallback(SDdateTime);
   #endif
    if ((_ready = SD.init(0,SDSelect)))
       myFile.init(&SD);
  #endif
  #ifdef SD_SD
     pinMode(10,OUTPUT); //Port 10 needs to be set output
    #ifdef SD_TIME
    SdFile::dateTimeCallback(SDdateTime);
    #endif
    pinMode(SDSelect,OUTPUT);
    _ready = SD.begin(SDSelect);
  #endif
  return _ready;
}

boolean SDWrapper::exists(const char *file){
  if (!SDCARD) return false;
  #ifdef SD_FAT16
    boolean ret = myFile.open(file,FILE_READ);
    myFile.close();
    return ret;
  #endif
  #ifdef SD_SD
    return SD.exists(file);
  #endif
}

boolean SDWrapper::remove(const char *file){
  if (!SDCARD) return false;
  #ifdef SD_FAT16
    return myFile.remove(file);
  #endif
 #ifdef SD_SD
    return SD.remove(file);
  #endif
}

//Create a global SDCard class to be used
SDWrapper SDCARD = SDWrapper();

//Open a database file and read the file size
boolean DB::open(const char *file){
  if (!SDCARD.exists(file)) return false;
  myDB.open(file,FILE_WRITE);
  myDB.seek(0);
  int fieldSize=myDB.parseInt();
  char readbuf[MAX_COLUMN_NAME+1];
  #ifdef DB_DYNAMIC
   columns = (byte *) malloc(sizeof(byte));
  #endif
  columns[0]=1; //The Delete maker columns
  char token=1;
  fieldCount=0;
  rowSize=3; //Delete + CRLF
  while (token && token!=10) {
    token=readColumn(readbuf);
    if (token=='(' || token==DELIMITER || (token==10 && strlen(readbuf)!=0)) {
      #ifndef DB_DYNAMIC
       if (fieldCount+1==DB_MAX_COLUMNS) return false;
      #endif
   	   fieldCount++;
   	   #ifdef DB_DYNAMIC
       columns=(byte *)realloc(columns,sizeof(byte)*(fieldCount+1));
       #endif
       columns[fieldCount]= token=='(' ? myDB.parseInt() : fieldSize;
       rowSize+=columns[fieldCount];
		}
  }
  curPos=dataStart=myDB.position();
  myDB.seek(curPos);
  return true;
}

//Create a fixed length database with definition at top
//first element of header is fieldsize of db.
//first byte of dataline is used for delete marker
//Defintion:column|column
boolean DB::create(const char *file,const char *definition,byte fieldSize){
  if (!myDB.open(file,FILE_WRITE)) return false;
  myDB.print(fieldSize);
  myDB.print(DELIMITER);
  myDB.println(definition);
  myDB.close();
  return open(file);
}

//Close the database and free the memory
void DB::close(){
  #ifdef DB_DYNAMIC
  free(columns);
  columns=NULL;
  #endif
  myDB.close();
}

//Read the next record
byte DB::next(byte showDeleted){
 //Next row skipping deleted ones
 unsigned long size= myDB.size();
 do {
	 if (curPos+rowSize>=size) return 0;
	 curPos+=rowSize;
   myDB.seek(curPos);
 } while (!showDeleted && isDeleted());
 return 1;
}

//Read the previous record
byte DB::prev(byte showDeleted){
 do {
 if (curPos==dataStart) return 0;
  curPos-=rowSize;
  myDB.seek(curPos);
 } while (!showDeleted && isDeleted());
 return 1;
}

//Check if we have records
byte DB::isEmpty(){
  return myDB.size()==dataStart ? 1 : 0;
}

//Check if the current record has been marked for delete
byte DB::isDeleted(){
  return myDB.peek()==DELETED ? 1 : 0;
}

//Move to the first record
byte DB::first(byte showDeleted){
	curPos=dataStart;
  myDB.seek(curPos);
  if (!showDeleted && isDeleted()) next(showDeleted);
  return !isEmpty();
}

//Move to the last record
byte DB::last(byte showDeleted){
  if (!isEmpty()) { //Check if we have records
    curPos=myDB.size()-rowSize;
    myDB.seek(curPos);
    if (!showDeleted && isDeleted()) return prev(showDeleted);
    return 1;
  }
  curPos=dataStart;
  myDB.seek(curPos);
  return 0;
}
//return the number of records is DB
unsigned long DB::count(byte includeDeleted){
  unsigned long count=0;
  if (includeDeleted) {
    count=(myDB.size()-dataStart)/rowSize;
  } else {
    unsigned long pos=curPos;
    if (first(includeDeleted)) {
		  count++;
      while (next(includeDeleted)) {count++;}
  	}
    curPos=pos;
    myDB.seek(curPos);
  }
  return count;
}

//Read the data of the current record by column
byte DB::read(byte column,char *data,int datalen){
	int pos=columnPos(column);
  if (isEmpty() || pos==0) return 0;
  byte len=columns[column];
  datalen= datalen==-1 ? len : min(datalen,len);
  myDB.seek(curPos+pos);
  myDB.read(data,datalen);
  //Trim the data read, by setting pointer to character before len
	char *p=data+datalen-1;
	while((p>=data) && (*p==' ' || *p=='\0')) *p-- = '\0';
  myDB.seek(curPos);
  return datalen;
}

//Read long
byte DB::read(byte column,unsigned long *data){
  int pos=columnPos(column);
  if (isEmpty() || pos==0) return 0;
  byte len=columns[column];
  char buf[len+1];
  buf[len]='\0';
  if (!read(column,buf,len)) return 0;
  *data=atol(buf);
  return len;
}

//Read int
byte DB::read(byte column,int *data){
  unsigned long d;
  if (!read(column,&d)) return 0;
  *data=(int)d;
  return columns[column];
}

byte DB::read(byte column,unsigned int  *data){
 unsigned long d;
  if (!read(column,&d)) return 0;
  *data=(unsigned int)d;
  return columns[column];
}

byte DB::read(byte column,long *data){
  unsigned long d;
  if (!read(column,&d)) return 0;
  *data=(long)d;
  return columns[column];
}

//Read single char
byte DB::read(byte column,char data){
 return read(column,&data,1);
}

byte DB::read(byte column,char *data){
 return read(column,data,-1);
}


//Write the data to current record
byte DB::write(byte column,char *data,int datalen){
	int pos=columnPos(column);
  if (isEmpty() || pos==0 ) return 0;
  byte len=columns[column];
  datalen= datalen==-1 ? len : min(datalen,len);
  myDB.seek(curPos+pos);
  char *p=data;
  while (p<data+datalen && *p!='\0'){myDB.write(*p);p++;}
  while (p<data+len){myDB.write(' ');p++;}
  myDB.seek(curPos);
  return datalen;
}

//Write a single char
byte DB::write(byte column,char data){
	return write(column,&data,1);
}

//Write a unsigned long value
byte DB::write(byte column,unsigned long data){
	char writebuf[columns[column]];
  sprintf(writebuf,"%lu",data);
  return write(column,writebuf);
}

//Write a long value
byte DB::write(byte column,long data){
	char writebuf[columns[column]];
  sprintf(writebuf,"%ld",data);
  return write(column,writebuf);
}
//Write a unsigned int
byte DB::write(byte column,unsigned int data){
 return write(column,(unsigned long)data);
}
//Write a signed int
byte DB::write(byte column,int data){
  return write(column,(long)data);
}

//Search the database for a specific value in a column
//Record is left on the find
byte DB::search(byte column,char *data,byte direction,byte includeDeleted){
	int pos=columnPos(column);
  if (isEmpty() || pos==0) return 0;
  unsigned long lastPos = curPos;
  byte len=columns[column];
  char readbuf[len+1];readbuf[len]='\0'; //Ensure our buffer is null terminated for string search to work
  byte cycle=0; //Used to fix looping when current record is deleted
  if (direction==0) first(includeDeleted); //Go to top if we don't have a direction
  else if (direction==DB_REVERSE && !prev(includeDeleted)) {last(includeDeleted);cycle++;}
	else if (direction==DB_FORWARD && !next(includeDeleted)) {first(includeDeleted);cycle++;}
	else if (direction==DB_CUR_FORWARD) {direction=DB_FORWARD;}
	else if (direction==DB_CUR_REVERSE) {direction=DB_REVERSE;}
  do {
	  read(column,readbuf);
	  if (!isDeleted() && strcmp(data,readbuf)==0) return 1;
	  if (direction==0 && !next(includeDeleted)) break;
	  else if (direction==DB_REVERSE && !prev(includeDeleted)) {last(includeDeleted);cycle++;}
	  else if (direction==DB_FORWARD && !next(includeDeleted)) {first(includeDeleted);cycle++;}
  } while (curPos!=lastPos && cycle<=1); //Stay in loop until we are on the same address
	//We did not find anything return to old position
	curPos=lastPos;
  myDB.seek(curPos);
  return 0;
}

//Search a single char
byte DB::search(byte column,char data,byte direction,byte includeDeleted){
  char searchbuf[2] = {data,'\0'};
  return search(column,searchbuf,direction,includeDeleted);
}
//Search a unsigned long
byte DB::search(byte column,unsigned long data,byte direction,byte includeDeleted){
	char searchbuf[columns[column]];
  sprintf(searchbuf,"%lu",data);
  return search(column,searchbuf,direction,includeDeleted);
}
//Search a signed long
byte DB::search(byte column,long data,byte direction,byte includeDeleted){
	char searchbuf[columns[column]];
  sprintf(searchbuf,"%ld",data);
  return search(column,searchbuf,direction,includeDeleted);
}
//Search by unsigned int
byte DB::search(byte column,unsigned int data,byte direction,byte includeDeleted){
  return search(column,(unsigned long)data,direction,includeDeleted);
}
//Search by int
byte DB::search(byte column,int data,byte direction,byte includeDeleted){
  return search(column,(long)data,direction,includeDeleted);
}

//Calcuate the columnSize
byte DB::columnSize(byte column){
	if (column<=0 || column>fieldCount) return 0;
	return columns[column];
}

//Return the record number
unsigned long DB::position() {
 return ((curPos-dataStart)/rowSize);
}

//Seek a database record
byte DB::seek(unsigned long position){
   position=dataStart + (position*rowSize);
   if (position>=myDB.size()) return 0;
   curPos = position;
   myDB.seek(curPos);
   return 1;
}

//Add a record to the database
//We will first search for deleted lines which can be overwrite when reused is marked
byte DB::append(byte reuse){
  if (reuse) {
    curPos=dataStart;
    myDB.seek(curPos);
    //Search the first free record
    while (!isDeleted() && curPos<myDB.size()-1){curPos+=rowSize;myDB.seek(curPos);}
  } else {
   curPos = dataStart + (((myDB.size()-dataStart)/rowSize)*rowSize);
   myDB.seek(curPos); //Goto End of File
  }
  //Write a empty record
  for (unsigned int i=2;i<rowSize;i++) { //Skip CRLF
    myDB.write(' ');
	}
  myDB.println();
  myDB.seek(curPos);
  return 1;
}

//Remove the current record, by write a comment (#) tag at the first postion
void DB::remove(){
  unsigned long curPos = myDB.position();
  myDB.write(DELETED);
  myDB.seek(curPos);
}

//Restore a deleted record into active
void DB::restore(){
  if (myDB.peek()==DELETED) {
    myDB.write(' ');
    myDB.seek(curPos);
  }
}

//Find the column for the given column name
byte DB::columnNo(char *column) {
  char readbuf[MAX_COLUMN_NAME+1];
  myDB.seek(0); //Goto begining of file
  char token=readColumn(readbuf); //This is the defaultSize
  byte i=0;
  while (strcmp(column,readbuf)!=0 && (token=readColumn(readbuf))!=10) {if (token==DELIMITER || token=='(') i++;}
  myDB.seek(curPos); // Set database back in original position
  byte ret= strcmp(column,readbuf)==0 ? i : 0;
  return ret;
}

// Protected functions

//Find the position of a column
int DB::columnPos(byte column){
  if (column<=0 || column>fieldCount) return 0;
  int len=0;
  for  (int i=0;i<column;i++) len+=columns[i];
  return len;
}

//Parse a column
char DB::readColumn(char *readbuf){
  int pos=0;
  int c;
  readbuf[pos]='\0';
  while (pos<MAX_COLUMN_NAME && (c=myDB.read())>=0) {
    if (c==13) {}//Just skip it
    else if (pos==0 && (c==DELIMITER || c=='(' || c==')')) {} //Skip token
    else if ( c==DELIMITER || c=='(' || c==')' || c==10) return c;
    else {
        readbuf[pos]=c;
        readbuf[++pos]='\0'; //Terminate string
    }
  }
  return 0;
}
