'use strict';
//=========================================================
//The framework application module
//=========================================================
angular.module('framework', [
  'ionic', // The inonic framework
  'ngCordova', //Load the ngCordova extentions
	'ngCookies', //Cookie support
  'config', // The application configuration
	'directive.Common', // Common directives,filters factories 
	'directive.GridMenu', //The GridMenu used on home page
	'directive.IonKeyPad', //The keypad used to enter pin
	'directive.IonSearchSelect', //Drop down search list
	'directive.MenuTree',  //The menu tree in side menu
	'provider.SecureStorage', //The implementation for secure storage
	'provider.ADAL',   // ADAL Authentication
	'provider.SyncQueue',   // ADAL Authentication
	'provider.LoadOnDemand',  // The dynamic loader for all modules
	'provider.ContentSync', //The provider used to do remote sync of modules and appcode
	'provider.FilePicker',  //Wrapper function for the cordova FilePicker-Phonegap-iOS-Plugin & cordova-filechooser
	'service.AppUser',  // The Service containing general app controls for user information
	'service.Notifications',  // The Service containing notifications
	'service.NetworkMonitor',  // The Service for network notifications
	'service.IdleStateTimer',  // The Service to keep time on idle state
	'pascalprecht.translate', // inject the angular-translate module
  'ion-datetime-picker', // Date time picker
	'$selectBox', // The select box component
	'nzTour',  //The tour wizard
  'canSwipeDirective', // Swipe Functions require special ionic version 
	'longSwipeDirective', //Allow left right long swipes
	'clickSwipeDirective', //Open swipebox on click
	'itemSwipePaneDirective' //Swipe functions	
])

//=========================================================
//The SideMenu controller is only fixed part of framework
//=========================================================
.controller('SideMenuCtrl', function($scope, $rootScope, $state, $window, $ionicHistory,AppUser, $loadOnDemand, Notifications,ENV, nzTour,$filter,$timeout) {
	$scope.AppUser=AppUser;
	$scope.Notifications=Notifications;
	$scope.titleApps=[]; 
	$scope.ENV = ENV;
  $scope.leftmenuItems = $loadOnDemand.menus;
  $scope.lockAllowed = !(!$rootScope.isApp && !Boolean.fromString(ENV.browserLock,true)) 
		                  || $rootScope.isDemo ;

	//When view changes we update the titleApps
	$scope.$on("$ionicView.afterEnter", function(event, data){
 		var t = [{
					state: $rootScope.homeState,
				  title: "FRW_HOME",
					ion: "ion-home",
				  hidden:$rootScope.homeState==$state.current.name,
			    click:$scope.doClick
				}];
		angular.forEach(AppUser.apps(),function(name){
			//Create grid item from application data
			var module = $loadOnDemand.getConfig(name);
			var stateName=name;
			if (!module) return;
			if (module.app && module.app.state) { 
				stateName=module.app.state;
				module = $loadOnDemand.byState(module.app.state);
				if (!module) return;
			}
			var i=0;
			for (;i<module.states.length;i++) if (module.states[i].state==stateName) break;
			//We only show apps for which state is valid, is not hidden and modules are loaded, 
			//and it is not a header app 
			var app =$loadOnDemand.getApp(name);
			if (i>=module.states.length || !app) return;
			if (app.topMenu){
				 t.push(angular.extend(
					 {click:$scope.doClick, 
						hidden: $state.current.name==module.states[i].state,
						enabled:AppUser.isValidState(module.states[i].state,true)
					 },app,module.states[i]));
			} 
		});
		t.push({
			state: "app.tour",
			title: "FRW_TOUR",
			ion: "ion-ios-help",
			hidden: !$loadOnDemand.hasTour($state.current.name),
			click: $scope.doTour
		});
  	if (!angular.equals($scope.titleApps, t)){
        $scope.titleApps=t;
		}
  });
  $scope.validateMenuItem = function(item){
    return AppUser.isValidState(item.state);
  };
 	$scope.doClick=function(item){
		$loadOnDemand.goApp(item.name || item.state);
	}
	
})


//=========================================================
//Run
//=========================================================
.run(function($ionicPlatform, $ionicHistory, $rootScope, $timeout, $state, $ionicPopup, $filter, $ionicConfig, $ionicSideMenuDelegate, $translate, ENV, AppUser, NetworkMonitor, idleStateTimer, $loadOnDemand, Notifications,$ionicLoading, $document,$secureStorage,$contentSync,$http,$q) {
  $ionicPlatform.ready(function() {
    console.log("Framework ready");
		$rootScope.bgProgress=0; //Set the backgroundProcess indicator to 0
		$rootScope.incProgress=function(){
			$rootScope.bgProgress++;
			if ($rootScope.bgProgressTimer) $timeout.cancel($rootScope.bgProgressTimer);
			$rootScope.bgProgressTimer=$timeout(function(){$rootScope.bgProgress=0},90000); //We keep background timer max 90 seconds
		//	console.log(">>>>>> ",$rootScope.bgProgress);
		}
		$rootScope.decProgress=function(){
			if ($rootScope.bgProgress){
			  $rootScope.bgProgress--;			
			}
		  if ($rootScope.bgProgressTimer) $timeout.cancel($rootScope.bgProgressTimer);
			if ($rootScope.bgProgress){
			  $rootScope.bgProgressTimer=$timeout(function(){$rootScope.bgProgress=0},90000); //We keep background timer max 90
			} else {
				$rootScope.bgProgressTimer=null;
			}
			//console.log("<<<<< ",$rootScope.bgProgress);
		}
		$rootScope.showLoadingSpinner=function(delay){
		 //$rootScope.incProgress();
		 $ionicLoading.show({
			template: '<ion-spinner class="spinner-energized"></ion-spinner>',
			delay: (typeof delay == "undefined" ? 250 : delay) //We prevent showing loading spinner if loading takes less time then animation
		});
	 }
	 $rootScope.hideLoadingSpinner=function(){
		// $rootScope.decProgress();
		 $ionicLoading.hide();
	 }
    // Hide the accessory bar by default (remove this to show the accessory bar above the keyboard
    // for form inputs)
    if (window.cordova && cordova.plugins.Keyboard) {
      cordova.plugins.Keyboard.disableScroll(true);
    }

    // Reload the Translates if needed automaticly
    $rootScope.$on('$translatePartialLoaderStructureChanged', function () {
      $translate.refresh();
    });
	  
   //The global object used for tranlsation text values 
   $rootScope.translationData = {};
		
	 //Define power management	
	 $rootScope.powerManagement= function(state){
			if (window.powermanagement) {
				if (state) {
					console.log("Enable Power Allways on");
					window.powermanagement.acquire();
				} else {
					console.log("Disable Power Allways on");
					window.powermanagement.release();
				}	
				$rootScope.powerManagementState=state;
			}
    }	
		
   //Set the appName/appVersion from ENV or Build in common and rootScope
   $rootScope.appName=ENV['appName']||'Undefined';
	 $rootScope.appOwner=ENV['appOwner'];
   $rootScope.appVersion=ENV['appVersion']||'0.0.1';
   $rootScope.appLogo=ENV['appLogo'];
   $rootScope.appTheme=ENV['appTheme']||'';
   //Set the global support settings
   $rootScope.supportPhone=ENV['supportPhone'];
   $rootScope.feedbackEmail=ENV['feedbackEmail'];
		
   //Convert a short image into a image location
   $rootScope.imageLoc=function(img,module){
     if (typeof img ==="undefined" || img.indexOf('//')>=0) return img;
		 if (module!==false){
			 return $loadOnDemand.toModulePath(module||$state.current.name,img,'img');
		 } else return (ENV['appImageLoc']||'')+img; 
   }
   //Set the demoMode
   $rootScope.isDemo=ENV['demoPin'] && Boolean.fromString(ENV['demoPin'],true);
	
	 //Function to getToken
	 $rootScope.http = ENV.tokenRefresh ? function(config){
		 var defer = $q.defer();
		 if (!$rootScope.isOnline) {
			 defer.reject("Offline");
		 } else {
		   AppUser.refreshApiToken(!$rootScope.isDemo).then(function(){
				 if (!config.headers && config.headers.Authorization){
					 if (!config.headers) config.headers={};
					 config.headers.Authorization= (ENV.prefixToken || '') + (ENV.karmaToken || $rootScope.apiToken );
				 }
				$http(config).then(defer.resolve,defer.reject);
			 },defer.reject);
		 }
		 return defer.promise;
	 } : $http;
		 
   //Monitor the online state
		
   $rootScope.isOnline=NetworkMonitor.isOnline();
	 NetworkMonitor.watch(function(networkState,networkMode){
      console.log("Network state changed",networkState);
      $rootScope.isOnline=networkState;
      $rootScope.$apply();
   });
    
	 Notifications.init().then(
		 function(){console.log("Notifications enabled")},
	   function(){console.log("Notifications disabled")});	
 
	 $rootScope.isApp=false;	
   //$rootScope.isApp=self.getParameterByName('device')=='electron';
   //We use cordova stuff so let system be ready before we start settings gobal vars
   if (window.cordova && window.cordova.getAppVersion) {
     window.cordova.getAppVersion.getAppName().then(function (value) {
       $rootScope.appName=value;
       window.cordova.getAppVersion.getVersionNumber().then(function (value) {
         $rootScope.appVersion=value;
         $rootScope.isApp=true;
       });		
     });
   }
		
	 //Start a tour	
	 $rootScope.doTour=function(item,name){
		$loadOnDemand.loadTour(name || $state.current.name).then(function(tour){
			var myTour=angular.merge({config: {
				mask: {
						visible: true, // Shows the element mask
						visibleOnNoTarget: false, // Shows a full page mask if no target element has been specified
						clickThrough: false, // Allows the user to interact with elements beneath the mask
						clickExit: true, // Exit the tour when the user clicks on the mask
						scrollThrough: true, // Allows the user to scroll the scrollbox or window through the mask
						color: 'rgba(0,0,0,0)' // The mask color
				},
				container: 'body', // The container to mask
				scrollBox: 'body', // The container to scroll when searching for elements
				previousText: $filter('translate')('Previous'),
				nextText: $filter('translate')('Next'),
				finishText: $filter('translate')('Finish'),
				showPrevious: true, // Setting to false hides the previous button
				showNext: true, // Setting to false hides the next button
				animationDuration: 200, // Animation Duration for the box and mask
				placementPriority: ['top', 'bottom', 'right', 'left'], //['bottom', 'right', 'top','left'],
				dark: true, // Dark mode (Works great with `mask.visible = false`)
				disableInteraction: false, // Disable interaction with the highlighted elements
				highlightMargin: 0, // Margin of the highglighted area
				disableEscExit: false, // Disable end of tour when pressing ESC,
//				onClose: function() {} //Function called when the tour is closed
//				onComplete: function() {} //Function called when the tour is completed
			}},item ||{}, tour);
		  //console.log("Tour",myTour);	
			nzTour.start(myTour);
		},function (){
			console.warn("Could not load tour for ",$state.current.name);
		})
	 };
	
	 //Keep the title in the WebBrowser on Application title 
		$document[0].title = $rootScope.appName;
		//TODO remove mouseover event
		$rootScope.$on('$ionicView.afterEnter', function(ev, data) {
      if (data && data.title) {
        $document[0].title = $rootScope.appName;
      }
    });
  	
   //Create a common function for goBack();
   $rootScope.goBack=function(count){
		 if (!$ionicSideMenuDelegate.isOpenRight()){
		 //(typeof(toState) != 'undefined' || allowed) && 	
			 $ionicHistory.goBack(count);
		 }
	 }
	 
	 //We use this to store home state, in case of single app we will change this
	 $rootScope.homeState = "app.home";
	 
	 //Create a home function 
	 $rootScope.goHome=function(){$state.go($rootScope.homeState);}
	 
	 //Catch the loggedin event to route use home
	 $rootScope.$on('app.loggedin',function(){
		//Check the number of user applications
		if (AppUser.apps().length==1) {
			if (!$rootScope.homeStateOrg) $rootScope.homeStateOrg=$rootScope.homeState;
			$rootScope.homeState=AppUser.apps()[0];			
			console.log("Single App usage",$rootScope.homeState);
		} else if ($rootScope.homeStateOrg){
   		$rootScope.homeState =$rootScope.homeStateOrg;
			delete $rootScope.homeStateOrg;
		}
		$rootScope.goHome();
	});
	    
   //Make the isLoggedIn function available through root Scope 
   $rootScope.isLoggedIn=AppUser.isLoggedIn;
    //Configure the hardware platform back button
    $ionicPlatform.registerBackButtonAction(function() {
      if ($ionicHistory.currentStateName() == $rootScope.homeState ||
        $ionicHistory.currentStateName() == "app.login") {
				if (navigator && navigator.Backbutton) { //Check if we have install the android back button
				 navigator.Backbutton.goHome(function() {
						console.log('success')
					}, function() {
						console.log('fail')
					});
				} else {
					var confirmPopup = $ionicPopup.confirm({
						title: $filter('translate')('FRW_EXIT_APP'),
						okText: $filter('translate')('FRW_YES'),
						cancelText: $filter('translate')('FRW_NO')	
					}).then(function(res) {
						if (res) ionic.Platform.exitApp();
					});
				}
      } else {
        if (!$ionicHistory.goBack()) {
          //Home Command, when we don't have a page to go back to
          $ionicHistory.clearHistory();
          $ionicHistory.nextViewOptions({
            disableBack: true
          });
          $rootScope.goHome();
        }
      }
    }, 100);
		
		
		//Force loading of the default app
		$loadOnDemand.autoLoad().then(function(){ 
			 //Enable the out lock
       idleStateTimer(ENV['idleLock'],'app.login');
      
			 //Catch state change to enforce login on menu state
			 $rootScope.$on('$stateChangeStart'
											, function (event, toState, toParams, fromState, fromParams) {
				 console.log('Change state:',fromState,toState);
				 $rootScope.showLoadingSpinner();
				 if (toState.name=='app.logout'){
					 console.log("Forced logout");
					 $rootScope.$broadcast("app.logout"); //On logout event also fire the broadcast
				 } 
         if (toState.name=='app.login' && (!$rootScope.isDemo &&
						 (!$rootScope.isApp && !Boolean.fromString(ENV.browserLock,true))) ) {
					 console.log("Forcing logout");
					 event.preventDefault();
					 $ionicHistory.nextViewOptions({disableBack: true});
					 $state.go('app.logout',{Logout:true});
				 } else if ((!toState.data || (toState.data && toState.data.auth !== false)) 
						 && !AppUser.isLoggedIn()) {
					 event.preventDefault();
					 $ionicHistory.nextViewOptions({disableBack: true});
					 $state.go('app.login',toParams);
				 } else if (!AppUser.isValidState(toState.name)) {
					 console.log("Not available ",toState.name);
					 if (fromState.name==$rootScope.homeState) {
						 $rootScope.hideLoadingSpinner();
						 document.getElementById('grid-template').hideContent(); 
					 }
					 event.preventDefault();
					 $state.go(fromState,fromParams);
				 } else if (fromState.name==$rootScope.homeState){
					   $ionicHistory.nextViewOptions({disableBack: false});
				 } else if ((toState.name=='app.login' && AppUser.lastUserId()==null) && !$rootScope.isDemo) {
					 $timeout(function(){AppUser.authenticate()},200);
			   }
			 });
			
			$rootScope.$on('$stateChangeSuccess', 
               function(event, toState, toParams, fromState, fromParams){ 
				$rootScope.hideLoadingSpinner();
				//Check if we should reload the application on entering the home page
					
				if (toState.name==$rootScope.homeState && Boolean.fromString($rootScope.reloadApp,false)){
				   console.log("Reloading APP");
					 delete $rootScope.reloadApp;
					 window.location.reload();
				}
				//Check if we should run a content sync
				if (ENV.contentSyncInterval && $rootScope.isOnline && ($rootScope.nextContentSync||0)*60000<(new Date()).getTime()){
						$rootScope.nextContentSync=(new Date().getTime())+(ENV.contentSyncInterval*60000);
						$timeout($contentSync.sync,100); //Just first finish the successstate 
				}
				
			});
			
			$rootScope.$on('$stateChangeError', 
         function(event, toState, toParams, fromState, fromParams, error){
				  console.log("Failed to change state",error);
					$rootScope.hideLoadingSpinner();
				  if (Boolean.fromString(ENV.removeAppOnError,false))
				       AppUser.removeApp(toState.name);
				  if (fromState.name==$rootScope.homeState) {
						var el=document.getElementById('grid-template');
						if (el) el.hideContent();
					} else $rootScope.goHome();
			})
			
			 //Catch state not found, allowing easy loading
			 $rootScope.$on('$stateNotFound', function(event, unfoundState, fromState, fromParams){ 
					console.log("Unfound State", unfoundState.to,unfoundState.toParams,unfoundState.options); 
					event.preventDefault();
					//Dynamic load the missing state, load it
					$loadOnDemand.go(unfoundState.to,unfoundState.toParams,unfoundState.options).then(
						null,function(){ 
							console.warn("Failed to load App ",unfoundState.name);
							$rootScope.hideLoadingSpinner();
							if (Boolean.fromString(ENV.removeAppOnError,false))
								 AppUser.removeApp(unfoundState.name);
							//Remove simulation of grid item
							if (fromState.name==$rootScope.homeState) {
								var el=document.getElementById('grid-template');
						    if (el) el.hideContent();
							} else $rootScope.goHome();
						});
			 });
			
			//On startup we always go to TourScreen,WhatsNew or Home screen
			console.log('Starting application ' + ($rootScope.isDemo ? 'in demo mode':''),navigator.userAgent.toLowerCase());
		  AppUser.init().finally(function(){
				$secureStorage.get(ENV.appName,'firstTimeStartupApp',true).then(function(value){
					if (value){
						$secureStorage.set(ENV.appName,'firstTimeStartupApp',false);
						$ionicHistory.nextViewOptions({disableBack: true});
						$state.go('app.firsttime');
					} else {
						$secureStorage.get(ENV.appName,'lastAppVersion').then(function(value){
							if (value!=ENV.appVersion){
								AppUser.lock();
								$ionicHistory.nextViewOptions({disableBack: true});
								$state.go('app.whatsnew');
							} else if (!AppUser.isLoggedIn()){
								$state.go('app.login');
							} else { // Force Validation of login by going home
								$ionicHistory.nextViewOptions({disableBack: true});
								$rootScope.goHome();
								Notifications.load();
							}
						});
					}
				}).finally(function(){
					//Set the latest AppVersion we used to 
					$secureStorage.set(ENV.appName,'lastAppVersion',ENV.appVersion);
				});

				//Last we hide the  splach screen 
				if (navigator && navigator.splashscreen) {
					// FullScreen will hide the status bar using the StatusBar plugin
					if (Boolean.fromString(ENV.fullscreen,true)){
						if (window.StatusBar) StatusBar.hide(); 
						ionic.Platform.fullScreen(); //Placed here to prevent problems with ionic view
					}
					setTimeout(function() {
						navigator.splashscreen.hide();
						//Wait one second for transition to complete
						setTimeout(function() {
							$rootScope.$broadcast('splashscreen.hide')
						}, 250);
					}, 200);
				}
			});

		});
  });
})

//=========================================================
//Config
//=========================================================
.config(function($stateProvider, $urlRouterProvider, $ionicConfigProvider, ENV , $translateProvider, $translatePartialLoaderProvider, $loadOnDemandProvider, $secureStorageProvider, $adalProvider,$syncQueueProvider) {
	console.log("Configuring framework");
  //Disable the transactions
  //$ionicConfigProvider.views.transition('none');

  //Align the navBar titles 
  $ionicConfigProvider.navBar.alignTitle('center');
  //Hide the Back Text
  $ionicConfigProvider.backButton.previousTitleText(false).text('');
	
  //Multilanguage support	
  //We cannot use the common factory so just use the ENV directly
  var dLang = ENV.defaultLanguage || 'en';
  var sLang = ENV.supportedLanguages || [dLang];
  var vLang = {
    '*': dLang
  };
  vLang[dLang + '_*'] = dLang;
  angular.forEach(sLang, function(value) {
    this[value + "_*"] = value;
  }, vLang);
  $translateProvider
    .registerAvailableLanguageKeys(sLang,vLang)
   	.fallbackLanguage(dLang) 
    .useSanitizeValueStrategy('escapeParameters');
  //Define the user language
  var uLang  = navigator ? navigator.language || navigator.userLanguage || null : null;
  if (uLang)
    $translateProvider.preferredLanguage(uLang);
  else
    $translateProvider.determinePreferredLanguage();
  console.log("Language Support",sLang, vLang, dLang,uLang);
	
	//Configure the secureStorage and adal
	$adalProvider.config(ENV.webConfig,ENV.nativeConfig);
	
	//Configure the SecureStorage
	$secureStorageProvider.config({
		cipher:ENV.appName + ENV.appOwner,
		enabled:ENV.secureStorage
	});
	
	//Configure the syncQeueuProvider
	$syncQueueProvider.config({
		 syncOnPush:true
		,ignoreOnError:Boolean.fromString(ENV.ignoreOnError,false)
		,duplicateCode:ENV.duplicateCode || 409
		,interval:30000
	});
	


  if (ENV.languageLoader==="CodePen") {
    $translateProvider.useLoader('codepenTranslationLoader', {
      prefix: ENV['appName']+'/locales/',
      suffix: '.json'
    });
  } else if (ENV.languageLoader==="Partial") {
     $translateProvider.useLoader('$translatePartialLoader', {
      urlTemplate: '{part}/locales/{lang}.json'
    });
		 $translatePartialLoaderProvider	.addPart('.');
  } else {
   $translateProvider.useStaticFilesLoader({
			 prefix: ENV['appName']+'/locales/',
			 suffix: '.json'
		});
  }
  console.log("Setting default language to " + dLang);
	
	//Configure the onDemand loader
	$loadOnDemandProvider.config([],'modules','menuContent');
	//Add the config listner for the syncQueue
	$loadOnDemandProvider.addConfigListener($syncQueueProvider.configListener);

	
	//The basic home page
  $stateProvider
    .state('app', {
      url: "",
      abstract: true,
      templateUrl: "html/sidemenu.html",
      controller: 'SideMenuCtrl'
    })
  ;
  $urlRouterProvider.otherwise("/");

})

//=========================================================
//For codepen we will load translation from script files
//Needs to be in same module as config
//=========================================================
.factory('codepenTranslationLoader', function($q, $timeout, $document) {
  return function(options) {
    var deferred = $q.defer(), translations = {},el;
    //CodePen has issue with adding translation="no" to header
    angular.element(document).find('body').removeAttr('translate'); 
    el = $document[0].getElementById(options.prefix + options.key + options.suffix);
      
    if (el) translations = angular.fromJson(angular.element(el).html());
    deferred.resolve(translations);
    return deferred.promise;
  };
})
;