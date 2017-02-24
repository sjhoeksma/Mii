'use strict';
angular.module('service.NetworkMonitor',[	
])

//Network state monitoring
.factory("NetworkMonitor",["$rootScope",function ($rootScope) {
	var self = {

		isOnline: function () {
			if (window.navigator) {
				return navigator.onLine;
			} 
			return false;
		}
		, isOffline: function () {
			return !self.isOnline();
		}
		, isOnCell: function () {
			if (window.navigator) {
				 return navigator.connection && navigator.connection.type? (navigator.connection.type == Connection.CELL) : false;
			} 
	    return false;
		}
		, isOnNetwork: function () {
			if (window.navigator) {
				return navigator.connection && navigator.connection.type ? (navigator.connection.type == Connection.WIFI || navigator.connection.type == Connection.ETHERNET) : navigator.onLine;
			} 
			return false;
		}
		, watch: function (fn) {
			self.startWatching(fn, fn);
			return self;
		}
		, startWatching: function (online, offline) {
      if (online && window.navigator) {
        window.addEventListener("online", function (e) {
          online(true,navigator.connection && navigator.connection.type ? navigator.connection.type : null);
        }, false);
      }
      if (offline && window.navigator) {
        window.addEventListener("offline", function (e) {
          offline(false,navigator.connection && navigator.connection.type ? navigator.connection.type : null);
        }, false);
      }
      return self;
		}

	}
	return self;
}])
;