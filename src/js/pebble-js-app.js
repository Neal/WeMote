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
	error: function(error) {
		appMessageQueue.clear();
		appMessageQueue.add({error:error});
		appMessageQueue.send();
	},
	toggle: function(index) {
		if (!this.server) return this.error('no_server_set');
		var xhr = new XMLHttpRequest();
		xhr.open('POST', 'http://' + this.server + ':5000/api/device/' + encodeURIComponent(this.devices[index].name), true);
		xhr.onload = function() {
			appMessageQueue.clear();
			try {
				var res = JSON.parse(xhr.responseText);
				var name = res.name.substring(0,32) || '';
				var host = res.host.substring(0,32) || '';
				var type = res.type.substring(0,16) || '';
				var state = res.state ? 'ON' : 'OFF';
				appMessageQueue.add({index:index, name:name, host:host, type:type, state:state});
				appMessageQueue.add({index:WeMo.devices.length});
			} catch(e) {
				appMessageQueue.add({index:index, name:WeMo.devices[index].name, host:'Error!', type:WeMo.devices[index].type, state:''});
				appMessageQueue.add({index:WeMo.devices.length});
			}
			appMessageQueue.send();
		};
		xhr.ontimeout = function() { WeMo.error('timeout'); };
		xhr.onerror = function() { WeMo.error('error'); };
		xhr.timeout = 10000;
		xhr.send(null);
	},
	refresh: function() {
		if (!this.server) return this.error('no_server_set');
		this.devices = [];
		var xhr = new XMLHttpRequest();
		xhr.open('GET', 'http://' + this.server + ':5000/api/environment', true);
		xhr.onload = function() {
			appMessageQueue.clear();
			try {
				var res = JSON.parse(xhr.responseText);
				var i = 0;
				for (var r in res) {
					var name = res[r].name.substring(0,32) || '';
					var host = res[r].host.substring(0,32) || '';
					var type = res[r].type.substring(0,16) || '';
					var state = res[r].state ? 'ON' : 'OFF';
					appMessageQueue.add({index:i++, name:name, host:host, type:type, state:state});
					WeMo.devices.push(res[r]);
				}
				appMessageQueue.add({index:i});
			} catch(e) {
				appMessageQueue.add({error:'server_error'});
			}
			appMessageQueue.send();
		};
		xhr.ontimeout = function() { WeMo.error('timeout'); };
		xhr.onerror = function() { WeMo.error('error'); };
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
	var uri = 'http://ineal.me/pebble/wemote/configuration?server=' + encodeURIComponent(WeMo.server);
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
