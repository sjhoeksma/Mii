//http://stackoverflow.com/questions/14771810/how-to-test-angularjs-custom-provider
describe('SecureStorage Unit Test', function(){
	  var provider;

		beforeEach(module('provider.SecureStorage', function($secureStorageProvider) {
				provider = $secureStorageProvider;
		}))

		it('tests the providers internal function', inject(function() {
			//	provider.config();
				//expect(provider.$get().mode).toBe('local');
		}));
});