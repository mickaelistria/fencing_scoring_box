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

function updateWeapon() {
	document.getElementById("weaponDialog").showModal();
}

//function listPorts() {
//  listSerialPorts();
//  setTimeout(listPorts, 2000);
//}
//// Set a timeout that will check for new serialPorts every 2 seconds.
//// This timeout reschedules itself.
//setTimeout(listPorts, 2000);

const port = new SerialPort({
  path: '/dev/ttyUSB0',
  baudRate: 9600,
});
// Switches the port into "flowing mode"
port.on('data', function (data) {
  const text = new TextDecoder().decode(data);
  const lines = text.split(/\r?\n/);
  lines.forEach(line => {
//	console.log(line);
	if (line.includes("hitOnTargA") && line.includes("1")) {
		document.getElementById('hitOnTargetA').style.backgroundColor = "red";
		document.getElementById('hitOnTargetA').innerText = "Touche!";
	} else if (line.includes("hitOffTargA") && line.includes("1")) {
		document.getElementById('hitOffTargetA').style.backgroundColor = "lightyellow";
		document.getElementById('hitOffTargetA').innerText = "Touche!";
	} else if (line.includes("hitOffTargB") && line.includes("1")) {
		document.getElementById('hitOffTargetB').style.backgroundColor = "lightyellow";	
		document.getElementById('hitOffTargetB').innerText = "Touche!";
	} else if (line.includes("hitOnTargB") && line.includes("1")) {
		document.getElementById('hitOnTargetB').style.backgroundColor = "green";		
		document.getElementById('hitOnTargetB').innerText = "Touche!";
	} else if (line.includes("Reset")) {
		document.getElementById('hitOnTargetA').style.backgroundColor = "#500000";
		document.getElementById('hitOnTargetA').innerText = "";
		document.getElementById('hitOffTargetA').style.backgroundColor = "grey";
		document.getElementById('hitOffTargetA').innerText = "";
		document.getElementById('hitOffTargetB').style.backgroundColor = "grey";
		document.getElementById('hitOffTargetB').innerText = "";
		document.getElementById('hitOnTargetB').style.backgroundColor = "#003000";
		document.getElementById('hitOnTargetB').innerText = "";
	} else if (line.includes("EPEE")) {
		document.getElementById("arme").innerText = "Epée";
	} else if (line.includes("SABRE")) {
		document.getElementById("arme").innerText = "Sabre";
	} else if (line.includes("FLEURET")) {
		document.getElementById("arme").innerText = "Fleuret";
	}
  });
});

const weapongDialog = document.getElementById("weaponDialog");
weapongDialog.addEventListener("close", (e) => {
	const ret = weapongDialog.returnValue;
    port.write(ret, err => {
		if (err) {
			console.log(err);
		}
	});
});
document.getElementsByName("armeButton").forEach(button => {
	button.addEventListener("click", (event) => {
	  event.preventDefault(); // Nous ne voulons pas soumettre ce faux formulaire
	  weapongDialog.close(button.value); // Il faut envoyer la valeur du sélecteur ici.
	});
});
