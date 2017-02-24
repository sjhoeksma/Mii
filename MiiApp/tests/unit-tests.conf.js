// Karma configuration
// Generated on Fri Nov 11 2016 14:17:08 GMT+0100 (CET)

module.exports = function(config) {
  config.set({

    // base path that will be used to resolve all patterns (eg. files, exclude)
    basePath: '../',

    // frameworks to use
    // available frameworks: https://npmjs.org/browse/keyword/karma-adapter
    frameworks: ['jasmine'],


    // list of files / patterns to load in the browser
    files: [
      'www/lib/ionic-gizmos/ionic/js/ionic.bundle.js',
			'www/lib/ionic-gizmos/item-swipe-pane/item-swipe-pane-directive.js',
		  'www/lib/ionic-gizmos/can-swipe/can-swipe-directive.js',
		  'www/lib/ionic-gizmos/long-swipe/long-swipe-directive.js',
		  'www/lib/ionic-gizmos/click-swipe/click-swipe-directive.js',
		  'www/lib/ion-datetime-picker/release/ion-datetime-picker.min.js',
		  'www/lib/nz-tour/dist/nz-tour.js',
		  'www/lib/angular-translate/angular-translate.js',
		  'www/lib/angular-translate-loader-static-files/angular-translate-loader-static-files.js',
		  'www/lib/angular-translate-loader-partial/angular-translate-loader-partial.js',   
		  'www/lib/ionic-select-control/src/SelectBox.js',
		  'www/lib/crypto-js/crypto-js.js',
		  'www/lib/Idle.Js/build/idle.js',
	 	  'www/lib/localforage/dist/localforage.js',
	    'www/lib/angular-localforage/dist/angular-localForage.js',
	    'www/lib/localforage-sessionstoragewrapper/src/localforage-sessionstoragewrapper.js',
	    'www/lib/adal-angular/lib/adal-angular.js',
	    'www/lib/adal-angular/lib/adal.js',
	    'www/lib/api-check/dist/api-check.js',
	    'www/lib/angular-formly/dist/formly.js',
			'www/lib/jquery/dist/jquery.js',
	    'www/lib/owl.carousel/dist/owl.carousel.js',
			'www/lib/ngCordova/dist/ng-cordova.js',
			'www/lib/ngCordova/dist/ng-cordova.js',
      'www/js/*.js',
      'www/lib/angular-mocks/angular-mocks.js',
      'tests/unit-tests/**/*.js'
    ],

    // list of files to exclude
    exclude: [
			'www/lib/adal-angular/lib/adal.js'
    ],
		
		plugins: [
				'karma-jasmine',
				'karma-phantomjs-launcher',
			  'karma-html-reporter',
//				'karma-requirejs',
				'karma-ng-html2js-preprocessor',
//				'karma-spec-reporter',
				{'preprocessor:varconst': ['factory', function() {
						return function(content, file, done) {
							var count = 0,
										constExpr = /(^|\s)const(\s|$)/g;
								if (/(^|\s)['"]use strict['"];?(\s|$)/.test(content)) {
										content.replace(constExpr, function() {
												count++;
										});
										content = content.replace(constExpr, '$1var$2');
										if (count > 0) {
												console.log(file.path,
														'Replaced ' + count + ' const keywords with var');
										}
								}
								done(null, content);
						};
				}]}
		],


    // preprocess matching files before serving them to the browser
    // available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
    preprocessors: {
			'**/*.js': 'varconst'
    },


    // test results reporter to use
    // possible values: 'dots', 'progress'
    // available reporters: https://npmjs.org/browse/keyword/karma-reporter
    reporters: ['progress','html'],

 	  // the default configuration
		htmlReporter: {
				outputDir: 'tests/results',
				templatePath: 'node_modules/karma-html-reporter/jasmine_template.html'
		},


    // web server port
    port: 9876,


    // enable / disable colors in the output (reporters and logs)
    colors: true,


    // level of logging
    // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
    logLevel: config.LOG_INFO,


    // enable / disable watching file and executing tests whenever any file changes
    autoWatch: true,


    // start these browsers
    // available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
    browsers: ['PhantomJS'],


    // Continuous Integration mode
    // if true, Karma captures browsers, runs the tests and exits
    singleRun: false,

    // Concurrency level
    // how many browser should be started simultaneous
    concurrency: Infinity
  })
}
