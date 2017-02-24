//Helper function to open an external webpage by
//use a iframe or cordova-plugin-inappbrowser or cordova-plugin-themeablebrowser

'use strict';
angular.module('app')

.factory('WebPage', ['$rootScope','$sce'
	, function ($rootScope,$sce) { 	
    var win=null;
		var self = {
		 close:function(){
			 if (win) win.close();
			 win=null
		 }
		 ,open:function(params){
			  if (Boolean.fromString(params.inBrowser,false)){ //We use cors
					  //console.log("Iframe URL",Boolean.fromString(params.addToken,true) ? params.url  + '#id_token=' + $rootScope.apiToken : params.url);
					  params.iframeUrl= $sce.trustAsResourceUrl(Boolean.fromString(params.addToken,true) ? params.url  + '#id_token=' + $rootScope.apiToken : params.url);
	  			  win=null;
					  return 0;
				} else if (window.cordova && cordova.ThemeableBrowser && Boolean.fromString(params.inBrowser,null)!=null) { //We open a themable browser  
          win=cordova.ThemeableBrowser.open(Boolean.fromString(params.addToken,false) ? params.url  + '#id_token='  + $rootScope.apiToken : params.url, params.target || '_blank', angular.extend({
						statusbar: {
								color: '#ff9900'
						},
						toolbar: {
								height: 44,
								color: '#ff9900'
						},
						title: {
								color: '#FFFFFF',
								showPageTitle: true
						},
						closeButton: {
								wwwImage: 'img/close.png',
								wwwImagePressed: 'img/close.png',
								wwwImageDensity: 1,
								align: 'right',
								event: 'closePressed'
						},
						/*
						menu: {
								wwwImage: 'img/menu.png',
								wwwImagePressed: 'img/menu.png',
							  wwwImageDensity: 1,
								title: 'Test',
								cancel: 'Cancel',
								align: 'left',
								items: [
										{
												event: 'helloPressed',
												label: 'Hello World!'
										},
										{
												event: 'testPressed',
												label: 'Test!'
										}
								]
						},*/
						backButtonCanClose: true
				},params.config)).addEventListener('backPressed', function(e) {
						if (typeof params.back=="function") params.back();
				}).addEventListener(cordova.ThemeableBrowser.EVT_ERR, function(e) {
						console.error(e.message);
				}).addEventListener(cordova.ThemeableBrowser.EVT_WRN, function(e) {
						console.warn(e.message);
				});
				return 1;
  		} else {
			 win=window.open(Boolean.fromString(params.addToken,false) ? params.url  +  '#id_token=' + $rootScope.apiToken : params.url, params.target || '_blank', 'location=false,toolbarposition=top');
			 return -1;	
		  }
	   }
		};
		
		return self;		
}])

.controller('WebPageCtrl', function($scope, $state, $rootScope, $ionicHistory,$stateParams,WebPage,$timeout,AppUser) {
	  $scope.config=angular.extend({target:"_blank"},$stateParams.json ? angular.fromJson($stateParams.json) : {});
	  $scope.iframeLoadedCallBack = function(){
      // do stuff
			console.log("WebPage iframe loaded");
    }
	  console.log("Opening webpage",$scope.config,$stateParams.json);
	  AppUser.refreshApiToken(Boolean.fromString($scope.config.addToken,true)).then(function(){
	    if (WebPage.open($scope.config)!=0) $timeout($rootScope.goBack,250); //Go back if we have inappbrowser active
		},function(){
			console.log("Failed Opening webpage",$scope.config);
			$rootScope.goBack();
		});
})
;
