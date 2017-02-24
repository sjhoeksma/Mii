//=========================================================
//Global Envrionment Settings
//=========================================================
angular.module('config', [])
  .constant('ENV', {  
     demoPin: false //When set to false it is disabled otherwise it is the pin true we will run in demo mode
	  ,mbaas : {
			
		}
	  ,secureStorage: true //Set to true is we are using secure storage to store local data 
	  ,pinCount: 4   //The number of digits in your pin
	  ,pinTry: 4     //The max number of pin tries
    ,idleLock: 0 //Time of inactivity (in minutes) before logging of user 0 is diabled	
  	,removeAppOnError: false //When set to true a app will be remove when causing an error
	  ,contentSyncInterval: 180 //Time in minutes to check for new packages. Set to 0 to disable
	  ,supportTouchId: true //Set to true if we allow touch id to be used
	  ,browserLock: true //When set to false, user will be loggedout on time out in browser, no lock available
	  ,fullscreen: false //Should we run the app full screen
    ,appVersion: '0.1.0' //Version of the application, cordova overwrites with buildinfo
    ,appName: 'MiiApp' //Name of the application, on cordova overwrites with buildinfo
	  ,appOwner: 'S.J.Hoeksma' // The owner of the app 
	  ,homePageStyle: "icon" //The style to be used on homepage ('icon','badge')
	  ,ignoreOnError: true //Should we remove the queue items on error
    ,appTheme: 'vf-theme' //The theme we will use
	  ,themeChangeable: true //Is the theme change able
    ,appImageLoc: 'img/' //Location for the application images
    ,appLogo: 'logo.png'  //The logo which will be used in menubar
    ,defaultAvatar: 'defaultavatar.jpg'
    ,adminConfig: 'http://cdn.instantshift.com/media/uploads/2013/07/admin-page-themes.jpg' //The admin config url
    ,adminWiki: 'https://en.wikipedia.org/wiki/Main_Page' //The application documentation
    ,defaultLanguage: 'en' //The default language to use
	  ,languageChangeable: true //Can user set his own langauge
    ,supportedLanguages: ['en', 'nl'] //A array of langauges [country code] supported by the APP 
    ,languageLoader: 'Partial'  //Loader types supported are : CodePen, Partial or default(static)
    ,feedbackEmail: 'feedback@myapp.com' //The support address to send feedback to
    ,supportPhone: '' //The phone number to call for support
  });