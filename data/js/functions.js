var ws = null;

function ge(s) {
    return document.getElementById(s);
}

function ce(s) {
    return document.createElement(s);
}

function stb() {
    window.scrollTo(0, document.body.scrollHeight || document.documentElement.scrollHeight);
}

function sendBlob(str) {
    var buf = new Uint8Array(str.length);
    for (var i = 0; i < str.length; ++i) buf[i] = str.charCodeAt(i);
    ws.send(buf);
}

function addMessage(m) {
    var msg = ce("div");
    msg.innerText = m;
    ge("dbg").appendChild(msg);
    stb();
}

function startSocket() {
    ws = new WebSocket('ws://' + document.location.host + '/ws', ['arduino']);
    ws.binaryType = "arraybuffer";
    ws.onopen = function(e) {
        //addMessage("Connected");
    };
    ws.onclose = function(e) {
        //addMessage("Disconnected");
    };
    ws.onerror = function(e) {
        console.log("ws error", e);
        addMessage("Error");
    };
    ws.onmessage = function(e) {
        var msg = "";
        if (e.data instanceof ArrayBuffer) {
            msg = "BIN:";
            var bytes = new Uint8Array(e.data);
            for (var i = 0; i < bytes.length; i++) {
                msg += String.fromCharCode(bytes[i]);
            }
        } else {
            //msg = "TXT:" + e.data;
        }
        //addMessage(msg);
    };
    ge("input_el").onkeydown = function(e) {
        stb();
        if (e.keyCode == 13 && ge("input_el").value != "") {
            ws.send(ge("input_el").value);
            ge("input_el").value = "";
        }
    }
}

function startEvents() {
    var es = new EventSource('/events');
    es.onopen = function(e) {
        //addMessage("Events Opened");
    };
    es.onerror = function(e) {
        if (e.target.readyState != EventSource.OPEN) {
            //addMessage("Events Closed");
        }
    };
    es.onmessage = function(e) {
        //addMessage("Event: " + e.data);
        const obj = JSON.parse(e.data);
        $('#dtptime').empty().append($('<p/>').text('Actual Time: ' + obj.time).html());

    };
    es.addEventListener('ota', function(e) {
        //addMessage("Event[ota]: " + e.data);
    }, false);
}

function onBodyLoad() {
    startSocket();
    startEvents();
}

let slider = document.getElementById("myRange");
let output = document.getElementById("demo");

slider.addEventListener('change', function() {
    output.innerHTML = this.value;
    ws.send(ge("demo").value);
    ge("demo").value = this.value;
    console.log("Value of slider: " + ge("demo").value);
});