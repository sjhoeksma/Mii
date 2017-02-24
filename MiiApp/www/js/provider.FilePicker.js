'use strict';
angular.module('provider.FilePicker',[	
])
 .provider('$filePicker', function (ENV) {
	
	var myConfig = {};
	
  this.$get=["$q", "$cordovaFile", "$cordovaFileTransfer",function ($q, $cordovaFile, $cordovaFileTransfer) {
      var downloadFile = function(sourceURI, targetDir, targetFile) {
        var defer = $q.defer();

        console.log("FileManager#downloadFile source (original): '" + sourceURI + "'");
        sourceURI = decodeURI(sourceURI);

        var targetPath = targetDir + targetFile;
        var trustHosts = true;
        var options = {};

        $cordovaFileTransfer.download(sourceURI, targetPath, options, trustHosts).then(
          function(result) {
            defer.resolve(result);

          }, function(error) {
            defer.reject(error);

          }, function (progress) {
            //$timeout(function () {
            //  $scope.downloadProgress = (progress.loaded / progress.total) * 100;
            //})
          });

        return defer.promise;
      };

      var getFileInfo = function (baseDir, filePath) {
        var defer = $q.defer();

        console.log("FileManager#checkFile baseDir = '" + baseDir + "', filePath = '" + filePath + "'");

        $cordovaFile.checkFile(baseDir, filePath).then(
          function (fileEntry) {
            fileEntry.getMetadata(
              function (result) {
                defer.resolve(result);
              },
              function (error) {
                defer.reject(error);
              }
            );
          },
          function (error) {
            defer.reject(error);
          }
        );

        return defer.promise;
      };

      var removeFile = function (baseDir, filePath) {
        console.log("FileManager#removeFile baseDir = '" + baseDir + "', filePath = '" + filePath + "'");

        return $cordovaFile.removeFile(baseDir, filePath);
      };

      //function checkFileType(path, fileExt) {
      //  return path.match(new RegExp(fileExt + '$', 'i'));
      //}

      var checkFileType = function (uri, fileType) {
        var defer = $q.defer();

        console.log("FileManager#checkFileType uri = " + uri + " fileType = " + fileType);

        // See: https://github.com/hiddentao/cordova-plugin-filepath and:
        // stackoverflow.com/questions/31338853/cordova-camera-plugin-obtain-full-image-path-from-gallery-android
				if (window.FilePath) {
					window.FilePath.resolveNativePath(uri,

						function (result) {
							var fileURI = 'file://' + result;

							console.log('FileManager#checkFileType uri = ' + uri + ' fileURI = ' + fileURI);

							// See: https://github.com/cfjedimaster/Cordova-Examples/wiki/PhoneGap-Cordova-File-System-FAQ#meta
							window.resolveLocalFileSystemURL(fileURI,
								function (fileEntry) {

									fileEntry.file(function(file) {
										var s = "";
										s += "name: " + file.name + " ";
										s += "localURL: " + file.localURL + " ";
										s += "type: " + file.type + " ";
										s += "lastModifiedDate: " + (new Date(file.lastModifiedDate)) + " ";
										s += "size: " + file.size;

										console.info('FileManager#checkFileType uri = ' + uri + ' fileURI = ' + fileURI + ' INFO: ' + s);

										if (file.type && file.type === fileType) {
											defer.resolve(file);
										} else {
											console.warn('FileManager#checkFileType uri = ' + uri + ' fileURI = ' + fileURI +
												' wrong file type: ' + file.type + ' instead of ' + fileType);

											defer.reject('wrong_file_type');
										}
									});
								},
								function (error) {
									console.error('FileManager#checkFileType uri = ' + uri + ' fileURI = ' + fileURI +
										' ERR = ' + angular.toJson(error));

									defer.reject(error);
								}
							);
						}, 
						function (error) {
							console.error('FileManager#checkFileType uri = ' + uri + ' ERR = ' + angular.toJson(error));

							defer.reject(error);
						}
					);
				} else {
					defer.reject();
				}

        return defer.promise;
      };

      function chooseFileOk(defer, uri) {
        console.log('FileManager#chooseFile - uri: ' + uri);

        defer.resolve(uri);
      }

      function chooseFileError(defer, source) {
        console.log('FileManager#chooseFile - ' + source + ' error: ' + angular.toJson(error));

        // assume operation cancelled
        defer.reject('cancelled');
      }

      var chooseFile = function () {
        var defer = $q.defer();

        // iOS (NOTE - only iOS 8 and higher): https://github.com/jcesarmobile/FilePicker-Phonegap-iOS-Plugin
        if (ionic.Platform.isIOS() && window.FilePicker) {

          FilePicker.pickFile(
            function (uri) {
              chooseFileOk(defer, uri);
            },
            function (error) {
              chooseFileError(defer, 'FilePicker');
            }
          );

        // Android: https://github.com/don/cordova-filechooser
        } else if (ionic.Platform.isAndroid() && window.fileChooser) {

          fileChooser.open(
            function (uri) {
              chooseFileOk(defer, uri);
            },
            function (error) {
              chooseFileError(defer, 'fileChooser');
            }
          );
        }

        return defer.promise;
      };
		
		  var available = function(){
				return (window.FilePicker && ionic.Platform.isIOS())  || (ionic.Platform.isAndroid() && window.fileChooser);
			}

      return {
        download: downloadFile,
        checkFileType: checkFileType,
        getFileInfo: getFileInfo,
        remove: removeFile,
        choose: chooseFile,
				available: available
      };
    }];
	
	this.config = function(_config) {
    myConfig=angular.extend(myConfig,_config);
	}

});