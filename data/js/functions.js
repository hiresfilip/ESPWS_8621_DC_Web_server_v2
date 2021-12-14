/* Funkce pro JavaScript a jQuery */

var ws = io();

/* Funkce pro naslouchání z HTML skrze ID */
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

/* Funkce, která se stará o poslání zpráv na HTML stránku */

function addMessage(m) {
    var msg = ce("div");
    msg.innerText = m;
    ge("dbg").appendChild(msg);
    stb();
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* Funkce, která se stará o komunikace technologií WebSocket */
function startSocket() {
    //ws = new WebSocket('ws://' + document.location.host + '/ws', ['arduino']);
    ws = new WebSocket('ws://' + '192.168.10.33' + '/ws', ['arduino']);
    ws.binaryType = "arraybuffer";

    /* Když je připojen WebSocket zašle zprávu o připojení */
    ws.onopen = function(e) {
        //addMessage("Connected");
    };

    /* Když WebSocket není připojený, zašle zprávu, že není připojen */
    ws.onclose = function(e) {
        //addMessage("Disconnected");
    };

    /* V případě erroru WebSocket zašle zprávu error a vysvětlení erroru*/
    ws.onerror = function(e) {
        console.log("ws error", e);
        addMessage("Error");
    };

    /* Funkce pro poslání zprávy */
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

    /* Funkce pro posílání dat do ESP skrze Web Socketu */
    ge("input_el").onkeydown = function(e) {
        stb();
        if (e.keyCode == 13 && ge("input_el").value != "") {
            ws.send(ge("input_el").value);
            ge("input_el").value = "";
        }
    }
    /*ge("#slider").onchange = function(e){
        stb();
            //ws.send(rgbToObj(e.rgb));
            ws.send(ge("#slider").rgbToObj(e.rgb));
            ge("#slider").value = e.rgb;
            //ws.send(rgbToObj(e.rgb));
    }*/      
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* Funkce pro posílání eventů (událostí) */
function startEvents() {
    var es = new EventSource('/events');

    /* Když jsou eventy dostupné, zašle zprávu o přístupnosti */
    es.onopen = function(e) {
        //addMessage("Events Opened");
    };

    /* Když jsou eventy nedostupné, zašle zprávu o nepřístupnosti */
    es.onerror = function(e) {
        if (e.target.readyState != EventSource.OPEN) {
            //addMessage("Events Closed");
        }
    };

    /* Funkce pro zasílání zpráv */
    es.onmessage = function(e) {
        //addMessage("Event: " + e.data);

        /* Do proměnné jsou uložena data z ESP ve formě JSON - konkrétně aktuální čas */
        const obj = JSON.parse(e.data);

        /* Funkce vypíše čas z DTP klienta do příslušného místa v HTML stránce do ID dtptime */
        $('#dtptime').empty().append($('<p/>').text(obj.time).html());
    };

    /* Funkce, která přenáší zprávy přes "vzduch" */
    es.addEventListener('ota', function(e) {
        //addMessage("Event[ota]: " + e.data);
    }, false);
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* Funkce načte funkce startSocket a startEvents */
function onBodyLoad() {
    startSocket();
    startEvents();
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* Jas LED pásku */
let brightness = document.getElementById("brightness");
let outputBrightness = document.getElementById("outputBrightness")

/* Funkce naslouchá posuvníků a zaznamenává jeho hodnoty, které následně zašle skrz Web Socket */
brightness.addEventListener('change', function() {
    outputBrightness.innerHTML = this.value;
    ws.send(ge("outputBrightness").value);
    ge("outputBrightness").value = this.value;

    /* Ovládání světelnosti na aktuálním čase a Color-pickeru v HTML stránce */
    $("#dtptime").css("opacity", (this.value / 100));
    $("#slider").css("opacity", (this.value / 100));

    console.log("Alfa: " + ge("outputBrightness").value);
})


/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
function rgbToObj(rgb) {
  
    let colors = ["red", "green", "blue", "alpha"]
  
    let colorArr = rgb.slice(
        rgb.indexOf("(") + 1, 
        rgb.indexOf(")")
    ).split(", ");
  
    let obj = new Object();
  
    colorArr.forEach((k, i) => {
        obj[colors[i]] = k
    })
  
    return obj;
}

/* Color-picker řešený pomocí jQuery */
$(document).ready(function() {
    $.fn.roundSlider.prototype.updateColor = function() {
        var angle = this._handle1.angle - 90;
        var hsl_color = "hsl(" + angle + ", 100%, 50%)";
        var rgb_color = this._colorBox.css({
            background: hsl_color
        }).css("background-color");
        this._colorValue.html(rgb_color);
        this._raise("colorChange", {
            hsl: hsl_color,
            rgb: rgb_color
        });
        console.log(rgb_color);
    }

    var _fn1 = $.fn.roundSlider.prototype._setValue;
    $.fn.roundSlider.prototype._setValue = function() {
        if (!this._colorBox) {
            this._colorBox = $("<div class='color_box'></div>");
            this._colorValue = $("<div class='color_val'></div>");
            this.innerBlock.append(this._colorBox, this._colorValue);
        }
        _fn1.apply(this, arguments);
        this._raiseEvent("change");
    }
    var _fn2 = $.fn.roundSlider.prototype._raiseEvent;
    $.fn.roundSlider.prototype._raiseEvent = function(e) {
        this.updateColor();
        return _fn2.call(this, e);
    }

    /* Posuvník pro Color-picker */

    $("#slider").roundSlider({
        radius: 110,
        width: 25,
        handleSize: "30,6",
        value: 25,
        showTooltip: false,

        colorChange: function(e) {
            // e.hsl returns the hsl format color
            // e.rgb returns the rgb format color
            //$("#title").css("color", e.rgb);
            $("#dtptime").css("color", e.rgb)

            //ws.send(ge(e.rgb));
            //ge(e.rgb);
            
            /*var json_arr = {};
            json_arr["name1"] = "value1";
            json_arr["name2"] = "value2";
            json_arr["name3"] = "value3";
    
            var json_string = JSON.stringify(json_arr);*/

            var json_arr = {};
            json_arr["barva"] = rgbToObj(e.rgb);
            var json_string = JSON.stringify(json_arr);
            console.log(json_string);
            ws.send(json_string);
            // ge(json_string) = this.json_string;
        }
    })
})
;