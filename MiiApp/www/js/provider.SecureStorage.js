'use strict';
angular.module('provider.SecureStorage', [
	'LocalForageModule'  //Localforage provider
])

.provider('$secureStorage',
['$controllerProvider', '$provide', '$compileProvider', '$filterProvider','$stateProvider',
function ($controllerProvider, $provide, $compileProvider, $filterProvider,$stateProvider) {
   var providers = {
				$controllerProvider: $controllerProvider,
				$compileProvider: $compileProvider,
				$filterProvider: $filterProvider,
				$provide: $provide, // other things
	      $stateProvider:$stateProvider
		},
	 native=0, 
	 instances={},
	 ciphers={},
	 dataCache={},
	 isolate=false,
	 baseConfig={
		 cipher:"WillBeChanged"
	 };
	 //We use var instead of const not liked by all browsers
	 var SecureStorage="$SecureStorage";
	 var BLOB = 'blob';
	 
	
   this.$get= ['$q', '$rootScope','$ionicPlatform','$localForage','ENV','$timeout','$http'
			, function ($q, $rootScope, $ionicPlatform,$localForage,ENV,$timeout,$http) { 	

		 function getInstance(name,config,createCipher){
			 var defer = $q.defer();
			 if (typeof createCipher == "undefined") createCipher=true;
			 if (name==null) name="$session";
	//		 console.log("GetInstance",name);
			 if (instances.hasOwnProperty(name)){
				 if (!ciphers.hasOwnProperty(name)) 
					 self.setCipher(name,isolate==false ? baseConfig.cipher : isolate);
				 defer.resolve(instances[name]);
			 } else { //We will have to create the instance
				 if (name=="$session"){ //Special version the session storage
		      localforage.defineDriver(window.sessionStorageWrapper).then(function() {
						 self._native=false;
	      		 var myConfig=angular.extend({},config,baseConfig,{name:name,driver:'sessionStorageWrapper'});
						 if (createCipher && !ciphers.hasOwnProperty(name)) 
							   self.setCipher(name,myConfig.cipher);
						 delete myConfig.cipher; //We don't want cipher stored in config 
						 try {
						   instances[name]=$localForage.createInstance(myConfig);
						 } catch(ex){
							 //Trick to catch double creates
							 if (!instances[name]) console.log("Failed to create instance",ex);
						 }
						 defer.resolve(instances[name]);
					},function(ex){
						console.err("Failed to load sessionstorage",ex);
					});
					} else {
					 var myConfig=angular.extend({},baseConfig,{name:name},isolate==false ? {} : {cipher:isolate},config);
					 if ((createCipher || isolate!=false) && !ciphers.hasOwnProperty(name))
						 self.setCipher(name,myConfig.cipher);
					 delete myConfig.cipher; //We don't want cipher stored in config 
					 instances[name]=$localForage.createInstance(myConfig);
					 defer.resolve(instances[name]);
					 //Keep listing of instance created by this Secure Storage with a isolate
					 if (name!=SecureStorage && isolate!=false && !(ciphers.hasOwnProperty(name) && ciphers[name].isolate==false)){
						 self.get(SecureStorage,name,{}).then(function(data){
							 var obj=angular.extend({},data);
							 obj.isolate=true;
							 self.set(SecureStorage,name,obj);
					 	 },function(){
						    self.set(SecureStorage,name,{isolate:true});	 
						 });
					 }
						 
				 }
			 } 
			 return defer.promise;
		 }
				
		 var queuePop = function(instanceName,key,callbackPromise){
			 //console.log("QueuPop",instanceName,key);
			 var defer = $q.defer();
			 self.get(instanceName,key).then(function(value){
				 if (typeof callbackPromise =="function"){
					 $q.when(callbackPromise(value)).then(function(){
						 self.remove(instanceName,key).then(function(){defer.resolve(value)},defer.reject);
					 },defer.reject);
				 } else { 
					 self.remove(instanceName,key).then(function(){defer.resolve(value)},defer.reject);
				 }
			 },defer.reject);
			 return defer.promise;
		 }		
		
		 //Use to store the last time key
		 var lastTimeKey =0;		
				
		 var self = {
			 _native:native
			 ,clearCache:function(instanceName){
			   if (typeof instanceName=="undefined") dataCache={};
				 else delete dataCache[instanceName];
			 }
			 ,UUID: function() {
  			 var fmt = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx';
				 return fmt.replace(/[xy]/g, function(c) {
						var r = Math.random()*16|0, v = c === 'x' ? r : (r&0x3|0x8);
						return v.toString(16);
			 	});
			 }
			 ,sysUUID:function(){
				  var defer = $q.defer();
					if (window.device && window.device.uuid){
							defer.resolve(window.device.uuid);
					} else {
						self.get(ENV.appName,'UUID',null).then(function (value){
							 if (value==null) {
								 value=self.UUID();
								 self.set(ENV.appName,'UUID',value);
					 		 }
							 defer.resolve(value);
						},defer.reject);
					}
				 return defer.promise;
			 }
			 ,isolate:function(cipher){ //set The isolate cipher
				 for (var key in ciphers){
					 if (ciphers[key].isolate){
						 delete ciphers[key];
						// console.log("Removed cipher for ",key);
					 }
				 }
				 if (cipher){
					 isolate=cipher; //Set the new isolation key
					 console.log("Data isolation set",cipher);
				 } else if (isolate!==false) {
					 console.log("Data isolation removed");
					 isolate=false;
				 }
			 }
			 ,setCipher:function(instanceName,cipher,forceIsolate){
				 if (instanceName==null) instanceName="$session";
				// console.log("Set Chiper",instanceName,cipher);
				 ciphers[instanceName]={cipher:cipher || baseConfig.cipher,isolate:forceIsolate==false ? false : isolate!==false};
			 }
			 ,createInstance:function(instanceName,config){
				 if (instanceName==null) instanceName="$session";
				 return getInstance(instanceName,config);
			 }
			 
			 ,createQueue:function(instanceName,config){ //Queue is instancename staring with #
				 return getInstance('#'+instanceName,config);
			 }
			 ,iterate:function(instanceName,callback){
				 var defer = $q.defer();
				 getInstance(instanceName).then(function (instance){
					   instance.iterate(function(value, key, iterationNumber){
							 //We need to decrypt the value, before calling the 
							 try { 
							   callback(angular.fromJson(CryptoJS.AES.decrypt(value, ciphers[instanceName].cipher).toString(CryptoJS.enc.Utf8)),key,iterationNumber);
							 } catch (ex){
								 console.warn("Failed to iterate ", key, value);
								 defer.reject();
							 }
						 }).then(defer.resolve,defer.reject);
				 },defer.reject);
				 return defer.promise;
			 }
			 ,keys:function(instanceName,callback){
				 var defer = $q.defer();
				 getInstance(instanceName).then(function (instance){
					   instance.keys().then(function(keys){
							 if (callback) callback(keys);
							 defer.resolve(keys);
						 },defer.reject);
				 },defer.reject);
				 return defer.promise;
			 }
			 
       ,get: function(instanceName,key,defaultValue){
			   var defer = $q.defer();
				 if (instanceName==null) instanceName="$session";
				 //Check if item is in cache
				 if (dataCache.hasOwnProperty(instanceName) && angular.isArray(dataCache[instanceName]) && key==BLOB){ 
				 	 defer.resolve(dataCache[instanceName]||defaultValue);
				 }else if (dataCache.hasOwnProperty(instanceName) && !angular.isArray(dataCache[instanceName])){ 
				 	 defer.resolve(dataCache[instanceName][key]||defaultValue);
				 } else {
				  getInstance(instanceName).then(function(instance){
					 if (!key) defer.reject();
					 else {
						 instance.getItem(key).then(function(value){
							 try {
					    	 if (value==null) defer.resolve(defaultValue);
								 else {
					//				console.log("Get Encrypt",instanceName,ciphers[instanceName].cipher,key,value, CryptoJS.AES.decrypt(value, ciphers[instanceName].cipher).toString(CryptoJS.enc.Utf8));
								  defer.resolve(angular.fromJson(CryptoJS.AES.decrypt(value, ciphers[instanceName].cipher).toString(CryptoJS.enc.Utf8)));
								 }
							 } catch (ex){
								 console.warn("Failed get",instanceName,ciphers[instanceName].cipher,key,ex);
								 if (typeof defaultValue === "undefined") defer.reject();
								 else defer.resolve(defaultValue);
							 }
						 },defer.reject);
					 }
				  },defer.reject);
				 }
				 return defer.promise;
		   }
			 
			 ,set:function(instanceName,key,value){
				 var defer = $q.defer();
				 if (instanceName==null) instanceName="$session";
				 getInstance(instanceName).then(function(instance){
					 if (!key) defer.reject();
					 else {
			//			 console.log("Set Encrypt",instanceName,ciphers[instanceName].cipher,key,value, CryptoJS.AES.encrypt(angular.toJson(value), ciphers[instanceName].cipher).toString());
						 instance.setItem(key,
             CryptoJS.AES.encrypt(angular.toJson(value),  ciphers[instanceName].cipher).toString())
							 .then(defer.resolve,defer.reject);
						 //Update cache if needed
						 if (dataCache.hasOwnProperty(instanceName)){
								if (angular.isArray(dataCache[instanceName])){
									if (key==BLOB) dataCache[instanceName]=value
									else delete dataCache[instanceName];
								} else {
									dataCache[instanceName][key]=value;
								}		
							}
					 }
				 },defer.reject); 
				 return defer.promise;
			 }
			 ,clearQueue:function(instanceName){
				 return self.clear('#'+instanceName);
			 }
			 ,push:function(instanceName,value){
				 instanceName='#'+instanceName;
				 var key = (new Date()).getTime();
				 if (key==lastTimeKey) key++;
				 lastTimeKey=key;
				 return self.set(instanceName,key,value);
			 }
			 ,pop:function(instanceName,callbackPromise){
				 instanceName='#'+instanceName;
				 var defer = $q.defer();
				 getInstance(instanceName).then(function(instance){
						 //Get the keys from the instance
						 instance.keys().then(function(keys){
							 if (keys.length==0) defer.reject();
							 else {
								 //Find the lowest key
								 var key=parseFloat(keys[0]);
								 for (var i=1,l=keys.length;i<l;i++) key=Math.min(parseFloat(keys[i]),key);
								 queuePop(instanceName,key,callbackPromise).then(defer.resolve,defer.reject);
							}
						 },defer.reject);
				 },defer.reject);
				 return defer.promise;
			 }
			 ,popAll:function(instanceName,callbackPromise){
				 instanceName='#'+instanceName;
				 var defer = $q.defer();
				 getInstance(instanceName).then(function(instance){
						 //Get the keys from the instance
						 instance.keys().then(function(keys){
							 if (keys.length==0) defer.reject();
							 else {
								 var sKeys=[];
								 for (var i=0,l=keys.length;i<l;i++) sKeys.push(parseFloat(keys[i]));
								 sKeys.sort(function(a,b){return a-b});
								 var promise = queuePop(instanceName,sKeys[0],callbackPromise);
								 var j=1;
								 for (var i=0,l=sKeys.length;i<l;i++){
		            		promise=promise.then(function(){
											if (j>=sKeys.length) {
												defer.resolve();
												return $q.resolve();
											} else {
											  return queuePop(instanceName,sKeys[j++],callbackPromise);
											}
										},function(){
											j=sKeys.length;
											defer.reject()
										});						 
								 }
							}
						 },defer.reject);
				 },defer.reject);
				 return defer.promise;
			 }
			 ,depth:function(instanceName){
				 instanceName='#'+instanceName;
				 var defer = $q.defer();
				 getInstance(instanceName).then(function(instance){
						 instance.length().then(defer.resolve,defer.reject);
				 },defer.reject);
				 return defer.promise;
			 }
			 ,remove:function(instanceName,key){
			   var defer = $q.defer();
				 if (instanceName==null) instanceName="$session";
				 getInstance(instanceName).then(function(instance){
				//		console.log("Removing SecureStorage item",instanceName,key); 
					  instance.removeItem(key).then(defer.resolve,defer.reject);
						//Update cache if needed
						if (dataCache.hasOwnProperty(instanceName)){
							if (angular.isArray(dataCache[instanceName])){
								delete dataCache[instanceName];
							} else {
								delete dataCache[instanceName][key];
							}		
						}
				 },defer.reject);
				 return defer.promise;
		   }
			 
			 ,clear:function(instanceName){
				 var defer = $q.defer();
				 if (instanceName==null) instanceName="$session";
				 self.remove(SecureStorage,instanceName);
				 getInstance(instanceName,null,false).then(function(instance){
					  instance.clear().then(defer.resolve,defer.reject);
						console.log("SecureStorage cleared ",instanceName);
					 delete dataCache[instanceName]; //Remove the item from the cache
				 });
				 return defer.promise;
			 }
			 
			 ,wipe:function(){
			   var defer = $q.defer();
				 self.clearCache();
					 getInstance(SecureStorage,null,false).then(function (instance){
					   instance.keys().then(function(keys){
							 if (keys.length==0){
								 defer.reject();
							 } else {
								 var j=0;
							   var promise = self.clear(keys[j++]);
								 for (var i=0,l=keys.length;i<l;i++){
									promise=promise.then(function(){
										if (j>=keys.length) {
											defer.resolve();
											return $q.resolve();
										} else {
											return self.clear(keys[j++]);
										}
									},function(){
										j=keys.length;
										defer.reject()
									});		
								 }
							 }
					   },defer.reject);
				   },defer.reject);
				return defer.promise;
		   }
			 
			 ,load:function(instanceName,loadConfig){ //Load the data from a instance into a object
				 var defer = $q.defer();
				 if ($rootScope.incProgress) $rootScope.incProgress();
				 if (!loadConfig)loadConfig={toArray:false,cache:false,block:-1};
				 var toArray = Boolean.fromString(loadConfig.toArray,false);
				 var loadData = true;
				 var block = loadConfig.block || -1;
				 if (Boolean.fromString(loadConfig.cache,false)){
					 //Check if we have the data for this element in cache
					 if (dataCache.hasOwnProperty(instanceName)){
						 if (toArray && angular.isArray(dataCache[instanceName])) {
							 loadData=false;
							 defer.resolve(dataCache[instanceName]);
						 } else if (!toArray && !angular.isArray(dataCache[instanceName])) {
							 loadData=false;
							 defer.resolve(dataCache[instanceName]);
						 }
						 if (!loadData) {
							 console.log("Loading from cache ",instanceName );
						 }
					 } 
				 }
				 
         if (loadData && Boolean.fromString(loadConfig.blob,false)) {
						self.get(instanceName,BLOB,toArray ? [] : {}).then(function(data){
							 defer.resolve(data);
							 if (Boolean.fromString(loadConfig.cache,false)){
								 dataCache[instanceName]=data;
							 }
						},defer.reject);
		      } else if (loadData){	 
					 getInstance(instanceName).then(function (instance){			
						 instance.keys().then(function(keys){
							 if (keys.length==0) defer.resolve(data);
							 else {
								 var promise = self.get(instanceName,keys[0]);
								 var j=1;
								 for (var i=0,l=keys.length;i<l;i++){
									 promise=promise.then(function(val){
										 if (j>=keys.length) {
											  if (toArray) data.push(val); else data[keys[j-1]]=val;
										    if (Boolean.fromString(loadConfig.cache,false)){
								           dataCache[instanceName]=data;
							          }
												defer.resolve(data);
												return $q.resolve();
											} else {
										   if (toArray) data.push(val); else data[keys[j-1]]=val;
										   if (block==j) defer.resolve(data);
												return self.get(instanceName,keys[j++]);
											}
									 },defer.reject);
								 }
							  }
						 },defer.reject)
				  },defer.reject);
				 }
				 defer.promise.finally($rootScope.decProgress);
				 return defer.promise;
			 }
			 ,resetSince : function(instanceName){
				  var defer = $q.defer();
				  self.get(SecureStorage,instanceName,{}).then(function(instanceData){		
					 delete instanceData.lastSync;
					 self.set(SecureStorage,instanceName,instanceData).then(defer.resolve,defer.reject);
					},defer.reject);
				  return defer.promise;
			 }
			 
			 ,remoteLoad:function(instanceName,loadConfig){ //Load the data from remote location
				 var defer = $q.defer();
				 if ($rootScope.incProgress) $rootScope.incProgress();
				 var lastSyncDate = (new Date().getTime());
				 var url = loadConfig.url ? loadConfig.url : 
				   (ENV.apiURL + (loadConfig.version ? loadConfig.version : 'v1')+ '/' +
						 (loadConfig.apiName ? loadConfig.apiName : instanceName));
				 var idTag = loadConfig.idTag ? loadConfig.idTag : 'id';
				 var sinceTag = loadConfig.sinceTag ? loadConfig.sinceTag : 'since';
				 var delta = Boolean.fromString(loadConfig.delta,false);
				 var headers = {
					 Authorization: (loadConfig.hasOwnProperty('prefixToken') ? loadConfig.prefixToken : 
							(ENV.prefixToken || '')) + (loadConfig.apiToken ||  ENV.karmaToken || $rootScope.apiToken ) 
				 }
				 if (loadConfig.hasOwnProperty('contentType')) headers['Content-Type']= loadConfig.contentType;
				 self.get(SecureStorage,instanceName,{}).then(function(instanceData){		
					 if (delta && instanceData.lastSync){
						 url+="?since="+instanceData.lastSync;
					 } else {
						 delta = false;
					 }
					 console.log("Loading data",loadConfig.apiToken ? "Config" : ENV.karmaToken ? "Karma" : 'API',url,headers,instanceData);
					 ($rootScope.http || $http)({
							method: 'GET',
							url: url,
							headers: headers,
						  timeout: loadConfig.timeout || 600000 
						}).then(function successCallback(response) {
							getInstance(instanceName).then(function (instance){
									//Sync data in localforage
									var syncData = function(){
									 if (angular.isArray(response.data)){ //We expect a array
										  for (var i=0,l=response.data.length;i<l;i++){
											 var value=response.data[i];
											 if ((value.hasOwnProperty(sinceTag) && value[sinceTag]>=0) || !value.hasOwnProperty(sinceTag)){
													delete value[sinceTag]; //Remove the sinceTag
													var key= value.hasOwnProperty(idTag) ? value[idTag] : self.UUID();
													instance.setItem(key,CryptoJS.AES.encrypt(angular.toJson(value),  ciphers[instanceName].cipher).toString())	
												} else if (value.hasOwnProperty(sinceTag) && value[sinceTag]<0 && value.hasOwnProperty(idTag)){
													instance.removeItem(value[idTag]);
												}
										 }
										 self.set(SecureStorage,instanceName,angular.extend({},instanceData,{lastSync:lastSyncDate})).then(defer.resolve,defer.reject);
										 
									 } else defer.reject();
								 };	
								
								 if (Boolean.fromString(loadConfig.blob,false)){
								//	 console.log("BLOB",instanceName,response.data);
									 if (delta) {
										 console.log("Delta Start",instanceName);
										 //First load all data into array
										 self.get(instanceName,BLOB,[]).then(function(data){
											 var oData = {};
											 //Convert array into object
											 angular.forEach(data,function(value,key){oData[value[idTag]]=value;})
											 //Loop the recieved data
											 for (var i=0,l=response.data.length;i<l;i++){
												 var rec = response.data[i];
												 var id = rec[idTag];
												 if (oData.hasOwnProperty(id)){
													 if (rec[sinceTag]<0) delete oData[id];
													 else oData[id]=rec;
												 } else if (rec[sinceTag]>=0) {
													 oData[id]=rec; //Add Record
												 }
											 }
										   //Now we convert the oData back to data
										   data=[];
											 angular.forEach(oData,function(value){data.push(value)});
										   //Store the data
										   self.clearCache(instanceName);
											
											 self.set(instanceName,BLOB,data).then(function(){
												 if (Boolean.fromString(loadConfig.cache,false)) 
											    dataCache[instanceName]=data;
												self.set(SecureStorage,instanceName,angular.extend({},instanceData,{lastSync:lastSyncDate})).then(defer.resolve,defer.reject);  
												 console.log("Delta finish",response.data.length,data.length); 
											 },defer.reject);
										 },defer.reject);
									 } else {
										  self.clearCache(instanceName);
										  self.set(instanceName,BLOB,response.data).then(function(){
										    if (Boolean.fromString(loadConfig.cache,false)) 
											    dataCache[instanceName]=response.data;
												self.set(SecureStorage,instanceName,angular.extend({},instanceData,{lastSync:lastSyncDate})).then(defer.resolve,defer.reject);
												console.log("Full finish",response.data.length);
											},defer.reject);
									 }
									
							   } else if (delta) { //We run in delta mode, add the since tag
									 syncData();
								 } else { //Full sync, clear old data first
										instance.clear().then(function (){
											syncData();
										},defer.reject);
								 }
							},defer.reject);
						}, function errorCallback(response) {
							console.error("LoadRemote failed",angular.toJson(response));
							defer.reject();
					  });
					},defer.reject);
				 defer.promise.finally($rootScope.decProgress);
				 return defer.promise;
			 }
		 
		 };

		 //Create the session storage
		 self.createInstance(null); 
							 
		 //When ready we can check if cordova plugins are loaded
		 $ionicPlatform.ready(function() {
			  //Create the internal SecureStorage store
		    self.createInstance(SecureStorage); 
							 
				//Check if we have a UUID if so we change basic cipher to value based on UUID
				self.sysUUID().then(function(uuid){
					 baseConfig.cipher=CryptoJS.SHA256(baseConfig.cipher+uuid).toString(); 
					 console.log("Setting base cipher",uuid,baseConfig.cipher);
				});
		 });

		 return self;

		}];
	
		this.config = function (_config) {
			 baseConfig=angular.extend({},_config);
		};
}])
;