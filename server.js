var express = require("express");
var app = express();
var bodyParser = require('body-parser');
var errorHandler = require('errorhandler');
var methodOverride = require('method-override');
var MS = require("mongoskin");
var hostname = process.env.HOSTNAME || 'localhost';
var port = 8080;

var accX, accY, accZ, gyroX, gyroY, gyroZ, activity;

var db = MS.db("mongodb://127.0.0.1:27017/sensorData");

app.get("/", function (req, res) {
    res.redirect("index.html")
});

app.get("/sendData", function (req, res) {
    console.log(req.query);
    accX = req.query.x_acc
    accY = req.query.y_acc
    accZ = req.query.z_acc
    gyroX = req.query.x_gyro
    gyroY = req.query.y_gyro
    gyroZ = req.query.z_gyro

    activity = req.query.Activity //|| "p"
    req.query.time = new Date().getTime();

    // console.log("Received data:", req.query);
    // console.log("accX:", accX, "accY:", accY, "accZ:", accZ, "pos:", pos);
    

   db.collection("data").insertOne(req.query, function(result, err){
       //console.log(result, err);
    res.send("1");
   });
});


app.get("/getData", function (req, res) {
    var ret = {
      x_acc: accX,
      y_acc: accY,
      z_acc: accZ,
      x_gyro: gyroX,
      y_gyro: gyroY,
      z_gyro: gyroZ,
      Activity: activity // Include the posture information
    };
  
    res.json(ret); // Send the response as JSON
  });
  

app.use(methodOverride());
app.use(bodyParser());
app.use(express.static(__dirname + '/public'));
app.use(errorHandler());

console.log("Simple static server listening at http://" + hostname + ":" + port);
app.listen(port);

