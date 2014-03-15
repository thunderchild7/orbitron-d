
var mConfig = {};

Pebble.addEventListener("ready", function(e) {
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {
  console.log("loadLocalData() " + JSON.stringify(config));
  

  localStorage.setItem("invert", parseInt(config.invert)); 
  localStorage.setItem("vibeminutes", parseInt(config.vibeminutes)); 
  
  loadLocalData();
}
function loadLocalData() {
	mConfig.invert = parseInt(localStorage.getItem("invert"));
	mConfig.vibeminutes = parseInt(localStorage.getItem("vibeminutes"));
	mConfig.configureUrl = "http://ewobo.co.uk/config2.html";


	if(isNaN(mConfig.invert)) {
		mConfig.invert = 0;
	}

	if(isNaN(mConfig.vibeminutes)) {
		mConfig.vibeminutes = 0;
	}   

}
function returnConfigToPebble() {
  console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({ 
    "invert":parseInt(mConfig.invert), 
    "vibeminutes":parseInt(mConfig.vibeminutes)
  });    
}