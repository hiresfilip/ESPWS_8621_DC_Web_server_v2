/* Funkce pro JavaScript a jQuery */
$(document).ready(function() {
    /*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
    let ws = new WebSocket('ws://' + document.location.host + '/ws', ['arduino']);
    let es = new EventSource('/events');
    /* Funkce pro zasílání zpráv */
    es.onmessage = function(e) {
        /* Do proměnné jsou uložena data z ESP ve formě JSON - konkrétně aktuální čas */
        const obj = JSON.parse(e.data);
        /* Funkce vypíše čas z DTP klienta do příslušného místa v HTML stránce do ID dtptime */
        $('#dtptime').empty().append($('<p/>').text(obj.time).html());
    };

    /* Jas LED pásku */
    let brightness = document.getElementById("brightness");
    let outputBrightness = document.getElementById("outputBrightness")

    /* Funkce naslouchá posuvníků a zaznamenává jeho hodnoty, které následně zašle skrz Web Socket */
    brightness.addEventListener('change', function() {
        outputBrightness.innerHTML = this.value;
        ws.send(this.value);
        /* Ovládání světelnosti na aktuálním čase a Color-pickeru v HTML stránce */
        $("#dtptime").css("opacity", (this.value / 100));
        $("#slider").css("opacity", (this.value / 100));
        console.log("Alfa: " + this.value);
    })

    /* Posuvník pro Color-picker */
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

    $.fn.roundSlider.prototype.updateColor = function() {
        let angle = this._handle1.angle - 90;
        let hsl_color = "hsl(" + angle + ", 100%, 50%)";
        let rgb_color = this._colorBox.css({
            background: hsl_color
        }).css("background-color");
        this._colorValue.html(rgb_color);
        this._raise("colorChange", {
            hsl: hsl_color,
            rgb: rgb_color
        });
    }

    let _fn1 = $.fn.roundSlider.prototype._setValue;
    $.fn.roundSlider.prototype._setValue = function() {
        if (!this._colorBox) {
            this._colorBox = $("<div class='color_box'></div>");
            this._colorValue = $("<div class='color_val'></div>");
            this.innerBlock.append(this._colorBox, this._colorValue);
        }
        _fn1.apply(this, arguments);
        this._raiseEvent("change");
    }
    let _fn2 = $.fn.roundSlider.prototype._raiseEvent;
    $.fn.roundSlider.prototype._raiseEvent = function(e) {
        this.updateColor();
        return _fn2.call(this, e);
    }

    $("#slider").roundSlider({
        radius: 110,
        width: 25,
        handleSize: "30,6",
        value: 25,
        showTooltip: false,
    });

    $("#slider").on('colorChange', function(e) {
        $("#dtptime").css("color", e.rgb);
        let json_string = JSON.stringify(rgbToObj(e.rgb));
        console.log(json_string);
        ws.send(json_string);
    });
});