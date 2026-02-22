Frontend application to display/control the fencing score board
===============================================================

The application allows to display information coming from a Fencing Score Board "L'arbitre de touche", and to control some of its behavior.

It currently supports
* Showing on a computer screen
  * Current weapon
  * Lights for hits
* Control to switch weapon

We hope to add support for
* Choosing the board to connect with
* Passing some configuration
* Allowing faster baudrate and shorter messages to improve reactivity
* targeting mobile device (eg Android) with eg a Cordova package 

## Technologies

* HTML, JS for rendering
* serialport-node for Serial connection
* Electron for packaging as application