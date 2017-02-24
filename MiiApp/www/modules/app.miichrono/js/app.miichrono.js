'use strict';
angular.module('app.miichrono')

.controller('MiiChronoSessionCtrl', function(
	$rootScope,$scope, $interval, MiiChronoService, AppUser, $loadOnDemand,$ionicPopup,$state,$stateParams,$ionicListDelegate
) {
	$scope.Mii = MiiChronoService;
	$scope.formatUser=MiiChronoService.formatUser;
	$scope.sessionName=$stateParams.sessionName;
	$scope.removeUser=MiiChronoService.removeUser;
	$scope.removeSession=function(){
		MiiChronoService.removeSession($stateParams.sessionName);
		$ionicListDelegate.closeOptionButtons();
		$rootScope.goBack();
	}
	$scope.resetSession=function(){
		MiiChronoService.resetSession($stateParams.sessionName);
		$ionicListDelegate.closeOptionButtons();
	}
	$scope.editUser = function(user) {
		if (!user){
			if (!$scope.nextUserId) $scope.nextUserId=1;
			while (MiiChronoService.users[$scope.nextUserId]) $scope.nextUserId++;
		}
		if (!$scope.data){
		  $scope.data = angular.extend({id:$scope.nextUserId},user);
		} else if (!user){
			$scope.data = {id:$scope.nextUserId,group:$scope.data.group,gender:$scope.data.gender};
		} else {
			$scope.data = angular.extend({},user); //Copy user
		}
		$scope.data.id=Number($scope.data.id);

		// An elaborate, custom popup
		var myPopup = $ionicPopup.show({
			template: '<input placeholder="user id" type="number" ng-model="data.id"><input placeholder="name" ng-model="data.name" focus-me ><input placeholder="group" ng-model="data.group"> <input placeholder="gender" ng-model="data.gender">',
			title: 'Enter new user',
			// subTitle: 'Subtitle',
			scope: $scope,
			buttons: [
				{ text: 'Cancel' },
				{
					text: '<b>Save</b>',
					type: 'button-positive',
					onTap: function(e) {
						if (!$scope.data.id || !$scope.data.name) {
							e.preventDefault();
						} else {
							return $scope.data;
						}
					}
				}
			]
		});
		myPopup.then(function(res) {
			if (res){
				if (user && user.id!=res.id) delete MiiChronoService.users[user.id]; 
				MiiChronoService.addUser(res.id,res);
				if (!user) $scope.nextUserId=$scope.data.id+1;
		 }
		});
	}

})

.controller('MiiChronoSettingsCtrl', function(
	$scope, $interval, MiiChronoService, AppUser, $loadOnDemand,$ionicPopup,$state
) {
	$scope.Mii = MiiChronoService;
	$scope.expertAllowed = true; //AppUser.hasRole('MiiChrono.expert');
	//Make power management options visible for Views
	$scope.hasPowermanagement =!(!window.powermanagement);
	$scope.settings= MiiChronoService.settings;
	$scope.init = function(){
		MiiChronoService.refreshBtList();
		if ($scope.settings.allwaysOn && window.powermanagement && !window.powermanagement.lock) {
			console.log("Disabling allways on settings");
			$scope.settings.allwaysOn=false;
		}
	}
	
	 $scope.addSession=function(){
		 $scope.data={name:""};
	// An elaborate, custom popup
		var myPopup = $ionicPopup.show({
			template: '<input ng-model="data.name" focus-me>',
			title: 'Add session',
			scope: $scope,
			buttons: [
				{ text: 'Cancel' },
				{
					text: '<b>Save</b>',
					type: 'button-positive',
					onTap: function(e) {
						if (!$scope.data.name) {
							e.preventDefault();
						} else {
							return $scope.data;
						}
					}
				}
			]
		});
	  myPopup.then(function(res) {
		 if (res && res.name) MiiChronoService.addSession(res.name);
    });
	}
	 
	$scope.editSession=function(sessionName){
		$state.go('app.miichrono-session',{sessionName:sessionName})
	}
})

.controller('MiiChronoCtrl', function(
	$scope, $interval, MiiChronoService,$ionicLoading,$timeout,$ionicPopup,$ionicPopover,$loadOnDemand
) {
		
	$scope.Mii = MiiChronoService;
				
	$scope.updateActiveTimes = function(){
		MiiChronoService.updateFreeze();
	  angular.forEach(MiiChronoService.activeList,function(timeRec){
			timeRec.fmtTime = MiiChronoService.formatTime(MiiChronoService.getTime()-timeRec.start,timeRec.state);		
		});
	}
	
	$scope.doChangeUser = function(data){
		 $scope.data={id:""};
	// An elaborate, custom popup
		var myPopup = $ionicPopup.show({
			template: '<input type="number" ng-model="data.id" focus-me>',
			title: 'New User ID',
			scope: $scope,
			buttons: [
				{ text: 'Cancel' },
				{
					text: '<b>Save</b>',
					type: 'button-positive',
					onTap: function(e) {
						if (!$scope.data.id) {
							e.preventDefault();
						} else {
							return $scope.data;
						}
					}
				}
			]
		});
	  myPopup.then(function(res) {
		 if (res && res.id) {
			 if ((data || $scope.popoverData).id!=res.id)
		       MiiChronoService.changeUser(data || $scope.popoverData,res.id);
			 if (!data) $scope.popover.hide();
		 }
    });
	}

	$scope.nextOnStart = function(){
		return MiiChronoService.formatUser(MiiChronoService.session.nextUser,true);
	}
	
	var lastSessionTime; //Cache for angular
	$scope.sessionTime = function(){
		var lst = MiiChronoService.formatTime(MiiChronoService.sessionTime()%86400000,0,false);
		if (lst!=lastSessionTime) lastSessionTime=lst;
		return lastSessionTime;
	}
	
	
	$scope.connect = MiiChronoService.btConnect;

	$scope.doDNF = function(data){
	  MiiChronoService.sendDNF(data);
		if (data) $scope.popover.hide();
	}
	
	$scope.changeUser = function(){
		$scope.nextUser="";
		//Only allow expert to changes settings
		if (MiiChronoService.settings.expert)
			$scope.keyboardoptions.visible=true;
	}
	
	$scope.doNoFinish = function(data){
		MiiChronoService.noFinish($scope.popoverData);
		if (!data) $scope.popover.hide();
	}
	$scope.doFixFinish = function(data){
		MiiChronoService.fixNoFinish(data || $scope.popoverData);
		if (!data) $scope.popover.hide();
	}
	
	$scope.inDeviceList = function(data){
		return MiiChronoService.inDeviceList(data || $scope.popoverData);
	}
	
	$scope.doDSQ = function(data){
		MiiChronoService.sendDSQ(data || $scope.popoverData);
		if (!data) $scope.popover.hide();
	}
	
	$scope.doRemove = function(data){
		MiiChronoService.sendREMOVE(data || $scope.popoverData);
		if (!data) $scope.popover.hide();
	}
	
	$ionicPopover.fromTemplateUrl($loadOnDemand.toModulePath('app.miichrono','app.miichrono-popover','html'), {
    scope: $scope
  }).then(function(popover) {
    $scope.popover = popover;
  });
	
	//User pressed long on time
	$scope.changeResultRec = function(timeRec,event){
		if (MiiChronoService.settings.expert){
			$scope.popoverData=timeRec;
		  $scope.popover.show(event);
		}
	}
	
	$scope.keyboardoptions = {
		visible:false,
		hideOnOutsideClick: true,
		leftControl: '<i class="icon ion-android-checkbox-outline"></i>',
    rightControl: '<i class="icon ion-backspace-outline"></i>',
    onKeyPress: function(value, source,event) {
			if (source === 'LEFT_CONTROL'  || (source == 'KEYBOARD' && value==13)) {
        $scope.keyboardoptions.visible = false;	
				MiiChronoService.setNextUser(parseInt($scope.nextUser));
      } else if (source === 'RIGHT_CONTROL' || (source == 'KEYBOARD' && value==8)) {
        $scope.nextUser = $scope.nextUser.substr(0, $scope.nextUser.length - 1);
      } else if (source === 'NUMERIC_KEY') {
        if ($scope.nextUser.length < 3) {
          $scope.nextUser += value;
        }
      } else if ((source == 'KEYBOARD' && value==27) || source=='CLICK_OUTSIDE')  { //Esc
					$scope.keyboardoptions.visible = false;	
			} else return false; 
			return true;
    }
  };
	
	var timer= $interval(function(){
		$scope.updateActiveTimes();
	},250);
	
	var idCount =1;
	//Test time freeze
	$scope.freeze = function(){
 		if (MiiChronoService.activeList[0]) {
		var rec = angular.copy(MiiChronoService.activeList[0]);
		rec.time=MiiChronoService.getTime()-rec.start;
		rec.state=200;
		MiiChronoService.add(rec);
		} else {
		MiiChronoService.add({
			  id : idCount++
			  ,start: MiiChronoService.getTime()
			  ,time: 7317
			  ,state: 10
			})	
		}
	}
	
    /*
     * if given group is the selected group, deselect it
     * else, select the given group
     */
    $scope.toggleGroup = function(group) {
      if ($scope.isGroupShown(group)) {
        $scope.shownGroup = null;
      } else {
        $scope.shownGroup = group;
      }
    };
    $scope.isGroupShown = function(group) {
      return $scope.shownGroup === group;
    };
	
	//For the button bar
	$scope.resultSort = "-lastStart";
	$scope.rank = 0;
	$scope.toggleRank = function() {
		$scope.rank++;
		if ($scope.rank>2) $scope.rank=0;
		$scope.resultSort = $scope.rank==1 ? "rankSort" : $scope.rank==2 ? "id" : "-lastStart" ;
	};
     
 })

;