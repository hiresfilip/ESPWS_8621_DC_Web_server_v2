var ws = null;

/* Funkce pro naslouchání z indexu skrze ID */
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

/* Funkce, která se stará o komunikace technologií WebSocket */
function startSocket() {
    ws = new WebSocket('ws://' + document.location.host + '/ws', ['arduino']);
    ws.binaryType = "arraybuffer";
    /* Když je připojen WebSocket */
    ws.onopen = function(e) {
        //addMessage("Connected");
    };
    /* Když WebSocket je nepřipojen */
    ws.onclose = function(e) {
        //addMessage("Disconnected");
    };
    /* V případě erroru WebSocket zašle zprávu */
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

/*let slider = document.getElementById("myRange");
let output = document.getElementById("demo");*/

/* Brightness of LED */
let brightness = document.getElementById("brightness");
let outputBrightness = document.getElementById("outputBrightness")

brightness.addEventListener('change', function() {
    outputBrightness.innerHTML = this.value;
    ws.send(ge("outputBrightness").value);
    ge("outputBrightness").value = this.value;
    console.log("Value of brightness: " + ge("outputBrightness").value);
})

/* ####################################################################################################################### */
$(document).ready(function() {
    //////###############################################//////
    //** code for color picker **//
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
        //////###############################################//////

    $("#slider").roundSlider({
        radius: 110,
        width: 25,
        handleSize: "30,6",
        value: 25,
        showTooltip: false,

        colorChange: function(e) {
            //e.hsl returns the hsl format color
            //e.rgb returns the rgb format color
            $("#title").css("color", e.rgb);
        }
    })
});

/* Color RED */
/*let red = document.getElementById("red");
let outputRed = document.getElementById("outputRed")

red.addEventListener('change', function() {
    outputRed.innerHTML = this.value;
    ws.send(ge("outputRed").value);
    ge("outputRed").value = this.value;
    console.log("Value of RED: " + ge("outputRed").value);
})*/

/* Color Green */
/*let green = document.getElementById("green");
let outputGreen = document.getElementById("outputGreen")

green.addEventListener('change', function() {
    outputGreen.innerHTML = this.value;
    ws.send(ge("outputGreen").value);
    ge("outputGreen").value = this.value;
    console.log("Value of Green: " + ge("outputGreen").value)
})

let blue = document.getElementById("blue");
let outputBlue = document.getElementById("outputBlue")

blue.addEventListener('change', function() {
    outputBlue.innerHTML = this.value;
    ws.send(ge("outputBlue").value);
    ge("outputBlue").value = this.value;
    console.log("Value of Blue: " + ge("outputBlue").value);
})
*/


//////###############################################//////
//** code for color picker **//
/*$(document).ready(function() {
    console.log("jQuery");
});*/
/*
$.fn.roundSlider.prototype.updateColor = function () {
    var angle = this._handle1.angle - 90;
    var hsl_color = "hsl(" + angle + ", 100%, 50%)";
    var rgb_color = this._colorBox.css({ background: hsl_color }).css("background-color");
    this._colorValue.html(rgb_color);
    this._raise("colorChange", { hsl: hsl_color, rgb: rgb_color });
  }
  var _fn1 = $.fn.roundSlider.prototype._setValue;
  $.fn.roundSlider.prototype._setValue = function () {
    if (!this._colorBox) {
      this._colorBox = $("<div class='color_box'></div>");
      this._colorValue = $("<div class='color_val'></div>");
      this.innerBlock.append(this._colorBox, this._colorValue);
    }
    _fn1.apply(this, arguments);
    this._raiseEvent("change");
  }
  var _fn2 = $.fn.roundSlider.prototype._raiseEvent;
  $.fn.roundSlider.prototype._raiseEvent = function (e) {
    this.updateColor();
    return _fn2.call(this, e);
  }
  //////###############################################//////
  
  $("#slider").roundSlider({
    radius: 110,
    width: 25,
    handleSize: "30,6",
    value: 25,
    showTooltip: false,
  
    colorChange: function (e) {
      // e.hsl returns the hsl format color
      // e.rgb returns the rgb format color
      $("#title").css("color", e.rgb);
    }
  })*/