'use strict';
angular.module('app.miichrono')
.factory('MiiChronoService',
 function($q,$rootScope,$ionicScrollDelegate,$timeout,$secureStorage) {
	const MII_DB = "MiiChrono";
	const MII_SETTINGS = "Settings";
	const MII_SESSIONS = "Sessions";
	const BLUETOOTH_START = "---MII";
	
	const BEST_TIME = 1;
	const LAST_TWO  = 2;
	const BEST_TWO  = 3;
		
	const MAX_TIMES            =4;
	
  const MII_CMD_SWITCH       =1;
  const MII_CMD_SET_DELAY    =2;
  const MII_CMD_DATA         =3;
  const MII_CMD_SET_USER     =4;
  const MII_CMD_LOG          =5;
  const MII_CMD_CLEAR        =6;
  const MII_CMD_SET_MODE     =7;
  const MII_CMD_SET_CONFIG   =8;
  const MII_CMD_SET_STATE    =9;
  const MII_CMD_SET_BOUNDARY =10;
  const MII_CMD_SET_EXTRA    =11;
  const MII_CMD_SET_SESSION  =12;
  const MII_CMD_GET_CONFIG   =13;
  const MII_CMD_SET_DATE     =14;
  const MII_CMD_SERIALID     =15;
  const MII_CMD_SWITCH_ACK   =16;
  const MII_CMD_SWAP_FINISH  =17;
  const MII_CMD_CHANGE_USER  =18;
  const MII_CMD_FIRMWARE     =19;
  const MII_CMD_NO_FINISH    =20;
  const MII_CMD_COUNT_DOWN   =21;
	
	//States used in communication
  const MII_STATE_FINISH   =200;
  const MII_STATE_FINISH_PREV =201;
  const MII_STATE_FINISH_LAST =202;
  const MII_STATE_DNF      =205;
  const MII_STATE_DSQ      =210;
  const MII_STATE_DSQ_L    =211;
  const MII_STATE_DSQ_R    =212;
  const MII_STATE_DNS      =215;
  const MII_STATE_DNS_L    =216;
  const MII_STATE_DNS_R    =217;
	const MII_STATE_REMOVED  =254;
	const MII_STATE_ERR      =255;
	
	const MII_MODE_TIME      = 0x80;
  const MII_MODE_PARALLEL  = 0x40;
  //Should we follow session list
  const MII_MODE_SESSIONLIST = 0x20;
	
	const INVALID_TIME       =9999999999;
	
	
	var sortHashCode = function(s,v) {
		var hash = 0, i, chr, len;
		if (s==null || s.length === 0) return -9999+v;
		for (i = 0, len = s.length; i < len; i++) {
			chr   = s.charCodeAt(i);
			hash  = ((hash << 5) - hash) + chr;
			//hash |= 0; // Convert to 32bit integer
		}
		return (hash*1000)+v;
	}
	var dataToString = function(data,len){
		var str = "";
		for (var i=0;i<len;i++) {
			var v = data.getUint8 ? data.getUint8(i) : data[i];
			if (v>32 && v<126) str+=String.fromCharCode(v);
			else str += "<" + v + ">";
		}
		return str;
	}
	
	var localNow = function(){
		var date = new Date(); 
    var newDate = new Date(date.getTime()+date.getTimezoneOffset()*60*1000);
    var offset = date.getTimezoneOffset() / 60;
    var hours = date.getHours();
    newDate.setHours(hours - offset);
    return newDate.getTime();   
	}
	
	var self={};
	//state: 0=disconnected, 1=connecting, 2=connected
	self.state=0;
	//0=Not scanning, 1=scanning
	self.btListState=0;
	

	self.clear = function(){
		self.resultsList = [];
	  self.activeList =[];
		self.freezeRec={};
		self.users={};
	}
	self.clear();
	
	self.session ={id:1,nextUser:'-',activeCount:0,time:0};
  self.sessions=[];
	//The settings

	self.settings={
		rankingModel: BEST_TIME,
		bluetooth : false,
		bluetoothid : "",
		rounding : false,
		showThousands: false,
		maxVisibleResults: 10,
		freezeTime: 15,
		expert:false,
		showRemoved:true,
		allwaysOn:false,
		uploadEnabled:false,
		sessionName:'default'
	};
	
	
	//Check if time if within the last received data list
	self.inDeviceList = function(timeRec){
		if (self.state<2 || !self.session.data ||!timeRec) return false;
		for (var i=0,l=self.session.data.length;i<l;i++){
			if (self.session.data[i].start==timeRec.start) return true;
		}
		return false;
	}
	
	//Restore session data
	self.selectSession = function (sessionName){
		self.clear();
		if (self.settings.sessionName!=sessionName) self.settings.sessionName=sessionName;
		console.log("Selection session",sessionName);
		$secureStorage.get(MII_DB,'MIU_'+sessionName,{}).then(function(users){
			console.log("Restored user",users);
			self.users=users;
		});
		$secureStorage.get(MII_DB,'MID_'+sessionName,[]).then(function(resultsList){
			console.log("Restored resultList",resultsList);
			self.resultsList = resultsList;
			self.updateRanking(true);
			self.fmtTimes();
		});
		self.addSession(sessionName);
	}
	
	self.saveSessionUsers = function(){
		console.log("Saving SessionUsers",self.settings.sessionName);
		$secureStorage.set(MII_DB,'MIU_'+self.settings.sessionName,self.users);
	}
	
	self.removeSession = function(sessionName){
		$secureStorage.remove(MII_DB,'MID_'+sessionName);
		$secureStorage.remove(MII_DB,'MIU_'+sessionName);
		if (sessionName==self.settings.sessionName){
			self.selectSession('default');
		}
		//Remove session from session list
	  for (var i=0,l=self.sessions.length;i<l;i++){
			if (self.sessions[i]==sessionName){
				self.sessions.splice(i,1);
				break;
			}
		}
	}
	
	self.resetSession= function(sessionName){
		$secureStorage.remove(MII_DB,'MID_'+sessionName);
		if (sessionName==self.settings.sessionName){
			self.sendCmd(MII_CMD_CLEAR);
			self.resultsList = [];
	    self.activeList =[];
		  self.freezeRec={};
		}
	}
	
	self.addSession=function(sessionName){
	  if (!self.sessions.contains(sessionName)){
			self.sessions.push(sessionName);
		}
	}
	
	self.saveSessionResults = function(){
		console.log("Saving SessionResult",self.settings.sessionName);
		$secureStorage.set(MII_DB,'MID_'+self.settings.sessionName,self.resultsList);
	}
	
	self.addUser = function(userid,userData){
		userData['id']=userid;
	  self.users[userid]=userData;
		self.saveSessionUsers();
		self.updateRanking(true);
		self.saveSessionResults(); //Make change permanent
	}
	
	self.removeUser = function(userid){
		if (self.users[userid]){
			delete self.users[userid];
			self.saveSessionUsers();
		  self.updateRanking(true);
		  self.saveSessionResults(); //Make change permanent
		}
	}
	
	//Load the session data and session data
	$secureStorage.get(MII_DB,MII_SETTINGS,	self.settings).then(function(settings){
		self.settings=settings;
		self.sessions=[];
		//Now load the session info	
		self.selectSession(self.settings.sessionName||'default');
		$secureStorage.keys(MII_DB,function(keys){
			angular.forEach(keys,function(key){
				if (key.indexOf('MID_')==0) {
					var v=key.substr(4);
					self.addSession(v);
				}
			})
		});
	});
	


	//Watch settings changes
	$rootScope.$watch(function() {return self.settings;}, 
		function watchCallback(newValue, oldValue) {
			//Check if bluetooth changed
			if (newValue.bluetooth!=oldValue.bluetooth) {
				if (newValue.bluetooth) {
					self.refreshBtList();
					if (newValue.bluetoothid) self.btConnect(newValue.bluetoothid);
				} else self.btDisconnect();
			}
			//Check if bluetooth id changed
			if (newValue.bluetoothid!=oldValue.bluetoothid) {
				if (newValue.bluetoothid) {
					 self.btConnect(newValue.bluetoothid);
				} else self.btDisconnect();
			} 
		
		 //Keep the screen on
		 if (oldValue.allwaysOn && !newValue.allwaysOn ) {
			$rootScope.powerManagement(false);
		 } 
		 if (newValue.allwaysOn) {
			$rootScope.powerManagement(true);
		 }

		 //Check if we should update timing model
		 if (newValue.showThousands!=oldValue.showThousands ||
				 newValue.rounding!=oldValue.rounding
			) self.fmtTimes();
		
		 //Check if we changed parallel mode
		 if (newValue.parallel!=oldValue.parallel){
			 var mode = newValue.parallel ? MII_MODE_PARALLEL : MII_MODE_TIME;
			 if (self.session && !(self.session.mode&mode)) self.sendCmdUint8(MII_CMD_SET_MODE,mode)
		 }
		

		 //Check if we should update ranking model
		 if (newValue.rankingModel!=oldValue.rankingModel ||
				 newValue.showThousands!=oldValue.showThousands ||
				 newValue.rounding!=oldValue.rounding
			) self.updateRanking();
		
		  //Check witch features we should disable because we are not expert
		  if (newValue.expert!=oldValue.expert && !newValue.expert) {
				if (newValue.uploadEnabled) self.settings.uploadEnabled=false;
				if (newValue.bluetooth) self.settings.bluetooth=false;
				if (newValue.rounding) self.settings.rounding=false;
			}
		 if (newValue.sessionName!=oldValue.sessionName) self.selectSession(newValue.sessionName);
		 //Store the changed settings
		 if (!angular.equals(newValue, oldValue)) {
			 //console.log("Changed Settings:" + JSON.stringify(newValue) + " ---- " + JSON.stringify(oldValue));
			$secureStorage.set(MII_DB,MII_SETTINGS,newValue);
		 }
	},true);

	//Send a DNF to user
	self.sendDNF = function (timeRec){
		return self.sendState(MII_STATE_DNF,timeRec); 
	}
	
	self.noFinish = function(timeRec){
		if (timeRec) return self.sendCmdUint32(MII_CMD_NO_FINISH,timeRec.start);
		return false;
	}
	
	self.fixNoFinish = function(timeRec){
		if (timeRec){
			self.sendState(MII_STATE_FINISH,timeRec);
		} else if (self.activeList.length && self.activeList[0].time && self.activeList[0].state==0) {
			self.sendState(MII_STATE_FINISH.self.activeList[0]);
		}
	}
	
	self.sendDSQ = function(timeRec){
		return self.sendState(MII_STATE_DSQ,timeRec);
	}
	
	self.sendREMOVE = function(timeRec){
		return self.sendState(MII_STATE_REMOVED,timeRec);
	}
	
	
	//Send a state change for a user only as expert
	self.sendState = function(state,timeRec){
		if (!timeRec && self.activeList.length) timeRec=self.activeList[0];			 
		if (!timeRec || !timeRec.start || !self.settings.expert) return $q.reject();
	  var buf = new Uint8Array(5);
		var vbuf=new DataView(buf.buffer);
		vbuf.setUint32(0,timeRec.start,true);
		vbuf.setUint8(4,state);
    //We need copy of record, otherwise update is skipped
		var rec=angular.copy(timeRec);
		rec.state=state;
		self.add(rec);		
		return self.sendCmd(MII_CMD_SET_STATE,buf);
	}
	
	//Set the next user id
	self.setNextUser = function (id){
		if (id!=self.session.nextUser){
		  self.session.nextUser=id;
		  self.sendCmdUint16(MII_CMD_SET_USER,id);
		}
	}
	
	//Change the user for a time rec
	self.changeUser = function(timeRec,id){
		// typedef struct {uint8_t fromState;uint32_t time;uint16_t user;} changeuser_t, *changeuser_ptr;
		self.remove(timeRec,false);
		var buf = new Uint8Array(7);
		var vbuf=new DataView(buf.buffer);
		vbuf.setUint8(0,timeRec.state);
		vbuf.setUint32(1,timeRec.start,true);
		vbuf.setUint16(5,id,true);
		timeRec.id=id;
		self.add(timeRec);
		return self.sendCmd(MII_CMD_CHANGE_USER,buf);
	}
	
	self.sendCmdUint16 = function (cmd,data){
		var buf = new Uint8Array(2)
		var vbuf=new DataView(buf.buffer);
		vbuf.setUint16(0,data,true);
		return self.sendCmd(cmd,buf);
	}
	self.sendCmdUint32 = function (cmd,data){
		var buf = new Uint8Array(4)
		var vbuf= new DataView(buf.buffer);
		vbuf.setUint32(0,data,true);
		return self.sendCmd(cmd,buf);
	}
	self.sendCmdUint8 = function (cmd,data){
		var buf=new Uint8Array(1)
		buf[0]=data;
		return self.sendCmd(cmd,buf);
	}
	
	self.bluetoothAvailable=!!window.bluetoothSerial;
			
	self.sendCmd= function (cmd,data){
	 	var def = $q.defer();
	 //We only send commands if we are connected to device	
	 if (!window.bluetoothSerial || !cmd || self.state<2 ) {
		 def.reject();
	 } else {
		  var len = BLUETOOTH_START.length + 1 + (data ? data.byteLength || data.length : 0);
		  var buf=new Uint8Array(len);
		  var i=0;
		  for (; i<BLUETOOTH_START.length; i++) {
	     buf[i]=BLUETOOTH_START.charCodeAt(i);
      }
		  buf[i++]=cmd;
		  for (var j=0;i<len;i++,j++) buf[i]=data[j];
		  console.log('Sending('+len+'):' + dataToString(buf,len));
		  bluetoothSerial.write(buf, def.resolve, def.reject);
	 }
		return def.promise;
	}
	

	//Process a Mii Command
	self.ProcessMiiCmd = function(cmd,len,data){
		//We got data sho update state
		self.state=self.state==2 ? 3 : 2;
		if (cmd==MII_CMD_DATA) {
			//uint16_t  sessionId;uint32_t  sessionDate; uint16_t  nextUser; uint16_t  extra; uint8_t mode; 
			//timeRec_t times[MAX_TIMES] (uint16_t user;uint8_t  state;uint32_t start; uint32_t time);
			self.session.id=data.getUint16(0,true);
			self.session.time=data.getUint32(2,true);
			self.session.nextUser=data.getUint16(6,true);
			self.session.extra=data.getUint16(8,true);
			self.session.mode=data.getUint8(10) & 0xf0;
			self.session.activeCount=data.getUint8(10) & 0x0f;
			//Now we will read the blocks
			var lastData=[];
			for (var i=0;i<MAX_TIMES;i++){
				var rec={};
				rec.id=data.getUint16(11+(i*11),true);
				rec.state=data.getUint8(13+(i*11),true);
				rec.start=data.getUint32(14+(i*11),true);
				rec.time=data.getUint32(18+(i*11),true);
	      if (rec.start) {
					lastData.push(rec);
					self.add(rec);
				}
			}
			self.session.data=lastData;
			//Check if we should set the session time
			if (self.session.time==0){
				self.sendCmdUint32(MII_CMD_SET_DATE,parseInt(localNow()/1000));
			}
			if (!(self.session.mode&(self.settings.parallel ? MII_MODE_PARALLEL : MII_MODE_TIME))){
				self.settings.parallel=(self.session.mode&MII_MODE_PARALLEL) ? true : false;
			}
		} else if (cmd==MII_CMD_LOG) {
			console.log("LOG:" + dataToString(data,len));
		} else if (cmd==MII_CMD_SERIALID) {
			self.deviceId=data.getUint32(0,true);
			//We should load the user data for this device now
			console.log("DeviceId:" + self.deviceId);
		} else if (cmd==MII_CMD_FIRMWARE) {
			self.firmware=data.getUint16(0,true);
			console.log("Firmware:" + self.firmware);
		}
		//console.log("Data C:" + cmd+ " L:" + len);
	}
	
	self.sessionTime = function(){
		if (self.session.time && self.btSendTime){
			return self.session.time*1000 + self.btSendTime;
		} else {
			return localNow();
		}
	}
	
	self.btShowError = function(error) {
    console.error("Bluetooth:" +JSON.stringify(error));
  } 
		
 //Bluetooth support
 self.bluetoothList ={};	
 self.refreshBtList =  function() {
	 if (!window.bluetoothSerial || !self.settings.bluetooth) return;
		var listPorts = function() {
			  self.btListState=1;
				bluetoothSerial.list(function(results) {
					 self.bluetoothList ={};
					 for (var i=0;i<results.length;i++) {
						// if (results[i].name
								 //&& results[i].name.indexOf("Mii-")==0
						//	  ) {
							 self.bluetoothList[results[i].address]={
								id : results[i].id,
								name : results[i].name,
								active: false
						  };
					//	 }
				   }
					 //Unpaired not working on ios
				 	 if (self.state==0 && (ionic.Platform.isAndroid() || ionic.Platform.isWindowsPhone())) {
						bluetoothSerial.discoverUnpaired(function(results){
						 for (var i=0;i<results.length;i++) {
							 if (results[i].name
									// && results[i].name.indexOf("Mii-")==0
									){
								 self.bluetoothList[results[i].address]={
						  		id : results[i].id,
						  		name : results[i].name,
									active:true
					   		};
							 }
						  }
							self.btListState=0;
					   } , function(error) {
					    self.btShowError(error);
							self.btListState=0;
				     })
					 }
					else self.btListState=0;
				 }, function(error) {
					 self.btListState=0;
					 self.btShowError(error);
				 });
		}

		 // check if Bluetooth is on:
		if (window.bluetoothSerial){
			bluetoothSerial.isEnabled(
				listPorts,
				function() {
					console.log("Bluetooth is not enabled.");
					//Not enabled, try to enabled it
					bluetoothSerial.enable(listPorts,
						function(){
						  self.settings.bluetooth=false; //Disable the bluetooth is used does not enable it
						});
				}
		);
		} else {
			console.log("Bluetooth not installed");
		}
  };
	
	self.btDisconnect = function(){
		self.activeList=[]; //On disconnect we clear the active list
		delete self.session.data; //Remove the last data recrod
		if (!window.bluetoothSerial) return;
		var def = $q.defer();
		bluetoothSerial.isConnected(function disconnect(){
			bluetoothSerial.disconnect(function (){
				bluetoothSerial.unsubscribeRawData(function (data) {
				  self.state=0;
					def.resolve();
				},function (msg){
					self.state=0;
			    self.btShowError(msg);
					def.reject();
				})
			}, function(msg){
				self.state=0;
				self.btListState=0;
				self.btShowError(msg)      // show the error if you fail
				def.reject();
			});
		},function(){
			self.state=0;
			self.btListState=0;
		  def.resolve(); //We where allready disconnected		
		});
		return def.promise;
	}
	
	self.btConnect = function(id) {
		if (!id) id = self.settings.bluetoothid;
		if (!window.bluetoothSerial) {
			self.settings.bluetoothid="";
			return;
		}
		console.log('BT Connecting ' + id);
		
		var connect = function(){
			// attempt to connect:
			  self.state=1;
			  self.btListState=0;
			  
				bluetoothSerial.connect( id,  // device to connect to
				  function() {
					  console.log('BT Connected');
					  self.state=2;
					  var phase=-1;
					  var pos=0;
					  var len=0;
					  var cmd;
					  var data=new DataView(new ArrayBuffer(255)); //
							// set up a listener to listen for newlines
							bluetoothSerial.subscribeRawData(function (buffer) {		
								var buf = new Uint8Array(buffer);
								for (var i = 0; i < buf.length; i++) {
									 //Check if we should rest the data
									 if (phase==-1) {
										 phase=0;
										 pos=0; //We start to compare
									 }
									 switch(phase) {
										case 0:	
											if (pos==0) pos=2; //We Skip always the first 2 '-' chars
												if (pos==6) {
													 cmd=buf[i];
													 phase=2;
													 pos=0;
												} else if (buf[i]==BLUETOOTH_START.charCodeAt(pos)) {
													pos++;
												} else {
													pos=0;
												}

										break;

										case 2: //Time
											data.setUint8(pos++,buf[i]);
											if (pos == 4) {
												self.btSendTime = data.getUint32(0,true);
												var timeDiff=(self.btSendTime+16)-new Date().getTime();//Bluetooth delay is 16ms
												if (!self.timeDiff || timeDiff+10>self.timeDiff || timeDiff-10<self.timeDiff)
													self.timeDiff=timeDiff; 
												self.btReceiveTime = self.getTime();
												phase = 3;
											}
											break;

										case 3: //Len
											len=buf[i];
											pos=0;
											phase=(len==0 ? 5 : 4);
											break;
										
										 case 4:
										  data.setUint8(pos++,buf[i]);
											if (len==255 && buf[i]==13) {
												len=pos-1;
												phase=5;
											} else if (len!=255 && pos==len) {
												phase=5;
											} else break;
										case 5:
											phase = -1;
											self.ProcessMiiCmd(cmd,len,data);
											break;
										 }
								}
							},function(){
							  self.state=0;
								self.btListState=0;
					      self.settings.bluetoothid="";
							});
					    //Now we are connected request device info
					    self.sendCmd(MII_CMD_SERIALID);
					
				  },    // start listening if you succeed
					function err(msg){		
					  self.state=0;
					  self.settings.bluetoothid="";
					  console.log('BT Disconnected');
				} 
				);
			  /*
			  //It looks like the disconnect is not allways thrown
			  $timeout(function(){bluetoothSerial.isConnected(function(){
					console.log('BT Connected, timeout check')
				},function(){
					self.state=0;
					self.settings.bluetoothid="";
					console.log('BT Disconnected, timeout check');
				})},15000);
				*/
		}
				
		// here's the real action of the manageConnection function:
		bluetoothSerial.isConnected(function (){
			self.btDisconnect(function(){
				console.log("BT Disconnected from existing connection");
				connect();
			},function(){
				console.log("BT Failed to disconnect existing connection")
				connect();
			});
		}, function(){
			console.log("BT No existing connection")
			connect();
		}
		);
   }
	
	//On first load we should true to reconnect when needed 
	if (self.settings.bluetooth && self.settings.bluetoothid) {
		self.btConnect();
	}
	
	//Data functions		
	self.formatTime = function (t,state,showMillis){
		if (state==MII_STATE_DNF) return "DNF";
		if (state==MII_STATE_DSQ) return "DSQ";
		if (state==MII_STATE_DSQ_L) return "DSQ L";
		if (state==MII_STATE_DSQ_R) return "DSQ R";
		if (state==MII_STATE_DNS) return "DNS";
		if (state==MII_STATE_DNS_L) return "DNS L";
		if (state==MII_STATE_DNS_R) return "DNS R";
		if (state==MII_STATE_REMOVED) return "DEL";
		if (state>MII_STATE_DNS_R) return "ERR";
		if (t>=INVALID_TIME) return "--:--" +  (showMillis ? ".--" : "");
		//state<MII_STATE_DNF	
		t=Math.abs(t);
		if(self.settings.rounding && !self.settings.showThousands) t+=5; //Rounding and hide thousands
		var fmt ="";
		if (state==MII_STATE_FINISH_LAST) fmt="R+";
		else if (state==MII_STATE_FINISH_PREV) fmt="L+";
		var h =  parseInt(t/3600000)%60;
		if (h>0) fmt=fmt+(h%24)+":";
		var m =  parseInt(t/60000)%60;
		//Minute
		if (m>0 || h>0) {
			if (m<10 && h>0) fmt=fmt+"0";
			fmt=fmt+m+":";
		}
		//Seconds
		var s=parseInt(t/1000)%60;
		if (s<10 && (h>0 || m>0)) fmt=fmt+'0';
   	fmt=fmt+s;

		if (state>=MII_STATE_FINISH || showMillis) {
			fmt=fmt+'.';
			if(!self.settings.showThousands) {
				t= parseInt(t/10)%100;
			} else {
			 t=t%1000;
			 if (t<100) fmt=fmt+'0';
			}
			if (t<10) fmt=fmt+'0';
			fmt=fmt+t;
		}
		return fmt;	
	}
	
	self.formatSort = function (user){
	 if (!user) return "-";
	 var usr =self.users[user.id];	
	 return  usr ? (usr.group ? usr.group : "") + (usr.gender ? "("+usr.gender+") " : ""):"";
	}

	var lastUser
	self.formatUser = function(user,cache){
	//console.log("Format",user);
	 if (!user) return "";
	 if (!angular.isNumber(user)) user=user.id;
	 var usr =self.users[user];
	 if (cache){
		 var lUser=usr ? (usr.name + " " + (usr.group ? "," + usr.group : "") + (usr.gender ? "("+usr.gender+") " : "")):"";
		 if (lUser!=lastUser) lastUser=lUser;
	   return lastUser;
	 } else {
		 return usr ? (usr.name + " " + (usr.group ? "," + usr.group : "") + (usr.gender ? "("+usr.gender+") " : "")):"";
	 }
	}
	
	//Return the system time in format of the connected device
	self.timeDiff= -(new Date().getTime());
	self.getTime = function (){
		return new Date().getTime()+self.timeDiff;
	}
	
	self.updateFreeze = function(timeout){
		if (!timeout) timeout=(self.settings.freezeTime)*1000;
		if (self.freezeRec.id && self.freezeRec.start+self.freezeRec.time+timeout<self.getTime()) {
			self.freezeRec={}; //Clear the freezeRec;
		}
	}
		
	self.updateRanking = function(force){
		 //Step one caluclate the best time per group / gender
		 var best=[];
		 angular.forEach(self.resultsList,function(user){
			if (!user.fmtSort || force){
				user.fmtName = self.formatUser(user);
				user.fmtSort = self.formatSort(user);
			}
			//Based on the ranking model we caculate the total time
			
			var cnt =0,st;
			user.state=st=MII_STATE_ERR;
			user.time=INVALID_TIME;
			user.complete=false;
			switch (self.settings.rankingModel) {
				case BEST_TWO:
					var result1,result2;
					angular.forEach(user.results,function (result){
						if (result.state<MII_STATE_DNF) {
						  if (!result1) {
								result1=result;
								user.state=result.state;
								user.time=result.time;
								cnt=1;
							} else if (!result2) {
								cnt=2;
								if (result.time>=result1.time){
									result2=result;
								} else {
									result2=result1;
									result1=result;
								}
								user.state=Math.max(result1.state,result2.state);
								user.time=result1.time+result2.time;
							}
							else { //Caculate who is the best
								if (result.time<result1.time) {
									result2=result1;
									result1=result;
								} else if (result.time<result2.time) {
									result2=result;
								}
								user.state=Math.max(result1.state,result2.state);
								user.time=result1.time+result2.time;
							}
						}
					});
					user.complete=cnt==2;
					user.state=st=Math.max(result1.state,(st>=MII_STATE_REMOVED ? (result2 ? result2.state : result1.state) : st));
				  best.push({id:user.id,time:user.time,grp:user.fmtSort,count:cnt,state:st,data:user}); 
					break;
				case LAST_TWO:
					//TODO: FIX STATE_REMOVED, backwards loop
					//cnt =  user.results.length>=2 ? 2 : user.results.length;
					for (var i=user.results.length-1;i>=0 && cnt!=2;i--) {
						var result = user.results[i];
						if (result.state<MII_STATE_REMOVED) { //Only count none removed objects
							cnt++;
					  	user.state=st=Math.max(result.state,(st>=MII_STATE_REMOVED ? result.state : st));
						  user.time=st>=MII_STATE_DNF ? INVALID_TIME : 
						              user.time==INVALID_TIME ? result.time : user.time+result.time;
						} else if (user.state==MII_STATE_ERR && result.state==MII_STATE_REMOVED) {
								user.state=MII_STATE_REMOVED;
						}
					}
					user.complete=cnt==2;
				  best.push({id:user.id,time:user.time,grp:user.fmtSort,count:cnt,state:st,data:user}); 
					break;
				case BEST_TIME: //TODO: State
				default:
					angular.forEach(user.results,function (result){
						if (result.state<MII_STATE_DNF) {
							cnt=1; //Set the count
							user.state=st=MII_STATE_FINISH;
						  if (result.time<user.time) 
								user.time=result.time;
							user.complete=true;
						} else {
							user.state=st=Math.min(result.state,(st==MII_STATE_ERR ? result.state : st));
						}
	 				});
				  best.push({id:user.id,time:user.time,grp:user.fmtSort,count:cnt,state:st,data:user}); 
			}
		});
		//Sort the best list
		best.sort(function(a,b) {return a.grp > b.grp ? 1 : 
																		b.grp > a.grp ? -1 :
																		a.state > b.state ? 1:
	                                	b.state > a.state ? -1:
																		a.count > b.count ? -1 :
																	  b.count > a.count ? 1 :
																	  a.time > b.time ? 1 :
		                                b.time > a.time ? -1 :
												      0;} );
    //Now update the original records
		var rank=0;
		var rankInc=1;
		var grp="±±±±±"; //We start with strange value to prevent empty group not being grouped
		var time;
		var btime;
		for (var i=0,l=best.length;i<l;i++) {
			var rec = best[i];
			if (rec.grp!=grp) {
				rank=0;
				grp=rec.grp;
				time=-1;
				btime=rec.time;
				rankInc=1;
			}
			if (rec.time!=time) {rank+=rankInc++;rankInc=1;} else rankInc++;
			time=rec.time;
			rec.data.diff= (self.settings.showThousands ||  self.settings.rounding) ? rec.time-btime : 
			              (parseInt(rec.time/10)-parseInt(btime/10))*10 ;
			rec.data.fmtDiff=self.formatTime(rec.data.diff,rec.data.state);
			rec.data.rank=rank;
			rec.data.rankSort=sortHashCode(rec.data.fmtSort,rank);
			rec.data.lastStart=rec.data.results[rec.data.results.length-1].start;
		}
	}
	
	//TODO: Should we not clear freeze on remove
	self.remove = function(timeRec,noRanking) {
		for (var i=0;i<self.activeList.length;i++){
			if (self.activeList[i].id==timeRec.id && self.activeList[i].start==timeRec.start){
				self.activeList.splice(i,1);
				break;
			}
		}
		//Check if for this user a result exists
		for (var i=0;i<self.resultsList.length;i++){	
	   	if (self.resultsList[i].id==timeRec.id){
				var rec =self.resultsList[i];
				//Check if we have the result allready in the list
				for (var j=0;j<rec.results.length;j++){
					if (rec.results[j].start==timeRec.start){ //Found it update
						rec.results.splice(j,1);
				    break;
					}
				}
				//Remove user also if not results
				if (rec.results.length==0) self.resultsList.splice(i);
				//Update the ranking
	    	 if (noRanking!==true) self.updateRanking();
				break;
	    }
		}
	}
	
	//Force update of time
	self.fmtTimes = function(){
	  angular.forEach(self.resultsList,function(user){
			angular.forEach(user.results,function (result){
				result.fmtTime=self.formatTime(result.time,result.state);		
	 		});
		});
	}
	
	//Check if we should automaticly 
	self.nextStartList = function(timeRec){
		if (self.settings.usestartlist) {
			//timeRec.id
			var minUser=9999,nextUser=9999;
			angular.forEach(self.users,function(rec,key){
				var id = parseInt(key);
				minUser=Math.min(id,minUser);
				if (id>timeRec.id && id<nextUser) nextUser=id;
			})
			if (nextUser!=9999) self.setNextUser(nextUser)
			else if (minUser!=9999) self.setNextUser(minUser);
		}
	}
	
	
	//Add a new time record, updating the ranking when item changed or added
	self.add = function(timeRec,noRanking) {
		var state=0;
		//Check if this time is still in active list, if so remove it
		for (var i=0;state==0 && i<self.activeList.length;i++){
			if (self.activeList[i].id==timeRec.id && self.activeList[i].start==timeRec.start){
				if (timeRec.state>=MII_STATE_FINISH) {
					self.activeList[i].state=timeRec.state;
					self.activeList[i].time=timeRec.time;
					//We freeze because time changed
					self.freezeRec=angular.copy(self.activeList[i]);
					self.freezeRec.fmtTime = self.formatTime(timeRec.time,timeRec.state,true);		

					//Now we can remove it
				  self.activeList.splice(i,1);
					state=1;
				} else {
					state=2;
					if (self.activeList[i].state!=timeRec.state ||	self.activeList[i].time!=timeRec.time){
				  	self.activeList[i].state=timeRec.state;
					  self.activeList[i].time=timeRec.time;
						//We freeze because time changed
					  self.freezeRec=angular.copy(self.activeList[i]);
						self.freezeRec.fmtTime = self.formatTime(timeRec.time,timeRec.state,true);
					}
				}
				break;
			}
		}
		//Check if we should add the rec to the active list
		if (timeRec.state<MII_STATE_FINISH && !state) {
			//add it based on start
			var rec ={start:timeRec.start
				      ,id:timeRec.id
				      ,time:timeRec.time
				      ,state:timeRec.state
				      ,fmtName:self.formatUser(timeRec)}; 
			var i=0;
			for (;i<self.activeList.length && self.activeList[i].start<timeRec.start;i++) {}
			if (i==self.activeList.length){ //Add at end of list
		    self.activeList.push(rec);
			} else {
				self.activeList.splice(i, 0, rec);
			}
			self.nextStartList(timeRec);
		}
		state=0;
		//Check if for this user a result exists
		for (var i=0;i<self.resultsList.length && !state;i++){	
	   	if (self.resultsList[i].id==timeRec.id){
				var rec =self.resultsList[i];
				//Check if we have the result allready in the list
				for (var j=0;j<rec.results.length;j++){
					if (rec.results[j].start==timeRec.start){ //Found it update
						//Check if we have reopend time, in this case remove from result list
						if (timeRec.state<MII_STATE_FINISH) {
						   rec.results.splice(j);
							 //Remove user also if not results
				       if (rec.results.length==0) self.resultsList.splice(i);
							 state=4;
							 break; //j
						} else {
							//Don't update if we did not change
							if 	(rec.results[j].time==timeRec.time && rec.results[j].state==timeRec.state){
								noRanking=true;
							} else {
								rec.results[j].time=timeRec.time || rec.results[j].time;
								rec.results[j].state=timeRec.state;
								rec.results[j].fmtTime=self.formatTime(timeRec.time,timeRec.state);
							}
						  state=3;
						  break; //j
						}
					} else if (rec.results[j].start>timeRec.start && timeRec.state>=MII_STATE_FINISH){ //Add Before
						rec.results.splice(j,0,{
							id:timeRec.id
							,start:timeRec.start
				      ,time:timeRec.time
				      ,state:timeRec.state
				      ,fmtTime:self.formatTime(timeRec.time,timeRec.state)
						});
						state=2;
						break; //j
					}
				}
				if (!state && timeRec.state>=MII_STATE_FINISH) {
					rec.results.push({
							id:timeRec.id
						  ,start:timeRec.start
				      ,time:timeRec.time
				      ,state:timeRec.state
				      ,fmtTime:self.formatTime(timeRec.time,timeRec.state)
					});
					state=1;
				}
				break;
			}
		}
		if (!state && timeRec.state>=MII_STATE_FINISH) { //No user found add user and result list
			state=6;
			self.resultsList.push({
			 id:timeRec.id,
			 results:[{
				  id:timeRec.id
			   ,start:timeRec.start
				 ,time:timeRec.time
				 ,state:timeRec.state
				 ,fmtTime:self.formatTime(timeRec.time,timeRec.state)
			  }]});
		}

		//Now we update the ranking and differences
    if (noRanking!==true && state) {
		  self.updateRanking();
		}
		if (noRanking!==true) self.saveSessionResults();
	//	$ionicScrollDelegate.resize();
	}

	//DUMMY DATA FROM HERE ON
	if ($rootScope.isDemo) $timeout(function(){
   console.log("Setting demo data");
	 //The translation table for users		
	  self.addUser(1 , {
		   name : "S.J.Hoeksma"
			,gender: null //"m"
			,group : null// "ASP"
			});
		self.addUser(	2 , {
			 group : null//"ASP"
			 ,name : "J.R.Hoeksma"
			 ,gender: null //"m"
			});
		self.addUser(	3 , {
			 group :null// "ASP"
			 ,name : "T.M.Hoeksma"
			 ,gender: null// "m"
			});
   self.addUser(  739 , {
			 group :null// "SCH"
			,name : "Wat"
			,gender: null //"f"
			});
   self.addUser(  787 , {
			group : null//"ASP"
			,name : "Iets anders"
			,gender: null// "f"
			});
		
		self.add({
			  id : 2
			  ,start: 14530
			  ,time: 31320
			  ,state: MII_STATE_FINISH
			},true);
		self.add({
			  id : 2
			  ,start: 13530
			  ,time: 30029
			  ,state: MII_STATE_DSQ
			},true);

		self.add({
			  id : 739
			  ,start: 15530
			  ,time: 19320
			  ,state: MII_STATE_FINISH
			},true);
	
		self.add({
			  id : 787
			 ,start: 17530
			  ,time: 18421
			  ,state: MII_STATE_FINISH
	},true);
	self.add({
			  id : 787
			  ,start: 17531
			  ,time: 18441
			  ,state: MII_STATE_FINISH
	},true);
	self.add({
			  id : 1
			  ,start: 13730
			  ,time: 29317
			  ,state: MII_STATE_FINISH
			},true);
	self.add({
			  id : 3
			  ,start: 15230
			  ,time: 29317
			  ,state: MII_STATE_FINISH
			},true);
	self.add({
			  id : 3
			  ,start: 3730
			  ,time: 39317
			  ,state: MII_STATE_FINISH
			},true);
	self.add({
			  id : 2
			  ,start: self.getTime()
			  ,time: 7317
			  ,state: 10
			},true); 
	 self.updateRanking(); //Last one update rakning
	 self.saveSessionResults();
	},300);
	return self; 
})


;