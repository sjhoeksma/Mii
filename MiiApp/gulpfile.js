var gulp = require('gulp');
var gutil = require('gulp-util');
var bower = require('bower');
var concat = require('gulp-concat');
var sass = require('gulp-sass');
var minifyCss = require('gulp-minify-css');
var rename = require('gulp-rename');
var sh = require('shelljs');
var karma = require('karma');

var paths = {
  sass: ['./scss/**/*.scss']
};

// Import at the top of the file
/**
* Test task, run test once and exit
*/
var karma = require('karma');
gulp.task('test', function(done) {
	server = new karma.Server({
        configFile:  './tests/unit-tests.conf.js',
        singleRun: true
    }, function() {
        done();
    });
	 server.start();
});


//Gulp taks to create css for modules
gulp.task('module-scss',function(done){
	gulp.src('./www/modules/**/*.scss')
   .pipe(sass())
	 .on('error', sass.logError)
 //  .pipe(gulp.dest('./www/css/'))
   .pipe(minifyCss({
      keepSpecialComments: 0
    }))
   .pipe(rename({ extname: '.min.css' }))
   .pipe(gulp.dest('./www/modules'))
   .on('end', done);
});

gulp.task('sass', function(done) {
  gulp.src('./scss/ionic.app.scss')
    .pipe(sass())
    .on('error', sass.logError)
    .pipe(gulp.dest('./www/css/'))
    .pipe(minifyCss({
      keepSpecialComments: 0
    }))
    .pipe(rename({ extname: '.min.css' }))
    .pipe(gulp.dest('./www/css/'))
    .on('end', done);
});

gulp.task('watch', function() {
  gulp.watch(paths.sass, ['sass','module-scss']);
});


gulp.task('install', ['git-check'], function() {
  return bower.commands.install()
    .on('log', function(data) {
      gutil.log('bower', gutil.colors.cyan(data.id), data.message);
    });
});


gulp.task('git-check', function(done) {
  if (!sh.which('git')) {
    console.log(
      '  ' + gutil.colors.red('Git is not installed.'),
      '\n  Git, the version control system, is required to download Ionic.',
      '\n  Download git here:', gutil.colors.cyan('http://git-scm.com/downloads') + '.',
      '\n  Once git is installed, run \'' + gutil.colors.cyan('gulp install') + '\' again.'
    );
    process.exit(1);
  }
  done();
});

gulp.task('default', ['sass','module-scss']);
