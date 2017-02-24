'use strict';
angular.module('directive.IonKeyPad',[	
])
/**
 * @ngdoc directive
 * @name directive.IonKeyPad:ionKeyPad
 * @element ion-key-pad
 * @param {object}options A object describing the options to the used for the keypad
 * $scope.options = {
		visible:true,
	  dummy:'test',
		hideOnOutsideClick: false,
	  rigthControl: '<i class="icon ion-backspace-outline"></i>',
		leftControl: '<i class="icon ion-close-round"></i>',
    onKeyPress: function(value, source,event) {
      if (source === 'RIGHT_CONTROL' || (source == 'KEYBOARD' && value==8)) {
			
      }else if (source == 'LEFT_CONTROL' || (source == 'KEYBOARD' && value==27) ) {
				
      } else if (source === 'NUMERIC_KEY') {
				
      } else return false;
			return true;
		}
  };
 * @description
 * Showing a numeric keypad, with configurable topmenu and 2 bottom buttons. Can be connected to a input box 
 *
 * @example
   <example>
			 <ion-key-pad class="flat"  options="options"></ion-key-pad>
	</example>
*/
.directive('ionKeyPad', ['$document', '$timeout', '$compile',
						function($document, $timeout, $compile) {

    /**
     * default options
     */
    var defaultsOpts = {
        visible: true, // visible
        hideOnOutsideClick: false, // do not hide by default to ensure backward compatibility
        leftControl: null, // no left control
        rightControl: '<i class="icon ion-backspace-outline"></i>',
        onKeyPress: angular.noop,
        button: null,
			  data: null
    };

    var resizeIonContent = function(isKeyboardVisible, ionContentElem, hasTopBar) {
      if (ionContentElem) {
        if (isKeyboardVisible) {
          ionContentElem.addClass('has-ion-key-pad');
          if (hasTopBar) {
            ionContentElem.addClass('has-ion-key-pad-top-bar');
          }
        }
        else {
          ionContentElem.removeClass('has-ion-key-pad');
          ionContentElem.removeClass('has-ion-key-pad-top-bar');
        }
      }
    }

    var self ={
      restrict: 'E',
      replace: true,
      template: '<div id="ionNumbericKeyBoard-template" click-outside="hide()" outside-if-not="key-pd-source" class="ion-key-pad ng-hide" ng-show="opts.visible" ng-class="{\'with-top-bar\': opts.button}">' +
                  '<div class="row ion-key-pad-top-bar" ng-show="opts.button">' +
                    '<div class="col">' +
                      '<button class="{{opts.button.class}}" ng-click="opts.button.onClick()" ng-bind-html="opts.button.content"></button>' + 
                    '</div>' + 
                  '</div>' +
                  '<div class="row">' +
                    '<button class="col key button" ng-click="onKeyPress(1, \'NUMERIC_KEY\',$event)" ng-bind-html="1"></button>'+
                    '<button class="col key button" ng-click="onKeyPress(2, \'NUMERIC_KEY\',$event)" ng-bind-html="2"></button>'+
                    '<button class="col key button" ng-click="onKeyPress(3, \'NUMERIC_KEY\',$event)" ng-bind-html="3"></button>'+
                  '</div>' +
                  '<div class="row">' +
                    '<button class="col key button" ng-click="onKeyPress(4, \'NUMERIC_KEY\',$event)" ng-bind-html="4"></button>'+
                    '<button class="col key button" ng-click="onKeyPress(5, \'NUMERIC_KEY\',$event)" ng-bind-html="5"></button>'+
                    '<button class="col key button" ng-click="onKeyPress(6, \'NUMERIC_KEY\',$event)" ng-bind-html="6"></button>'+
                  '</div>' +
                  '<div class="row">' +
                    '<button class="col key button" ng-click="onKeyPress(7, \'NUMERIC_KEY\',$event)" ng-bind-html="7"></button>'+
                    '<button class="col key button" ng-click="onKeyPress(8, \'NUMERIC_KEY\',$event)" ng-bind-html="8"></button>'+
                    '<button class="col key button" ng-click="onKeyPress(9, \'NUMERIC_KEY\',$event)" ng-bind-html="9"></button>'+
                  '</div>' +
                  '<div class="row">' +
                    '<button class="col key button control-key left-control-key" ng-click="onKeyPress(opts.leftControl, \'LEFT_CONTROL\',$event)" ng-bind-html="opts.leftControl" ng-show="opts.leftControl"></button>'+
                    '<div class="col key  control-key right-control-key" ng-show="!opts.leftControl"></div>' +
                    '<button class="col key button" ng-click="onKeyPress(0, \'NUMERIC_KEY\',$event)" ng-bind-html="0"></button>'+
                    '<button class="col key button control-key right-control-key" ng-click="onKeyPress(opts.rightControl, \'RIGHT_CONTROL\',$event)" ng-bind-html="opts.rightControl" ng-show="opts.rightControl"></button>'+
                    '<div class="col key  control-key right-control-key" ng-show="!opts.rightControl"></div>' +
                  '</div>' +
                '</div>',
      scope: {
            options: '='
            
      },
      link: function(scope, element, attr) {
        // add default css to <head>
         angular.element(document).find('head').prepend('<style type="text/css" id="ionKeyPad-css" >' + 
        '.ion-key-pad {z-index: 12;bottom: -5px;left: 0;right: 0;position: absolute;width: 100%;}' +
        '.backdrop-key-pd {background-color: transparent;}' +
         //'.ion-key-pad .ion-key-pad-top-bar {border-top: 1px #eee solid;border-bottom: 1px #eee solid;}'+                                                 
        '.ion-key-pad .row {padding: 0;margin:0;}' +
        '.ion-key-pad .key {border: 0;border-radius: 0;padding: 0;background-color: transparent;font-size: 195%;border-style: solid;color: #fefefe;border-color: #444;background-color: #333;margin-bottom:5px;}' +
        '.ion-key-pad .key.control-key { background-color: #242424;}' +
        '.ion-key-pad .key.activated {box-shadow: inset 0 1px 4px rgba(0, 0, 0, .1);background-color: rgba(68, 68, 68, 0.5);}' +
        '.ion-key-pad .row:nth-child(1) .key {border-top-width: 1px;}.ion-key-pad .row:nth-child(1) .key,.ion-key-pad .row:nth-child(2) .key,.ion-key-pad .row:nth-child(3) .key ,.ion-key-pad .row:nth-child(4) .key { border-bottom-width: 1px;}' +
        '.ion-key-pad .row .key:nth-child(1),.ion-key-pad .row .key:nth-child(2), .ion-key-pad .row .key:nth-child(3), .ion-key-pad .row .key:nth-child(4) {border-right-width: 1px;}' +
        '.ion-key-pad-top-bar {background-color: #242424;}' +
                                                        
        '.ion-key-pad.flat .key {border: none;}' +
      //  '.ion-key-pad.flat .key.control-key {background-color: #fff;}'+
        '.ion-key-pad.flat .ion-key-pad-top-bar{border-top: none !important;border-bottom: none !important;}' +
                                                        
        '.ion-key-pad.transparent .key {background-color: rgba(230, 230, 230, 0.2);color: #000;border-color: #8e8eb8;}'+
                                                        
        '.ion-key-pad.blue .key {background-color: rgba(230, 230, 230, 0.2);color: #fff;border-color: #8e8eb8;}'+                         
        '.ion-key-pad.blue .key.control-key,.ion-key-pad.transparent .key.control-key {background-color: rgba(230, 230, 230, 0.4);}'+
        '.ion-key-pad.blue .key.activated,.ion-key-pad.transparent .key.activated {box-shadow: inset 0 1px 4px rgba(0, 0, 0, .1);background-color: rgba(230, 230, 230, 0.5);}'+
         '.ion-key-pad.blue { background: #387ef5;}'+
         '.ion-key-pad.blue .ion-key-pad-top-bar,.ion-key-pad.transparent .ion-key-pad-top-bar {background-color: rgba(230, 230, 230, 0.2);border-top: 1px #c1c1c1 solid;border-bottom: 1px #8e8eb8 solid;margin-bottom:5px;}'+
         '.ion-key-pad.blue .ion-key-pad-top-bar button ,.ion-key-pad.transparent .ion-key-pad-top-bar button {color: #fff;}'+
                                                        
         '.ion-key-pad.white .key {background-color: #fff;color: #5995DC;border-color: #eee;}'+
         '.ion-key-pad.white .key.control-key {background-color: #fafafa;}'+
         '.ion-key-pad.white .key.activated {box-shadow: inset 0 1px 4px rgba(0, 0, 0, .1);background-color: rgba(230, 230, 230, 0.5);}'+
         '.ion-key-pad.white .ion-key-pad-top-bar {background-color: #fafafa;border-top: 1px #eee solid;border-bottom: 1px #eee solid;}'+
         '.ion-key-pad.white .ion-key-pad-top-bar button {color: #5995DC;}'+
                                                        
        '.has-ion-key-pad {bottom: 188px;}' +
        '.has-ion-key-pad.has-ion-key-pad-top-bar {bottom: 228px;}' +
                                                     
         '.ion-key-pad.slide-up {-webkit-transform: translate3d(0,0,0); transform: translate3d(0,0,0);-webkit-backface-visibility: hidden; backface-visibility: hidden; -webkit-perspective: 1000;perspective: 1000;}' +
        '.ion-key-pad.slide-up.ng-hide-remove.ng-hide-remove-active {-webkit-animation:250ms slide-up;animation:250ms slide-up;}' +
        '.ion-key-pad.slide-up.ng-hide-add.ng-hide-add-active {-webkit-animation:250ms slide-down;animation:250ms slide-down;}'+
        '.ion-key-pad.with-top-bar.slide-up.ng-hide-remove.ng-hide-remove-active {-webkit-animation:250ms slide-up-with-top-bar;animation:250ms slide-up-with-top-bar;}'+
        '.ion-key-pad.with-top-bar.slide-up.ng-hide-add.ng-hide-add-active {-webkit-animation:250ms slide-down-with-top-bar; animation:250ms slide-down-with-top-bar;}'+
     '</style>');
        // ion content element
        var ionContentElem = element.parent().find('ion-content');

        scope.hide = function() {
          if (scope.opts.hideOnOutsideClick && scope.opts.onKeyPress(0,"CLICK_OUTSIDE")!==true) {
						scope.opts.visible = false;
						resizeIonContent(scope.opts.visible, ionContentElem);
          }
        }
			
				//We have problem that sometimes options are read later, so enable filling through watch
				scope.optionsSet = !!scope.options;
        // merge options
        scope.opts = angular.merge({}, defaultsOpts, scope.options || {});
			
        // if the keyboard is visible, the ion content needs to be resized
        resizeIonContent(scope.opts.visible, ionContentElem, scope.opts.button !== null);

        // watch the options to update the visibility of the keyboard
        scope.$watchCollection('options', function(newValue, oldValue){
          if (newValue !== oldValue || !scope.optionsSet) {
						scope.optionsSet=true;
            scope.opts = angular.merge({}, defaultsOpts, newValue);
            resizeIonContent(scope.opts.visible, ionContentElem, scope.opts.button !== null);
          }
        });
				
				scope.onKeyPress= function(key,source,event) {
					if (window.nativeclick) window.nativeclick.trigger();
					return scope.opts.onKeyPress(key,source,event);
				};
				
				//When cordova is installed assume we don't have native keyboard attached
				if (!window.cordova) {					
					var visible = function (el) {
						if (el.length) el=el[0];
						var rect = el.getBoundingClientRect();
						var clw = (window.innerWidth || document.documentElement.clientWidth);
						var clh = (window.innerHeight || document.documentElement.clientHeight) ;
						// checks if element is fully visible
						//return (rect.top >= 0 && rect.bottom <= clh) && (rect.left >= 0 && rect.right <= clw) && 
						//	      (rect.bottom-rect.top!=0 && rect.right-rect.left!=0);
						
						

						// checks if part of element is visible
						if (!(rect.left <= clw && 0 <= rect.right && rect.top <= clh && 0 <= rect.bottom) &&
						       (rect.bottom-rect.top!=0 && rect.right-rect.left!=0)) return false;
						
						var backDrop = document.querySelector('.modal-backdrop');
						if (backDrop && angular.element(backDrop).hasClass('active')){
							return angular.element(backDrop.querySelector('#ionNumbericKeyBoard-template'))[0]==element[0];
						}
						return true;
					};
					
					var keySound= function(){
						if (window.cordova && window.cordova.nativeclick)
						  window.cordova.nativeclick.trigger();
					}
						
					var keyHandler = function(e){
						var el = angular.element(element); 
					
						if(el.hasClass("ng-hide") || !scope.opts.visible || !visible(el)){
							 return;
						}
						
						if (!e.altKey && !e.ctrlKey && !e.metaKey ) {
						  var key = (e.which || e.keyCode) * (e.shiftKey ? 1000 : 1);
							if (((key>=48 && key<=59) || key==224)) { //(0)1-9 || 0 on firefox
					        scope.opts.onKeyPress(String.fromCharCode(key),"NUMERIC_KEY",e);
								  keySound();
								  e.preventDefault();
							} else if (key && (scope.opts.onKeyPress(key,"KEYBOARD",e) || (key==8))) {
								  keySound();
									e.preventDefault();
							}
							scope.$apply();
						}
					}

					$document.on('keydown',keyHandler);
					scope.$on('$destroy', function() {
						 $document.off('keydown', keyHandler);
					});
				}
   		
      }
    };
		return self;
  }])  

 //an angular directive to detect a click outside of an elements scope
 .directive('clickOutside', ['$document', '$parse', function ($document, $parse) {
    return {
      restrict: 'A',
      link: function($scope, elem, attr) {
        var classList = (attr.outsideIfNot !== undefined) ? attr.outsideIfNot.replace(', ', ',').split(',') : [],
          fn = $parse(attr['clickOutside']);

        // add the elements id so it is not counted in the click listening
        if (attr.id !== undefined) {
          classList.push(attr.id);
        }

        var eventHandler = function(e) {
          //check if our element already hiden
          if(angular.element(elem).hasClass("ng-hide")){
              return;
          }

          var i = 0,
              element;

          // if there is no click target, no point going on
          if (!e || !e.target) {
              return;
          }

          // loop through the available elements, looking for classes in the class list that might match and so will eat
          for (element = e.target; element; element = element.parentNode) {
            var id = element.id,
                classNames = element.className,
                l = classList.length;

            // Unwrap SVGAnimatedString
            if (classNames && classNames.baseVal !== undefined) {
              classNames = classNames.baseVal;
            }

            // loop through the elements id's and classnames looking for exceptions
            for (i = 0; i < l; i++) {
              // check for id's or classes, but only if they exist in the first place
              if ((id !== undefined && id.indexOf(classList[i]) > -1) || (classNames && classNames.indexOf(classList[i]) > -1)) {
                // now let's exit out as it is an element that has been defined as being ignored for clicking outside
                return;
              }
            }
          }

          // if we have got this far, then we are good to go with processing the command passed in via the click-outside attribute
          return $scope.$apply(function () {
            return fn($scope);
          });
        };
				
        // assign the document click handler to a variable so we can un-register it when the directive is destroyed
        $document.on('click', eventHandler);
				
        // when the scope is destroyed, clean up the documents click handler as we don't want it hanging around
        $scope.$on('$destroy', function() {
          $document.off('click', eventHandler);
        });
      }
    };
 }])


.directive('onFinishRender', function ($timeout) {
    return {
        restrict: 'A',
        link: function (scope, element, attr) {
            if (scope.$last === true) {
                $timeout(function () {
                    scope.$emit('ngRepeatFinished');
                });
            }
        }
    }
})
;