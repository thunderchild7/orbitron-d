var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");

var root = path.join(__dirname, "..");
var html = fs.readFileSync(path.join(root, "config", "index.html"), "utf8");
var phone = fs.readFileSync(path.join(root, "src", "pkjs", "pebble-js-app.js"), "utf8");
var match = html.match(/<script id="config-logic">([\s\S]*?)<\/script>/);
var sandbox = { console: console };
var settings;
var choices;
var keys;

assert.ok(match, "config page should contain the logic script");
vm.createContext(sandbox);
vm.runInContext(match[1], sandbox);

function sameSettings(actual, expected) {
	assert.strictEqual(JSON.stringify(actual), JSON.stringify(expected));
}

settings = sandbox.readSettings({});
sameSettings(settings, {
	battery: 0,
	vibeminutes: 0,
	language: 0,
	datestyle: 0,
	bluetooth: 0,
	connlost: 0,
	showseonds: 0,
	layout: 0
});

settings = sandbox.readSettings(sandbox.parseQuery(
	"?v=1.5&invert=1&battery=2&vibeminutes=20&language=4&datestyle=1&bluetooth=1&connlost=1&showseonds=1&layout=1"
));
assert.strictEqual(settings.battery, 2);
assert.strictEqual(settings.vibeminutes, 20);
assert.strictEqual(settings.language, 4);
assert.strictEqual(settings.datestyle, 1);
assert.strictEqual(settings.bluetooth, 1);
assert.strictEqual(settings.connlost, 1);
assert.strictEqual(settings.showseonds, 1);
assert.strictEqual(settings.layout, 1);

settings = sandbox.readSettings({
	language: "8",
	invert: "1",
	battery: "9",
	vibeminutes: "90",
	datestyle: "3",
	bluetooth: "-1",
	connlost: "2",
	showseonds: "4",
	layout: "9"
});
sameSettings(settings, {
	battery: 0,
	vibeminutes: 0,
	language: 0,
	datestyle: 0,
	bluetooth: 0,
	connlost: 0,
	showseonds: 0,
	layout: 0
});

choices = sandbox.withSelected(sandbox.VIBE_CHOICES, 20, function(value) {
	return "Every " + value + " minutes";
});
assert.strictEqual(
	JSON.stringify(choices.map(function(choice) { return choice.value; })),
	JSON.stringify([0, 5, 10, 15, 20, 30, 60])
);

choices = sandbox.withSelected(sandbox.VIBE_CHOICES, 60, function(value) {
	return "Every " + value + " minutes";
});
assert.strictEqual(choices.length, sandbox.VIBE_CHOICES.length);

keys = Object.keys(sandbox.readSettings({ animate: "1", russian: "1" }));
assert.strictEqual(JSON.stringify(keys), JSON.stringify([
	"battery", "vibeminutes", "language", "datestyle", "bluetooth", "connlost", "showseonds", "layout"
]));

assert.ok(html.indexOf('name="invert"') === -1);
assert.ok(html.indexOf('name="layout"') !== -1);
assert.ok(html.indexOf("Orbitron B") !== -1);
assert.ok(html.indexOf('name="showseonds"') !== -1);
assert.ok(html.indexOf("pebblejs://close#") !== -1);
assert.ok(html.indexOf("Воскресенье") === -1);
assert.ok(html.indexOf("Nederlands") !== -1);
assert.ok(html.indexOf("Dansk") !== -1);
assert.ok(html.indexOf('id="face"') !== -1);
assert.ok(html.indexOf('id="layout-d"') !== -1);
assert.ok(html.indexOf('id="face-island"') !== -1);
assert.ok(html.indexOf('id="face-vibe"') !== -1);
assert.ok(html.indexOf("hourvibe_20_20.png") !== -1);
assert.ok(html.indexOf('id="face-caption"') !== -1);

var face = sandbox.previewFace(sandbox.readSettings({}));
assert.strictEqual(face.time, "10:43");
assert.strictEqual(face.seconds, "");
assert.strictEqual(face.day, "Saturday");
assert.strictEqual(face.date, "15th Mar. 2014");
assert.strictEqual(face.slots.day.y, 128);
assert.strictEqual(face.slots.day.align, sandbox.FACE_ALIGN_RIGHT);
assert.strictEqual(face.slots.day.visible, true);
assert.strictEqual(face.slots.seconds.visible, false);
assert.strictEqual(face.batteryVisible, false);
assert.strictEqual(face.bluetoothVisible, false);
assert.strictEqual(face.batterySrc, "../resources/images/battery_80.png");
assert.strictEqual(face.island.visible, false);
assert.strictEqual(face.island.w, 0);
assert.strictEqual(face.island.h, 25);
assert.strictEqual(face.caption, "Battery shows only while charging. Vibrate is off.");

face = sandbox.previewFace(sandbox.readSettings({
	showseonds: "1",
	datestyle: "1",
	language: "1",
	battery: "2",
	bluetooth: "2",
	vibeminutes: "15",
	connlost: "1"
}));
assert.strictEqual(face.seconds, "42");
assert.strictEqual(face.day, "Samstag");
assert.strictEqual(face.date, "März, 15. 2014");
assert.strictEqual(face.batteryVisible, true);
assert.strictEqual(face.bluetoothVisible, true);
assert.strictEqual(face.bluetoothSrc, "../resources/images/bluetooth_on.png");
assert.strictEqual(face.island.visible, true);
assert.strictEqual(face.island.x, 7);
assert.strictEqual(face.island.y, 1);
assert.strictEqual(face.island.w, 129);
assert.strictEqual(face.island.h, 25);
assert.strictEqual(face.island.bluetooth.x, 4);
assert.strictEqual(face.island.bluetooth.y, 4);
assert.strictEqual(face.island.battery.x, 52);
assert.strictEqual(face.island.battery.y, 4);
assert.strictEqual(face.island.vibe.x, 105);
assert.strictEqual(face.island.vibe.y, 2);
assert.strictEqual(face.island.vibe.visible, true);
assert.ok(face.caption.indexOf("Double pulse every 15 minutes.") !== -1);
assert.ok(face.caption.indexOf("blinks the icon") !== -1);
assert.ok(face.caption.indexOf("Battery shows") === -1);

face = sandbox.previewFace(sandbox.readSettings({
	language: "3",
	datestyle: "0",
	battery: "1",
	bluetooth: "1",
	vibeminutes: "60"
}));
assert.strictEqual(face.day, "Samedi");
assert.strictEqual(face.date, "15 Mars 2014");
assert.strictEqual(face.batteryVisible, false);
assert.strictEqual(face.bluetoothVisible, false);
assert.strictEqual(face.island.visible, true);
assert.strictEqual(face.island.w, 28);
assert.strictEqual(face.island.x, 58);
assert.strictEqual(face.island.bluetooth.visible, false);
assert.strictEqual(face.island.battery.visible, false);
assert.strictEqual(face.island.vibe.x, 4);
assert.strictEqual(face.island.vibe.y, 2);
assert.strictEqual(face.island.vibe.w, 20);
assert.strictEqual(
	face.caption,
	"Battery shows at 20% or below, and while charging. Bluetooth shows when the phone disconnects. Double pulse each hour."
);

assert.strictEqual(
	sandbox.formatPreviewDate(3, 0, 1, 0, 2014),
	"1 Janv. 2014"
);
assert.strictEqual(
	sandbox.formatPreviewDate(4, 1, 2, 1, 2014),
	"Febbr., 2º 2014"
);
assert.strictEqual(sandbox.weekdayName(6, 6), "Lørdag");

face = sandbox.previewFace(sandbox.readSettings({ layout: "1", showseonds: "1" }));
assert.strictEqual(face.slots.day.y, 32);
assert.strictEqual(face.slots.day.align, sandbox.FACE_ALIGN_CENTER);
assert.strictEqual(face.slots.time.y, 51);
assert.strictEqual(face.slots.date.y, 91);
assert.strictEqual(face.seconds, "");
assert.ok(face.caption.indexOf("Seconds stay off on this layout.") !== -1);

face = sandbox.previewFace(sandbox.readSettings({ layout: "2", showseonds: "1", battery: "2" }));
assert.strictEqual(face.slots.time.y, 47);
assert.strictEqual(face.slots.day.visible, false);
assert.strictEqual(face.slots.date.visible, false);
assert.strictEqual(face.seconds, "");
assert.ok(face.caption.indexOf("Day, date, and seconds stay off.") !== -1);

assert.strictEqual(sandbox.isRound("1"), true);
assert.strictEqual(sandbox.isRound("chalk"), true);
assert.strictEqual(sandbox.isRound("gabbro"), true);
assert.strictEqual(sandbox.isRound("basalt"), false);
assert.strictEqual(sandbox.isRound(0), false);
assert.strictEqual(sandbox.supportedLayout(0, true), 1);
assert.strictEqual(sandbox.supportedLayout(2, true), 2);
assert.strictEqual(sandbox.readSettings({ layout: "0", round: "1" }).layout, 1);
assert.strictEqual(sandbox.readSettings({ layout: "2", round: "1" }).layout, 2);
assert.strictEqual(Object.prototype.hasOwnProperty.call(
	sandbox.readSettings({ layout: "0", round: "1" }), "round"), false);

face = sandbox.previewFace(sandbox.readSettings({ layout: "0", showseonds: "1" }), true);
assert.strictEqual(face.slots.day.y, 32);
assert.strictEqual(face.slots.day.align, sandbox.FACE_ALIGN_CENTER);
assert.strictEqual(face.seconds, "");
assert.ok(face.caption.indexOf("Orbitron D stays off on a round watch.") !== -1);
assert.ok(face.caption.indexOf("Seconds stay off on this layout.") !== -1);

assert.ok(phone.indexOf("https://thunderchild7.github.io/orbitron-d/config/index.html?v=1.6") !== -1);
assert.ok(phone.indexOf("function isRound()") !== -1);
assert.ok(phone.indexOf('platform === "chalk"') !== -1);
assert.ok(phone.indexOf('platform === "gabbro"') !== -1);
assert.ok(phone.indexOf('url += "&round=1"') !== -1);
assert.ok(phone.indexOf("ewobo.co.uk") === -1);

console.log("config page checks passed");
