'use strict';
angular.module('app',null)

.controller('NotificationCtrl', function($scope, $rootScope,$sce, $state, $stateParams,$ionicHistory, Notifications,$ionicListDelegate, $ionicPopup,$timeout ) {
	$scope.Notifications=Notifications;
	$scope.read=function(index){
		Notifications.read(index);
		$ionicListDelegate.closeOptionButtons();

	}
	$scope.delete=function(index){
		Notifications.delete(index);
		$ionicListDelegate.closeOptionButtons();
	}
  $scope.unread=function(index){
		Notifications.unread(index);
		$ionicListDelegate.closeOptionButtons();		
	}
	$scope.addMsg = function(){
		Notifications.push("Message " + (new Date()).toString());
	}
	$scope.doubleClick=function(item){
		 var myPopup = $ionicPopup.show({
			template: item.msg,
			title: 'Notifications Info',
		 subTitle: 'Please take action',
			scope: $scope,
			buttons: [{
					text: '<b>Close</b>',
					type: 'button-positive'
				}
			]
    });
	}
})
;