var configuration = {
  blackBackground: false,
  showDate: true,
  showBattery: true,
  showBluetooth: true,
  vibrateOnConnect: true,
  vibrateOnDisconnect: true
};

var localStorageKey = "pebbleSettings";

function updateFromLocalstorage(){
    var newConfig = JSON.parse(decodeURIComponent(localStorage.getItem(localStorageKey)));
    for (var attr in newConfig) {
      if (newConfig.hasOwnProperty(attr) && configuration.hasOwnProperty(attr)) {
        configuration[attr] = newConfig[attr];
      }
    }
}

function updateToLocalstorage(){
  localStorage.setItem(localStorageKey, encodeURIComponent(JSON.stringify(configuration)));
}

function settingsToInt(){
  var data = 0;
  if(configuration.blackBackground) {data += 1;}
  if(configuration.showDate) {data += 2;}
  if(configuration.showBattery) {data += 4;}
  if(configuration.showBluetooth) {data += 8;}
  if(configuration.vibrateOnConnect) {data += 16;}
  if(configuration.vibrateOnDisconnect) {data += 32;}
  return data;
}

function sendSettingsToPebble(){
  Pebble.sendAppMessage( { '5': settingsToInt() },
  function(e) {
    console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
  },
  function(e) {
    console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
  }
);
}

Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
    if(Pebble.getActiveWatchInfo) {
      var watch = Pebble.getActiveWatchInfo();
      console.log('platform: ' + watch.platform);
      console.log('model: ' + watch.model);
      console.log('language: ' + watch.language);
      console.log('firmware: ' + watch.firmware.major + "." + watch.firmware.minor + "." + watch.firmware.patch + " " + watch.firmware.suffix);
    } else {
      console.log('Can\'t get Watch Info!');
    }
    updateFromLocalstorage();
    sendSettingsToPebble();
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
  }
);

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  updateFromLocalstorage();
  Pebble.openURL('http://tschrock.net/Pebble/api/generic/config.php?oldcfg=' + encodeURIComponent(JSON.stringify(configuration)));
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    //console.log('Configuration window returned: ' + e.response);
    var newConfig = JSON.parse(decodeURIComponent(e.response));
    for (var attr in newConfig) {
      if (newConfig.hasOwnProperty(attr) && configuration.hasOwnProperty(attr)) {
        configuration[attr] = newConfig[attr];
      }
    }
    updateToLocalstorage();
    sendSettingsToPebble();
  }
);