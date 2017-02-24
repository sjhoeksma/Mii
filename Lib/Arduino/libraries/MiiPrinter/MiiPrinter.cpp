#include <MiiPrinter.h>

MiiPrinter::MiiPrinter(Stream &stream):
 serial(stream),
 _step(0xff)
{}

bool MiiPrinter::begin(const char* name,uint32_t pinCode){
  clearBuf();
  strcpy(_name,name);
  _pinCode=pinCode;
  _wait=MII_BLUETOOTH_CMD_WAIT; //We have to bluetooth mode time to start
  _step=0;
  return true;
}

bool MiiPrinter::isConnected(){
  return _step==BT_READY;
}

bool MiiPrinter::connect(){
  while (!isConnected()) {
    doProcess();
  }
  return true;
}

void MiiPrinter::reconnect(){
  _step=0;
}

void MiiPrinter::onError(){
  _step=0xFF; //Failure
  clearBuf();
}

//Config
//https://arduino-info.wikispaces.com/BlueTooth-HC05-HC06-Modules-How-To
void MiiPrinter::doProcess() {
  if (_step>=BT_READY) return;
  if (_wait>millis() && _step!=17) {
    return; //We still need to wait before we can process next command
  }
  int cmd=0;

  switch (_step) {
     case 0 :
      serial.println(F("AT"));
      goto processWaitExit;
/*
     case  0 :
       serial.println(F("AT+DISC"));  //Disconnect, can throw error
       goto processWaitExit;
     case 1:
       clearBuf(); ///Ignore error of not connected
       _step++;
       break;
*/
     case 2 :
      serial.println(F("AT+RMAAD"));  //will release the module from any previous PAIR.
      goto processWaitExit;
     case 4 :
      serial.println(F("AT+ROLE=1"));  //Set mode to Master
      goto processWaitExit;
     case 6 :
      serial.println(F("AT+RESET"));  //After changing role, reset is required
      goto processWaitExit;
     case 8:
      serial.println(F("AT+CMODE=1"));  //Allow connection to any address
      goto processWaitExit;
     case  10:
      serial.print(F("AT+INQM=0,")); serial.print(BT_SEARCH_COUNT);serial.print(',');serial.println(BT_SEARCH_TIME);  //Inquire mode - Standard, stop after 5 devices found or after 5 seconds
      goto processWaitExit;
     case  12:
      serial.print(F("AT+PSWD="));serial.println(_pinCode);  //Set the pinCode
      goto processWaitExit;
     case  14 :
      serial.println(F("AT+INIT"));  //Start serial Port Profile (SPP) ( If Error(17) returned - ignore as profile already loaded)
      goto processWaitExit;
     case 15  :
       clearBuf(); ///Ignore error of 14
       _step++;
       break;

     case 16 :
       clearBuf();
       serial.println(F("AT+INQ"));  //Start searching for devices, we will fall through to 17
       _step++;
       _wait=millis()+(BT_SEARCH_TIME+5)*1000; //We will wait the search time and 5 second before erroring out
       _listPos=0;
       break;

     case 17 :
       if (_wait<millis()) {
        if (_listPos>0) { //Should not happend but just to be sure.
          _step++;
        } else {
           onError(); //OOPS We timed out, go back to begining
        }
       } else {
         while (serial.available()) {
           cmd=serial.read();
           //Fill the address table--> +INQ:2:72:D2224,3E0104,FFBC <part1>:<part2>:<part3> , type, signal
           if (cmd=='+') {
              //SKIP INQ:
              uint8_t pos = 4;
              while (pos>0) { if (serial.available()) {cmd=serial.read();pos--;}}
              pos=0;
              while (pos<sizeof(btaddress_t) && cmd!=','){
                if (serial.available()) {
                  cmd=serial.read();
                  _list[_listPos].address[pos++]=cmd==',' ? '\0' : cmd==':' ? ',' : cmd;
                }
              }
              _listPos++;
              //Remove till end of line
              while (serial.read()!='\r'){}
           } else if (cmd=='O') {
             while (!serial.available() &&  _wait<millis()){}
             if (serial.read()=='K') {
              clearBuf();
              _step++;
              return;
             }
           }
         }
       }
       break;

     case 18:
      //We found a bluetooth device and stored them in the list
      //Now loop and query name for each found device and see if we have match
      clearBuf();
      if  (_listPos>0) {
        _listPos--;
        serial.print(F("AT+RNAME?"));serial.println(_list[_listPos].address);
        _wait=millis()+4000;
        _step++;
      } else {
        _step=16; //No Data search again
      }
      break;

     case 19:
        while (serial.available()) {
          cmd=serial.read();
          //Fill the name--> +RNAME:NAME and match it
          if (cmd=='+') {
              //SKIP RNAME:
              uint8_t pos = 6;
              while (pos>0) { if (serial.available()) {cmd=serial.read();pos--;}}
              pos=0;
              bool match=false;
              while (serial.available() && (cmd=serial.read())!='\r') {
                if (_name[pos++]==cmd) {
                  match=true;
                } else match=false;
              }
             if (cmd=='\r' && match) { //Now we have found match do a link
               _step++;
               return;
             }
          }
        }
        clearBuf();
        _step=18;
     break;

     case 20:
      clearBuf();
      serial.print(F("AT+LINK="));serial.println(_list[_listPos].address);
      _wait=millis()+4000;
      _step++;
      break;

     case 21:
       //Check for Failure
       while (serial.available() && (cmd=serial.read())!='F') {}
       if (cmd=='F' && serial.read()=='A') { //FAILURE
         onError(); //Error state
         return;
       }
       _step=BT_READY;
       onConnected();

     case BT_READY: //This is the final step we are connected
      break;


    case 1: case 3: case 5: case 7: case 9: case 11: case 13:
       while (serial.available() && (cmd=serial.read())!='O') {}
       if (cmd=='O' && serial.read()=='K') {
         _step++;
       } else {
         onError(); //Error state
       }
       break;
  }
  return;
processWaitExit:
  _step++;
  _wait=millis()+MII_BLUETOOTH_CMD_WAIT;
}

void MiiPrinter::clearBuf(){
 while (serial.available() && serial.read()>=0){};
}


void MiiPrinter::tab(uint16_t pos){
 while (_printPos<pos-1) write(' ');
}

void MiiPrinter::line(uint16_t len){
 while (len>0) {len--;write('=');}
 println();
}

void MiiPrinter::time(uint32_t t,bool rounding){
  uint16_t h = (t/3600000)%60;
  if (h>0) {
     print(h%24);
     print(':');
  }
  uint16_t m = (t/60000)%60;
  //Minute
  if (m>0 || h>0) {
    if (m<10 && h>0) print('0');
    print(m);
    print(':');
  }
  //Seconds
  uint16_t s= (t/1000)%60;
  if (s<10 && (h>0 || m>0)) {
       print('0');
  }
  print(s);
  //Print Mills
  print('.');

  if(rounding) {
    t=(t+MII_ROUNDING)/10; //Rounding and remove thousands
    t=t%100;
  } else {
    t=t%1000;
    if (t<100) print('0');
  }
  if (t<10) print('0');
  print(t);
}

//Print the date
void MiiPrinter::date(time_t date,bool time){
  print(day(date));
  print('/');
  print(month(date));
  print('/');
  print(year(date)%100);
  if (time) {
    print(' ');
    print(hour(date));
    print(':');
    print(minute(date));
    print(':');
    print(second(date));
  }
}

size_t MiiPrinter::write(uint8_t character) {
 //Update print pos always have to take place otherwise system can hang in tab function
 if (character==10 || character==13) _printPos=0; else _printPos++;
 if (!isConnected()) return 0;
 return serial.write(character); /*Code to display letter when given the ASCII code for it*/
}

