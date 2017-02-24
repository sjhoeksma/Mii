'use strict';
angular.module('directive.IonSearchSelect',[	
])
//ionSearchSelect Combobox and search in one
//<div><ion-select label="Search" label-field="lastname" provider="employees"  ng-model="mPeopleFinder" placeholder="Search" on-select="myCallBack(key,item)" limit="10"/></div> 
.directive('ionSearchSelect', function($timeout) {
  return {
    restrict: 'EAC',
    scope: {
      label: '@',
      labelField: '@',
      placeholder: '@',
      limit: '@',
      provider: '=',
      ngModel: '=?',
      ngValue: '=?',
      onSelect: '&'
    },
    require: '?ngModel',
    transclude: false,
    replace: false,
    template: '<div class="searchSelectContainer">' +
      '<div class="item item-input item-stacked-label">' +
      '<span ng-if="label" class="input-label" translate translate-values="{{translationData}}">{{label}}</span>' +
      '<div class="item item-input-inset">' +
      '<label class="item-input-wrapper">' +
      '<i class="icon ion-ios-search placeholder-icon"></i>' +
      '<input id="location_filter" class="search" placeholder="{{placeholder | translate:translationData}}"  type="search" ng-model="ngModel" ng-value="ngValue" ng-keydown="onKeyDown($event)"/>' +
      '</label>' +
      '<button class="button button-small" ng-click="openList()">' +
      '<i class="icon ion-chevron-down"></i>' +
      '</button>' +
      '</div>' +
      '</div>' +
      '<div class="optionList padding-left padding-right" ng-show="showHide">' +
      '<ion-scroll>' +
      '<ul class="list">' +
      '<li class="item" ng-click="selectItem(item)" ng-repeat="item in provider | dynamicFilter:[labelField,ngModel] | limitTo : (limit ? limit : 20)" translate translate-values="{{translationData}}" >{{item[labelField]}}</li>' +
      '</ul>' +
      '</ion-scroll>' +
      '</div>' +
      '</div>',
    controller: ['$scope', '$rootScope', function($scope, $rootScope) {
       $scope.translationData=angular.extend({},$scope.$parent.translationData,$rootScope.translantionData);
       angular.element(document).find('head').prepend('<style type="text/css" id="ionSearchSelect-css" >' +   
      '.optionList{position:absolute;width:100%;z-index:100;}' +
      '.searchSelectContainer {border:0px;}'+
      '.searchSelectContainer .item{padding:0px; }'+
      '</style>');
    }],
    link: function(scope, element, attrs, ngModel) {
      scope.ngValue = scope.ngValue !== undefined ? scope.ngValue : '';
      scope.selectItem = function(item) {
        ngModel.$setViewValue(item);
        scope.showHide = false;
        if (typeof scope.onSelect == 'function') {
          scope.onSelect({'item':item,'key':element.find('input').val()});
        }
      };

      element.bind('click', function() {
        element.find('input').triggerHandler('focus');
      });

      scope.openList = function() {
        scope.ngModel = undefined;
        $timeout(function() {
          return scope.showHide = !scope.showHide;
        }, 100);
      };
      scope.onKeyDown = function(event) {
        var char = event.which || event.keyCode;
        if (char == 27) {
          //clean out the input and close restore 
          element.find('input').val(scope.ngValue);
          scope.showHide=false;
        } else if (char == 13) {
          scope.showHide=false;
          var items=element.find('li');
          if (items.length!=0) {
            $timeout(function() {
            angular.element(items[0]).triggerHandler('click'); 
            },0);
          } else {
            scope.onSelect({key:element.find('input').val(),item:null});
          }
         
          //Fire click event on first element of the list
        } else scope.showHide = true;
      };

      scope.$watch('ngModel', function(newValue, oldValue) {
        if (newValue !== oldValue) {
          if (scope.showHide === false) {
            element.find('input').val(newValue ? newValue[scope.labelField] : '');
          }
        }
        if (!scope.ngModel) {
          scope.showHide = false;
        }
      });

    },
  };
})
;