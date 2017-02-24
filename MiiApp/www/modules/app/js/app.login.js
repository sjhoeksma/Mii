'use strict';
angular.module('app')
.controller('LoginCtrl', function($rootScope,$scope, $sce, $state, $stateParams, AppUser, $ionicSideMenuDelegate, $ionicHistory,$filter,$timeout,$window,ENV) {
	 //Check if we should broadcast a logout Broadcast logout
	 $scope.pinTry = (ENV.pinTry || 6);
	 $scope.pinCount = (ENV.pinCount || 4);
	// console.log("States LOGIN",angular.toJson($stateParams));
	
	 if ($rootScope.isDemo) $scope.translateData={demoPin: ENV.demoPin+" "};
	
	 $scope.passcode="";
   $scope.AppUser = AppUser; 
   $scope.pinkeyboardoptions = {
		visible:true,
		button: {
      content: $filter('translate')('APP_FORGOT_PIN') + " >"
      ,class: 'button button-clear button-light float-right'
      ,onClick: function() {
 				AppUser.authenticate().then(
				function(){
					console.log("Login Completed");
				},function(){
					console.log("Login Failed");
				});
			}
    },
    rigthControl: '<i class="icon ion-backspace-outline"></i>',
		//leftControl: '<i class="icon ion-ios-paw"></i>',
    onKeyPress: function(value, source,event) {
      if (source === 'RIGHT_CONTROL' || (source == 'KEYBOARD' && value==8)) {
      	 $scope.passcode = $scope.passcode.substr(0, $scope.passcode.length - 1)
      }else if (source === 'LEFT_CONTROL' ) {
      	 $scope.showTouchId();
      } else if (source === 'NUMERIC_KEY') {
				if ($scope.passcode.length < $scope.pinCount) {
					$scope.passcode = $scope.passcode + value;
					if ($scope.passcode.length >= $scope.pinCount) {
						$timeout(function (){
							AppUser.validatePin($scope.passcode).then(function (msg) {
								//finally, we  have logged in, routing home is done by event
							},function(count){
								if (count<0 && navigator.vibrate) 
									navigator.vibrate(550);
							}).finally(function(){
								$scope.passcode = ""; //remove password
						});
					},50);
					}
				}
      } else if (source == 'KEYBOARD' && value==27) {
				$scope.passcode='';
			} else return false;
			return true;
		}
  };
	$scope.showTouchId=function(){
		//Check if system has touchID
		if (Boolean.fromString(ENV.supportTouchId,false) && window.plugins && window.plugins.touchid) {
			window.plugins.touchid.isAvailable(function(){
				 window.plugins.touchid.has(ENV.appName,function(){
					 $scope.pinkeyboardoptions.leftControl='<i class="icon ion-ios-paw"></i>';
					 window.plugins.touchid.verify(ENV.appName,$filter('translate')('APP_TOUCH_MSG'),function(code){
              AppUser.validatePin(code,true);
					 });
				 });
			});
		}
	}
	
  //Force the login page to lock on every load
  $scope.$on('$stateChangeSuccess',function(event,toState) {
     //console.log("State Change",event,toState);
     if (toState.name==="app.login" || toState.name==="app.logout") {
        //On Small phones remove the titel bar
        $scope.showTitle=$window.innerHeight>500;
        //Clear the entered pincode
        $scope.passcode="";
        //Remove the history
        $ionicHistory.clearHistory();
        //Lock the user
        AppUser.lock();
     }
    }); 
   //Disable and enable slideback of menu
   $scope.$on('$ionicView.enter', function(){
		 $ionicSideMenuDelegate.canDragContent(false);
		 $scope.showTouchId();
   });
   $scope.$on('$ionicView.leave', function(){$ionicSideMenuDelegate.canDragContent(true);});
  

	$scope.range = function(min, max, step) {
    step = step || 1;
    var input = [];
    for (var i = min; i <= max; i += step) {
        input.push(i);
    }
    return input;
};
})

;
