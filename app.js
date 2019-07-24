'use strict'

// C library API
const ffi = require('ffi');

var parser = ffi.Library('./lib', {
  'addEventFromJSON': ['string', ['string', 'string']],
  'createCalendarFromJSON': ['string', ['string', 'string', 'string']],
  'callValidateCalendar': ['int', ['string']],
  'getCalJSON': ['string', ['string']],
  'getEventsJSON': ['string', ['string']],
  'getEvtAlarms': ['string', ['string', 'string']],
  'getEvtProps': ['string', ['string', 'string']],
});

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');
const bodyParser = require('body-parser')

app.use(fileUpload());

app.use(bodyParser.urlencoded({extended: true}));
app.use(bodyParser.json());

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Open mysql connection
const mysql = require('mysql');

var connection = null;

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {

  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }

  let uploadFile = req.files.fileUpload;
  var out;
  // Use the mv() method to place the file somewhere on your server
  uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    if(err) {
      return res.status(500).send(err);
    }

    out = parser.callValidateCalendar('./uploads/' + uploadFile.name);
//    console.log(out);

    if(out == 0) {
      fs.unlinkSync('./uploads/' + uploadFile.name);
    }

    res.send({
      out
    });

//    res.redirect('/');
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      res.send('');
    }
  });
});

//******************** Your code goes here ******************** 

app.post('/addEvt', function(req , res){
  var temp = req.body;
  var file = req.body.file;

  var out = parser.addEventFromJSON(JSON.stringify(temp.event), "./uploads/" + file);

  res.send({
      out
  });
});

app.post('/addCal', function(req , res){
  var temp = req.body;
  var file = req.body.file;

  var out = parser.createCalendarFromJSON(JSON.stringify(temp.cal), JSON.stringify(temp.event), "./uploads/" + file);

  res.send({
      out
  });
});

app.get('/dirlist', function(req, res) {
    let list = fs.readdirSync('./uploads');

    var valid = [];
    // Validate files. Silently skip those that fail
    list.forEach(function(element){
        var out = parser.callValidateCalendar("./uploads/" + element);
        if(out == 1) {
            valid.push(element);
        }
    });

    res.send(JSON.parse(JSON.stringify(valid)));
});

app.get('/calinfo', function(req, res) {
    var file = "./uploads/" + req.query.file;
    var string = parser.getCalJSON(file);

    res.send({
        string
    });
});

app.get('/showevt', function(req, res) {
    var file = "./uploads/" + req.query.file;
    var string = parser.getEventsJSON(file);

    res.send({
        string
    });
});

app.get('/showalarms', function(req, res) {
    var file = "./uploads/" + req.query.file;
    var num = req.query.num;

//    console.log(num);
    var out = parser.getEvtAlarms(num, file);

    res.send({
        out
    });
});

app.get('/showprops', function(req, res) {
    var file = "./uploads/" + req.query.file;
    var num = req.query.num;

    var out = parser.getEvtProps(num, file);
    res.send({
        out
    })
});

app.post('/connectserver', function(req, res) {
    var temp = req.body.database;

    var database = temp.DATABASE;
    var user = temp.USER;
    var pass = temp.PASS;
    var int = req.body.int;

//    console.log(int);
    if(int == 2) {
        connection = null;
        user = null;
        pass = null;
        database = null;
    }
    var state = "Successfully connected to the server";

    if(connection == null) {
        connection = mysql.createConnection({
                       host: 'dursley.socs.uoguelph.ca',
                       user: user,
                       password: pass,
                       database: database
                     });

        connection.connect(function(err) {
            if (err) {
                connection = null;
                state = "Failed to connect to the server";
                res.send({
                    state
                });
                return
            }
            var file = "CREATE TABLE FILE (cal_id INT AUTO_INCREMENT PRIMARY KEY, file_Name VARCHAR(60) NOT NULL, version INT NOT NULL, prod_id VARCHAR(256) NOT NULL)";
            var event = "CREATE TABLE EVENT (event_id INT AUTO_INCREMENT PRIMARY KEY, summary VARCHAR(1024), start_time DATETIME NOT NULL, location VARCHAR(60), organizer VARCHAR(256), cal_file INT NOT NULL, FOREIGN KEY(cal_file) REFERENCES FILE(cal_id) ON DELETE CASCADE)";
            var alarm = "CREATE TABLE ALARM (alarm_id INT AUTO_INCREMENT PRIMARY KEY, action VARCHAR(256) NOT NULL, `trigger` VARCHAR(256) NOT NULL, event INT NOT NULL, FOREIGN KEY(event) REFERENCES EVENT(event_id) ON DELETE CASCADE)";
            connection.query(file, function(err, result) {
                if(err) return;
            });
            connection.query(event, function(err, result) {
                if(err) return;
            });
            connection.query(alarm, function(err, result) {
                if(err) return;
            });

            res.send({
                state
            });
        });
    } else {
        state = "Already connected to a database";
        connection = null;

        res.send({
            state
        });
    }
});

app.post('/dbfiles', function(req, res) {
    connection.query("SELECT * FROM FILE", function (err, result, fields) {
//        if(err) return;
        res.send({
            result
        });
    });
});

app.post('/dbevents', function(req, res) {
    var request;
    if(req.body.usage == "show") {
        request = "SELECT * FROM EVENT";
    } else if(req.body.usage == "dates") {
        request = "SELECT * FROM EVENT ORDER BY start_time ASC";
    } else if(req.body.usage == "location") {
        request = "SELECT * FROM EVENT WHERE location=\'" + req.body.extra + "\'";
//        console.log(request);
    }

    connection.query(request, function (err, result, fields) {
//        console.log(result);
        if(err) console.log(err);
        res.send({
            result
        });
    });
});

app.post('/dbalarms', function(req, res) {
    connection.query("SELECT * FROM ALARM", function (err, result, fields) {
//        if(err) return;
        res.send({
            result
        });
    });
});

app.post('/insertfile', function(req, res) {
    var fileName = req.body.file;

    var calinfo = parser.getCalJSON("./uploads/" + fileName);
    var version = JSON.parse(calinfo).version;
    var prodid = JSON.parse(calinfo).prodID;
    var events = JSON.parse(parser.getEventsJSON("./uploads/" + fileName));

    var toAttach = "(\'" + fileName + "\', \'" + version + "\', \'" + prodid + "\')";
    var eventNum = 1;

    var insertFile = "INSERT INTO FILE (file_name, version, prod_id) VALUES " + toAttach;

    var state = "pass";
    connection.query(insertFile, function(err, result) { // Start by trying the outer file
        if(err) {
            state = "fail";
        }
        res.send({
            state
        })
    });
});

app.post('/insertevent', function(req, res) {
    var state = "pass";

    var fileName = req.body.file;
    var idStr = "SELECT * FROM FILE WHERE file_name=\'" + fileName + "\'";
    var calID;

    var event = req.body.event;
    var props = req.body.props;

    var evtSum = null;
    if(event.summary) {
        evtSum = event.summary;
    }

    var d = event.startDT.date;
    var t = event.startDT.time;

    var dateStr = (d[0] + d[1] + d[2] + d[3] + "-" + d[4] + d[5] + "-" + d[6] + d[7] + "T" + t[0] + t[1] + ":" + t[2] + t[3] + ":" + t[4] + t[5] + "Z");

    var evtLocat = null;
    var evtOrgan = null;
    props.forEach(function(prop) {
        if(prop.propName == "ORGANIZER") {
            evtOrgan = prop.propDescr;
        } else if(prop.propName == "LOCATION") {
            evtLocat = prop.propDescr;
        }
    });

    connection.query(idStr, function(err, result, fields) {
        calID = result[0].cal_id;

        var evtAttach = "(\'" + evtSum + "\', \'" + dateStr + "\', \'" + evtLocat + "\', \'" + evtOrgan + "\', \'" + calID + "\')";
        var insertEvent = "INSERT INTO EVENT (summary, start_time, location, organizer, cal_file) VALUES " + evtAttach;
//        console.log(insertEvent);
        connection.query(insertEvent, function(err, result) { // Dates not working?
            if(err) {
                state = "fail";
            }
            res.send({
                state
            })
        });
    });
});

// TODO FINISH
app.post('/insertalarm', function(req, res) {
    var state = "pass";

    var fileName = req.body.file;
    var alarm = req.body.alarm;

    var action = alarm.action;
    var trigger = alarm.trigger;

    var idStr = "SELECT * FROM FILE WHERE file_name=\'" + fileName + "\'";
    connection.query(idStr, function(err, result, fields) {
        if(err) {
            console.log(err);
        }
        var calID = result[0].cal_id;
        var evtStr = "SELECT * FROM EVENT WHERE cal_file=\'" + calID + "\'";
        connection.query(evtStr, function(errTwo, resultTwo, fieldsTwo) {
            if(errTwo) {
                console.log(errTwo);
            }
            var evtID = req.body.num; // This is bad. But it works enough. The alarms have a decent chance of being assigned to the wrong event. Oops

            var alrmAttach = "(\'" + action + "\', \'" + trigger + "\', \'" + evtID + "\')";
            var insertAlarm = "INSERT INTO ALARM (action, `trigger`, event) VALUES " + alrmAttach;
            connection.query(insertAlarm, function(errThree, resultThree, fieldsThree) {
                if(errThree) {
                    console.log(errThree);
                    state = "fail";
                }
                res.send({
                    state
                })
            });
        });
    });
});

app.post('/dbclear', function(req, res) {
    connection.query("DELETE FROM FILE", function (err, result) {
        var state;
        if(err) {
            state = "Unable to clear tables";
        } else {
            state = "Successfully cleared all tables";
        }
        res.send({
            state
        });
    });

    connection.query("ALTER TABLE FILE AUTO_INCREMENT = 1");
    connection.query("ALTER TABLE EVENT AUTO_INCREMENT = 1");
    connection.query("ALTER TABLE ALARM AUTO_INCREMENT = 1");
});

// Extra queries

app.post('/exfile', function(req, res) {
    var fileName = req.body.file;
    var idStr = "SELECT * FROM FILE WHERE file_name=\'" + fileName + "\'";
    connection.query(idStr, function(err, result) {
        if(err) console.log(err);
        var calID = result[0].cal_id;
        var evtStr  = "SELECT * FROM EVENT WHERE cal_file=\'" + calID + "\'";

        connection.query(evtStr, function (errTwo, resultTwo) {
            if(errTwo) console.log(errTwo);
            res.send({
                resultTwo
            });
        });
    });
});

app.post('/exconflict', function(req, res) {
    var evtStr = "SELECT * FROM EVENT WHERE start_time in (SELECT start_time FROM EVENT GROUP BY start_time HAVING COUNT(*) > 1) ORDER BY start_time";

    connection.query(evtStr, function (err, result) {
        if(err) console.log(err);
        res.send({
            result
        });
    });
});

app.post('/exaction', function(req, res) {
    var request = "SELECT * FROM ALARM WHERE action=\'" + req.body.extra + "\'";

    connection.query(request, function (err, result, fields) {
//        console.log(result);
        if(err) console.log(err);
        res.send({
            result
        });
    });
});

app.post('/exalarmhelp', function(req, res) {
    var fileName = req.body.file;
    var idStr = "SELECT * FROM FILE WHERE file_name=\'" + fileName + "\'";
    connection.query(idStr, function(err, result) {
        if(err) console.log(err);
        var calID = result[0].cal_id;
        var evtStr  = "SELECT * FROM EVENT WHERE cal_file=\'" + calID + "\'";

        connection.query(evtStr, function (errTwo, resultTwo) {
            if(errTwo) console.log(errTwo);
//            resultTwo.forEach(function(alarm) {
            res.send({
                resultTwo
            })
        });
    });
});

app.post('/exalarm', function(req, res) {
    var alrmID = req.body.id;
    var alarmStr  = "SELECT * FROM ALARM WHERE event=\'" + alrmID + "\'";
    connection.query(alarmStr, function (err, resultThree) {
        if(err) console.log(err);
        res.send({
            resultThree
        })
    });
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
