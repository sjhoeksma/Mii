'use strict';
//=========================================================
//Common AngularJS code used in multiple projects
//=========================================================
angular.module('directive.Common',[])

//=========================================================
//Directives
//=========================================================
 .directive('toggleClass', function() {
    return {
        restrict: 'A',
        link: function(scope, element, attrs) {
            element.bind('click', function() {
                element.toggleClass(attrs.toggleClass);
            });
        }
    };
 })

.directive("hideKeyboardOnEnter", function ($window) {
    return {
        restrict: "A",
        link: function (scope, element) {
            if ($window.cordova && $window.cordova.plugins.Keyboard) {
                element.bind("keyup", function (key) {
                    if (key.keyCode === 13) {
                        $window.cordova.plugins.Keyboard.close();
                        element[0].blur();
                    }
                });
            }
        }
    };
})

.directive('iframeOnload', function(){
  return {
    scope: {
        callBack: '&iframeOnload'
    },
    link: function(scope, element, attrs){
        element.on('load', function(){
            return scope.callBack();
        })
    }
	}
})

/**
 * @ngdoc directive
 * @name directive.Common:textarea
 * @element textarea
 * @function
 * @restrict E
 *
 * @description
 * Resizes textareas automatically to the size of its content. Extends the regular textarea element.
 *
 * **Note:** The auto resizing only works if there is a ng-model defined on the element.
 * or if the auto-size attribute is set
 *
 * @example
   <example module="main">
     <file name="example.html">
         <textarea ng-model="testModel"></textarea>
     </file>
   </example>
*/
.directive('textarea', function () {
	return {
		restrict: 'E',
		link: function (scope, element, attr) {
			if (attr.hasOwnProperty("ngModel") || 
					(attr.hasOwnProperty("autoSize") && Boolean.fromString(attr.autoSize,false))){
				var update = function () {
					element.css("height", "auto");
					var height = element[0].scrollHeight;
					if (height !== 0) { element.css("height", height + "px"); } 
				};
				scope.$watch(attr.ngModel, function () {
					update();
				});
			}
		}
	};
 })

//Used toCompare to values in ngMessages
.directive("compareTo", function () {
  return {
    require: "ngModel"
    , scope: {
      otherModelValue: "=compareTo"
    }
    , link: function (scope, element, attributes, ngModel) {

      ngModel.$validators.compareTo = function (modelValue) {
        return (modelValue == scope.otherModelValue);
      };

      scope.$watch("otherModelValue", function () {
        ngModel.$validate();
      });
    }
  };
})

//Directive to do a focus of element
.directive('focusMe', ['$timeout', function ($timeout) {
  return {
    link: function (scope, element, attrs) {
      if (attrs.focusMeDisable === "true") {
        return;
      }
      $timeout(function () {
				element = element[0] || element;
				element.focusMeKeyboard=attrs.focusMeKeybaord !== "false"
				if (!(element.disabled || element.readonly)) {
          //element.triggerHandler('focus');
		      element.focus();
      	  if (element.focusMeKeyboard && ionic.Platform.isAndroid() &&
						window.cordova && window.cordova.plugins && cordova.plugins.Keyboard) {
         	  //  cordova.plugins.Keyboard.disableScroll(true); //Enable scrolling of keyboard
              cordova.plugins.Keyboard.show(); //open keyboard manually
					}
        }
      }, 350); //Slow down because of transitions
    }
  };
}])


//=========================================================
//Filters
//=========================================================
//Add filter option to your data by key
.filter('objectByKeyFilter', function () {
return function (input, filterKey, filterVal) {
    var filteredInput ={};
     angular.forEach(input, function(value, key){
       if(value[filterKey] && value[filterKey].toString().indexOf(filterVal)>=0) {
          filteredInput[key]= value;
        }
     });
     return filteredInput;
}})

.filter('dynamicFilter', ["$filter", function ($filter) {
    return function (array, keyValuePairs) {
        var obj = {}, i;
        for (i = 0; i < keyValuePairs.length; i += 2) {
            if (keyValuePairs[i] && keyValuePairs[i+1]) {
                obj[keyValuePairs[i]] = keyValuePairs[i+1];
            }
        }
        return $filter('filter')(array, obj);
    }
}])

/* Filter to convert a object to an array
 <div ng-repeat="release in releases | object2Array | orderBy:'environment_id'">{{release.environment_id}}</div>
*/
.filter('object2Array', function() {
	return function(input) {
		var out = []; 
		for(var i in input){
			out.push(input[i]);
		}
		return out;
	}
})

//Factory to do a focus an element by object id
.factory('focusMe', ['$timeout', '$window', function($timeout, $window) {
    return function(id) {
	    // timeout makes sure that it is invoked after any other event has been triggered.
      // e.g. click events that need to run before the focus or
      // inputs elements that are in a disabled state but are enabled when those events
      // are triggered.
      $timeout(function() {
	      var element = typeof id ==="string" ? $window.document.getElementById(id) : id;
				element = element[0] || element;
				if(element && !(element.disabled || element.readonly)) {
						element.triggerHandler('focus');
					  if (element.focusMeKeyboard  && ionic.Platform.isAndroid() &&
							window.cordova && window.cordova.plugins && window.cordova.plugins.Keyboard) {
         	   //	cordova.plugins.Keyboard.disableScroll(true); //Enable scrolling of keyboard
              cordova.plugins.Keyboard.show(); //open keyboard manually
	          }
			  }
      }, 450);
    };
 }])

;