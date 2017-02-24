'use strict';
angular.module('provider.ContentSync',[	
])
 .provider('$contentSync', function (ENV) {
	
	var myConfig = {};
	var inProgress=false;
	//var progressState
	
  this.$get=['$rootScope', '$q' ,'ENV' ,'$http', 'AppUser','$loadOnDemand'
			, function ($rootScope, $q ,ENV, $http, AppUser,$loadOnDemand) { 
		var self = {		
	   sync: function(){
			 if ($rootScope.incProgress) $rootScope.incProgress();
			 var defer = $q.defer();
			 if (window.ContentSync && !inProgress && myConfig.srcURL) {
				 inProgress=true;
				 console.log("ContentSync started");
				 var reloadApp = false;
				 var proms =[];
				 var headers = {
					 Authorization: (myConfig.hasOwnProperty('prefixToken') ? myConfig.prefixToken : (ENV.prefixToken || '')) + (myConfig.apiToken ||  ENV.karmaToken || $rootScope.apiToken ) 
				 }
				 //Step 1 check all modules installed, if we should update them
				 var modDef = $q.defer();
				 proms.push(modDef);
				 ($rootScope.http || $http)({
						method: 'GET',
						url: myConfig.srcURL + '/modules/modules.json',
						headers: headers
				 }).then(function successCallback(response) { 
					 angular.forEach(AppUser.apps,function(appName){
						 var update = false;
						 var module=$loadOnDemand.getConfig(appName);
						 var appVersion=module && module.version ? module.version : '1.0';
						 
						 angular.forEach(response.data,function(remoteModule){
							 //TODO: Check version of module
							 if (appName.equals(remoteModule.name) && (remoteModule.version!=appVersion)) {
								 update=true;
							 }
						 })
					   if (update){
							 var def = $q.defer();
				       proms.push(def);
							 var appSync = ContentSync.sync({
									src: myConfig.srcURL + '/modules/' + appName + '.zip',
									id:  appName,
							 });
							 appSync.on('complete', function(data) {
								 console.log("ContentSync completed update module",appName);
								 reloadApp=true;
								 def.resolve();
							 });
							 appSync.on('error', def.reject);
							 appSync.on('cancel', def.reject);
						 }	
					 });
					 modDef.resolve();
				 }, function errorCallback(response) {
						modDef.reject(response);
				 });

				 
				 //Step 2 check the framework module if we should update
				 var appDef = $q.defer();
				 proms.push(appDef);
				 ($rootScope.http || $http)({
						method: 'GET',
						url: myConfig.srcURL + '/version.json',
						headers: headers
				 }).then(function successCallback(response) {
					 if (response.version!=ENV.version) {
							var appSync = ContentSync.sync({
								src: myConfig.srcURL + '/app' + response.version + '.zip',
								id:  ENV.appName,
								copyRootApp: true,
								manifest: 'manifest.json'
						 });
						 appSync.on('complete', function(data) {
							 console.log("ContentSync completed main app");
							 reloadApp=true;
							 appDef.resolve();
						 });
						 appSync.on('error', appDef.reject);
						 appSync.on('cancel', appDef.reject);
					  } else { 
							appDef.resolve();
						}
					}, function errorCallback(response) {
						appDef.reject(response);
					});
				 

				 //Step 3 set the $rootScope.reloadApp to true so next time application goes to app.home
				 $q.all(proms).then(function(){
					 inProgress=false;
					 if (reloadApp) {
						 $rootScope.reloadApp=true; 
					   $rootScope.$broadcast('reloadApp');
					 }
					 defer.resolve();
				 },function(){
					 inProgress=false;
					 defer.reject();
				 }).finally(function(){
					  console.log("ContentSync finished");
				 });
			 } else {
				 console.log("ContentSync ",window.ContentSync ? "active" : "not installed");
				 defer.reject();
			 }
			 defer.promise.finally($rootScope.decProgress);
			 return defer.promise;
		 }
	 }
	 return self;
  }];
	
	this.config = function(_config) {
    myConfig=angular.extend(myConfig,_config);
	}

});