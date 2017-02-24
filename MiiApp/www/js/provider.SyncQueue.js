'use strict';
angular.module('provider.SyncQueue',[	
	'provider.SecureStorage' //We required Secure Storage to save the data
])
 .provider('$syncQueue', function (ENV,$httpProvider) {
	
	var myConfig={name:'syncQueue',endpoints:{}},
	httpProvider = $httpProvider;
	
	function _configListener(config){
		if (config && config.hasOwnProperty('endpoints')){
			angular.forEach(config.endpoints,function(endpointConfig,endpointName){
				 myConfig.endpoints[endpointName]=endpointConfig;
				 console.log("SyncQueue added endpoint",endpointName)
			});
		}
	}

  this.$get=['$rootScope', '$q', '$log', '$timeout', '$secureStorage', '$http', '$interpolate'
			, function ($rootScope, $q, $log, $timeout, $secureStorage, $http,$interpolate) { 

    var inSync =false;
		var _backGround;
		var _interval=0;
    var self = {
			addEndpoint:function(endpointName,endpointConfig){
			 if (endpointName) myConfig.endpoints[endpointName]=endpointConfig;	
			}
			,push: function(item,endpointName,extra){
				var data = angular.extend({endpoint:endpointName||'default',_data:item},extra);
				var ret = $secureStorage.push(myConfig.name,data);
				if (Boolean(myConfig.syncOnPush,false)) {
					$timeout(self.sync,100); //Start the sync
				}
				return ret;
			}
			,remove:function(item,endpointName){
				//TODO: Remove the item from the queue
				console.error("SyncQueue.remove needs to be implemented",item,endpointName);
			}
			,sync: function(){
				var defer = $q.defer();
				if ($rootScope.incProgress) $rootScope.incProgress();
				
				var updateItem = function(item,instanceName,id,tag,value){
					 var defer = $q.defer();
					 if (instanceName && id && tag && (typeof value !="undefined") &&
						 item.hasOwnProperty(id) && item.hasOwnProperty(instanceName) ) {
						$secureStorage.get(item[instanceName],item[id],null).then(function(obj){ 
							var nested = tag.indexOf(':')>0 ? tag.split(":") : item[tag];
							var d = obj,i,l;
							for (i=0,l=nested.length-1;i<l && d;i++){
								d=d[nested[i]];
							}
							d[nested[i]]=value;
							//console.log("Updated item",item,tag,item[id],obj);
							$secureStorage.set(item[instanceName], item[id],obj).then(defer.resolve,defer.reject);
						},defer.reject);
					 } else defer.resolve(); //No action required
					return defer.promise;
				}
			  //Pop a item van stack
				if (inSync) {
					
					defer.reject();
				} else {
			  	//console.log("Doing Sync");
					inSync=true;
					var count =0;
					$secureStorage.popAll(myConfig.name,function(item){
						 //console.log("Pop Sync",item);
						 var def = $q.defer();
						 if (!item.endpoint) {
							 def.reject();
						 } else {
							 count++;
							 var endpointCfg = {
								 endpoint: item.endpoint,
								 toJson:true,method:'POST',
								 contentType:'application/json', 
								 idTag:'id',
								 storeTag:'store'};
							 if (typeof item.endpoint =="string" && myConfig.endpoints.hasOwnProperty(item.endpoint)){
									 endpointCfg=angular.extend(endpointCfg,myConfig.endpoints[item.endpoint]); 
							 }
							 var url = (endpointCfg.url ? endpointCfg.url : ENV.apiURL) +
									(endpointCfg.version ? endpointCfg.version : 'v1')+ '/' +
									(endpointCfg.apiName ? endpointCfg.apiName : item.endpoint);
							 var data=Boolean.fromString(endpointCfg.toJson,false) ? angular.toJson(item._data || item.data) : item._data || item.data;
							 if (endpointCfg.urlTemplate) 
								 url=$interpolate(endpointCfg.urlTemplate)(endpointCfg,item,$rootScope,this);
							 if (endpointCfg.dataTemplate) 
								 data=$interpolate(endpointCfg.dataTemplate)(endpointCfg ,item,$rootScope,this);
							 var headers = {
								 Authorization: (endpointCfg.hasOwnProperty('prefixToken') ? endpointCfg.prefixToken : (ENV.prefixToken || '')) + (endpointCfg.apiToken ||   ENV.karmaToken || $rootScope.apiToken ) 
							 }
							 if (endpointCfg.hasOwnProperty('contentType')) headers['Content-Type']= endpointCfg.contentType;
							 //console.log("SyncQeue sending",url,endpointCfg,item,data,headers);
							 console.log("SyncQeue sending",url,headers);
					
							 ($rootScope.http || $http)({
									method: endpointCfg.method,
									url: url,
									data: data,
									headers: headers
							 }).then(function successCallback(response) {
								  updateItem(item,endpointCfg.storeTag,endpointCfg.idTag,
														 endpointCfg.updateTag,endpointCfg.updateValue).then(def.resolve,
									function(){
											if (myConfig.ignoreError) def.resolve(null)
												else def.reject(response)
									});
							 }, function errorCallback(response) {
								  //Check if we should ignore message
								  if (response.status==myConfig.duplicateCode){
										updateItem(item,endpointCfg.storeTag,endpointCfg.idTag,
														 endpointCfg.updateTag,endpointCfg.updateValue);
										def.resolve(); //Ingnore the duplicate
									} else {
										 console.log(response.status+" Error",angular.toJson(response));
										 updateItem(item,endpointCfg.storeTag,endpointCfg.idTag,
														 endpointCfg.errorTag,endpointCfg.errorValue)
										if (myConfig.ignoreOnError) def.resolve(null)
										else def.reject(response);
									}
							 })
						 }
						 return def.promise;
					}).then(defer.resolve,defer.reject).finally(
						function(){
							inSync=false;
							if (count!=0) {
							 $rootScope.$broadcast('$syncQueue.done',count);
							}
					});
				}
				defer.promise.finally($rootScope.decProgress);
				return defer.promise;
		  }
			
		  ,backGround : function(interval){
			 var intervalSet = interval || myConfig.interval;
			 if (intervalSet && intervalSet!=_interval && interval!=0) {
				  if (_backGround) clearInterval(_backGround);
				  _interval = intervalSet;
					console.log("Starting background SyncQueue",intervalSet);
					_backGround = setInterval(function(){
						self.sync();
					},intervalSet);
				  self.sync();
				}	else if (interval==0){
					if (_backGround) {
						clearInterval(_backGround);
						console.log("Canceled background SyncQueue");
					}
					_backGround=null;
					_interval=0;
				}	
		  },
			configListener : _configListener
			
		};
		
		//If we have specific cipher key or driver we should create the storage before using it.
		if (myConfig.cipher || myConfig.driver)
			$secureStorage.createInstance(myConfig.name,myConfig);
				
		//Check if we should start a SyncQueue on interval
		console.log("SyncQueue",myConfig);
		
		return self;
	}];
						 
	this.config = function(_config) {
    myConfig=angular.extend(myConfig,_config);
	}
	this.configListener = _configListener;

  });