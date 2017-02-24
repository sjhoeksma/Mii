'use strict';
angular.module('app')
.controller('HomeCtrl', function($scope, $rootScope, $state, $window, $ionicHistory, $sce, $stateParams, AppUser, Notifications, $loadOnDemand, $timeout, ENV) {
	$scope.userApps= function(){
	  var a = [];
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
			if (!app.topMenu){
				 a.push(angular.extend({
					 enabled:AppUser.isValidState(module.states[i].state,true) 
				 },app,module.states[i]));
			}
		});
		if (!angular.equals($scope._userApps, a)) $scope._userApps=a;
    return $scope._userApps;
	}
	
  $scope.trustAsHtml = $sce.trustAsHtml;
	//Set the home page style
  $scope.homePageStyle=ENV.homePageStyle || 'icon';
})

;