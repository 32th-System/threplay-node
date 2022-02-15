const fs = require('fs');
const prompt = require("prompt-sync")({sigint:true});
const threplay = require("./build/Debug/threplay.node");

var filename = prompt("Enter replay filename to read:");
fs.readFile(filename, function(err, data) {
    if(err) throw err;
    console.log(threplay.get_replay_data(data));
});