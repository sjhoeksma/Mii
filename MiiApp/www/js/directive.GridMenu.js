'use strict';
angular.module('directive.GridMenu',[	
])

/**
 * @ngdoc directive
 * @name directive.GridMenu:gridBasis
 * @element menu-basis
 * @param {string} id The name of the template must be set to "grid-template"
 *
 * @description
 * Create a home screen like application list with ions/badges or text. Scalling depending on the window size
 *
 * @example
   <example>
		<div  id="grid-template" grid-basis >
			 <div class="home-grid" ng-class="homePageStyle" back-to-grid back-state="app.home">
				<section class="grid">
				<a grid-state="{{item.state}}"  grid-params="{{item.params}}"  grid-title="{{item.title}}" grid-item class="grid__item" ng-repeat="item in userApps()" params-to-json="{{item.paramsToJson}}" grid-enabled="{{item.enabled}}"  >
					<div ng-if="homePageStyle=='badge' && item.short" >
					 <span ng-class="item.color ? item.color +'-badge-large' : 'black-badge-large'">{{item.short}}</span>
						<div class="title" translate>{{item.title}}</div>
				 </div>
				 <div ng-if="item.icon && (homePageStyle=='icon' || !item.short)">
					 <img ng-src="{{item.icon}}" class="full-image" />
					 <div class="title title--preview" translate>{{ item.title}}</div>
				 </div>
				 <div ng-if="!item.icon && (homePageStyle=='icon' || !item.short)">
					 <div class="title title--preview" translate>{{ item.title}}</div>
					 <div class="loader" ></div>
					 <div class="meta meta--preview" ng-bind-html="trustAsHtml(item.preview)"></div>
				 </div>
				</a>
			 </section>
		 </div>  
		</div>
	</example>
*/
.directive('gridBasis', function() {
    return {
      controller: ['$scope', '$rootScope', function($scope, $rootScope) { 
      $scope.translationData=angular.extend({},$scope.$parent.translationData,$rootScope.translantionData); 
       angular.element(document).find('head').prepend('<style type="text/css" id="gridBasis-css" >' +    
       '.grid {position: relative; }'+
       ".grid::after {content: '';position: absolute; pointer-events: none; top: 0; left: 0; width: 100%; height: 100%; -webkit-transition: opacity 0.5s; transition: opacity 0.5s; }" +
       '.placeholder,.grid__item { padding: 45px 55px 30px;position: relative;color: white;background: #0085c7;min-height: 150px;cursor: pointer;text-align: center;display: -webkit-box;display: -webkit-flex;display: -ms-flexbox;display: flex;-webkit-box-direction: normal;-webkit-box-orient: vertical;-webkit-flex-direction: column;-ms-flex-direction: column; flex-direction: column;-webkit-justify-content: center;justify-content: center; }' +
      '.grid__item__icon {padding: 0!important;margin:  0!important;}' +
      '.grid-icon {height: auto; width: auto; max-width: 50px; max-height: 50px;}'+
      '.grid-icon-col {max-width: 50px !important; display: block;margin-left: auto;margin-right: auto}'+
      // Placeholder element (dummy that gets animated when we click on an item) 
      '.placeholder {pointer-events: none;position: absolute;width: calc(100% + 5px); height: calc(100vh + 5px);z-index: 100;padding:10px;top: 0;left: 0;-webkit-transform-origin: 0 0; transform-origin: 0 0; font-size: 200%; line-height: 200% }'+
      // Grid item "border" 
      ".grid__item::before {position: absolute;content: '';top: 0px;right: 55px;bottom: 0px; left: 55px;border-bottom: 1px solid rgba(74, 74, 74, 0.075); }" +
      '.placeholder *,.grid__item * {z-index: 10; }'+
      // grid item element and full content element styles 
     '.placeholder .title, .grid__item .title { margin: 0;font-size: 1.5em; text-align: center; }'+
     '.placeholder .title--preview, .grid__item .title--preview {-webkit-flex: 1;flex: 1; display: -webkit-flex; display: flex; -webkit-align-items: center; align-items: center; -webkit-justify-content: center; justify-content: center; width: 100%; font-weight: bold;line-height: 30px; }'+
     '.placeholder .loader, .grid__item .loader {height: 1px;width: 40%;margin: 1em auto;position: relative;background: rgba(0, 0, 0, 0.1); }'+
     ".placeholder .loader::before,.grid__item .loader::before {content: '';position: absolute; width: 100%; height: 3px;background: #a4e7ad; left: 0; top: -1px; -webkit-transform: scale3d(0, 1, 1); transform: scale3d(0, 1, 1); -webkit-transform-origin: 0% 50%;transform-origin: 0% 50%; }"+
     '.placeholder--loading .loader::before, .grid__item--loading .loader::before {-webkit-transition: -webkit-transform 1s;transition: transform 1s;-webkit-transition-timing-function: cubic-bezier(0.165, 0.84, 0.44, 1); transition-timing-function: cubic-bezier(0.165, 0.84, 0.44, 1); -webkit-transform: scale3d(1, 1, 1); transform: scale3d(1, 1, 1); }'+
    '.grid__item .title--preview,.placeholder .loader,.grid__item .loader {-webkit-transition: -webkit-transform 0.2s, opacity 0.2s;transition: transform 0.2s, opacity 0.2s;-webkit-transition-timing-function: cubic-bezier(0.7, 0, 0.3, 1);transition-timing-function: cubic-bezier(0.7, 0, 0.3, 1); }'+
    // closing (delays)
    '.grid__item .title--preview {-webkit-transition-delay: 0.15s;transition-delay: 0.15s; }'+
    '.placeholder .loader,.grid__item .loader {-webkit-transition-delay: 0.1s;transition-delay: 0.1s; }'+
    // opening 
    '.grid__item--animate .title--preview {-webkit-transition-delay: 0s;transition-delay: 0s;opacity: 0;-webkit-transform: translate3d(0, -20px, 0); transform: translate3d(0, -20px, 0); }'+
   '.grid__item--animate .loader {-webkit-transition-delay: 0.1s;transition-delay: 0.1s;opacity: 0;-webkit-transform: translate3d(0, -30px, 0);transform: translate3d(0, -30px, 0); }'+
   '.grid__item .meta {font-size: 0.765em;text-align: left; }'+
   ".grid__item .meta:before, .meta:after { display: table;content: ''; }"+
   '.grid__item .meta:after {clear: both; }'+
   '.grid__item--disabled {opacity: 0.2;filter: alpha(opacity=20); background-color: #000;}' +	
   //* scroll helper wrap 
   '.scroll-wrap {position: absolute;width: 100%;height: 100%;left: 0;top: 0;z-index: 1;overflow-y: scroll;-webkit-overflow-scrolling: touch; }' +
   '.placeholder.placeholder--trans-in {-webkit-transition: -webkit-transform 0.5s;transition: transform 0.5s;-webkit-transition-timing-function: cubic-bezier(0.165, 0.84, 0.44, 1);transition-timing-function: cubic-bezier(0.165, 0.84, 0.44, 1); }'+
  '.placeholder.placeholder--trans-out {-webkit-transition: -webkit-transform 0.5s;transition: transform 0.5s; }'+
  // Viewport sizes based on column number  1 column, 2 columns , 3 columns , 4 columns , 5 columns , 6 columns 
  '@media screen and (min-width: 100px) {html,body,.container,.main { height: 50vh; }.main { height: 100%; }.grid__item {padding: 5px 20px 15px; }}'+
  '@media screen and (min-width: 300px) {.grid {display: -webkit-box;display: -webkit-flex;display: -ms-flexbox;display: flex;-webkit-flex-wrap: wrap; -ms-flex-wrap: wrap;flex-wrap: wrap; }.grid__item {width: 33.333%;border: none; }.grid__item::before {top: 5px;right: 5px;bottom: 5px;left: 5px;border: 1px solid rgba(74, 74, 74, 0.075);-webkit-transition: opacity 0.3s;transition: opacity 0.3s; } .grid__item:hover::before, .grid__item:focus::before { border: 3px solid white; }.grid__item--loading.grid__item::before {opacity: 0; }}'+
  '@media screen and (min-width: 600px) {.grid__item {width: 25%; } }'+
  '@media screen and (min-width: 900px) {.grid__item {width: 20%; } }'+
  '@media screen and (min-width: 1200px) {.grid__item { width: 16.666%; } }'+
  '@media screen and (min-width: 1400px) {.grid__item {width: 10%; } }'+
   // small screen changes for sidebar (it becomes an off-canvas menu) 
  '@media screen and (max-width: 299px) {.title--full {font-size: 2em; }}'+
 '</style>');
        var gridViewContainer = document.getElementById('grid-template');
        gridViewContainer.animationFlag = false;
        var docElem = window.document.documentElement;
        $scope.onEndTransition = function( el, callback ) {
          var onEndCallbackFn = function( ev ) {
            if( support.transitions ) {
              if( ev.target != this ) return;
              this.removeEventListener( transEndEventName, onEndCallbackFn );
            }
            if( callback && typeof callback === 'function' ) { callback.call(this); }
          };
          if( support.transitions ) {
            el.addEventListener( transEndEventName, onEndCallbackFn );
          } else {
            onEndCallbackFn();
          }
        };

        $scope.getViewport = function (axis){
          var client, inner;
          if( axis === 'x' ) {
            client = docElem['clientWidth'];
            inner = window['innerWidth'];
          } else if( axis === 'y' ) {
            client = docElem['clientHeight'];
            inner = window['innerHeight'];
          }
          return client < inner ? inner : client;
        }
        $scope.scrollX = function() { return window.pageXOffset || docElem.scrollLeft; }
        $scope.scrollY = function() { return window.pageYOffset || docElem.scrollTop; }
      }]
    }
  })
/**
 * @ngdoc directive
 * @name directive.GridMenu:gridItem
 * @element grid-item
 * @param {string}grid-state The state used to go to when clicked on the item
 * @param {object}grid-params The grid parmas passed to the state when clicking on item
 * @param {string}grid-title The Title to be used for item
 * @param {boolean}params-to-json When true parameters will be converted into a json string
 * @param {boolean}grid-enabled= When set to false item will be disabled and not clickable
 *
 * @description
 * A item shown within the GridBasis
*/ 
.directive('gridItem', function ($timeout, $state, $rootScope, $ionicScrollDelegate,$ionicHistory) {
    return {
    	require: '^^gridBasis',
        link: function (scope, elem, attrs) {
        	var gridViewContainer = document.getElementById('grid-template');
          angular.forEach(elem, function(item, pos) {
					 if (Boolean.fromString(attrs.gridEnabled,true)){
            item.addEventListener('click', function(ev) {
              angular.element(item).addClass('grid__item--loading');
              $timeout(function(){
                angular.element(item).addClass('grid__item--animate');
                  var params = attrs.gridParams;
                  if (!params) params = {Title: attrs.gridTitle};
                  if (params && typeof params === "string") 
                          params = angular.fromJson(params); 
                  if (attrs.disableAnimate) {
                     if (attrs.disableTransition) {$ionicHistory.nextViewOptions({disableAnimate: true});}
                     $state.go(attrs.gridState,Boolean.fromString(attrs.paramsToJson,false) ? {json: angular.toJson(params)} : params);
                  } else {
                    animateItem(item);              
                    gridViewContainer.animationFlag = true;
                    $timeout(function(){
                      if (attrs.disableTransition) {$ionicHistory.nextViewOptions({disableAnimate: true});}
                      $state.go(attrs.gridState,Boolean.fromString(attrs.paramsToJson,false) ? {json: angular.toJson(params)} : params);
                    }, 450);
                  } 
              }, 150);  
            })
					} else {
						 angular.element(item).addClass('grid__item--disabled');
					}
			})

			function animateItem(item) {
				var dummy = document.createElement('div');
        var adummy =  angular.element(dummy);
        //dummy.className = 'grid__item';
        adummy.addClass('placeholder'); 
				adummy.addClass('placeholder--loading');  
        adummy.html(angular.element(item).html());
        
        dummy.style.WebkitTransform = 'translate3d(' + (item.offsetLeft - 5) + 'px, ' + (item.offsetTop - 5) + 'px, 0px) scale3d(' + item.offsetWidth/gridViewContainer.offsetWidth + ',' + item.offsetHeight/scope.getViewport('y') + ',1)';
				dummy.style.transform = 'translate3d(' + (item.offsetLeft - 5) + 'px, ' + (item.offsetTop - 5) + 'px, 0px) scale3d(' + item.offsetWidth/gridViewContainer.offsetWidth + ',' + item.offsetHeight/scope.getViewport('y') + ',1)';
				adummy.addClass('placeholder--trans-in');
				gridViewContainer.appendChild(dummy);
				setTimeout(function() {
					dummy.style.WebkitTransform = 'translate3d(-5px, ' + (scope.scrollY() - 5) + 'px, 0px)';
					dummy.style.transform = 'translate3d(-5px, ' + (scope.scrollY() - 5) + 'px, 0px)';
					$ionicScrollDelegate.freezeAllScrolls(true);
				}, 25);
			 }
      }
    }
  })


/**
 * @ngdoc directive
 * @name directive.GridMenu:backToGrind
 * @element div
 * @param back-to-grid Set this to the contrainer where you would like to have animation in contained 
 * @param {string} back-state The state which should be seen a home state of the grid
 *
 * @description
 * Back to grid indentifies the container in which we will run the zoom in and zoom out aninimation
 * 
 * @example
 *  <div class="home-grid" ng-class="homePageStyle" back-to-grid back-state="app.home">
*/ 
	
.directive('backToGrid', function ($timeout, $rootScope, $ionicScrollDelegate) {
    return {
    	require: '^^gridBasis',
        link: function (scope, elem, attrs) {
        	var gridViewContainer = document.getElementById('grid-template');
        	$rootScope.$on('$stateChangeStart', function (event, toState, toParams, fromState) {
							if (toState.name === (attrs.backState || $rootScope.homeState || 'app.home') ) {
								 gridViewContainer.animationFlag = false;
								 hideContent();
							 }
					});
					gridViewContainer.hideContent=hideContent;

			function hideContent() {
				var dummy = gridViewContainer.querySelector('.placeholder'),
		    gridItem = elem[0].querySelector('a.grid__item--animate');
				$timeout(function() {
          try {gridViewContainer.removeChild(dummy);} catch (e){};
          angular.element(gridItem).removeClass('grid__item--loading');
          angular.element(gridItem).removeClass('grid__item--animate');				
          $ionicScrollDelegate.freezeAllScrolls(false);
        }, 50);
			}
    }
   }
})
;