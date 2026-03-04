// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.

const { SerialPort } = require('serialport')
const tableify = require('tableify')

async function listSerialPorts() {
  await SerialPort.list().then((ports, err) => {
    if(err) {
      document.getElementById('error').textContent = err.message
      return
    } else {
      document.getElementById('error').textContent = ''
    }
    console.log('ports', ports);

    if (ports.length === 0) {
      document.getElementById('error').textContent = 'No ports discovered'
    }

    tableHTML = tableify(ports)
    document.getElementById('ports').innerHTML = tableHTML
  })
}

function updateConnectionValues() {
	document.getElementById("serialConnection").value = port.path;
	document.getElementById("baudrate").value = port.baudRate;
}

function applyWeapon(weapon) {
	port.write(weapon + "\n");
	document.getElementById("weaponDialog").close(weapon);
}
function applyTimes() {
	// TODO consider setting a class and factorize using getElementsByClass
	port.write("SABRE.depress=" + document.getElementById("SABRE.depress").value + "\n");
	port.write("SABRE.lockout=" + document.getElementById("SABRE.lockout").value + "\n");
	port.write("EPEE.depress=" + document.getElementById("EPEE.depress").value + "\n");
	port.write("EPEE.lockout=" + document.getElementById("EPEE.lockout").value + "\n");
	port.write("FLEURET.depress=" + document.getElementById("FLEURET.depress").value + "\n");
	port.write("FLEURET.lockout=" + document.getElementById("FLEURET.lockout").value + "\n");
	document.getElementById("configuration").close(null);
	port.write("?\n"); // to get updated values
}
function resetFIERules() {
	document.getElementById("FLEURET.depress").value=14000;
	document.getElementById("FLEURET.lockout").value=300000
	document.getElementById("EPEE.depress").value=2000
	document.getElementById("EPEE.lockout").value=45000
	document.getElementById("SABRE.depress").value=1000
	document.getElementById("SABRE.lockout").value=170000
}

var port = {};

//function listPorts() {
//  listSerialPorts();
//  setTimeout(listPorts, 2000);
//}
//// Set a timeout that will check for new serialPorts every 2 seconds.
//// This timeout reschedules itself.
//setTimeout(listPorts, 2000);

var serialLog = [];
var port = null;

var osc = null;
function buzz() {
	if (osc == null) {
		const context = new (window.AudioContext || window.webkitAudioContext)();
		osc = context.createOscillator(); // instantiate an oscillator
		osc.type = 'sawtooth'; // this is the default - also square, sawtooth, triangle
		osc.frequency.value = 220; // Hz
		osc.connect(context.destination); // connect it to the destination
		osc.start(); // start the oscillator
	}
}
// one context per document

function setupSerialPort(path, baudrate) {
	if (port) {
		port.close();
		port = null;
	}
	serialLog = [];
	port = new SerialPort({
	  path: path,
	  baudRate: baudrate,
	});
	var currentLine = "";
	// Switches the port into "flowing mode"
	port.on('data', function (data) {
	  const text = new TextDecoder().decode(data);
	  serialLog.push({text: text, time: Date.now(), isError: false});
	  var lines = text.split(/\r?\n/);
	  lines[0] = currentLine + lines[0];
	  if (text.charAt(text.length - 1) != '\n') {
		currentLine = lines[lines.length - 1];
		lines = lines.slice(0, -1);
	  } else {
		currentLine = "";
	  }
	  lines.forEach(line => {
		console.log(line);
		if (line.startsWith("R")) {
			document.getElementById('hitOnTargetA').style.backgroundColor = "red";
			document.getElementById('hitOnTargetA').innerText = "Touche!";
			buzz();
		} else if (line.startsWith("r")) {
			document.getElementById('hitOffTargetA').style.backgroundColor = "lightyellow";
			document.getElementById('hitOffTargetA').innerText = "Touche!";
			buzz();
		} else if (line.startsWith("g")) {
			document.getElementById('hitOffTargetB').style.backgroundColor = "lightyellow";	
			document.getElementById('hitOffTargetB').innerText = "Touche!";
			buzz();
		} else if (line.startsWith("G")) {
			document.getElementById('hitOnTargetB').style.backgroundColor = "lightgreen";		
			document.getElementById('hitOnTargetB').innerText = "Touche!";
			buzz();
		} else if (line.startsWith("0")) {
			if (osc) {
				osc.stop();
				osc = null;
			}
			document.getElementById('hitOnTargetA').style.backgroundColor = "#100000";
			document.getElementById('hitOnTargetA').innerText = "";
			document.getElementById('hitOffTargetA').style.backgroundColor = null;
			document.getElementById('hitOffTargetA').innerText = "";
			document.getElementById('hitOffTargetB').style.backgroundColor = null;
			document.getElementById('hitOffTargetB').innerText = "";
			document.getElementById('hitOnTargetB').style.backgroundColor = "#001000";
			document.getElementById('hitOnTargetB').innerText = "";
		} else {
			const segments = line.split("=");
			if (segments.length == 2) {
				const elt = document.getElementById(segments[0]);
				if (elt) {
					if (elt.tagName === "INPUT") {
						elt.value = segments[1];
					} else {
						elt.innerHTML = segments[1];
					}
				}
			}
		}
	  });
	});
	port.on('error', err => {
		console.log(err);
		serialLog.push({text: err.message, time: Date.now(), isError: true});
	});
	updateConnectionValues();
}


function updatePortFromUI() {
	document.getElementById("arme").innerHTML = "⚙️ Connection en cours...";
	const baudrate = parseInt(document.getElementById("baudrate").value);
	const serialPath = document.getElementById("serialConnection").value;
	setupSerialPort(serialPath, baudrate);
}
setupSerialPort('/dev/ttyUSB0', 57600);

