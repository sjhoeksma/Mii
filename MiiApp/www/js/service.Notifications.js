'use strict';
angular.module('service.Notifications',[	
//	'provider.SecureStorage' //We required Secure Storage to save the data
])
.factory('Notifications',['$secureStorage','$rootScope','$q','ENV',function($secureStorage,$rootScope,$q,ENV){
	var _data={
		 list : [],
		 groups: {},
		 noVibrate : false
	};
	
	function updateBadge(){
		if (window.cordova && window.cordova.plugins && window.cordova.plugins.notification){
			var c= self.count(null,'new');
			if (c==0) cordova.plugins.notification.badge.clear();
			else cordova.plugins.notification.badge.set(c);
		}
	}
		
	function saveState(){
		return $secureStorage.set('Notifications','_notifications',_data);
	}
	var self = {
		  insertTime:function(){return _data.insertTime||0;},
		  readTime:function(){return _data.readTime||0;},
		  list:function(group){
				if (!group) return _data.list;
				var list = [];
				angular.forEach(_data.list,function(item){
					if (item.group==group) list.push(item);
				});
				return list;
			},
 	   	count : function(group,mode){
				var c=0;
				var cr=0;
				for (var i=0;i<_data.list.length;i++){
						if (!group || _data.list[i].group==group) {
							if (typeof mode  == 'undefined'){
								//When  unread is undefined we only count the new unread messages
								if (!_data.list[i].hasOwnProperty('readTime')){		
									if (_data.list[i].insertTime>(_data.readTime || 0)) c++; else cr++;
								}
							}else if (mode  == 'new'){
								//When  unread is undefined we only count the new unread messages
								if (!_data.list[i].hasOwnProperty('readTime')){		
									if (_data.list[i].insertTime>(_data.readTime || 0)) c++; 
								}
							}
							else if (unread && !_data.list[i].hasOwnProperty('readTime')) c++;
							else if (!unread && _data.list[i].readTime) c++;
						}
				}
			  return c || cr;
			},
		  subscribe : function(group,params){
				if (_data.groups)_data.groups={};
				_data.groups[group]=params || _data.groups[group] || {};
				saveState();
			},
		  unsubscribe: function(group){
				if (_data.groups && _data.groups.hasOwnProperty(group)){
					delete _data.groups[group];
					saveState();
				}
			},
		  subscribed: function(group){
				if (!group || !_data.groups || _data.groups.length==0) return true;
				return _data.groups.hasOwnProperty(group);
			},
		  hasUnread: function(){
				return (_data.insertTime || 0) > (_data.readTime ||0);
			},
		  setVibrate:function(state){
				if (state==_data.noVibrate){
					_data.noVibrate!=state;
					saveState();
				}
			},
	    push: function(notify,group){
				if (!self.subscribed(group)) return;
				_data.list.push({group:group||'default',msg:notify,insertTime:(new Date()).getTime()});
				_data.insertTime=(new Date()).getTime();
				$rootScope.$broadcast('notification',{msg:notify,group:group});
				updateBadge();
				saveState();
				//Should we run a vibrate
				if (!_data.noVibrate && navigator.vibrate) navigator.vibrate(400);
			},
			clear: function(group){
				if (group){
					for (var i=_data.list.length-1;i>=0;i--){
						if (_data.list.group==group) _data.list.splice(i,1);
					}
				} else {
				 _data.list=[];
				}
				_data.readTime=(new Date()).getTime();
				updateBadge();
				saveState();
			},
		  peek: function(group){
				var notify = null;
			  if (group){
					for (var i=0;i<_data.list.length;i++){
						if (_data.list[i].group==group){
						   notify = _data.list[i];
							 break; //Exit for
					  }
					}
				} else {
					notify = _data_list.length > 0 ? _data.list[_data_list.length-1] : null;
				}
	    	return notify.msg;
			},
			pop:  function(group){
				var notify = null;
			  if (group){
					for (var i=0;i<_data.list.length;i++){
						if (_data.list[i].group==group){
						   notify = _data.list.splice(i,1)[0];
							 break; //Exit for
					  }
					}
				} else {
					notify = _data.list.length>0 ? _data.list.shift() : null;
				}
	      if (notify){
					_data.readTime=(new Date()).getTime();
					updateBadge();
					saveState();
					return notify.msg;
				}
	      return null;
			},
		  get: function(index,group){
				if (!isNaN(parseFloat(index)) && isFinite(index)){
					if (group){
						for (var i=0,j=0;i<_data.list.length;i++){
							if (_data.list[i].group==group){
								if (j==index) return _data.list[i];
								j++;
							}
						}
					} else if (index>=0 || index<_data.list.length) return  _data.list[index];
				} else { //We search on object
					for (var i=0;i<_data.list.length;i++){
						if (_data.list[i].msg==index || _data.list[i]==index) return _data.list[i];
					}
				}
				return null;
			},
		  delete: function(index,group){
			if (!isNaN(parseFloat(index)) && isFinite(index)){
					if (group){
						for (var i=0,j=0;i<_data.list.length;i++){
							if (_data.list[i].group==group){
								if (j==index) {
									_data.list.splice(i,1);
									_data.readTime=(new Date()).getTime();
					        updateBadge();
					        saveState();
									return;
								}
								j++;
							}
						}
					} else if (index>=0 || index<_data.list.length) {
						_data.list.splice(index,1);
					  _data.readTime=(new Date()).getTime();
						updateBadge();
						saveState();
					}
				} else { //We search on object
					for (var i=0;i<_data.list.length;i++){
						if (_data.list[i].msg==index || _data.list[i]==index){
							_data.list.splice(i,1);
							_data.readTime=(new Date()).getTime();
					    updateBadge();
					    saveState();
						  return;
						} 
					}
				}	
			},
		  read: function(index){
				var obj = self.get(index);
				if (obj) {
					obj.readTime=(new Date()).getTime();
					_data.readTime=(new Date()).getTime();;
					updateBadge();
					saveState();
					return obj.msg;
				}
        return null;
			},
		  unread: function(index){
				var obj = self.get(index);
				if (obj) {
					delete obj['readTime'];
					var readTime=0;
					angular.forEach(_data.list,function(notify){
						if (notify.readTime && notify.readTime> readTime) readTime=notify.readTime;
					})
					_data.readTime=readTime;								
					saveState();
					return obj.msg;
				}
				return null;
			},
		  load: function(){
		   $secureStorage.get('Notifications','_notifications',{list:[],groups:{},noVibrate:false}).then(function(value){
				 _data=value;
		     updateBadge();
			 });
	    },
		  init: function(){
				if (window.PushNotification && ENV.notifications){
					var push = PushNotification.init({
							android: {
									senderID: ENV.notifications.senderID
							},
							browser: {
									pushServiceURL: ENV.notifications.serviceURL
							},
							ios: {
									alert: ENV.notifications.alert || "true",
									badge: ENV.notifications.baged || "true",
									sound: ENV.notifications.sound || "true"
							},
							windows: {}
					}); 

					push.on('registration', function(data) {
						 console.log("Notifcation service registered", data.registrationId);
					});

					push.on('notification', function(data) {
						self.push(data);
					});

					push.on('error', function(e) {
							// e.message
						 console.warn("Notification error",e.message);
					});
					return $q.resolve();
				}
				return $q.reject();
			}
		
	}
	$rootScope.$on('app.loggedin',function(){
		self.load();
	});
	$rootScope.$on('app.lock',function(){
		_data={list:[],read:true,groups:{}};
		updateBadge();
	});
	return self;
}])
;