# 1. MiiApp Ionic Framework
Thanks for your interest in the **MiiApp Ionic Framework**. This framework will reduce the effort you require to build great hybrid applications.
We have combined the most common used native functions and with some great functions like dynamic load, so you can just focus on writting your code in HTML5.

## 1.1 Install Android Studio and XCode
Before you can build a native application your need to install Android Studio and XCode
1. Android: https://developer.android.com/studio/index.html
1. Apple: XCode from Appstore

## 1.2 Installing from scratch
1. Download nodejs from https://nodejs.org/en/download/ and install it
1. sudo npm install -g ionic
1. sudo npm install -g bower
1. sudo npm install -g cordova
1. sudo npm install -g gulp

Run cordova to complete the cordova installation

```
cordova
```

Install the following pacakges to install the IOS simulator on MacOS.

1. sudo npm install -g ios-sim
1. sudo npm install -g ios-deploy --unsafe-perm=true  --allow-root


## 1.3 Clone the repository
Make a clone of our repository with your GIT tool.

```
git clone https://github.com/sjhoeksma/dwp-ss.git
cd dwp-ss
```


## 1.4 Download all required packages
1. npm install
1. bower install
1. ionic state reset
1. ionic resources

## 1.5 Additional Platforms
By default we install IOS and Android would you like to add another platform just list type **ionc platform ls** and then add type **ionic platform add XXX**
 
# 2. Start your development
You have now successfully installed Ionic and our framework. You will need to configure it. Open the file [www/js/config.js](https://github.com/sjhoeksma/dwp-ss/blob/master/www/js/config.js) and set your preferences.
We support different kind of secuirty elements enabling you to make enterprise grade applications. By default all security and backend services are **disabled**.

## 2.1 Understanding the architecture

### Evergreen process of modules (remote sync)
We use a evergreen process to keep your mobile application updated to latest version. This will require you to deploy the framework towards a webserver. During the build process we create a **modules.json** file containing all available modules and there version number. For all modules you have installed we will check if there is a never version available on the webserver and then download and install it. Next time you application goes to the home state **app.home** the application will reload and you have the latest version of all modules.

## 2.2 Create your first Module


## 2.3 Build or Test your first Module
Now you have you created your module you want to test it. You should first test your app in the browser by using the following command from the root directory of your project.

```
ionic serve -c -l
```
This will start up your browser and the live reload services and dumps all message of the console.

If you are happy with you code you can build a real version and test it in an emulator like this

```
ionic emulate ios -c -l
or
ionic emulate android -c -l
```
This will start the emulator of your choice, in live reload. If you want to have a real package remove *-c -l*. 

**Warning: On android no cordova plugins are loaded when using live reload**

When you are finally happy with your result you can run in on a real device by using.

```
ionic run
```
or even build the real version

```
ionic build --production
or
ionic build [--development]
```
The prodcution build will create a compressed version of the code, without debug infomation. Therefor you should not use it for testing purposes. Development is the default behaivour. You will need to sign your app before you can deploy them to a appstore or any apple device.

## 2.4 Setup signing of application
### Android

### Apple

## 2.5 Developer documentation
The framework comes with a number of states, which you may want to overwrite of use
* **app.home** The default home screen contains a single app or a grid of apps when more installed
* **app.settings** Screen where use can changes general settings (language,theme) or module specific
* **app.store** A store where a user can add or remove a module
* **app.notifications** A webpage showing all notifications
* **app.about** Text about your app (basic info in language files)
* **app.firsttime** Text show when user starts application for fist time (basic info in language files)
* **app.whatsnew** Text shown when a new version of the application is started after download from app store (basic info in language files)

#### Pages normally not modified (except language files)
* **app.login** The login page.
* **app.logout** This state will redirect automactily back to the app.login state
* **app.webpage** Opens the webpage in iframe of in appbrowser depending on the config you specified
* **app.privacy** This page shows general information about how you should use this app if you want to protect you privacy as a end user
* **app.opensource** This page shows the opensource license and all links to the packages we have used.

* **app.newpin** This is a model view page which is shown in case an user needs to set a pin. This function can be used by enterprise who use a claim based authorization model and want to lock the user after x minutues of in activity, but don't require the user to retreive a new claim. When a pin code is set we will use this pincode to encrypt data in the app.

All the Factories,Providers and Services within the framework are document using ngDoc. We do our best to make usable documentation, but please help us making it exellent.

**Directives**
* [IonKeyPad]()
* [MenuTree]()
* [GridMenu]()

**Providers**
* [SecureStorage]()
* [LoadOnDemand]()
* [SyncQueue]()
* [FilePicker]()

**Services**
* [AppUser]()
* [IdelStateTimer]()
* [NetworkMonitor]()
* [Notifications]()


# 3. Technical information
This section explains the installation process of all packages used by the framework. You don't need to do this is you have followed the instructions at the beginning of this document

## 3.1 Codova plugins packages
Cordova plugins are automaticly to packages.json and therefor reloaded during the state reset command of ionic. If for some reason this did not happend you can add the packages yourself.
* ionic plugin add cordova-plugin-whitelist  --save
* ionic plugin add cordova-plugin-appavailability --save
* ionic plugin add cordova-plugin-camera --save
* ionic plugin add cordova-plugin-network-information --save
* ionic plugin add cordova-plugin-file --save
* ionic plugin add cordova-plugin-powermanagement --save
* ionic plugin add cordova-plugin-vibration --save
* ionic plugin add cordova-plugin-app-version --save
* ionic plugin add cordova-plugin-keychain-touch-id --save
* ionic plugin add cordova-plugin-crosswalk-webview@1.7 --save
* ionic plugin add cordova-plugin-nativeclicksound --save
* ionic plugin add cordova-plugin-badge --save
* ionic plugin add cordova-plugin-ms-adal --save
* ionic plugin add cordova-plugin-themeablebrowser --save 
* ionic plugin add phonegap-plugin-push --variable SENDER_ID=123232 --save
* ionic plugin add phonegap-plugin-contentsync --save
* ionic plugin add cordova-custom-config --save
* ionic plugin add cordova-plugin-file-transfer --save
* ionic plugin add http://github.com/don/cordova-filechooser.git --save
* ionic plugin add cordova-plugin-backbutton --save
* ionic plugin add cordova-plugin-bluetooth-serial --save

## 3.2 Javascript libraries
JavaScript libraries are also automaticly added during the ionic state reset command because they where added to bower.json.
* bower install --save angular-translate
* bower install --save angular-translate-loader-static-files
* bower install --save angular-translate-loader-partial
* bower install --save angular-cookies
* bower install --save crypto-js
* bower install --save ngCordova
* bower install --save ion-datetime-picker
* bower install --save jquery 
* bower install --save nz-tour
* bower install --save crypto-js
* bower install --save idle.js
* bower install --save Leaflet
* bower install --save adal-angular
* bower install --save https://github.com/sjhoeksma/ionic-gizmos.git
* bower install --save https://github.com/sjhoeksma/nzTour.git
* bower install --save ionic-select-control
* bower install --save localforage 
* bower install --save angular-localforage
* bower install --save https://github.com/localForage/localForage-sessionStorageWrapper.git
* bower install --save angular-formly
* bower install --save owl.carousel
* bower install --save pdfmake

## 3.3 Setting up testing with karma 
Based on the following links: [link1](http://mcgivery.com/unit-testing-ionic-app) and [link2](http://gonehybrid.com/how-to-write-automated-tests-for-your-ionic-app-part-2/) we have created some unit test. To run a test you can use the command **gulp test** from the command line.

```
npm install karma karma-jasmine jasmine-core karma-phantomjs-launcher karma-html-reporter karma-chrome-launcher karma-mocha-reporter karma-ng-html2js-preprocessor karma-html-reporter --save
bower install angular-mocks#1.5.x --save
sudo npm install -g karma-cli
sudo npm install -g phantomjs
mkdir -p tests/unit-tests
cd tests
karma init unit-tests.conf.js
```

## 3.4 Known Issues
1. When login you can be prompted to validate adfs again we trigger autheticate while user entered Valid Pin
2. 
