'use strict';
angular.module('app',null)

.controller('StoreCtrl', function($scope, $rootScope,$sce, $state, $stateParams,$ionicHistory,$loadOnDemand, AppUser,ENV) {
	$scope.showReorder=false;
	$scope.toggleReorder=function(){
		$scope.showReorder=!$scope.showReorder;
	}
	$scope.moveItem=AppUser.sortApp;
	$scope.AppUser=AppUser;
  $scope.applications=function(){
		var apps = [];
		angular.forEach($loadOnDemand.applications,function(app,name){
			if (!app.hidden && (!Boolean.fromString(app.demoOnly,false) || $rootScope.isDemo)) apps.push(app);
		});
		
		if (angular.equals($scope.prevAllApps, apps)) {
      return $scope.prevAllApps;
    }
    $scope.prevAllApps = apps;
		return apps;
	}
	$scope.userApps=function(){
		var apps = [];
		angular.forEach(AppUser.apps(),function(name){
			var app = $loadOnDemand.getApp(name);
			if (app && !app.hidden) apps.push(app);
		});
		
		if (angular.equals($scope.prevApps, apps)) {
      return $scope.prevApps;
    }
    $scope.prevApps = apps;
		return apps;								
	}
	$scope.installApp=function(name){
		AppUser.installApp(name)}
	$scope.removeApp=function(name){AppUser.removeApp(name)}
})

;