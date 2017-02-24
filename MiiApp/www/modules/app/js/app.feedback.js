/*feedback

{

emailTo:
emailFrom:
message:
}
*/

'use strict';
angular.module('app')
.controller('FeedbackCtrl', function($rootScope,$scope,ENV,$syncQueue,AppUser) {
	
	$scope.push('app.feedback',{emailTo:'',emailFrom:AppUser.lastEmail(),})
}
