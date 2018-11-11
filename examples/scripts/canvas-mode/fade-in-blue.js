#!/usr/bin/env node
var Matrix = require('../../../index.js');

class Sample extends Matrix {

    run() {
        var ctx = this.canvas.getContext('2d');

        // Fill matrix with blue
        ctx.fillStyle = 'blue';
        ctx.fillRect(0, 0, this.width, this.height);

        // Fade in using 500 steps
        this.render({blend:500});

        // Delay a second
        setTimeout(() => {}, 1000);
    }
};

var sample = new Sample({mode:'canvas', width:32, height:32});
sample.run();