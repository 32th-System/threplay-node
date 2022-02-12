const fs = require('fs');
const prompt = require("prompt-sync")({sigint:true});
const thjson = require("./build/Debug/thjson.node");

var filename = prompt("Enter replay filename to read:");
fs.readFile(filename, function(err, data) {
    if(err) throw err;
    console.log(thjson.get_replay_data(data));
});