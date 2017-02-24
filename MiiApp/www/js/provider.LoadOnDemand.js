'use strict';
angular.module('provider.LoadOnDemand',[
  'formly'  //The form wizard
])
//Load Module Directive
.factory('scriptCache', ['$cacheFactory', function ($cacheFactory) {
		return $cacheFactory('scriptCache', {
				capacity: 50
		});
} ])

.provider('$loadOnDemand',
['$controllerProvider', '$provide', '$compileProvider', '$filterProvider','$stateProvider','formlyConfigProvider',
function ($controllerProvider, $provide, $compileProvider, $filterProvider,$stateProvider,formlyConfigProvider) {
   var modules = { },
		tours = { },
	  settings ={},
		applications={},
		modulePath = '',
		menuContent = '',
		menus=[],
		regModules = [], 
		providers = {
				$controllerProvider: $controllerProvider,
				$compileProvider: $compileProvider,
				$filterProvider: $filterProvider,
				$provide: $provide, // other things
	      $stateProvider:$stateProvider,
			  formlyConfigProvider:formlyConfigProvider
		};
	  var configListeners = [];
	
	  function _addConfigListener(listener){
			for (var i=0,l=configListeners.length;i<l;i++){
				if (configListeners[i]==listener) return;
			}
			configListeners.push(listener);
		}
		
		function _removeConfigListener(listener){
			for (var i=0,l=configListeners.length;i<l;i++){
				if (configListeners[i]==listener) {
					configListeners.splice(i,1);
					return;
				}
			}
		}

	  function getRequires(module) {
        var requires = [];
        angular.forEach(module.requires, function (requireModule) {
            if (regModules.indexOf(requireModule) == -1) {
                requires.push(requireModule);
            }
        });
        return requires;
    }
	
    function moduleExists(moduleName) {
        try {
            angular.module(moduleName);
        } catch (e) {
            if (/No module/.test(e)) {
                return false;
            }
        }
        return true;
    }
	
    function register($injector, providers, registerModules) {
        var i, ii, k, invokeQueue, moduleName, moduleFn, invokeArgs, provider;
        if (registerModules) {
            var runBlocks = [];
            for (k = registerModules.length-1; k >= 0; k--) {
                moduleName = registerModules[k];
                regModules.push(moduleName);
                moduleFn = angular.module(moduleName);
                runBlocks = runBlocks.concat(moduleFn._runBlocks);
                try {
                    for (invokeQueue = moduleFn._invokeQueue, i = 0, ii = invokeQueue.length; i < ii; i++) {
                        invokeArgs = invokeQueue[i];

                        if (providers.hasOwnProperty(invokeArgs[0])) {
                            provider = providers[invokeArgs[0]];
                        } else {
                            return console.error("unsupported provider " + invokeArgs[0]);
                        }
                        provider[invokeArgs[1]].apply(provider, invokeArgs[2]);
                    }
                } catch (e) {
                    if (e.message) {
                        e.message += ' from ' + moduleName;
                    }
                    console.error("Failed to register",moduleName,e.message);
                    throw e;
                }
                registerModules.pop();
            }
            angular.forEach(runBlocks, function(fn) {
                $injector.invoke(fn);
            });
        }
        return null;
    }
	  function init(element) {
        var elements = [element],
            appElement,
            module,
            names = ['ng:app', 'ng-app', 'x-ng-app', 'data-ng-app'],
            NG_APP_CLASS_REGEXP = /\sng[:\-]app(:\s*([\w\d_]+);?)?\s/;

        function append(elm) {
            elm && elements.push(elm);
        }

        angular.forEach(names, function (name) {
            names[name] = true;
            append(document.getElementById(name));
            name = name.replace(':', '\\:');
            if (element.querySelectorAll) {
                angular.forEach(element.querySelectorAll('.' + name), append);
                angular.forEach(element.querySelectorAll('.' + name + '\\:'), append);
                angular.forEach(element.querySelectorAll('[' + name + ']'), append);
            }
        });

        angular.forEach(elements, function (elm) {
            if (!appElement) {
                var className = ' ' + element.className + ' ';
                var match = NG_APP_CLASS_REGEXP.exec(className);
                if (match) {
                    appElement = elm;
                    module = (match[2] || '').replace(/\s+/g, ',');
                } else {
                    angular.forEach(elm.attributes, function (attr) {
                        if (!appElement && names[attr.name]) {
                            appElement = elm;
                            module = attr.value;
                        }
                    });
                }
            }
        });
        if (appElement) {
            (function addReg(module) {
                if (regModules.indexOf(module) == -1) {
                    regModules.push(module);
                    var mainModule = angular.module(module);
                    angular.forEach(mainModule.requires, addReg);
                }
            })(module);
        }
    }
	
   this.$get = ['scriptCache', '$timeout',  '$document', '$injector','$state','$q','$http', '$translatePartialLoader', '$translate','$interpolate','$rootScope','$ionicModal','$controller','$sce',
		function (scriptCache, $timeout,$document, $injector,$state, $q, $http, $translatePartialLoader, $translate, $interpolate,$rootScope,$ionicModal,$controller,$sce ) {
			
			 var interploteSubMenu=function(subMenus){
			   angular.forEach(angular.isArray(subMenus) ? subMenus :[subMenus], 
							function(menu){
								angular.forEach(menu,function(value,key){
									if (key.charAt(0)=='$') {
										 menu[key]= $interpolate(menu[key]);
									}
								});
					      if (menu.items) interploteSubMenu(menu.items);
						});
			 }
			
				var self = {
					 toModulePath: function(moduleName,name,type){
						 var path=''
						 if (name && name.indexOf('/')<0){
							 path = modulePath + moduleName + '/' + (type ? type+'/': '');
						 }
						 if (type=='img') type="";
						 else if (type=="formly") type="html";
						 //console.log("toModulePath",path + name + (type && name.indexOf('.'+type)<0 ? '.'+type : ''));
						 return path + name + (type && name.indexOf('.'+type)<0 ? '.'+type : '');
					 },
					 loadLocales:function(module){
						 var config = self.getConfig(module);
						 if (config && Boolean.fromString(config.hasLocales,false)){
							 	console.log("Loading translation",modulePath + module);
			          $translatePartialLoader.addPart(modulePath + module);
			          //$translate.refresh(); //Uncomment this when not using auto refresher
						 }
		       },
					 hasTour:function(state){
						 for (var i in modules) {
								 if (modules[i].states){
									 for (var j=0;j<modules[i].states.length;j++){
										 if (modules[i].states[j].state==state) { 
											 return Boolean.fromString(modules[i].states[j].tour,false);
										 }
									 }
								 }
							 }
						 return false;
					 },
					 loadTour:function(state){
						 var defer = $q.defer();
						 if (tours.hasOwnProperty(state)) defer.resolve(tours[state]);
						 else {
							 var module = self.byState(state);
							 if (module) {
								 
								 $http.get(modulePath + module.name + '/tour/' + state + '.json').then(function (response) {
									 try {
										 tours[name]=angular.fromJson($interpolate(angular.toJson(response.data))($rootScope));
									 } catch (ex) {
									   tours[name]=response.data;
									 }
									 defer.resolve(tours[name]);
									},defer.reject);
							 } else defer.reject();
						 }
						 return defer.promise;
					 },
					 addApp: function(name,config,version){
						 if (!name || !config) return;
						 //Change the icon application icon the full path
						 config.icon=self.toModulePath(name,config.icon);
						 if (!config.name) config.name=name;
						 if (!config.version) config.version=version;
						 applications[name]=config;
					 },		 
					 getApp: function(name){
						 return applications[name];
					 },
					 getRequiredApps:function(){
						 var a=[];
						 angular.forEach(applications,function(config,key){
							 if (config.required) a.push(key);
						 });
						 console.log("Required apps",a);
						 return a;
					 },
					 addUiRoutes :function (module) {
						 var states = $state.get();
						 if (!module || !module.states) return false;
						 for (var i=0;i<module.states.length;i++){
							 var state = module.states[i];
							 //Create the app
							 var stateName=(state.state || module.name);
							 var id = stateName.replace('app.','');
							 var config = {
							  url: state.url || '/' + stateName.replace('.','_'),
							  data: state.data || module.data || {},
							  cache: state.cache===true ? false : true
							 };
							 //TODO: Handle custom view
							 if (state.menuContent!==false || menuContent) {
								config.views={};
								config.views[state.menuContent || menuContent]={
								 templateUrl: self.toModulePath(module.name,state.templateUrl || state.state,'html')
							  };							
						    if (state.controller) 
								  config.views[(state.menuContent || menuContent)].controller = state.controller;
							 } else {
								 if (state.controller) 
								  config.controller = state.controller;
							 }
							 var j=0;
							 for (j=0;j<states.length;j++) {
							   if (states[j].name==stateName){
								  //console.log("Ignoring state",config);
									break;
							   }
							 }
						   if (j>=states.length) {
								 //console.log("Dynamicly Adding State",config)
						     $stateProvider.state(stateName,config);
							 }
						 }
						 return true;
						},
					  add: function(config){
							var defer = $q.defer();
							var prom=[];
							var configs = angular.isArray(config) ? config : [config];
							angular.forEach(configs, function (moduleConfig) {
							 var def =$q.defer();prom.push(def.promise);
							 if (!modules[moduleConfig.name]){
	              		modules[moduleConfig.name] = angular.copy(moduleConfig);
										if (Boolean.fromString(moduleConfig.autoLoad,false)) {
											self.load(moduleConfig.name,function(){
												 console.log("Autoloaded app", moduleConfig.name);
											 	 def.resolve();
											});
									  } else { //We not autoloading alleast add to application list
											if (moduleConfig.app){
											  self.addApp(moduleConfig.name,moduleConfig.app,moduleConfig.version);
												def.resolve();
											}
										}
								}
							});
							$q.all(prom).then(function(){
								defer.resolve();
							})
							return defer.promise;
						},
						getConfig: function (name) {
								if (!modules[name]) {
										return null;
								}
								return modules[name];
						},
					  byState: function (state) {
							  for (var i in modules) {
								 if (modules[i].states){
									 for (var j=0;j<modules[i].states.length;j++){
										 if (modules[i].states[j].state==state) return modules[i];
									 }
								 }
							 }
							return null;
						},
					  getModule: function(name,byState){
							var defer = $q.defer(),
							module = byState? self.byState(name) : self.getConfig(name);
							if (!module){
							  $http.get(modulePath + name + '/' + name + '.json').then(function (response) {
									self.add(response.data);
					        module = self.getConfig(name);
									//console.log("response",response.data,module,modules);
				 	        if (module) defer.resolve(module); else defer.reject();
					      },defer.reject);
								
							} else defer.resolve(module);
							return defer.promise;
						},
					  getStateConfig: function(stateName){
							var defer = $q.defer();
							self.getModule(stateName,true).then(function (module){					
								for (var j=0,l=module.states.length;j<l;j++){
									if (module.states[j].state==stateName){
										 var config = angular.extend({},module.states[j].state);
										 config['_templateUrl']=self.toModulePath(module.name,module.states[j].templateUrl || state.state,'html');
										 config['_module']=module.name;
										 defer.resolve(config);
										return;
									 } 
								 }
								 defer.reject();
							},defer.reject);
							return defer.promise;
						},
					  goApp: function(appName,toParams,options){
							var app = self.getApp(appName);
							if (app) return self.go(app.state || appName,
								app.paramsToJson ? {json: angular.toJson(angular.extend(toParams||{}, app.params))} : 
																			angular.extend(toParams||{},app.params) ,options);
							else return self.go(appName,toParams,options); //Normal go
						},
					  go: function (stateName,toParams,options){ //Go to a state
							var defer = $q.defer();

							//Find the module by state
							self.getModule(stateName,true).then(function (moduleConfig){
								//Load the module files
								self.load(moduleConfig.name, function() {
									//See if route is added the route
									if (self.byState(stateName)){
									// console.log("Loaded module automaticly",moduleConfig);
										$state.go(stateName,toParams,options);
									 defer.resolve();	
									} else defer.reject();
								},defer.reject);
							},defer.reject);
							 return defer.promise;
						},
					  goModal:function(stateName,parameters,options){
							var defer = $q.defer();
	          	//Find the module by state
   						self.getModule(stateName,true).then(function (moduleConfig){
								//Load the module files
								self.load(moduleConfig.name, function() {
									 //We have loaded the state
									//Now get controller and template from stateConfig
									var state;
									for (var i=0,l=moduleConfig.states.length;i<l;i++){
										if (moduleConfig.states[i].state==stateName){
											state=moduleConfig.states[i];
											break;
										}
									}
									var templateUrl = self.toModulePath(moduleConfig.name,state.templateUrl || state.state,'html');
									//Now show the state as modal
                  self.showModal(templateUrl, state.controller, angular.extend(state.params||{},parameters||{}), angular.extend(state.options||{},options||{})).then(defer.resolve,defer.reject);
	              },defer.reject);
							},defer.reject);
							return defer.promise;
						},
					  showModal:function (templateUrl, controller, parameters, options) {
							// grab the injector and create a new scope
							console.log("ShowModal",templateUrl,controller);
							if ($rootScope.showLoadingSpinner) $rootScope.showLoadingSpinner();
	
							var defer = $q.defer(),
							ctrlInstance,
							modalScope = $rootScope.$new(),
							thisScopeId = modalScope.$id,
							defaultOptions = {
								animation: 'slide-in-up',
								focusFirstInput: false,
								backdropClickToClose: true,
								hardwareBackButtonClose: true,
								modalCallback: null
							};
							var _cleanup = function (scope) {
								//console.log("Modal Destroy");
								scope.$destroy();
								if (scope.modal) {
									scope.modal.remove();
								}
							};
							var _evalController = function(ctrlName) {
								var result = {
									isControllerAs: false,
									controllerName: '',
									propName: ''
								};
								var fragments = (ctrlName || '').trim().split(/\s+/);
								result.isControllerAs = fragments.length === 3 && (fragments[1] || '').toLowerCase() === 'as';
								if (result.isControllerAs) {
									result.controllerName = fragments[0];
									result.propName = fragments[2];
								} else {
									result.controllerName = ctrlName;
								}
								return result;
							};
							options = angular.extend({}, defaultOptions, options);

							$ionicModal.fromTemplateUrl(templateUrl, {
								scope: modalScope,
								animation: options.animation,
								focusFirstInput: options.focusFirstInput,
								backdropClickToClose: options.backdropClickToClose,
								hardwareBackButtonClose: options.hardwareBackButtonClose
							}).then(function (modal) {
							//	console.log("Modal created");
								modalScope.modal = modal;

								modalScope.openModal = function () {
									modalScope.modal.show();
								};
								modalScope.closeModal = function (result) {
									defer.resolve(result);
									modalScope.modal.hide();
								};
								modalScope.$on('modal.hidden', function (thisModal) {
									if (thisModal.currentScope) {
										var modalScopeId = thisModal.currentScope.$id;
										if (thisScopeId === modalScopeId) {
											defer.resolve(null);
											_cleanup(thisModal.currentScope);
										}
									}
								});

								// invoke the controller
								var locals = { '$scope': modalScope, 'parameters': parameters };
								var ctrlEval = _evalController(controller);
								ctrlInstance = $controller(controller, locals);
								if (ctrlEval.isControllerAs) {
									ctrlInstance.openModal = modalScope.openModal;
									ctrlInstance.closeModal = modalScope.closeModal;
								}

								
								modalScope.modal.show()
									.then(function () {
										modalScope.$broadcast('modal.afterShow', modalScope.modal);
										if ($rootScope.hideLoadingSpinner) $rootScope.hideLoadingSpinner();
										//console.log("Modal show");
									});

								if (angular.isFunction(options.modalCallback)) {
									options.modalCallback(modal);
								}

							}, function (err) {
								console.warn("Failed to showModal",err);
								defer.reject(err);
							});
							return defer.promise;
						},
					  addFormly:function(moduleName,config){			
			  			if (config && config.formly) config=config.formly;
							if (!config) return;
							//Add all types
							angular.forEach(config.types,function(typeConfig){
								//Change template path to module directory
								if (typeConfig.templateUrl)
									typeConfig.templateUrl=self.toModulePath(moduleName,typeConfig.templateUrl,'formly'); 
								providers.formlyConfigProvider.setType(typeConfig);
								console.log("Added formly type",typeConfig.templateUrl);
							});
							//Add all wrappers
							angular.forEach(config.wrappers,function(typeConfig){
								//Change template path to module directory
								if (typeConfig.templateUrl)
									typeConfig.templateUrl=self.toModulePath(moduleName,typeConfig.templateUrl,'formly'); 
								providers.formlyConfigProvider.setWrapper(typeConfig);
								console.log("Added formly wrapper",typeConfig.templateUrl);
							});
						},
					  addHttpMenu: function(url){
							var defer = $q.defer();
							  $http.get(url).then(function (response) {
									self.addMenu(response.data);
					        defer.resolve();
					      },defer.reject);
							return defer.promise;
						},
						addMenu:function(config){
							var module = config;
							if (!module.hasOwnProperty('menus')) module={menus:config};
							angular.forEach(angular.isArray(module.menus) ? module.menus :[module.menus], 
									function(menu){
								    interploteSubMenu(menu);
								    menus.push(menu);
								});
							 //Now sort menu's on id
							 menus.sort(function(a,b){
								 if (a.hasOwnProperty('id') && !b.hasOwnProperty('id')) return -1;
								 if (!a.hasOwnProperty('id') && b.hasOwnProperty('id')) return 1;
								 if (!a.hasOwnProperty('id') && !b.hasOwnProperty('id')) return 0;
								 return a.id-b.id;
							 });
						},
					  menus: menus,
						modules: modules,
					  settings: settings,
					  applications: applications,
						load: function (name, callback,onfail) {
								var self = this,
										config = self.getConfig(name),
										resourceId = 'script:' + name, //config.script,
										moduleCache = [];
								moduleCache.push = function (value) {
										if (this.indexOf(value) == -1) {
												Array.prototype.push.apply(this, arguments);
										}
								};
								if (!config) {
										var errorText = 'Module "' + name + '" not configured';
										console.error(errorText);
										throw errorText;
								}
							  if (!config.scripts) config.scripts=config.name;

								function loadScript(url, onLoadScript,onFail) {
										var scriptId = 'script:' + url,
												scriptElement;
										if (!scriptCache.get(scriptId)) {
												scriptElement = $document[0].createElement('script');
												scriptElement.src = url;
												scriptElement.onload = onLoadScript;
												scriptElement.onerror = function () {
														console.error('Error loading "' + url + '"');
														scriptCache.remove(scriptId);
													  if (onFail) onFail(url);
												};
												$document[0].documentElement.appendChild(scriptElement);
												scriptCache.put(scriptId, 1);
										} else {
												$timeout(onLoadScript);
										}
								}
							  function loadCss(module) {
									if (module.css) 
										angular.forEach(angular.isArray(module.css) ? module.css :[modules.css], 
											function(css){
											 // console.log("Loading css ", self.toModulePath(module.name,css,'css'));
										    angular.element(document).find('head')
										  	.prepend('<link href="'+ self.toModulePath(module.name,css,'css')
																 + '" id="'+module.name+'-css" rel="stylesheet" />');
										});
								}
							
								function loadDependencies(moduleName, allDependencyLoad) {
										if (regModules.indexOf(moduleName) > -1) {
												return allDependencyLoad();
										}
									  
										var loadedModule = angular.module(moduleName,null); //Find existing modules							
									  var			requires = getRequires(loadedModule);

										function onModuleLoad(moduleLoaded) {
												if (moduleLoaded) {

														var index = requires.indexOf(moduleLoaded);
														if (index > -1) {
																requires.splice(index, 1);
														}
												}
												if (requires.length === 0) {
														$timeout(function () {
																allDependencyLoad(moduleName);
														});
												}
										}

										var requireNeeded = getRequires(loadedModule);
										angular.forEach(requireNeeded, function (requireModule) {
												moduleCache.push(requireModule);
												if (moduleExists(requireModule)) {
														return onModuleLoad(requireModule);
												}
												var requireModuleConfig = self.getConfig(requireModule);
												if (requireModuleConfig) {
													 angular.forEach(angular.isArray(requireModuleConfig.scripts) ? requireModuleConfig.scripts :  [requireModuleConfig.scripts], function(script){
														 loadScript(self.toModulePath(requireModuleConfig.name,script,'js'), function() {
																	loadDependencies(requireModule, function requireModuleLoaded(name) {
																			onModuleLoad(name);
																	});
															});
													 });
												} else {
														console.warn('module "' + requireModule + "' not loaded and not configured");
														onModuleLoad(requireModule);
												}
												return null;
										});

										if (requireNeeded.length == 0) {
												onModuleLoad();
										}
										return null;
								}

							
								if (!scriptCache.get(resourceId)) {
									  //TODO: it looks like we are loading twice
									  loadCss(config);										
										if (config.hasOwnProperty('menus')) self.addMenu(config);
										//Load translations if needed
										self.loadLocales(config.name);
										//Load routes if needed
										self.addUiRoutes(config);
										//Add Applications
										self.addApp(config.name,config.app,config.version);
									  //Add the formly stuff
									  self.addFormly(config.name,config.formly);
										//Add Settings config, must be after application
										if (config.hasOwnProperty('settings')){
											settings[config.name]=
												angular.merge(config.hasOwnProperty('app') ?{
													icon:config.app.icon,
													ion:config.app.ion
												 } :{},config.settings);
											 console.log("Added settings for",config.name);
										}
									  //We can load multiple modules so ensure callback by promise
									 var prom=[];
									 angular.forEach(angular.isArray(config.scripts) ? config.scripts : [config.scripts],function(script){
											console.log("Loading script",self.toModulePath(config.name,script,'js'));
										  var defer = $q.defer();prom.push(defer.promise)
										  //Create a empty module
										  angular.module(config.name,[]); //Create empty module so it exists
											loadScript(self.toModulePath(config.name,script,'js'), function () {
												moduleCache.push(name);	
												loadDependencies(name, function () {
														register($injector, providers, moduleCache);
														defer.resolve();
												});
										  },defer.reject);
								    });
									  //Call all other config listeners, may be a promise
									  angular.forEach(configListeners,function(listener){
										 if (typeof listener == "function"){
											 prom.push($q.when(listener(config,config.name)));
										 }
									 });
									  //Wait for all promisses to be completed
									  $q.all(prom).then(function () {
											$timeout(function () {
												  scriptCache.put(resourceId, 1);
													callback(false);
											});
										},function(){
											if (typeof onFail == "function") onFail(); else callback(false);
										});
								} else {
										$timeout(function () {
												callback(true);
										});
								}
						},
					  autoLoad:function(modules){
							var defer = $q.defer();
							var prom=[];
							if (!modules){
								//Load the configuration of all modules stored in home
								var configName = modulePath.substr(0,modulePath.length-1).replace(/^.*[\\\/]/, '');
								if (!configName) configName="modules";
								var def = $q.defer();prom.push(def.promise);
								$http.get(modulePath + configName +'.json').then(function (response) {
									 if (angular.isArray(response.data)) {
										angular.forEach(response.data, function (module) {
											   var def = $q.defer();prom.push(def.promise);
												//Add loaded module 
												 $http.get(modulePath + module.name + '/' + module.name +'.json').then(function (response) {
													 self.add(response.data).then(def.resolve,def.reject);	
													 
												 },def.reject);
										});
									}
									def.resolve();
								}).finally(function(){
										$q.all(prom).then(function(data){
								      defer.resolve();
							      });
									});	
							} else {
								var config = angular.isArray(modules) ? modules : [modules];
								angular.forEach(config, function (module) {
								  var def = $q.defer();
								  prom.push(def.promise);
								  //Add loaded module 
								  $http.get(modulePath + module.name + '/' + module.name +'.json').then(function (response) {
										 self.add(response.data).then(def.resolve,def.reject);
									});
								});
								$q.all(prom).then(function(data){
									defer.resolve();
								});
							}				
		  				return defer.promise;
						},
					  addConfigListener : _addConfigListener,
		        removeConfigListener : _removeConfigListener
				};
			  
				return self;	
		}];
		this.config = function (config,_modulePath,_menuContent,_menus) {
			  var self=this;
			  modulePath=_modulePath||'';
			  menuContent = _menuContent;
			  menus = _menus || [];
			  if (modulePath.length!=0 && modulePath.charAt(modulePath.length-1)!='/') modulePath+='/'
				init(angular.element(window.document));
				if (angular.isArray(config)) {
						angular.forEach(config, function (moduleConfig) {
								modules[moduleConfig.name] = moduleConfig;
						});
				} else {
						modules[config.name] = config;
				}
		};
		this.addConfigListener = _addConfigListener;
		this.removeConfigListener = _removeConfigListener;
}])

.directive('loadOnDemand', ['$http', 'scriptCache',  '$loadOnDemand', '$compile', '$timeout',
function ($http, scriptCache, $loadOnDemand, $compile, $timeout) {
		return {
				link: function (scope, element, attr) {
						var srcExp = attr.loadOnDemand,
								childScope;

					function clearContent() {
								if (childScope) {
										childScope.$destroy();
										childScope = null;
								}
								element.html('');
						}

						function loadTemplate(url, callback) {
								var resourceId = 'view:' + url,
										view;
								if (!scriptCache.get(resourceId)) {
										$http.get(url).
												success(function(data) {
														scriptCache.put(resourceId, data);
														callback(data);
												})
												.error(function(data) {
														console.error('Error load template "' + url + "': " + data);
											      callback(null);
												});
								} else {
										view = scriptCache.get(resourceId);
										$timeout(function() {
												callback(view);
										}, 0);
								}
						}

						scope.$watch(srcExp, function(moduleName) {
							  if (!moduleName) moduleName=srcExp;
								var moduleConfig = $loadOnDemand.getConfig(moduleName);

								if (moduleName) {
										$loadOnDemand.load(moduleName, function() {		
											  $loadOnDemand.loadLocales(moduleConfig.name);
												if (!moduleConfig.templates) {
														return;
												}
											  angular.forEach(angular.isArray(moduleConfig.templates) ? moduleConfig.templates : [moduleConfig.templates],function(templateName){
												  loadTemplate(templateName, function(template) {
														if (template){
															childScope = scope.$new();
															element.html(template);
															var content = element.contents(),
																	linkFn = $compile(content);
															linkFn(childScope);
														}
												});
										   });

										});
								} else {
										clearContent();
								}
						});

				}
		};
}]) 
;