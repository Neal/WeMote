var appMessageQueue = {
	queue: [],
	numTries: 0,
	maxTries: 5,
	add: function(obj) {
		this.queue.push(obj);
	},
	clear: function() {
		this.queue = [];
	},
	isEmpty: function() {
		return this.queue.length === 0;
	},
	nextMessage: function() {
		return this.isEmpty() ? {} : this.queue[0];
	},
	send: function() {
		if (this.queue.length > 0) {
			var ack = function() {
				appMessageQueue.numTries = 0;
				appMessageQueue.queue.shift();
				appMessageQueue.send();
			};
			var nack = function() {
				appMessageQueue.numTries++;
				appMessageQueue.send();
			};
			if (this.numTries >= this.maxTries) {
				console.log('Failed sending AppMessage: ' + JSON.stringify(this.nextMessage()));
				ack();
			}
			console.log('Sending AppMessage: ' + JSON.stringify(this.nextMessage()));
			Pebble.sendAppMessage(this.nextMessage(), ack, nack);
		}
	}
};

var WeMo = {
	server: localStorage.getItem('server') || '',
	devices: [],
	noServerSet: function() {
		appMessageQueue.clear();
		appMessageQueue.add({host:true});
		appMessageQueue.send();
	},
	success: function(res) {
		if (res.model) res = [res];
		appMessageQueue.clear();
		var index = 0;
		for (var d in res) {
			var name = res[d].name.substring(0,32) || '';
			var host = res[d].host.substring(0,32) || '';
			var type = res[d].type.substring(0,16) || '';
			var state = res[d].state ? 'ON' : 'OFF';
			appMessageQueue.add({index:index++, name:name, host:host, type:type, state:state});
			this.devices.push(res[d]);
		}
		appMessageQueue.add({index:index});
		appMessageQueue.send();
	},
	fail: function() {
		appMessageQueue.clear();
		appMessageQueue.add({name:true});
		appMessageQueue.send();
	},
	toggle: function(index) {
		if (!this.server) return this.noServerSet();
		var xhr = new XMLHttpRequest();
		xhr.open('POST', 'http://' + this.server + ':5000/api/device/' + encodeURIComponent(this.devices[index].name), true);
		xhr.onload = function() { try { WeMo.success(JSON.parse(xhr.responseText)); } catch(e) { WeMo.fail(); } };
		xhr.ontimeout = this.fail;
		xhr.onerror = this.fail;
		xhr.timeout = 10000;
		xhr.send(null);
	},
	refresh: function() {
		if (!this.server) return this.noServerSet();
		var xhr = new XMLHttpRequest();
		xhr.open('GET', 'http://' + this.server + ':5000/api/environment', true);
		xhr.onload = function() { try { WeMo.success(JSON.parse(xhr.responseText)); } catch(e) { WeMo.fail(); } };
		xhr.ontimeout = this.fail;
		xhr.onerror = this.fail;
		xhr.timeout = 10000;
		xhr.send(null);
	}
};

Pebble.addEventListener('ready', function(e) {
	WeMo.refresh();
});

Pebble.addEventListener('appmessage', function(e) {
	if (typeof(e.payload.index) != 'undefined') WeMo.toggle(e.payload.index); else WeMo.refresh();
});

Pebble.addEventListener('showConfiguration', function() {
	var uri = 'http://neal.github.io/WeMote/configuration/index.html?server=' + encodeURIComponent(WeMo.server);
	Pebble.openURL(uri);
});

Pebble.addEventListener('webviewclosed', function(e) {
	if (e.response) {
		var data = JSON.parse(decodeURIComponent(e.response));
		WeMo.server = data.server;
		localStorage.setItem('server', WeMo.server);
		WeMo.refresh();
	}
});
