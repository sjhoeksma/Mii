'use strict';
angular.module('directive.MenuTree',[	
])

/**
 * @ngdoc directive
 * @name directive.MenuTree:menuTree
 * @element menu-tree
 * @param {string} title When set the title will be shown as first item of menu
 * @param {array} menu-items The name of the scope variable containing the array menu structure. 
 *[
    {  "id":0,
		   "name": "Level 1.Item ",
       "icon": "ion-person",
       "style": {"font-weight": "bold"}
    },
    {
			"id": 20,
      "name": "Level 1.item",
      "icon": "ion-android-people",
      "items": [{ 
        "name": "Level 2.item",
        "icon": "ion-chatbox",
        "state": "app.feedback",
      },{
        "name": "TRANSLATABLE_ID",
        "icon": "ion-monitor",
        "state": "app.support",
      }]
    }
	 ]
 * @param {boolean} menu-state When set to true the menu item.id is seen as state (defaults to false) 
 * @param {function} menu-validate When set this function will be called foreach menuitem returning false will hide menu item. Also you can use the $validate item for this in your json structure
 * @restrict E
 *
 * @description
 * Create a colabsable menu based on a json file. Elements in the menu can be hidden based on conditions.
 * We support the translate module to do your translation.
 *
 * @example
   <example>
			<menu-tree menu-items="leftmenuItems" menu-validate="validateMenuItem"></menu-tree>
	</example>
*/
.directive('menuTree', function() {
  return {
    restrict: 'E',
    scope: {
      title: '@',
      menuItems: '=',
      menuState: '@', // default: FALSE
      menuValidate: '='
    },
    replace: false,
    template: 
     '<ion-list class="menu-tree">' +
    '<ion-item class="item-divider menu-title" ng-if="title">{{title | translate:translationData}}</ion-item>'+
    '<div ng-repeat="level1 in menuItems" >' +
    '<ion-item class="item-menu-1" ng-click="toggleMenu(level1)" ng-class="isActive(level1)" ng-class="level1.class" ng-style="level1.style" ng-show="isValidMenu(level1)" >' +
    '<i class="icon" ng-class="level1.icon" ></i>' +
    '{{level1.name | translate:translationData}}'+
    '<span class="item-note">'+
    '<i class="icon" ng-if="level1.items.length" ng-class="isMenuShown(level1) ? \'ion-chevron-down\' : \'ion-chevron-right\'"></i>'+
    '</span>'+
    '</ion-item>'+
    '<div ng-repeat="level2 in level1.items">'+
    '<ion-item class="item-menu-2" ng-click="toggleSubMenu(level2)" ng-show="isMenuShown(level1) && isValidMenu(level2)" ng-class="isActive(level2)" ng-class="level2.class" ng-style="level2.style">' +
    '<i class="icon" ng-class="level2.icon"></i>' +
    '{{level2.name | translate:translationData }}' +
    '<span class="item-note">'+ 
    '<i class="icon icon-inline" ng-if="level2.items.length" ng-class="isSubMenuShown(level2) ? \'ion-android-remove\' : \'ion-android-add\'"></i>' +
    '</span>'+
    '</ion-item>'+
    '<ion-item class="item-menu-3" ng-repeat="level3 in level2.items" ng-show="isSubMenuShown(level2) && isValidMenu(level3)" ng-click="checkLink(level3)" ng-class="isActive(level3)" ng-class="level3.class" ng-style="level2.style" >'+
    '<i class="icon" ng-class="level3.icon"></i>' +
    '{{level3.name | translate::translationData}}' +
    '</ion-item>'+
    '</div>'+
    '</div>'+
    '</ion-list>',
    controller: ['$scope','$rootScope','$ionicSideMenuDelegate', '$state', '$stateParams', '$ionicHistory', '$location', function($scope, $rootScope, $ionicSideMenuDelegate, $state, $stateParams, $ionicHistory, $location) {
      // Use ui-router's state to specify the active menu item
      var MENU_STATE = $scope.menuState || false;  
		  $scope.translationData=angular.extend({},$scope.$parent.translationData,$rootScope.translantionData);
      angular.element(document).find('head').prepend('<style type="text/css" id="menuTree-css">' +    
      '.menu-tree .item-menu-1, .menu-tree .item-menu-2, .menu-tree .item-menu-3{'+
        'border:none;line-height: 48px;padding-top: 0;padding-bottom: 0; color: white;' +
        'background: #387ef5;'+
  /*    'background: -webkit-gradient(linear, 301deg, color-stop(20%, #3E62F6), color-stop(90%, #7ABCFE));'+
        'background: -webkit-linear-gradient(301deg, #3E62F6 20%, #7ABCFE 90%);'+
        'background: -o-linear-gradient(301deg, #3E62F6 20%, #7ABCFE 90%);'+
        'background: -ms-linear-gradient(301deg, #3E62F6 20%, #7ABCFE 90%);*/    
      '}'+
      '.menu-tree .item-menu-1:hover,.menu-tree .item-menu-2:hover,.menu-tree .item-menu-3:hover {background-color: #86b0f9 !important}' +
      '.menu-tree .menu-title {background: #387ef5 !important; color: white; line-height: 1.8em; border-top:0;}'+
      '.menu-tree .item-menu-2 {padding-left:3em;}'+
      '.menu-tree .item-menu-3 {padding-left:5em;}'+
      '.menu-tree .item.menu-item-2.ng-hide,.menu-tree .item.menu-item-3.ng-hide { line-height: 0px;}'+
      '.menu-tree .active {color: white;'+
        'background: #073a92;'+                                             
  /*    'background: -webkit-gradient(linear, 301deg, color-stop(20%, #7ABCFE), color-stop(90%, #3E62F6));'+
        'background: -webkit-linear-gradient(301deg, #7ABCFE 20%, #3E62F6 90%);'+
        'background: -o-linear-gradient(301deg, #7ABCFE 20%, #3E62F6 90%);'+
        'background: -ms-linear-gradient(301deg, #7ABCFE 20%, #3E62F6 90%);'+ */
       '}'+
      '.menu-tree .icon {color: #fff;padding-right:8px;}'+
      '.menu-tree .icon-inline {position: inherit!important;display: inline-block!important;}'+
      '</style>');


      $scope.checkLink = function(item) {
        if (angular.isUndefined(item.items) || item.items.length == 0) {
          if (item.url) {
            $location.path(item.url);
          } else if (item.state) {
            $state.go(item.state,angular.extend({id: item.id, Title: item.name},item.params));
          }else if (MENU_STATE && item.id) {
            $state.go(MENU_STATE,angular.extend({id: item.id, Title: item.name},item.params)); 
          } else {
            console.log("No action from menu",item);
          }
          $scope.toggleLeft();
        }
      }

      $scope.isActive = function(item) {
        if ((item.url && $location.path().indexOf(item.url) !== -1) ||
            (MENU_STATE && $state.includes(MENU_STATE) && $stateParams.id == item.id)
           ) {
          return 'active';
        } 
      }
      
      $scope.isValidMenu = function(item){
				
        var result = true;
		 	  angular.forEach(item,function(value,key){
					if (key.charAt(0)=='$'  && typeof  item[key]=='function') {
							item[key.substring(1)]=item[key]($scope.$parent);
						  try{ 
								item[key.substring(1)]=angular.fromJson(item[key.substring(1)]);
							} catch (ex){}
					}
				});
        
        if (typeof $scope.menuValidate == "function") {
          result = $scope.menuValidate(item);
        }
        return result && Boolean.fromString(item.validate,true); 
      }

      $scope.toggleMenu = function(item) {
        $scope.checkLink(item);
        $scope.activeMenu = $scope.isMenuShown(item) ? null:  item;
        $scope.activeSubMenu = null;// Also hide all SubMenus
      };

      $scope.isMenuShown = function(item) {
        return $scope.activeMenu === item;
      };

      $scope.toggleSubMenu = function(item) {
        $scope.checkLink(item);
        $scope.activeSubMenu = $scope.isSubMenuShown(item) ? null : item;
      };

      $scope.isSubMenuShown = function(item) {
        return $scope.activeSubMenu === item;
      };

      $scope.toggleLeft = function() {
        $ionicSideMenuDelegate.toggleLeft();
        //$ionicHistory.nextViewOptions({disableBack: true});
      };
    }]
  }
})
;