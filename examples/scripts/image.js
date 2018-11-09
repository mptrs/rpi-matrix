var path   = require("path");
var Matrix = require('../../index.js');



function main() {

    var matrix = new Matrix({width:32, height:32});
    
    var canvas = matrix.getCanvas();
    var ctx = canvas.getContext('2d');

    ctx.clearRect(0, 0, matrix.width, matrix.height);
    canvas.render();

    canvas.loadImage(path.join(__dirname, '../images/32x32/50.png').then((image) => {
        console.log('image loaded', image);
        ctx.drawImage(image, 0, 0);
        canvas.render();

        return Promise.resolve();

    })
    .then(() => {
        return new Promise(function(resolve, reject) {
            setTimeout(resolve, 2000);
        });
    })
    .catch((error) => {
        console.log(error);
    });

    

 
}

main();




