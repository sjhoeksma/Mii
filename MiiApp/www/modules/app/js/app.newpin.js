'use strict';
angular.module('app',null)
.controller('NewPinCtrl', function($scope, $rootScope, $state, $window, $ionicHistory, $sce, $stateParams, AppUser, Notifications, $loadOnDemand,$timeout,ENV) {
 $scope.pinCount = ENV.pinCount || 4;	
 $scope.reason="";
 $scope.valid=true;
 $scope.AppUser=AppUser;
 $scope.validateReason=function(){
	 if ($scope.passcode.length <=$scope.pinCount){
			AppUser.isPinAllowed($scope.passcode).then(function(){
				$scope.reason="";
			},function(data){
				if (data && data.reason) {
					$scope.reason=data.reason;
					$scope.valid=false;
					return false;
				} else {
					$scope.reason="";
				}
			});
		}
	  $scope.valid=true;
	  return true;
 }
 $scope.newpinkey = {
		visible:true,
	  dummy:'test',
	  rigthControl: '<i class="icon ion-backspace-outline"></i>',
		leftControl: '<i class="icon ion-close-round"></i>',
    onKeyPress: function(value, source,event) {
      if (source === 'RIGHT_CONTROL' || (source == 'KEYBOARD' && value==8)) {
				$scope.passcode = $scope.passcode.substr(0, $scope.passcode.length - 1);
				$scope.validateReason();
      }else if (source == 'LEFT_CONTROL' || (source == 'KEYBOARD' && value==27) ) {
				$scope.reason="";
				if ($scope.passcode.length>$scope.pinCount) 
					$scope.passcode=$scope.passcode.substr(0,$scope.pinCount);
				else
					$scope.passcode="";
				$scope.validateReason();
      } else if (source === 'NUMERIC_KEY') {
				if ($scope.valid && $scope.passcode.length < $scope.pinCount*2) {
					if ($scope.passcode.length < $scope.pinCount*2)
						$scope.passcode = $scope.passcode + value;
					$scope.validateReason();
					if ($scope.passcode.length >= $scope.pinCount*2 &&
						 $scope.passcode.substr(0,$scope.pinCount)==$scope.passcode.substr($scope.pinCount)) {
						$timeout(function(){
							$scope.closeModal($scope.passcode.substr(0,$scope.pinCount));
						},50);
					}
				}
      } else return false;
			return true;
		}
  };
	$scope.pinCompare=function(pos){
		if ($scope.passcode.length>=pos+$scope.pinCount){
			if ($scope.passcode[pos-1]!=$scope.passcode[pos+$scope.pinCount-1]) return false;
		}
		return true;
	}
	
	$scope.passcode="";
	
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