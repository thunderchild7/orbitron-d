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
	showseonds: 0
});

settings = sandbox.readSettings(sandbox.parseQuery(
	"?v=1.5&invert=1&battery=2&vibeminutes=20&language=4&datestyle=1&bluetooth=1&connlost=1&showseonds=1"
));
assert.strictEqual(settings.battery, 2);
assert.strictEqual(settings.vibeminutes, 20);
assert.strictEqual(settings.language, 4);
assert.strictEqual(settings.datestyle, 1);
assert.strictEqual(settings.bluetooth, 1);
assert.strictEqual(settings.connlost, 1);
assert.strictEqual(settings.showseonds, 1);

settings = sandbox.readSettings({
	language: "8",
	invert: "1",
	battery: "9",
	vibeminutes: "90",
	datestyle: "3",
	bluetooth: "-1",
	connlost: "2",
	showseonds: "4"
});
sameSettings(settings, {
	battery: 0,
	vibeminutes: 0,
	language: 0,
	datestyle: 0,
	bluetooth: 0,
	connlost: 0,
	showseonds: 0
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
	"battery", "vibeminutes", "language", "datestyle", "bluetooth", "connlost", "showseonds"
]));

assert.ok(html.indexOf('name="invert"') === -1);
assert.ok(html.indexOf('name="showseonds"') !== -1);
assert.ok(html.indexOf("pebblejs://close#") !== -1);
assert.ok(html.indexOf("Воскресенье") === -1);
assert.ok(html.indexOf("Nederlands") !== -1);
assert.ok(html.indexOf("Dansk") !== -1);

assert.ok(phone.indexOf("https://thunderchild7.github.io/orbitron-d/config/index.html?v=1.5") !== -1);
assert.ok(phone.indexOf("ewobo.co.uk") === -1);

console.log("config page checks passed");
