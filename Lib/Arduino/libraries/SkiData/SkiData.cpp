#include "SkiData.h"

//Set the sync time compared to start device
//Time is time from start
void SkiData::setSyncMillis(long diffTime){
    _timeDiff=diffTime;
}

// Function to retuned the synchonized millis between start and other devices
unsigned long SkiData::getSyncMillis(unsigned long Time,boolean sub){
 if (!_timeDiff) return Time;
 if (sub) return Time - _timeDiff;
 return Time+_timeDiff;
}

//Reset the variables
void SkiData::reset(){
  _listStart=0;
  _listSize=0;
  memset(_list,0,sizeof(timing_t)*SKI_MAX_DATA);
}

byte SkiData::size(){
  return _listSize;
}

byte SkiData::listEnd(){
  int _listEnd=_listStart+_listSize;
  if (_listEnd>=SKI_MAX_DATA) _listEnd-=SKI_MAX_DATA;
  return _listEnd;
}

//Protected function called to add a record to the active list
boolean SkiData::add(timing_ptr rec,boolean update){
   if (!rec->start) return false; //Start should be set and we should be syncing
   //See if the record is allready in the list and if not where is should be added
   int i=_listStart;
   for (byte j=0;j<SKI_MAX_DATA && j!=_listSize;j++){
     if (_list[i].start==rec->start) {
         if (update) _list[i] = *rec; //Just update the data
         return false; //All ready in list so we don't add it
     }
     if (rec->start<=_list[i].start) break; //Exit the for loop we found a position to insert
     //Make sure we look to begining of chain
     if ((++i)==SKI_MAX_DATA) i=0;
   }

   int f=listEnd();

   //Check if there is capacity to add
   if (_listSize==SKI_MAX_DATA){
      if (!noSync) return false;
      remove(&_list[f],false); //When no sync and list full just remove last
    }
   //Check if we should Add at the end of the list or insert it
   int t=f-1;
   if (t<0) t=SKI_MAX_DATA-1;
   while (f!=i) {
      _list[f] = _list[t];
      f=t;
      if ((--t)<0) t=SKI_MAX_DATA-1;
   }
   _list[i] = *rec;
   _listSize++; //We just added one record
   return true;
}

boolean SkiData::remove(timing_ptr rec,boolean update){
    //See if the record is allready in the list and if not where is should be added
   int i=_listStart;
   for (byte j=0;j<SKI_MAX_DATA && j!=_listSize;j++){
     if (_list[i].start==rec->start) { //Found the record
         if (update) _list[i]=*rec; //Just update the data
         if (i!=_listStart) { //We are at middle of list move record to start
            int t=i;
            //timing_t _rec=*rec; //Copy data becaue we could remove ower own record
            while (j>0) {
              if ((i--)<0) i=SKI_MAX_DATA-1;
              _list[t]=_list[i];
              t=i;
              j--;
            }
            _list[_listStart]=*rec; //Move te record to _listStart
         }
         _listStart++; //Reduce the list start
         if (_listStart==SKI_MAX_DATA) _listStart=0;
         _listSize--; //Reduce the list Size
         return true;
     }
     if ((++i)==SKI_MAX_DATA) i=0;
   }
   return false;
}

int SkiData::finishPos(int pos){
  int i=_listStart-pos-1;
  while (i<0) {i+=SKI_MAX_DATA;}
  return i;
}

int SkiData::realPos(int pos){    //Return the real pos
  if (pos<0)  return pos;
  byte i=pos+_listStart;
  while (i>=SKI_MAX_DATA){i-=SKI_MAX_DATA;}
  return i;
}

boolean SkiData::read(byte pos,timing_ptr rec){
  *rec=_list[realPos(pos)];
  return (pos<_listSize);
}

boolean SkiData::remove(byte pos) {
  timing_t rec;
  if (!read(pos,&rec)) return false;
  return remove(&rec,false);
}

int SkiData::find(uint32_t start,boolean deleted){
   int i=_listStart;
   for (byte j=0;j<SKI_MAX_DATA && (j!=_listSize || deleted);j++){
     if (_list[i].start==start) return j;
      //Make sure we look to begining of chain
     if ((++i)==SKI_MAX_DATA) i=0;
   }
   return -1;
}

//Create a synchronize record for the list based on the deviceType, when deviceTypeIn is set sync will return if it made changes
//Start record is only update with intermediate and finish
//Intermediate and Finish will accept the start record and add it.
boolean SkiData::sync(char deviceType,timing_ptr data,char deviceTypeIn){
  if (noSync) return false;
  boolean ret=false;
  int pos;
  if (deviceTypeIn!=0 && deviceType!=deviceTypeIn) {
      //Find Record even if it is deleted
      pos=realPos(find(data->start,deviceType==FINISH_DEVICE || deviceType==CLIENT_DEVICE));

    switch (deviceType) {
        case START_DEVICE: //I only update my date if i still have the record
         if (pos!=-1) {
           if (deviceTypeIn==FINISH_DEVICE) {
             ret|=(_list[pos].finish!=data->finish || data->finish==DNF_LONG);
             _list[pos].finish=data->finish;
             _list[pos].sessiongroup=data->sessiongroup;
           } else if (deviceTypeIn==INTERMEDIATE_DEVICE) {
             ret|=(_list[pos].intermediate!=data->intermediate);
             _list[pos].intermediate=data->intermediate;
             _list[pos].sessiongroup=data->sessiongroup;
           }
         }
         break;

        case INTERMEDIATE_DEVICE:
         if (pos!=-1) {
           if (deviceTypeIn==FINISH_DEVICE) {
             ret|=(_list[pos].finish!=data->finish);
             _list[pos].finish=data->finish;
             _list[pos].sessiongroup=data->sessiongroup;
           }
         } else if (deviceTypeIn==START_DEVICE) {
           ret|=add(data);
         }
         break;

        case FINISH_DEVICE:
         if (pos!=-1) {
            if (deviceTypeIn==INTERMEDIATE_DEVICE) {
             ret|=(_list[pos].intermediate!=data->intermediate);
             _list[pos].intermediate=data->intermediate;
             _list[pos].sessiongroup=data->sessiongroup;
           }
           if (data->finish==DNF_LONG) {
             ret|=(_list[pos].finish!=data->finish);
             _list[pos].finish=data->finish;
             _list[pos].sessiongroup=data->sessiongroup;
           }
         } else if (deviceTypeIn==START_DEVICE) {
           ret|=add(data);
         }
         break;

         case CLIENT_DEVICE:  //A clients device will build list on all data received
          if (pos!=-1) {
           if (deviceTypeIn==FINISH_DEVICE) {
             ret|=_list[pos].finish!=data->finish;
             _list[pos].finish=data->finish;
             _list[pos].sessiongroup=data->sessiongroup;
             ret|=remove(data,false);   //Data is in active list just remove it
            } else if (deviceTypeIn==INTERMEDIATE_DEVICE) {
             ret|=(_list[pos].intermediate!=data->intermediate);
             _list[pos].intermediate=data->intermediate;
             _list[pos].sessiongroup=data->sessiongroup;
            }
           } else if (deviceTypeIn==START_DEVICE) {
             ret|=add(data);
           } else if (deviceTypeIn==FINISH_DEVICE) {
             //Check if we have a newer finish which was not added
             if (_list[finishPos(0)].start<data->start) {
               ret|=add(data);
               ret|=remove(data,false);
             }
           }
         break;

      }
  }
  if (ret) {
    read(find(data->start,true),data);
  }
  return ret;
}
