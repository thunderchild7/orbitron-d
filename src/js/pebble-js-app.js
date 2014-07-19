// URL to your configuration page
var config_url = "http://ewobo.co.uk/pebble/index2b.html?v=1.0";

Pebble.addEventListener("ready", function(e) {

});

Pebble.addEventListener("showConfiguration", function(_event) {
	var url = config_url;

	for(var i = 0, x = localStorage.length; i < x; i++) {
		var key = localStorage.key(i);
		var val = localStorage.getItem(key);

		if(val !== null) {
			url += "&" + encodeURIComponent(key) + "=" + encodeURIComponent(val);
		}
	}

	console.log(url);

	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(_event) {
	if(_event.response) {
		var values = JSON.parse(decodeURIComponent(_event.response));

		for(var key in values) {
			localStorage.setItem(key, values[key]);
		}

		Pebble.sendAppMessage(values,
			function(_event) {
				console.log("Successfully sent options to Pebble");
			},
			function(_event) {
				console.log("Failed to send options to Pebble.\nError: " + _event.error.message);
			}
		);
	}
});