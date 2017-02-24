#include <Time.h>
#include <Fat16.h>             //Small foot printed SD Lib
//#include <SD.h>                //The SD Card software library
#include <DB.h>

//The constansts used
#define SDSelect 10           //The pin to select the SD Card
#define dbfile "my.db"

//The database class
DB myDB;

void dumpRec(){
  // Serial.print("Record Position/Count:");Serial.print(myDB.position());Serial.print('/');Serial.println(myDB.count());
   char buf[21];
   Serial.print(myDB.position());
   if (!myDB.isDeleted()) Serial.print(" -> "); else Serial.print(" #> ");
   for (byte i=1;i<=myDB.columnCount();i++) {
       myDB.read(i,buf,sizeof(buf));
      Serial.print(buf);
      if (i!=myDB.columnCount()) Serial.print(',');
   }
   Serial.println();
}

void dumpDb(byte showDeleted=0){
   Serial.println("Start Dump database");
   myDB.first(showDeleted);
   do {
     dumpRec();
   } while (myDB.next(showDeleted));
   Serial.println("End Dump database");
}

void setup(){
   Serial.begin(9600);
   Serial.println("Database test start");

    //Init the SD Card
   if (!SDCARD.begin(SDSelect)) {
     Serial.println("Failed to mount SD card");
     return;
   } else {
     Serial.println("SD Mounted");
     SDCARD.remove(dbfile);
   }

   if (!myDB.open(dbfile)) {
     Serial.println("Failed to open database");
     if (!myDB.create(dbfile,"Id(10),Value1(20),Value2(15)")) {
        Serial.println("Failed to create database");
        return;
     } else {
       Serial.println("Creating Database");
       myDB.append();myDB.write(1,10);myDB.write(2,"X");myDB.write(3,"Test");

       for (int i=0;i<15;i++) {
         if (!myDB.append()) {
            Serial.println("Could not create new record");
            return;
         }
         if (!myDB.write(1,i)) Serial.println("Failed to write the ID");
         myDB.write(2,i+1);
         myDB.write(myDB.columnNo("Value2"),i*10);
       }
      Serial.print("Record Position/Count:");Serial.print(myDB.position());Serial.print('/');Serial.println(myDB.count());
     }
   }

   dumpDb();
   if (!myDB.search(1,10)){
     Serial.println("Search item not found");
     return;
   } else {
     myDB.remove();
     dumpRec();
     if (!myDB.search(1,10,DB_REVERSE)) {
       Serial.println("inCorrect not found");
     } else {
       Serial.println("Found again");
       myDB.remove();
     }
     dumpRec();
      if (!myDB.search(1,10,DB_REVERSE)) {
       Serial.println("Correct not found");
     } else {
       Serial.println("Error:Found again");
     }
     dumpRec();
   }
   //Check if we can use the empty records
   dumpDb(DB_YES);
   myDB.append(DB_YES);
   myDB.write(1,100);myDB.write(2,"Y");myDB.write(3,"Reuse");
   myDB.append(DB_YES);
   myDB.write(1,101);myDB.write(2,"Z");myDB.write(3,"Reuse2");
   myDB.append(DB_YES);
   myDB.write(1,102);myDB.write(2,"A");myDB.write(3,"Reuse3");
   dumpDb(DB_YES);

   //Update
   myDB.search(1,101);
   myDB.write(2,"Update");
   dumpDb();

   myDB.close();
   Serial.println("Database test done");
 }

 void loop(){
 }