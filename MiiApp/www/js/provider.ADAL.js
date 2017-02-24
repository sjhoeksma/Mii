/*
TODO:
On native device we have to keep token valid also in background
https://github.com/transistorsoft/cordova-plugin-background-fetch
http://www.gajotres.net/prevent-ionic-application-from-going-to-sleep-in-background
https://github.com/katzer/cordova-plugin-background-mode
*/


'use strict';
angular.module('provider.ADAL',[	
])
  .provider('$adal', function (ENV,$httpProvider) {
	
	var webConfig =null,nativeConfig=null,
			startUpHash=null,authContextAdalJs=null;
	var httpProvider = $httpProvider;
	
	this.$get= ['$q', '$rootScope','ENV','$http','$timeout','$secureStorage'
			, function ($q, $rootScope,ENV,$http,$timeout,$secureStorage) { 	

	  var mobileDevice =window.cordova && nativeConfig ? true : false; //TODO: ADALJS implementation
		var disabledUser = {
			  tokens:{
				 id: ENV.userEmail || "Default",
				 email: ENV.userEmail || "Default"
				},
			  pin: ENV.demoPin || '0000',
			  accessToken: ENV.apiToken || "default"	
	
		};
		var adalStore = 'ADAL';
	  var adalCache = {};			
		$secureStorage.sysUUID().then(function(uuid){	
			var cipher;
			if (webConfig)
			  cipher = CryptoJS.SHA256(webConfig  ? webConfig.appId+uuid+webConfig.clientId
																: ENV.appName + ENV.appOwner).toString();
	    $secureStorage.setCipher(adalStore,cipher,false);
		});
		
		//We Overwrite the adaljs functions or reading and writing data to store
		AuthenticationContext.prototype._saveItem = function (key, obj) {
			  adalCache[key]=obj;
	      $secureStorage.set(adalStore,key, obj);
        return true;
    };				
		AuthenticationContext.prototype.clearCache = function(){
			console.log("Clear ADAL cache");
			authContextAdalJs=null; //Remove the instance (incl cache)
			adalCache = {};
		  return $secureStorage.clear(adalStore); //Clear the adal store
		}		
    AuthenticationContext.prototype._getItem = function (key) {
		 //We use a internal cache object, because $secureStorage uses promise	
		 //console.log("Retreiving",key,adalCache[key]);	
		 return adalCache[key]; //result;
    };	
				
				
		var decodeAdfsJwt = function (authResponse) {
			var defer = $q.defer();
			var jwt =  authResponse.accessToken;
			//console.log('decoding...', jwt);
			var body = jwt.substring(jwt.indexOf('.') + 1, jwt.lastIndexOf('.')); 
			var content = CryptoJS.enc.Base64.parse(body);
			var string = CryptoJS.enc.Utf8.stringify(content);
			console.log("Token As JSON",angular.fromJson(string));
			defer.resolve(angular.extend({tokens:angular.fromJson(string)},authResponse));
			return defer.promise;
		};
				

		//Get the Authentication Context
		var getAuthContext = function(config,onADFS){
			var defer = $q.defer();
		  ionic.Platform.ready(function () {
				if (mobileDevice){
					var authority = config.authority;
					var authContext = new Microsoft.ADAL.AuthenticationContext(authority,config.serverType=="adfs" ? false : true);
		  		authContext.tokenCache.readItems().then(function (items) {
						if (items.length > 0) {
							if (items[0].authority!=authority){
							  authority = items[0].authority;
							  authContext = new Microsoft.ADAL.AuthenticationContext(authority);
							}
						}
						defer.resolve(authContext);
					},function(repsonse){defer.reject({response:response,authContext:authContext})});
				} else { //ADAL JS
					//TODO CHeck: We need to keep the object or loose loaded info
					if (!authContextAdalJs) {
						$secureStorage.load(adalStore).then(function(data){
				      adalCache = data || {};
							//console.log("Loaded adal",startUpHash,angular.toJson(adalCache));
							authContextAdalJs = new AuthenticationContext(config);
							//Store token in AdalJS user storage
					    if (startUpHash && startUpHash.indexOf('#'+authContextAdalJs.CONSTANTS.ID_TOKEN)==0) {
								authContextAdalJs.saveTokenFromHash(authContextAdalJs.getRequestInfo(startUpHash));
					   	  adalCache[authContextAdalJs.CONSTANTS.STORAGE.IDTOKEN]=
								    startUpHash.substr(('#'+authContextAdalJs.CONSTANTS.ID_TOKEN).length);
								$secureStorage.set(adalStore,authContextAdalJs.CONSTANTS.STORAGE.IDTOKEN,
																	 adalCache[authContextAdalJs.CONSTANTS.STORAGE.IDTOKEN]);
							} else startUpHash=null;
			 			  defer.resolve(authContextAdalJs);
						},defer.reject);
				  } else{
						defer.resolve(authContextAdalJs);
					}
			   }
			});
			return defer.promise;																				
		}
		
	 //Run a authentication	
	 var authenticate = function (config) {
		  var defer = $q.defer();
		 	 if (mobileDevice) {
				getAuthContext(config).then(function(authContext){
					// attempt to authorize user silently
					authContext.acquireTokenSilentAsync(config.resourceUrl,config.appId)
						.then(defer.resolve, function (response) {
							// we are not logged in
							defer.reject({response:response,authContext:authContext});
						});
				},defer.reject);
			} else {
				 getAuthContext(webConfig).then(function(authContext){
					  var res =authContext._extractIdToken(
								 adalCache[authContext.CONSTANTS.STORAGE.IDTOKEN]);
					 	if (res) {
					    res.accessToken=adalCache[authContext.CONSTANTS.STORAGE.IDTOKEN];
							defer.resolve(res);
						} else { 
		          defer.reject();          
						}
				 },defer.reject);
			}
		  return defer.promise;
		};	
						
		
		var self = {
		 //Refresh the token on background
		 refresh :  function () {
		  if (!webConfig) return $q.resolve(disabledUser); 
			var defer = $q.defer();
			if (!mobileDevice){
				 getAuthContext(webConfig).then(function(authContext){
					  var res = authContext._extractIdToken(
								 adalCache[authContext.CONSTANTS.STORAGE.IDTOKEN]);
					 if (res) {
			   	   res.accessToken=adalCache[authContext.CONSTANTS.STORAGE.IDTOKEN];
						 defer.resolve(res);
						} else { 
							defer.reject();
						}
					 /*
					 authContext.acquireToken(authContext.config.loginResource,function(resource,token,error){
					  if (token){
							var user = authContext.getCachedUser();
							if (user) {
								defer.resolve(user);
							} else { 
								defer.reject();
							}
						} else defer.reject();
						
			    });*/
				 },defer.reject);
			} else {
				authenticate(nativeConfig). then(function(authResponse){
					// get user info from adfs token
					if (nativeConfig.serverType=="adfs"){
						decodeAdfsJwt(authResponse).then(function(data) {
							defer.resolve(data); // user for ADFS
						});
					} else {
						defer.resolve(authResponse)
					}
					$rootScope.$broadcast("adal.valid",authResponse);
				}, function (err) {
					console.log('Failed to authenticate: ' + err.response);
				  defer.reject(err);
				});
			}
		  return defer.promise;
		 }
			
		 ,login:function(){
			 if (!webConfig) return $q.resolve(disabledUser); 
			 var defer = $q.defer();
			 if (!mobileDevice){
				 authenticate(webConfig).then(function(data){
					 console.log("Valid data",data);
					 defer.resolve(data);
				 },function(){ //Failed
					 getAuthContext(webConfig).then(function(authContext){
						//if (webConfig.popUp){
							// that.handleWindowCallback(popupWindow.location.hash);
							//authContext.handleWindowCallback
						//} else {
								// Check Login Status, Update UI
						  
							var res = authContext._extractIdToken(
								 adalCache[authContext.CONSTANTS.STORAGE.IDTOKEN]);
						 if (res) {
							  res.accessToken=adalCache[authContext.CONSTANTS.STORAGE.IDTOKEN];
								defer.resolve(res);
							} else {
	//							 defer.reject();
								 authContext.login(); //This will start rediect forms
							}
					//	}
           
					 },defer.reject);
				 });
			 } else {//Login n
				 //Check if we can do a silant authentication
				 authenticate(nativeConfig).then(
				  function(data){
						defer.resolve(data);
					}, function(){
				   getAuthContext(nativeConfig).then(function(authContext) {
						authContext.acquireTokenAsync(nativeConfig.resourceUrl,nativeConfig.appId, nativeConfig.redirectUrl)
							.then(function(authResponse){
									// get user info from adfs token
							   authResponse['serverType']=nativeConfig.serverType;
								 if (nativeConfig.serverType=="adfs"){
									 decodeAdfsJwt(authResponse).then(function(data) {
										 defer.resolve(data); // user for ADFS
									 }); 
								 } else defer.resolve(authResponse);
							   $rootScope.$broadcast("adal.valid",authResponse);
						 },function(ms){
							if (ms=="Error: The operation was cancelled."){
								//We found out that we sometimes get error but second time is ok
								$timeout(function(){
									authContext.acquireTokenAsync(nativeConfig.resourceUrl,nativeConfig.appId, nativeConfig.redirectUrl)
									.then(function(authResponse){
											// get user info from adfs token
										 authResponse['serverType']=nativeConfig.serverType;
										 if (nativeConfig.serverType=="adfs"){
											 decodeAdfsJwt(authResponse).then(function(data) {
												 defer.resolve(data); // user for ADFS
											 }); 
										 } else defer.resolve(authResponse);
										 $rootScope.$broadcast("adal.valid",authResponse);
									},defer.reject);
								},250);
							} else {
								console.error("Adal error",ms.toString());
								defer.reject(ms);
							}
						});
				  },defer.reject);
				 });
			 }
	     return defer.promise;
		 }	
		 
		,logout:function(noEvent){
			if (!webConfig) return $q.resolve();
			var defer = $q.defer();
			var makeUrl = function(config){
				var url;
				if (config.serverType=="adfs"){
					url = (config.authority || (config.instance + config.tenant + '/')) + 'ls/?wa=wsignout1.0';
					if (config.redirectUri) {
						url+='&wreply={'+ encodeURIComponent(config.redirectUri) + '}';
					}
				} else { 
					url = (config.authority || (config.instance + config.tenant + '/')) + 'oauth2/logout';
					if (config.redirectUri) {
						 url+='?post_logout_redirect_uri=' + encodeURIComponent(config.redirectUri);
					}
				}
				return url;
			}
			var callLogout = function(config,authContext){
				$http({
					method: 'GET', //POST returns whole webpage
					timeout: 2500, //We set timeout of 2.5 seconds
					url: makeUrl(config)
				}).then(function successCallback(response) {
           //alert("OK"+angular.toJson(response));
					 defer.resolve(authContext);
					 if (!noEvent) $rootScope.$broadcast("adal.logout");	
					 console.log("Logout success",response);	
					 
				}, function errorCallback(response) {
					//alert("FAIL"+angular.toJson(response));
					defer.reject({response:response,authContext:authContext});
					if (!noEvent) $rootScope.$broadcast("adal.logout");	
					console.log("Logout failed",angular.toJson(response));
				});
				// Does also not remove settings
				
				/*
				// Use a hidden inappbrowser view to navigate to the logout url
        var ref = window.open(makeUrl(config), '_blank', 'hidden=yes');
        ref.addEventListener('loadstop', function(){
				  ref.close();
				  defer.resolve(authContext);
					if (!noEvent) $rootScope.$broadcast("adal.logout");	
					console.log("Logout success",response);
				});
				*/
				
			}
			if (!mobileDevice) {
				getAuthContext(webConfig).then(function (authContext) {
				//Prevent logging out if no user info or startup login	
				 if (!startUpHash && authContext.getCachedUser()){ 
					 authContext.clearCache().finally(function(){
						 startUpHash=null;
						 window.location.replace(makeUrl(webConfig));
					});
					
				 } else defer.resolve(authContext);
				},defer.reject);
			} else {
				getAuthContext(nativeConfig).then(function (authContext) {
					authContext.tokenCache.clear().then(function (unset) {
					callLogout(nativeConfig,authContext);
				  },defer.reject);
			  },defer.reject);
		  }
		 return defer.promise;
		}
 	 }
	 return self;
	
	}]
	
	this.config = function (_webConfig,_nativeConfig) {
		if (_webConfig) {
			webConfig=angular.extend({},_webConfig);
			if (!webConfig.hasOwnProperty('redirectUri')){
				webConfig['redirectUri']= window.location.origin +  window.location.pathname;
			}
			if (_nativeConfig) nativeConfig=angular.extend({},_webConfig,_nativeConfig);
			startUpHash=window.location.hash;
		}
	}
    
})

;