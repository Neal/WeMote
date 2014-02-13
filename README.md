# WeMote

WeMote is a remote app for the Pebble Smartwatch to *flip* your WeMo switches from your wrist.

It requires the [Ouimeaux](http://ouimeaux.readthedocs.org/en/latest/index.html) REST API running on a computer on the network to be able to work. An update in the future **_may_** remove this requirement.

## Running Ouimeaux REST API

* Following instructions [here](http://ouimeaux.readthedocs.org/en/latest/installation.html) to install Ouimeaux.
* Run `wemo server` in terminal to start the webserver. (pro-tip: run it in a `tmux` or a `screen` session)
* Note down this computers' local IP address as that's what you'll enter in the configuration page.

## License

WeMote is available under the MIT license. See the LICENSE file for more info.
