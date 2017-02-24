'use strict';
// <script src="//rawgit.com/hawnmclean/Idle.js/master/build/idle.min.js"></script>
angular.module('service.IdleStateTimer',[	
//	'provider.SecureStorage' //We required Secure Storage to save the data
])
.factory('idleStateTimer',['$state',function($state){
  var idleClock;
  return function (minutes,state,params){
    //We enforce lock  
    if (!idleClock) idleClock=new Idle({ onAway: function(){
      if ($state.current.name != state) {
        console.log("Forcing state change",state);
        $state.go(state,params);
      }
    }});
    if (minutes) {
      console.log("Setting auto state change to " + minutes + " minutes towards " + state);
      idleClock.setAwayTimeout(minutes*60000);
      idleClock.start();
    } else idleClock.stop();	
  };
   
}])
;