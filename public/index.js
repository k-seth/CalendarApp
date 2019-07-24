// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    // I want to update the tables on page load
    $('#entryBox').val("");
    updateFileList();
    databaseCheck(2);
    $('#alarmTable').hide();
    $('#propertyTable').hide();

    // Clear the status panel
    $('#statusClear').click(function(s){
        $('#statusText').html("No Older Updates");
        s.preventDefault();
    });

    // Upload a file to server
    $('#fileUpload').change(function(e) {
        e.preventDefault();
        var fileName = ($('#fileUpload').get(0).files[0].name);

        var regex = new RegExp("^.*\.(ics)");

        if(regex.test(fileName) == false) { // Using a regex, check if the file has the correct extension
            updateStatus(fileName + " is not the right extension type. ics is expected");
            return;
        }

        var formData = new FormData();
        formData.append('fileUpload', $('#fileUpload').get(0).files[0]);

        $.ajax({
            type: 'post',
            dataType: 'json',
            url: '/upload',

            data: formData,

            cache: false,
            processData: false,
            contentType: false,
            success: function(msg) {
                if(msg['out'] == 0) {
                    updateStatus('The file ' + fileName + ' was invalid and not saved');
                } else {
                    updateStatus('The file ' + fileName + ' was uploaded to the server');
                    updateFileList();
                }
            }
        });
        $('#fileUpload')[0].value = ''; // This is needed to allow duplicate uploads
    });

    /* Modal related handlers */
    $('#createCalModal').on('shown.bs.modal', function () {
        $('#myInput').trigger('focus');
    })

    $('#addEventModal').on('shown.bs.modal', function () {
        $('#myInput').trigger('focus');
    })

    $('#cancelEvt').click(function(e) {
        updateStatus("The new event was cancelled before being saved");
    });

    $('#cancelCal').click(function(e) {
        updateStatus("The new calendar was cancelled before being saved");
    });

    $('#saveCal').click(function(e) {
        var file = $('#createCalForm').find("input[id='fileBox']").val();

        if(validateCal($('#createCalForm'), file) == false) {
            return;
        }
        if(validateEvt($('#createEventForm')) == false) {
            return;
        }

        let retCal = $('#createCalForm').serializeArray()
            .reduce(function(a, x) { a[x.name] = x.value; return a; }, {})

        let retEvt = $('#createEventForm').serializeArray()
            .reduce(function(a, x) { a[x.name] = x.value; return a; }, {})

        // Hand off to C code
        $.ajax({
            type: 'post',
            dataType: 'json',
            url: '/addCal',

            data: {cal: retCal, event: retEvt, file: file},

//            cache: false,
//            processData: false,
//            contentType: false,

            success: function(msg) {
                var ret = JSON.parse(JSON.stringify(msg));
                updateStatus(msg['out']);
                updateFileList();
            }
        });

        $('#createCalModal').modal('hide');

        // Clear the extra tables
        $('#eventsBody').find('tr[name="dynEvtRow"]').each(function() {
            $('tr[name="dynEvtRow"]').remove();
        });
        $('#alarmTable').hide();
        $('#propertyTable').hide();
        $('#eventsBlank').show();
    });

    $('#eventAdd').click(function(e) {
        var file = $('#addEventForm').find("[id='inputState']").val();

        if(file == 'Choose...') {
            updateStatus("No valid file has been selected. Calendar not saved.");
            return;
        } else if(validateEvt($('#addEventForm')) == false) {
            return;
        }

        let ret = $('#addEventForm').serializeArray()
            .reduce(function(a, x) { a[x.name] = x.value; return a; }, {})

        // Hand off to server to call C code
        $.ajax({
            type: 'post',
            dataType: 'json',
            url: '/addEvt',

            data: {event: ret, file: file},

//            cache: false,
//            processData: false,
//            contentType: false,

            success: function(msg) {
                var ret = JSON.parse(JSON.stringify(msg));
                updateStatus(msg['out']);
                updateFileList();
            }
        });

        $('#addEventModal').modal('hide');

        // Clear the extra tables
        $('#eventsBody').find('tr[name="dynEvtRow"]').each(function() {
            $('tr[name="dynEvtRow"]').remove();
        });
        $('#alarmTable').hide();
        $('#propertyTable').hide();
        $('#eventsBlank').show();
    });
    /* End of modal related handlers */

    $('#saveDropSel').change(function() {
        var selectOpt = $('#saveDropSel').val();

        $('#eventsBody').find('tr[name="dynEvtRow"]').each(function() {
            $('tr[name="dynEvtRow"]').remove();
        });

        $('#alarmTable').hide();
        $('#propertyTable').hide();

        if(selectOpt == 'Choose...') {
            $('#eventsBlank').show();
            return;
        }

        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/showevt',

            data: {file: selectOpt},
            success: function(string) {
                $('#eventsBlank').hide();

                var parsed = JSON.parse(string.string);
                var i = 1;
                parsed.forEach(function(element) {
                    var single = parsed[i-1];
                    var dt = single.startDT;
                    var date = dt.date[0] + dt.date[1] + dt.date[2] + dt.date[3] + "/" + dt.date[4] + dt.date[5] + "/" + dt.date[6] + dt.date[7];
                    var time = dt.time[0] + dt.time[1] + ":" + dt.time[2] + dt.time[3] + ":" + dt.time[4] + dt.time[5];

                    if(dt.isUTC == true) {
                        time = time + " (UTC)";
                    }

                    var alrmID = i;
                    var propID = i;
                    var toAdd = "<tr class='tr' name='dynEvtRow'><td class='td'><div class='tableDiv'>" + i + "</div></td><td class='td'><div class='tableDiv'>" + single.numAlarms + "   <button name='evtAlrm' type='button' class='btn btn-sm backColour' id=" + alrmID + ">Show</button></div></td><td class='td'><div class='tableDiv'>" + single.numProps + "   <button name='evtProp' type='button' class='btn btn-sm backColour' id=" + propID + ">Show</button></div></td><td class='td'><div class='tableDiv'>" + date + "</div></td><td class='td'><div class='tableDiv'>" + time + "</div></td><td class='td'><div class='tableDiv'>" + single.summary + "</div></td></tr>";
                    $('#eventsBody').append(toAdd);
                    i += 1;
                });
            }
        });
    });


    $('#eventsBody').on('click', 'button[name=evtAlrm]', function() {
        var file = $('#saveDropSel').val();

        if(file == 'Choose...'){
            $('#alarmTable').hide();
            updateStatus("No calendar is selected. Please re-select a calendar, and try again");
            return;
        }

        $('#alarmBody').find('tr[name="dynAlrmRow"]').each(function() {
            $('tr[name="dynAlrmRow"]').remove();
        });

        var num = this.id;
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/showalarms',

            data: {num: num, file: file},
            success: function(string) {
                $('#alarmTable').show();

                if(string.out == "[]") {
                    $('#alarmsBlank').show();
                    return;
                }

                $('#alarmsBlank').hide();
                var parsed = JSON.parse(string.out);
                i = 1;

                parsed.forEach(function(element) {
                    var single = parsed[i-1];
                    var toAdd = "<tr class='tr' name='dynAlrmRow'><td class='td'><div class='tableDiv'>" + i + "</div></td><td class='td'><div class='tableDiv'>" + single.numProps + "</div></td><td class='td'><div class='tableDiv'>" + single.action + "</div></td><td class='td'><div class='tableDiv'>" + single.trigger + "</div></td></tr>";
                    $('#alarmBody').append(toAdd);
                    i += 1;
                });
            }
        });
    });

    $('#eventsBody').on('click', 'button[name=evtProp]', function() {
        var file = $('#saveDropSel').val();

        if(file == 'Choose...'){
            $('#propertyTable').hide();
            updateStatus("No calendar is selected. Please re-select a calendar, and try again");
            return;
        }

        $('#propertyBody').find('tr[name="dynPropRow"]').each(function() {
            $('tr[name="dynPropRow"]').remove();
        });

        var num = this.id;
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/showprops',

            data: {num: num, file: file},
            success: function(string) {
                $('#propertyTable').show();

                if(string.out == "[]") {
                    $('#propertiesBlank').show();
                    return;
                }

                $('#propertiesBlank').hide();
                var parsed = JSON.parse(string.out);
                i = 1;

                parsed.forEach(function(element) {
                    var single = parsed[i-1];
                    var toAdd = "<tr class='tr' name='dynPropRow'><td class='td'><div class='tableDiv'>" + i + "</di9v></td><td class='td'><div class='tableDiv'>" + single.propName + "</div></td><td class='td'><div class='tableDiv'>" + single.propDescr + "</div></td></tr>";
                    $('#propertyBody').append(toAdd);
                    i += 1;
                });
            }
        });
    });

    $('#connect').click(function() {
        databaseCheck(0);
    });

    $('#dbShow').click(function() {
        $.ajax({ // Make a call to get content of the file table
            type: 'post',
            dataType: 'json',
            url: '/dbfiles',

            success: function(files) {
              //updateStatus(
              var numFile = JSON.parse(JSON.stringify(files)).result.length
              if(numFile == 0) {
                  updateStatus("Database has no files");
              } else {
                  $.ajax({ // Make a call to get contents of the event table
                      type: 'post',
                      dataType: 'json',
                      url: '/dbevents',
                      data: {usage: "show"},

                      success: function(events) {
                          var numEvt = JSON.parse(JSON.stringify(events)).result.length
                          $.ajax({ // Make a call to get contents of the alarm table
                              type: 'post',
                              datatype: 'json',
                              url: '/dbalarms',

                              success: function(alarms) {
                                  var numAlrm = JSON.parse(JSON.stringify(alarms)).result.length
                                  updateStatus("Database has " + numFile + " files, " + numEvt + " events, and " + numAlrm + " alarms");
                              },
                          });
                      },
                  });
              }
            },
        })
    });

    $('#dbStore').click(function() {
        $.ajax({ // Get directory list
            type: 'get',
            dataType: 'json',
            url: '/dirlist',

            success: function(list) {
                var alarmNumTwo = 0;
                list.forEach(function (file) {
                    var eventNum = 0;
                    var alarmNum = 0;
                    $.ajax({ // Insert each file
                        type: 'post',
                        dataType: 'json',
                        url: '/insertfile',

                        data: {file: file},

                        success: function(msg) {
                           if(msg.status == "fail") {
                               updateStatus("The file " + file + " failed to upload to the database");
                               return;
                            }
                            updateStatus("The file " + file + " was uploaded to the database");
                            $.ajax({ // get event json
                                type: 'get',
                                dataType: 'json',
                                url: '/showevt',

                                data: {file: file},

                                success : function(evtList) {
                                    JSON.parse(evtList.string).forEach(function(event) {
                                        eventNum ++;
                                        $.ajax({ // get event props
                                            type: 'get',
                                            dataType: 'json',
                                            url: '/showprops',

                                            data: {file: file, num: eventNum},

                                            success: function(evtProps) {
                                                $.ajax({ // insert event
                                                    type: 'post',
                                                    dataType: 'json',
                                                    url: '/insertevent',

                                                    data: {file: file, event: event, props: JSON.parse(evtProps.out)},

                                                    success: function(msgTwo) {
                                                        if(msgTwo.status == "fail") {
                                                            updateStatus("The file " + file + " failed to upload to the database");
                                                            return;
                                                        }
                                                        alarmNum ++;
                                                        $.ajax({ // Get alarm
                                                            type: 'get',
                                                            dataType: 'json',
                                                            url: '/showalarms',

                                                            data: {file: file, num: alarmNum},

                                                            success: function(alrmList) {
                                                                alarmNumTwo ++;
                                                                //console.log(alrmList.out);
                                                                JSON.parse(alrmList.out).forEach(function(alarm) {
                                                                    $.ajax({
                                                                        type: 'post',
                                                                        dataType: 'json',
                                                                        url: '/insertalarm',

                                                                        data: {file: file, alarm: alarm, num: alarmNumTwo},

                                                                        success: function(msgThree) {
                                                                            if(msgThree.status == "fail") {
                                                                                updateStatus("The file " + file + " failed to upload to the database");
                                                                                return;
                                                                            }
                                                                        },
                                                                    })
                                                                });
                                                            }
                                                        })
                                                    }
                                                })
                                            }
                                        })
                                    });
                                }
                            })
                        }
                    })
                });
            },
        })
    });

    $('#dbClear').click(function() {
//        $.ajax({
                $.ajax({
                    type: 'post',
                    datatype: 'json',
                    url: '/dbclear',

                    success: function(msg) {
                        updateStatus(msg.state);
                    },
                })
//            },
//        })
    });

    $('#extraQueries').change(function() {
        $('#dbEvtDropSel').hide();
        $('#dbEvtInput').hide();
        $('#dbSearch').hide();

        var selectOpt = $('#extraQueries').val();
        if(selectOpt == "Display All Events (By: Date)") {
//            $('#extraQueries').val("Choose...");
            $.ajax({
                type: 'post',
                dataType: 'json',
                url: '/dbevents',

                data: {usage: "dates"},

                success: function(msg) {
//                    console.log(msg.result);
                    updateStatus("Events ordered oldest to newest *Note*: Times are 4 hours off (SQL converted them to UTC on insertion, and I couldn't figure out how to stop it):");
                    (msg.result).forEach(function(evt) {
//                        console.log(evt.start_time);
                        var splitStr = (evt.start_time).split("T")
                        updateStatus(splitStr[0] + " @ " + splitStr[1] + ": " + evt.summary);
                    });
                },
            })
        } else if(selectOpt == "Display All Events (By Select: File)") {
            $('#dbEvtDropSel').show();
        } else if(selectOpt == "Display All Events (By: Conflict)") {
            $.ajax({
                type: 'post',
                dataType: 'json',
                url: '/exconflict',

                success: function(msg) {
                    updateStatus("All Events that conflict (more than 1 entry for same date):");
                    (msg.result).forEach(function(evt) {
                        var splitStr = (evt.start_time).split("T")
                        updateStatus(splitStr[0] + " @ " + splitStr[1] + ": " + evt.summary);
                    });
                }
            })
        } else if(selectOpt == "Display All Events (By Input: Location)") {
            $('#dbEvtDropSel').hide();
            $('#dbEvtInput').show();
            $('#dbSearch').show();
        } else if(selectOpt == "Display All Alarms (By Input: Action)") {
            $('#dbEvtDropSel').hide();
            $('#dbEvtInput').show();
            $('#dbSearch').show();
        } else if(selectOpt == "Display All Alarms (By Select: File)") {
            $('#dbEvtDropSel').show();
        }
    });

    $('#dbSearch').click(function() {
        var table = $('#extraQueries').val();
        var field = $('#dbEvtInput').val();
        if(table == "Display All Events (By Input: Location)") {
            $.ajax({
                type: 'post',
                dataType: 'json',
                url: '/dbevents',

                data: {usage: "location", extra: field},

                success: function(msg) {
                    updateStatus("All Events at the location: " + field);
                    if(msg.result) {
                        (msg.result).forEach(function(event) {
                            var splitStr = (event.start_time).split("T")
                            updateStatus(splitStr[0] + " @ " + splitStr[1] + ": " + event.summary);
                        });
                    } else {
                        updateStatus("No Events Found");
                    }
                }
            })
        } else if(table == "Display All Alarms (By Input: Action)") {
            $.ajax({
                type: 'post',
                dataType: 'json',
                url: '/exaction',

                data: {extra: field},

                success: function(msg) {
                    updateStatus("All Alarms with the action: " + field);
                    if(msg.result) {
                        (msg.result).forEach(function(alarm) {
                            updateStatus(alarm.trigger);
                        });
                    } else {
                        updateStatus("No Events Found");
                    }
                }
            })
        }
    });

    $('#dbEvtDropSel').change(function() {
        var file = $('#dbEvtDropSel').val();
        if($('#extraQueries').val() == "Display All Alarms (By Select: File)"){

            if(file != "Choose...") {
                $('#dbEvtDropSel').val("Choose...");
                $.ajax({
                    type: 'post',
                    dataType: 'json',
                    url: '/exalarmhelp',

                    data: {file: file, usage: "files"},
                    success: function(msg) {
                        updateStatus("Alarms from " + file + ":");
                        (msg.resultTwo).forEach(function(alarm) {
                            $.ajax({
                                type: 'post',
                                dataType: 'json',
                                url: '/exalarm',

                                data: {id: alarm.event_id},

                                success: function(out) {
                                    if(out.resultThree) {
                                        (out.resultThree).forEach(function(element) {
                                              updateStatus(element.action + ": " + element.trigger);
                                          });
                                    }
                                }
                            })
                        });
                    }
                })
            }
        } else if ($('#extraQueries').val() == "Display All Events (By Select: File)") {
            if(file != "Choose...") {
                $('#dbEvtDropSel').val("Choose...");
                $.ajax({
                    type: 'post',
                    dataType: 'json',
                    url: '/exfile',

                    data: {file: file, usage: "files"},
                    success: function(msg) {
                        updateStatus("Events from " + file + ":");
                        (msg.resultTwo).forEach(function(evt) {
                            var splitStr = (evt.start_time).split("T")
                            updateStatus(splitStr[0] + " @ " + splitStr[1] + ": " + evt.summary);
                        });
                    },
                })
            }
        }
    });
});

// Try a database connection
function databaseCheck(initial) {
//    if(initial == 2) {
//       return;
//    }

   let ret = $('#loginForm').serializeArray()
       .reduce(function(a, x) { a[x.name] = x.value; return a; }, {})

    $.ajax({
        type: 'post',
        dataType: 'json',
        url: '/connectserver',

        data: {database: ret, int: initial},

        success: function(msg) {
            if(initial != 1 && initial != 2) {
                updateStatus(msg.state);
            }
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: '/dirlist',

                success: function(data) {
                    // Make sure the blank table line is present if no files are present
                    if ((msg.state == "Successfully connected to the server")) {
//                        $('#connect').hide();
                        $('#tableManagement').show();
                        if(data.length > 0) {
                            $('#dbFiles').show();
                        }
                        return;
                    }
                }
            });
        }
    });
}

// Update the status panel
function updateStatus(message) {
  $('#statusText').append("</br>" + message);
}

// Get the list of files and update the tables/menus
function updateFileList(){
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/dirlist',

        success: function(data) {
            // Make sure the blank table line is present if no files are present
            if(data.length == 0) {
                $('#filesBlank').show();
                return;
            }

            // Delete all previously created dynamic rows to allow the creation of new ones
            $('#filesBody').find('tr[name="dynRow"]').each(function() {
                $('tr[name="dynRow"]').remove();
            });

            // Delete dynamically added drop menus
            $('#saveDropSel').find('[name="dynDrop"]').each(function() {
                $('[name="dynDrop"]').remove();
            });

            $('#dbEvtDropSel').find('[name="dynDrop"]').each(function() {
                $('[name="dynDrop"]').remove();
            });

            $('#inputState').find('[name="dynOpt"]').each(function() {
                $('[name="dynOpt"]').remove();
            });

            // Otherwise, make sure the row is hidden and add new rows to the table for each file
            $('#filesBlank').hide();
            data.forEach(function(element) {
                var parsed;

                // Nested Ajax call allows for each file to populate its table row
                $.ajax({
                    type: 'get',
                    dataType: 'json',
                    url: '/calinfo',

                    data: {file: element},
                    success: function(string) {
                        var parsed = JSON.parse(string.string);
//                        console.log(JSON.parse(string.string));
                        var toAdd = "<tr class='tr' name='dynRow'><td class='td'><div class='tableDiv'><a href=./uploads/" + element + ">" + element + "</a></div></td><td class='td'><div class='tableDiv'>" + parsed.numEvents + "</div></td><td class='td'><div class='tableDiv'>" + parsed.numProps + "</div></td><td class='td'><div class='tableDiv'>" + parsed.version + "</div></td><td class='td'><div class='tableDiv'>" + parsed.prodID + "</div></td></tr>";
                        $('#filesBody').append(toAdd);


                        // Modify the drop down menus
                        var dropAdd = "<option id='dynDrop' name='dynDrop'>" + element + "</option>";
                        $('#saveDropSel').append(dropAdd);
                        $('#dbEvtDropSel').append(dropAdd);

                        var optAdd = "<option name='dynOpt'>" + element + "</option>";
                        $('#inputState').append(optAdd);
                    }
                });
            });
//            databaseCheck(1);
        },
        fail: function(error) {
            updateStatus('Error trying to read all files present on the server');
            console.log(error);
        }
    });
}

// Validate the Calendar form by checking various properties
function validateCal(form, file) {
//    var file = $(form).find("input[name='FILENAME']").val();
    var regex = new RegExp("^.*\.(ics)");

    if(regex.test(file) == false) { // Using a regex, check if the file has the correct extension
        updateStatus(file + " is not the right extension type. ics is expected. Calendar not saved");
        return false;
    } else if(file.length < 5) {
        updateStatus(file + " is too short. Calendar not saved");
        return false;
    }

    $.ajax({
        type: 'get',
        url: '/dirlist',

        success: function(data) {
            data.forEach(function(element) {
                if(element == file) {
                    updateStatus("A file by the same name as " + file + " already exists and will be over written");
                }
            });
        },
        fail: function(error) {
            updateStatus("An unexpected file I/O error occured. Calendar not saved");
            return false;
        }
    });

    var ver = $(form).find("input[name='VERSION']").val();
    if($.isNumeric(ver) == false) {
        updateStatus("Version must be a numeric value. Calendar not saved");
        return false;
    }
    var prod = $(form).find("input[name='PRODID']").val();
    if(prod.length < 1) {
        updateStatus("Product ID must not be empty. Calendar not saved");
        return false;
    } else if(prod.length > 999) {
        updateStatus("Product ID is too long. Max length is 999. Currently at length " + prod.length + ". Calendar not saved");
        return false;
    }
    return true;
}

// Validate the event form by testing the various properties
function validateEvt(form) {
    var uid = $(form).find("input[name='UID']").val();
    if(uid.length < 1) {
        updateStatus("UID must not be empty. Calendar not saved");
        return false;
    } else if(uid.length > 999) {
        updateStatus("UID is too long. Max length is 999. Currently at length " + uid.length + ". Calendar not saved");
        return false;
    }

    var regexU = new RegExp("^([0-2]{1})([0-9]{3})([0-9]{2})([0-9]{2})T([0-9]{6})Z"); // Match UTC
    var regex = new RegExp("^([0-2]{1})([0-9]{3})([0-9]{2})([0-9]{2})T([0-9]{6})"); // Match not UTC

    var stamp = $(form).find("input[name='DTSTAMP']").val();
    var start = $(form).find("input[name='DTSTART']").val();
    if(stamp.length != start.length) {
        updateStatus("DTSTAMP and DTSTART must be the same length. Calendar not saved");
        return false;
    }

    // Test stamp and start vs the regex. It isn't super precise, but it will do the job
    if(regex.test(stamp) == false && regexU.test(stamp) == false) {
        updateStatus("The DTSTAMP does not meet format requirements. Expected YYYYMMDDTHHMMSS(Z) - Z is optional. Calendar not saved");
        return false;
    }

    if(regex.test(start) == false && regexU.test(start) == false) {
        updateStatus("The DTSTART does not meet format requirements. Expected YYYYMMDDTHHMMSS(Z) - Z is optional. Calendar not saved");
        return false;
    }

//    var summ = $(form).find("input[name='SUMMARY']").val(); // No Error checking. Left for my sake
}
