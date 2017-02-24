'use strict';
angular.module('service.AppUser', [])

//Login Service for Demo Resons
//The services we will use to validate login
.factory('AppUser', ['$q', '$rootScope', '$state', '$secureStorage',  '$filter', 'NetworkMonitor','ENV','$adal','$loadOnDemand','$timeout','$syncQueue','$translate','$ionicHistory'
	, function ($q, $rootScope, $state, $secureStorage, $filter, NetworkMonitor, ENV, $adal,$loadOnDemand,$timeout,$syncQueue,$translate,$ionicHistory) { 	
 
		//The user object
		var userObject = {
				id    :  $rootScope.isDemo ? "demo" : null,
				name  :  $rootScope.isDemo ? "demo" : null,
			  email :  $rootScope.isDemo ? "demo@demo.com" : null,
				avatar: ENV['defaultAvatar'],
				apps  : [], //The application list of user
			  loginState:0,
			  language  : null, //The user preffered landuage
			  theme     : null //Default system theme
		};
		var restoreLogin = function(defer){			
			userObject.loginState=1;
			$secureStorage.set(ENV.appName,'loginState',userObject.loginState);
		 //Restore the user apps
			$secureStorage.get('AppUser','appState',[]).then(function(value){
				userObject.apps=value;
				//Ensure the translations are loaded for the default apps
				angular.forEach(userObject.apps,function(appName){
						$loadOnDemand.loadLocales(appName);
				});
				//TODO: get external api about state of apps
				self.loadSecurityGroups().then(defer.resolve,defer.reject);
				$secureStorage.get('AppUser','apiToken',null).then(
					function(value){$rootScope.apiToken=value},
					function(){$rootScope.apiToken=null})
					.finally(function(){
						$syncQueue.backGround(); //Start the background job
						$rootScope.$broadcast('app.loggedin');
						console.log("Restore APP appState",value);
					});
				});
			//TODO RESTORE the ADFS token
		}
		var lastRes;
		
		var saveUserObj = function(){
			return $secureStorage.set(ENV.appName,'userinfo',{id:userObject.id,name:userObject.name,avatar:userObject.avatar,email:userObject.email,language:userObject.language,theme:userObject.theme});
		}
		
		var loadUserObj = function(){
			var defer = $q.defer();
			$secureStorage.get(ENV.appName,'userinfo',null).then(function(rec){
				if (rec!=null) 
					userObject=angular.merge(userObject,rec);
				  if (userObject.language){
						console.log("User language",userObject.language)
 					  $translate.use(userObject.language || $translate.preferredLanguage());
					}
				  if (userObject.theme){
						$rootScope.appTheme=userObject.theme;
					}
				  //Ensure the translations are loaded for the default apps
				  angular.forEach(userObject.apps,function(appName){
						$loadOnDemand.loadLocales(appName);
					});
				 defer.resolve(userObject);
				},function(){
				 if ($rootScope.isDemo){
					 defer.resolve(userObject);
				 } else {
					 defer.reject
				 }
			  });
			return defer.promise;
		}
		
		var makeHashKey = function(id,pin,uuid){
			//TODO: Change position of pin based on pin value
			var hashKey= CryptoJS.SHA256(id + pin + uuid).toString();
			console.log("NEW HASH",hashKey);
			return hashKey; 
		}
		
		var self = {
			loginState: function(){return userObject.loginState}
		  , lastUserAvatar: function(){return userObject.avatar}
      , lastUserName: function(){return userObject.name}
			, lastUserId: function(){return userObject.id}
			, lastUserEmail: function(){return userObject.email}
			, apps: function(){return userObject.apps}
			, hasUser: function(){return userObject.id!=null}
			, language: function(){return userObject.language}
			, setLanguage: function(uLang){
				if (userObject.language!=uLang){
					userObject.language=uLang;
					saveUserObj();
					$translate.use(userObject.language|| $translate.preferredLanguage()); // Sets the active language to
					$ionicHistory.clearCache();
					console.log("Language changed",uLang);
				}
			}
			,theme: function(){return userObject.theme}
			,setTheme:function(val){
				userObject.theme=val;
				saveUserObj();
				if ($rootScope.appTheme!=val) {
					$rootScope.appTheme=val;
			  	console.log("Theme set",val);
				}
			}
      , securityGroups: {}
		        
			,hasRole: function(groupName){
				if (!self.isLoggedIn()) return false;
        if (!groupName) return true;
        return groupName in self.securityGroups;
      }
			
      ,isValidState: function (state,notFoundValue){
		    if (typeof state == "undefined" || state=="" || state=="^") return true;
        var states = $state.get();
        for (var i=0;i<states.length;i++) {
          if (states[i].name==state) {
             //Check if there is authentication required for action
            if ( (!states[i].data && !self.isLoggedIn()) || (states[i].data && Boolean.fromString(states[i].data.auth,true) && !self.isLoggedIn()) ) {
              return false;    
            }
           //Check if the sevice is only available online
           if ((states[i].data && Boolean.fromString(states[i].data.online,false) && !$rootScope.isOnline)){
			        return false;
            }
		        //Check if the item is protected by roles
            if (states[i].data && states[i].data.security) {
		          return self.hasRole(states[i].data.security);
            } 
            //No more items to check so user is allowed to access
            return true;
          }
        }
				//TODO: State not found, check if state is in loaded apps
				
        return typeof notFoundValue == 'undefined' ? false : notFoundValue;  
				//State not found in possible states so it is invalid
      }
			, installApp:function(appName){
				if (self.hasApp(appName)) return ;
				userObject.apps.push(appName);
				$loadOnDemand.loadLocales(appName);
				$secureStorage.set('AppUser','appState',userObject.apps);
				//TODO: inform external api about state
			}
			, removeApp:function(appName){
				if (!self.isLoggedIn()) return ;
				for (var i=0;i<userObject.apps.length;i++){
					if (userObject.apps[i]==appName){
						userObject.apps.splice(i,1);
					  $secureStorage.set('AppUser','appState',userObject.apps);
				    //TODO: inform external api about state
					}
				}
			}
			, hasApp:function(appName){
				if (!self.isLoggedIn()) return false;
				for (var i=0;i<userObject.apps.length;i++){
					if (userObject.apps[i]==appName)return true;
				}
				return false;
			}
			,sortApp:function(item,fromIndex,toIndex){
				 var apps = angular.copy(userObject.apps);
				 for (var i=0;i<apps.length;i++){
					 if (apps[i]==item.name){
				     apps.splice(i, 1);
             apps.splice(toIndex, 0, item.name);
				     userObject.apps=apps;
				     $secureStorage.set('AppUser','appState',userObject.apps);
						 return true;
					 }
				 }
				return false;
			}
			,sendWipeInfo:function(){
				//We retrieve the wipeId for the user or set it
				if (ENV.deviceApi) $secureStorage.get(ENV.appName,'wipeId',0).then(function(s){
					var wuuid= s==0 ? $secureStorage.UUID() : s;
					$secureStorage.sysUUID().then(function (uuid){
						var wipeInfo ={uuid:userObject.id,duuid:uuid,wuuid:wuuid,userName:userObject.name,deviceInfo:ionic.Platform.device()};
						console.log("Adding wipeinfo to SyncQueue",wipeInfo);
						$syncQueue.push(wipeInfo,ENV.deviceApi.apiName);
						if (s==0) $secureStorage.set(ENV.appName,'wipeId',wuuid);
					});
				});
			}
      , lock:function(){ 
				$syncQueue.backGround(0); //Stop the background job
        userObject.loginState=0;
        self.securityGroups={};
				userObject.apps=[]; //Clear user apps
        $secureStorage.set(ENV.appName,'loginState',userObject.loginState);
        $secureStorage.remove(null,'lastValidPin');
				$secureStorage.isolate(false);
        console.log('User locked');
				$rootScope.$broadcast('app.lock');				
      }
      , loadSecurityGroups: function(){
        var defer = $q.defer();
         //TODO:Load security groups for this user
         if ($rootScope.isDemo && $rootScope.isAdmin) 
            self.securityGroups={'admin':{fulldescription:'demo admin'}};
         defer.resolve();
        return defer.promise;
      }
			, installRequiredApps:function(){
				angular.forEach($loadOnDemand.getRequiredApps(),self.installApp);
			}
			, authenticate:function(){
				console.log("Starting authenticate");
				var defer = $q.defer();
				$syncQueue.backGround(0); //Stop the background job
				//Wipe the touch id if there
				if (window.plugins && window.plugins.touchid) {
					 window.plugins.touchid.delete(ENV.appName);
				}
				$secureStorage.sysUUID().then(function (uuid){
					$secureStorage.remove(ENV.appName,'userinfo'); //Remove the last user object
					$secureStorage.wipe(); //Wipe all personal storage
					$adal.logout(true).finally(function(){ //Logout without firing event
						self.refreshApiToken(true).then(function(res){
							//Ask user for a pin, by prompting a modal view
							//If there is a pin set in the resolve object we will not show dialog
							(res.pin ? $q.resolve(res.pin) : $loadOnDemand.goModal("app.newpin",userObject)).then(function(pin){
								
							  userObject.loginState=1; //User is now logged in
              	//Make sure we isolate for this user
								var hashKey=makeHashKey(self.lastUserId(), pin ,uuid); 
								$secureStorage.isolate(hashKey);
								$secureStorage.setCipher('AppUser',hashKey,true);
								
							  
								//We retrieve the wipeId for the user or set it
								self.sendWipeInfo();
								
								//Save pin to touchId if allowed
								if (Boolean.fromString(ENV.supportTouchId,false) && window.plugins && window.plugins.touchid){
									window.plugins.touchid.save(ENV.appName,hashKey,function(){
										console.log("Saved hash to touchid");
									});
								}

								var prom = [];
								prom.push($secureStorage.set(null,'lastValidPin',(new Date()).getTime()));
								prom.push($secureStorage.set(null,'isolate',hashKey));
								//Set Api token agian because token could have changed
								prom.push($secureStorage.set('AppUser','apiToken',$rootScope.apiToken));
								
								prom.push(saveUserObj());
								
								//Set UUID in user context is used in validatePin
								prom.push($secureStorage.set('AppUser','UUID',uuid));
								//Set the security groups from the login tokens
								self.securityGroups={}
								if (res.tokens) angular.forEach(res.tokens.Groups,function(value){
									self.securityGroups[value]={fulldescription:value};							
								})
							
								//Add all required apps to home screen
								self.installRequiredApps();
								
								//TODO: Load user infromation from remote location and add apps to home screen
								
								
								$q.all(prom).then(function(){
									$syncQueue.backGround(); //Start the background job
									//Pass the login information 
									defer.resolve(res);
									//Fire the app loggedin event --> Redirect to home
									$rootScope.$broadcast('app.loggedin');
								},defer.reject)		
							},function(){
								console.log("Login failed");
								defer.reject();
								//Reopen the login dialog, forceing login
								$timeout(function(){self.authenticate()},100);
							});
					},defer.reject);
				 },defer.reject);
				});
				return defer.promise;
			}
			, refreshApiToken : function(force,failOnRefresh){
				var defer = $q.defer();
				if (typeof force==="undefined") force=true;
				var _refreshApiToken = function(res){
					lastRes=res;
					if (!res) { //There is now token
						defer.reject();
					} else if (res.oid){
						userObject.id= res.oid || res.unique_name || res.given_name;
						userObject.name=res.name || res.given_name  || res.upn;
						userObject.email=res.upn;
						//WE NEED TO FIND THE CORRECT TOKEN
						$rootScope.apiToken = res.accessToken;
						if (res.auth_time) {
						  $rootScope.apiTokenFrom = new Date(res.auth_time);
						  $rootScope.apiTokenTill = res.expiresOn.getTime();
						} else {
							$rootScope.apiTokenFrom = (new Date()).getTime();
							$rootScope.apiTokenTill = $rootScope.apiTokenFrom + 1000000;
						}
						//$rootScope.apiTokenValid = new Date(res.auth_time);
						console.log("Refresh of tokens succesfull",$rootScope.apiTokenFrom,$rootScope.apiTokenTill);
						
						$secureStorage.set('AppUser','apiToken',$rootScope.apiToken).then(
							function(){defer.resolve(res)},defer.reject);
					} else {
						var usr = res.tokens || {};
						userObject.id= usr.id || usr.email || usr.given_name;
						userObject.name=usr.given_name || usr.email;
						userObject.email=usr.email;
						$rootScope.apiToken = res.accessToken;
						if (usr.auth_time) {
						  $rootScope.apiTokenFrom = (new Date(usr.auth_time)).getTime();
						  $rootScope.apiTokenTill = res.expiresOn.getTime();
						} else {
							$rootScope.apiTokenFrom = (new Date()).getTime();
							$rootScope.apiTokenTill = $rootScope.apiTokenFrom + 1000000;
						}
					  console.log("Refresh of tokens succesfull",$rootScope.apiTokenFrom,$rootScope.apiTokenTill);
						$secureStorage.set('AppUser','apiToken',$rootScope.apiToken).then(
							function(){defer.resolve(res)},defer.reject);
					}
				}
				
			  if (!force){
					console.log("Disabled forced login");
					defer.resolve(null); //We don't do a refresh
				} else {
					//Check if token still valid, We expire token 30 min before it would expire
					if ($rootScope.apiTokenTill-(30*60000)>(new Date()).getTime()){
						 console.log("RefreshTokens still valid ", lastRes.expiresOn);
						 defer.resolve(lastRes);
					} else {
						$adal.refresh().then(function(res){
							_refreshApiToken(res);
						},function(){
							console.log("RefreshTokens failed, force=",force);
							if (force){
							 // Open the 	console.log("Refresh failed");
								$adal.login().then(function(res){
								 _refreshApiToken(res);
							 },defer.reject);	
							} else if (failOnRefresh) {
								defer.reject();
							} else {
								defer.resolve(null);
							}
						});
					}
				}
				return defer.promise;
			}
      , validatePin:function(pin,isHash){
        var defer = $q.defer();
				$secureStorage.sysUUID().then(function (uuid){
					var hashKey=isHash ? pin : makeHashKey(self.lastUserId() , pin , uuid); 
				  $secureStorage.isolate(hashKey);
					$secureStorage.setCipher('AppUser',hashKey,true);
					//When user logins the sysUUID is encrypted stored and can therefor be checked 
					$secureStorage.get('AppUser','UUID',null).then(function(value){
						if (value==null && !$rootScope.isDemo){
							console.log("No valid user found,forcing login");
							defer.reject();
							self.authenticate();
						}else if ((value!=null && value==uuid) || ($rootScope.isDemo && pin==ENV.demoPin)){
							console.log("Valid Pin");
							self.refreshApiToken($rootScope.isOnline && ENV.tokenRefresh && !$rootScope.isDemo ,$rootScope.isOnline).then(function(res){
								var prom = [];
								prom.push($secureStorage.set(null,'lastValidPin',(new Date()).getTime()));
								prom.push($secureStorage.set(null,'isolate',hashKey));
								$q.all(prom).then(function(){
								  restoreLogin(defer);
									defer.finally(self.installRequiredApps); //Check if we should install extra apps
								},defer.reject);		
							},function(){ //Failed to refresh token while we where online
									defer.reject();
							    self.authenticate();
							});	
						} else {
							$secureStorage.isolate(false);
							userObject.loginState--;
							$secureStorage.set(ENV.appName,'loginState',userObject.loginState);
							defer.reject(userObject.loginState);
							//Check if we should for a authenication
							if (userObject.loginState<-(ENV.pinTry || 6))
							 self.authenticate();
						}	
					},function(){
						$secureStorage.isolate(false);
						defer.reject(userObject.loginState);
					});	
				},function(){defer.reject(userObject.loginState)});
				
				return defer.promise;
      }
			, isLoggedIn: function () {
				 return userObject.loginState==1;
			}
			, isPinAllowed:function(pin){ //Check if pin would be allowed
				var defer = $q.defer()
				if (!pin) defer.reject();
				else {
					var i=1;
					for (var l=pin.length;i<l;i++){
						if (pin.charAt(i)==pin.charAt(i-1)) {
							defer.reject({reason: 'APP_PIN_DOUBLE_CHAR'});break;
						} else if (i>=2 && 
							((pin.charCodeAt(i)==pin.charCodeAt(i-1)+1 && pin.charCodeAt(i-1)==pin.charCodeAt(i-2)+1 ) ||
							 (pin.charCodeAt(i)==pin.charCodeAt(i-1)-1 && pin.charCodeAt(i-1)==pin.charCodeAt(i-2)-1 ))) {
							defer.reject({reason: 'APP_PIN_SEQUENCE'});break;
						}
					}
					//If we reached end of loop, then we are fine
					if (i==pin.length) defer.resolve();
				}
				return defer.promise;
			}
			,init : function(){		
				var defer = $q.defer();
				self.loaded=defer.promise;
				userObject.loginState=0;
				
			  if (ENV.deviceApi) $syncQueue.addEndpoint(ENV.deviceApi.apiName,ENV.deviceApi)
			
				var forceAuthenticate = function(){
					console.log("Force Authenticate");
					$secureStorage.isolate(false);
					self.authenticate();
					defer.reject();
				}				
			  
				//Now startup the login check	
				$secureStorage.get(ENV.appName,'loginState',0).then(function(s){  //Restore the invalid pincount
					userObject.loginState= s>=0 ? 0 : s;
					loadUserObj().then(function(usr){
						 $secureStorage.get(null,'lastValidPin',0).then(function(value){
								if ((ENV['idleLock'] ||0)==0  || value+(ENV['idleLock']||0)*60000>(new Date()).getTime()){
									$secureStorage.get(null,'isolate').then(function(value){
										console.log("Pin still valid, restore login")
										$secureStorage.isolate(value);
										$secureStorage.setCipher('AppUser',value,true);
										self.refreshApiToken($rootScope.isOnline && ENV.tokenRefresh ,$rootScope.isOnline).then(function(res){
											 if (!$rootScope.apiToken) { //Can happen we no pin behavior is set
												 $rootScope.apiToken=value;
											   $secureStorage.set('AppUser','apiToken',$rootScope.apiToken);	 
											 }
											 restoreLogin(defer);
										},forceAuthenticate);
									},defer.reject);
								} else {
									defer.reject();
									console.log("Pin not valid anymore",value);
								}
						 },forceAuthenticate);
					},forceAuthenticate);
				},forceAuthenticate);
			  return defer.promise;
			 }
		};
		
		//We catch the wipe event
		$rootScope.$on('app.wipe',function(data){
			$secureStorage.sysUUID().then(function(sysUUID){
				$secureStorage.get(ENV.appName,'wipeId',0).then(function(wipeId){
					if (data.UUID==sysUUID && data.WIPE==wipeId){						
					 $secureStore.wipe();
					 $secureStorage.isolate(false);
					 self.authenticate();
					}
				})
			})
		});
		
	  //We create a loop here to check if system is on line
		if (ENV.tokenRefresh) setInterval(function(){
			self.refreshApiToken($rootScope.isOnline,false).then(function(data){
				console.log("User token refreshed on background",data);
			},function(){
				console.log("User token refreshed on background failed");
			});
		},(ENV.tokenRefresh||20)*60000);
		
		 //We catch app.logout to force logout and authenicate again
		 $rootScope.$on('app.logout',function(){
			  console.log("System logout");
			  delete $rootScope.apiTokenFrom;
				delete $rootScope.apiTokenTill;
			  
				self.authenticate();					 
		 });
		
		return self;

}])
;