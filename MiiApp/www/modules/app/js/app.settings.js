'use strict';
angular.module('app',null)
.controller('SettingsCtrl', function($scope, $rootScope,$sce, $state, $stateParams, $ionicHistory, AppUser,$loadOnDemand, $secureStorage,ENV,$filter) {
 $scope.refreshSecurityGroup = function(){
   AppUser.loadSecurityGroups();
 } 
 $scope.themeChangeable=Boolean.fromString(ENV.themeChangeable,true);
 $scope.languageChangeable=Boolean.fromString(ENV.languageChangeable,true);
 $scope.themes=[{name:"default",theme:'base-theme'},{name:"orange",theme:'vf-theme'},{name:'ionic',theme:''}];
 $rootScope.uLang = AppUser.language();  //Select box only works on rootscope

 $scope.languages=[{name:$filter('translate')('APP_LANGUAGE_SYSTEM'),language:null}];	
 var dLang = ENV.defaultLanguage || 'en';
 angular.forEach(ENV.supportedLanguages || [dLang],function(lang){
	 $scope.languages.push({name:$filter('translate')(('APP_LANGUAGE_'+lang).toUpperCase()),language:lang})
 });	

 $rootScope.$watch('uLang',function(oVal,nVal){
	 if (oVal!=AppUser.language()){
		 AppUser.setLanguage(oVal);
		 $rootScope.goBack(); //We need to goback so all modals are loaded in correct language
	 }
 });	
 $rootScope.$watch('appTheme',function(oVal,nVal){
	 if (oVal!=AppUser.theme()){
		 AppUser.setTheme(oVal);
	 }
 });		
	
 $scope.settings=$loadOnDemand.settings;	
 $scope.resetStartup=function(){
	 $secureStorage.set(ENV.appName,'firstTimeStartupApp',true);
 }
 $scope.isValidSetting=function(setting){
	 var module = $loadOnDemand.byState(setting.state);
	 return module && AppUser.isValidState(setting.state) && AppUser.hasApp(module.name);
 }	
})
;