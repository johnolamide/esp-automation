var gateway = "ws:/" + "/" + location.host + ":81";
var webSocket;
window.addEventListener("load", onLoad);

var relay1State = 0;
var relay2State = 0;

function initWebSocket() {
    console.log("Trying to open a webSocket connection");
    webSocket = new WebSocket(gateway);
    webSocket.onopen = onOpen;
    webSocket.onclose = onClose;
    webSocket.onerror = onError;
    webSocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log("Connection Open");
}

function onClose(event) {
    console.log("Connection Closed");
    setTimeout(initWebSocket, 2000);
}

function onError(event) {
    console.log("Error in Connection");
}

function onMessage(event) {
    var Data = JSON.parse(event.data);
    console.log(Data);
    var relay1state;
    var relay2state;
    relay1state = ((Data["Relay1State"] == 1 ? "ON" : "OFF"));
    relay2state = ((Data["Relay2State"] == 1 ? "ON" : "OFF"));
    document.getElementById('relay1state').innerHTML = relay1state;
    document.getElementById('relay2state').innerHTML = relay2state;
}

function toggleRelay1() {
    console.log("toggleRelay1");
    if (relay1State == 0) {
        relay1State = 1;
    } else {
        relay1State = 0;
    }
    send_data();
}

function toggleRelay2() {
    console.log("toggleRelay2");
    if (relay2State == 0) {
        relay2State = 1;
    } else {
        relay2State = 0;
    }
    send_data();
}

function initButton() {
    document.getElementById('relay1button').addEventListener('click', toggleRelay1);
    document.getElementById('relay2button').addEventListener('click', toggleRelay2);
}

function onLoad(event) {
    initWebSocket();
    initButton();
}

function send_data() {
    var data = '{"Action":"toggle","Relay1State": '+relay1State+',"Relay2State": '+relay2State+'}';
    webSocket.send(data);
}