/**
 *
 * Author: Seth Kuipers
 * Class: CIS*2750
 * Due Date: February 4, 2018
 * Project: Assignment One
 *
 */

#include "CalendarParser.h"
#include "HelperFunctions.h"


// Below are all required parser functions

ICalErrorCode createCalendar(char *fileName, Calendar **obj) {
  FILE *fPtr;
  char *calendarText = NULL, *startPtr, *endPtr, *tempPtr;
  char versionString[] = "VERSION", prodString[] = "PRODID", startString[] = "BEGIN:VCALENDAR", endString[] = "END:VCALENDAR";
  char letter;
  char *upperStringOne = NULL, *upperStringTwo = NULL;
  int i = 0;
  bool tagChecker = false;
  bool inEvent = false, inAlarm = false;
  bool hasEvent = false;

  Alarm *alarm = NULL;
  ICalErrorCode returnedError = OK;
  Event *tempEvent = NULL;
  Property *property = NULL;

  if(fileName == NULL) {
    return INV_FILE;
  } else if(strstr(fileName, ".ics") == NULL) {
    return INV_FILE;
  }

  fPtr = fopen(fileName, "r");

  if(fPtr == NULL) {
    return INV_FILE;
  }

  while((letter = fgetc(fPtr)) != EOF) {
    if((letter == ' ' || letter == '\t') && calendarText[i-1] == '\n' && calendarText[i-2] == '\r') {
      i-= 2; // This will back track i to the spot that is held by \r so it can be overridden, and igornes the space
    } else {
      calendarText = realloc(calendarText, (sizeof(char)*i) + sizeof(char));
      calendarText[i] = letter;
      i++;
    }
  }

  fclose(fPtr);

  calendarText[i - 1] = '\0';

  upperStringOne = toUpper(calendarText);
  tagChecker = checkTags(startString, endString, upperStringOne);
  free(upperStringOne);

  // Ensure that the structure of the calendar is at least partially correct
  if(!tagChecker) {
    free(calendarText);
//    deleteCalendar(*obj);
    return INV_CAL;
  }

  // Initialize the entire Calendar
  *obj = malloc(sizeof(Calendar));
  (*obj)->version = -100;
  (*obj)->prodID[0] = '\0';
  (*obj)->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
  (*obj)->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);

  startPtr = strstr(calendarText, startString);
  startPtr += strlen(startString) + 2; // This ensures that the ptr is at the start
  endPtr = strstr(calendarText, endString); // Find the end of the file, and test to see if there are any events that we need to stop at

  tempPtr = strchr(startPtr, '\r'); // going from the first line after the begin, find the nearest end of line

  if(tempPtr == '\0' || *(tempPtr + 1) != '\n') {
    free(calendarText);
    return INV_FILE; // This means no newlines were found, means this file has garbage formatting
  }

  while(tempPtr < endPtr && tempPtr != NULL) {
    property = parseProperty(startPtr, tempPtr);

    // Check 1 in always false. (No a huge issue, but fix for future?
    if(property->propName == '\0' && strcmp(property->propDescr, ";") == 0) {
      free(property);
      continue;
    }

    upperStringOne = toUpper(property->propName);
    upperStringTwo = toUpper(property->propDescr);

    if((strcmp(upperStringOne, "END") == 0 && strcmp(upperStringTwo, "VEVENT") == 0)) { // Check if it is the end of an event
      if(upperStringOne != NULL) {
        free(upperStringOne);
      }
      if(upperStringTwo != NULL) {
        free(upperStringTwo);
      }

      free(property);
      if(!inEvent) {
        free(calendarText);
        return INV_EVENT; // Should not try to end an event we are not in
      } else if(inAlarm) { // If we are are in an alarm when exiting an event, that is bad
        free(calendarText);
        deleteAlarm(alarm);
        deleteEvent(tempEvent);
        return INV_ALARM;
      } else if(strlen(tempEvent->UID) == 0 || strlen(tempEvent->creationDateTime.date) == 0 || strlen(tempEvent->creationDateTime.time) == 0 || strlen(tempEvent->startDateTime.date) == 0 || strlen(tempEvent->startDateTime.time) == 0) {
        free(calendarText);
        deleteEvent(tempEvent);
        return INV_EVENT;
      }
      insertBack((*obj)->events, tempEvent);
      inEvent = false;
    } else if(inEvent) { // As this does not require reading the property name here, just make a check rather than doing the whole if branch
      if(returnedError != OK) { // Something failed with event reading!
        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        return returnedError;
      }

      // Check for invalid alarm is not outside being in an alarm, as it wasn't tripped
      if(strcmp(upperStringOne, "END") == 0 && strcmp(upperStringTwo , "VALARM") == 0) { // Exit alarm
        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        free(property);

        if(!inAlarm) {
          free(calendarText);
          deleteEvent(tempEvent);
          return INV_ALARM;
        } else if(strcmp(alarm->action, "\0") == 0 || alarm->trigger == NULL) {
          free(calendarText);
          deleteAlarm(alarm);
          deleteEvent(tempEvent);
          return INV_ALARM;
        }

        insertBack(tempEvent->alarms, alarm);
        inAlarm = false;
      } else if(inAlarm) {
        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        if(strlen(property->propName) == 0 || strlen(property->propDescr) == 0) { // Comment or malformed line
          //if(strcmp(property->propDescr, ";") != 0) { // This will catch the case of any stray comments
          //If an invalid property is returned, exit
          free(calendarText);
          free(property);
          deleteAlarm(alarm);
          deleteEvent(tempEvent);
          return INV_ALARM;
          //}
        //free(property);
        } else {
          returnedError = parseAlarm(property, &alarm);
        }
      } else {
        if(strlen(property->propName) == 0 || strlen(property->propDescr) == 0) { // Comment or malformed line
          if(upperStringOne != NULL) {
            free(upperStringOne);
          }
          if(upperStringTwo != NULL) {
            free(upperStringTwo);
          }

          if(strcmp(property->propDescr, ";") != 0) {
            free(property);
            return INV_EVENT;
          }

          free(property);
        } else if(strcmp(upperStringOne, "BEGIN") == 0 && strcmp(upperStringTwo, "VALARM") == 0) { // Enter alarm
          if(upperStringOne != NULL) {
            free(upperStringOne);
          }
          if(upperStringTwo != NULL) {
            free(upperStringTwo);
          }

          free(property);

          if(inAlarm) { // If we are in an alarm when entering an alarm, that is bad
            free(calendarText);
            deleteEvent(tempEvent);
            return INV_ALARM;
          }

          alarm = malloc(sizeof(Alarm));
          strcpy(alarm->action, "\0");
          alarm->trigger = NULL;
          alarm->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);

          inAlarm = true;
        } else {
          if(upperStringOne != NULL) {
            free(upperStringOne);
          }
          if(upperStringTwo != NULL) {
            free(upperStringTwo);
          }

          returnedError = parseEvent(property, &tempEvent);
        }
      }
    } else {
      if(strcmp(upperStringOne, versionString) == 0) {
        if(strcmp(property->propDescr, "\0") == 0) {
          free(property);
          free(calendarText);
          return INV_VER;
        }

        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        if((*obj)->version != -100) { // This means that VERSION has already been found, and duplicates are not allowed
          free(property);
          free(calendarText);
          return DUP_VER;
        }

        (*obj)->version = atof(property->propDescr);
        free(property);
      } else if(strcmp(upperStringOne, prodString) == 0) {
        if(strcmp(property->propDescr, "\0") == 0) {
          free(property);
          free(calendarText);
          return INV_PRODID;
        }

        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        if(strlen((*obj)->prodID) != 0) { // This means that PRODID  has already been found, and duplicates are not allowed
          free(property);
          free(calendarText);
          return DUP_PRODID;
        }

        strcpy((*obj)->prodID, property->propDescr);
        free(property);
      } else if((strcmp(upperStringOne, "BEGIN") == 0 && strcmp(upperStringTwo, "VEVENT") == 0)) { // Check if it is the beginning of an event
        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        free(property);

        if(inEvent) {
          free(calendarText);
          deleteEvent(tempEvent);
          return INV_EVENT; // The calendar should never read 2 events at the same time
        }

        tempEvent = malloc(sizeof(Event));
        tempEvent->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
        tempEvent->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
        tempEvent->UID[0] = '\0';
        tempEvent->creationDateTime.date[0] = '\0';
        tempEvent->creationDateTime.time[0] = '\0';
        tempEvent->creationDateTime.UTC = false;
        tempEvent->startDateTime.date[0] = '\0';
        tempEvent->startDateTime.time[0] = '\0';
        tempEvent->startDateTime.UTC = false;

        hasEvent = true;
        inEvent = true;
      } else {
        if(upperStringOne != NULL) {
          free(upperStringOne);
        }
        if(upperStringTwo != NULL) {
          free(upperStringTwo);
        }

        insertBack((*obj)->properties, property);
      }
    }

    startPtr = tempPtr + 2;
    tempPtr += 2; // Progress the ptr past the \r\n and then find the next one
    if(tempPtr < endPtr) {
      tempPtr = strstr(startPtr, "\r\n");
    }
  }

  free(calendarText);

  if((*obj)->version == -100) {
    return INV_CAL;
  } else if(strlen((*obj)->prodID) == 0) {
    return INV_CAL;
  } else if(inAlarm) {
    deleteEvent(tempEvent);
    return INV_ALARM;
  } else if(inEvent) {
    deleteEvent(tempEvent);
    return INV_EVENT;
  } else if (tempPtr == NULL || tempPtr > endPtr){
    deleteCalendar(*obj);
    *obj = NULL;
    return INV_CAL; // If END:VCALENDAR is present but commented, tempPtr may exceed the endPtr
  } else if(!hasEvent) {
    deleteCalendar(*obj);
    *obj = NULL;
    return INV_CAL;
  }

  return OK;
}

char *printCalendar(const Calendar *obj) {
  char *printSentence = malloc(sizeof(char)*15000);

  if(obj == NULL) {
    sprintf(printSentence, "No calendar exists. Try again.");
  } else {
    char *tempProps = toString(obj->properties);
    char *tempEvents = toString(obj->events);

    sprintf(printSentence, "BEGIN:VCALENDAR\r\nVERSION:%lf\r\nPRODID:%s\r\n%s%sEND:VCALENDAR\r\n", obj->version, obj->prodID, tempProps, tempEvents);

    free(tempProps);
    free(tempEvents);
  }

  return printSentence;
}

void deleteCalendar(Calendar *obj) {
  if(obj == NULL) {
    return;
  }
  if(obj->properties != NULL) {
    freeList(obj->properties);
  }
  if(obj->events != NULL) {
    freeList(obj->events);
  }

  free(obj);

  return;
}

char *printError(ICalErrorCode err) {
  char *returnString = malloc(150);
  switch(err) {
    case INV_FILE:
      sprintf(returnString, "The supplied file is invalid");
      return returnString;
    case INV_CAL:
      sprintf(returnString, "The body of the Calendar does not follow correct formatting");
      return returnString;
    case INV_VER:
      sprintf(returnString, "The calendar version is improperly formatted");
      return returnString;
    case DUP_VER:
      sprintf(returnString, "Version appears more than once, but is not allowed to");
      return returnString;
    case INV_PRODID:
      sprintf(returnString, "The product ID is improperly formatted");
      return returnString;
    case DUP_PRODID:
      sprintf(returnString, "The product ID appears more than once, but is not allowed to");
      return returnString;
    case INV_EVENT:
      sprintf(returnString, "One of the internal events is improperly formatted");
      return returnString;
    case INV_DT:
      sprintf(returnString, "One of the interal dates/times is improperly formatted");
      return returnString;
    case INV_ALARM:
      sprintf(returnString, "One of the internal alarms is improperly formatted");
      return returnString;
    case WRITE_ERROR:
      sprintf(returnString, "There was an issue writing the calendar to file");
      return returnString;
    case OK:
      sprintf(returnString, "Calendar is properly formatted");
      return returnString;
    default:
      sprintf(returnString, "An error of unknown specifications occured (OTHER_ERROR).");
      return returnString;
  }
}

// Below are all required helper functions

void deleteEvent(void *toBeDeleted) {
  Event *temp = (Event *)toBeDeleted;
  if(temp == NULL) {
    return;
  }

  if(temp->properties != NULL) {
    freeList(temp->properties);
  }
  if(temp->alarms != NULL) {
    freeList(temp->alarms);
  }

  free(temp);
  temp = NULL;

  return;
}

int compareEvents(const void *first, const void *second) { // Consider date time?
  if(strcmp(((Event *)first)->UID, ((Event *)second)->UID) != 0) {
    return -1;
  } else if(compareProperties(((Event *)first)->properties, ((Event *)second)->properties) != 0) {
    return -1;
  } else if(compareAlarms(((Event *)first)->alarms, ((Event *)second)->alarms) != 0) {
    return -1;
  }
  return 0;
}

char *printEvent(void *toBePrinted) {
  if(toBePrinted == NULL) {
    return "";
  }

  Event *tempEvent = (Event *)toBePrinted;
  char *tempString = malloc(sizeof(char)*15000);
  char *tempAlarms = toString(tempEvent->alarms);
  char *tempProps = toString(tempEvent->properties);

  if(tempEvent->creationDateTime.UTC == true) {
    sprintf(tempString, "BEGIN:VEVENT\r\nUID:%s\r\nDTSTAMP:%sT%sZ\r\nDTSTART:%sT%sZ\r\n%s%sEND:VEVENT\r\n", tempEvent->UID, tempEvent->creationDateTime.date, tempEvent->creationDateTime.time, tempEvent->startDateTime.date, tempEvent->startDateTime.time, tempProps, tempAlarms);
  } else {
    sprintf(tempString, "BEGIN:VEVENT\r\nUID:%s\r\nDTSTAMP:%sT%s\r\nDTSTART:%sT%s\r\n%s%sEND:VEVENT\r\n", tempEvent->UID, tempEvent->creationDateTime.date, tempEvent->creationDateTime.time, tempEvent->startDateTime.date, tempEvent->startDateTime.time, tempProps, tempAlarms);
  }

  free(tempProps);
  free(tempAlarms);

  return tempString;
}


void deleteAlarm(void *toBeDeleted) {
  Alarm *temp = (Alarm *)toBeDeleted;

  if(temp == NULL) {
    return;
  }

  if(temp->properties != NULL) {
    freeList(temp->properties);
  }
  if(temp->trigger != NULL) {
    free(temp->trigger);
  }

  free(temp);
  temp = NULL;

  return;
}

int compareAlarms(const void *first, const void *second) {
  if(strcmp(((Alarm *)first)->action, ((Alarm *)second)->action) != 0) {
    return -1;
  } else if(strcmp(((Alarm *)first)->trigger, ((Alarm *)second)->trigger) != 0) {
    return -1;
  } else if(compareProperties(((Alarm *)first)->properties, ((Alarm *)second)->properties) != 0) {
    return -1;
  }

  return 0;
}

char *printAlarm(void *toBePrinted) {
  if(toBePrinted == NULL) {
    return "";
  }

  Alarm *tempAlarm = (Alarm *)toBePrinted;
  char *tempString = malloc(sizeof(char)*6000);
  char *tempProps = toString(tempAlarm->properties);

  sprintf(tempString, "BEGIN:VALARM\r\nACTION:%s\r\nTRIGGER:%s\r\n%sEND:VALARM\r\n", tempAlarm->action, tempAlarm->trigger, tempProps);
  free(tempProps);

  return tempString;
}


void deleteProperty(void *toBeDeleted) {
  if(toBeDeleted == NULL) {
    return;
  }
  free((Property *)toBeDeleted);
  return;
}

int compareProperties(const void *first, const void *second) { //Iterator?
  if(strcmp(((Property *)first)->propName, ((Property *)second)->propName) != 0) {
    return -1;
  } else if(strcmp(((Property *)first)->propDescr, ((Property *)second)->propDescr) != 0) {
    return -1;
  }

  return 0;
}

char *printProperty(void *toBePrinted) {
  if(toBePrinted == NULL) {
    return "";
  }

  Property *tempProp = (Property *)toBePrinted;
  char *tempString = malloc(sizeof(char)*3000);

  sprintf(tempString, "%s:%s\r\n", tempProp->propName, tempProp->propDescr);

  return tempString;
}


void deleteDate(void* toBeDeleted) {
  if(toBeDeleted != NULL) {
    free(toBeDeleted);
  }
  return;
}

int compareDates(const void *first, const void *second) {
  return 0;
}

char *printDate(void *toBePrinted) {
  if(toBePrinted == NULL) {
    return "";
  }

  DateTime *tempDate = (DateTime *)toBePrinted;
  char *tempString = malloc(sizeof(char)*50);

  if(tempDate->UTC == true) {
    sprintf(tempString, "%sT%sZ\r\n", tempDate->date, tempDate->time);
  } else {
    sprintf(tempString, "%sT%s\r\n", tempDate->date, tempDate->time);
  }
  return tempString;
}

ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {
  if(obj == NULL) { // NULL calendar is bad
    return WRITE_ERROR;
  }

  if(fileName == NULL || strlen(fileName) < 5 || strstr(fileName, ".ics") == NULL) { // Check for a null name, and then check if it is long enough (1 char + .ics) and finally, it has a proper exension
    return WRITE_ERROR;
  }

  FILE *fPtr = fopen(fileName, "w");

  if(fPtr == NULL) { // This *should* never be able to happen
    return WRITE_ERROR;
  }

  fprintf(fPtr, "BEGIN:VCALENDAR\r\n");
  fprintf(fPtr, "VERSION:%.0f\r\n", obj->version);
  fprintf(fPtr, "PRODID:%s\r\n", obj->prodID);

  writePropertyList(fPtr, obj->properties);
  writeEventList(fPtr, obj->events);

  fprintf(fPtr, "END:VCALENDAR\r\n");

  fclose(fPtr);
  return OK;
}

ICalErrorCode validateCalendar(const Calendar* obj) { // TO DO: use to upper for checks
  // Start at the top level for error checks and work down
  // Starting with calendar struct
  if(obj == NULL) {
    return INV_CAL;
  }

  if(obj->version < 0) { // My default is -100, and no version should ever be this low
    return INV_CAL;
  } else if(strlen(obj->prodID) == 0 || strlen(obj->prodID) > 999) { // 0 length or at or exceeding max length
    return INV_CAL;
  } else if(obj->events == NULL || getLength(obj->events) < 1) { // check to ensure events is not NULL and has atleast one item.
    return INV_CAL;
  } else if(obj->properties == NULL || getLength(obj->properties) > 2) { // Greater than 2 means there are duplicates
    return INV_CAL;
  } // Use of an if-else branch is to keep conditions short

  // Next we will want to make sure to iterate through calendar properties looking for malformity of 
  // Properties that can be in the calendar spec:
  /*
   * CALSCALE. Not required, but can only occur once
   * METHOD. Not required, but can only occur once
   * PRODID. Required, may only occur once. -- Just need to handle multipe occurences
   * VERSION. Required, may only occur once. -- Just need to handle multiple occurences
   */
  ListIterator calProps = createIterator(obj->properties);
  Property *tempProp = NULL;
  bool foundMethod = false, foundScale = false;

  while((tempProp = (Property *)nextElement(&calProps)) != NULL) {
    if(strcmp(tempProp->propName, "CALSCALE") == 0) {
      if(foundScale || strcmp(tempProp->propDescr, "\0") == 0) { // If it has already been found, or the descr doesn't exist...
        return INV_CAL;
      }
      foundScale = true;
    } else if(strcmp(tempProp->propName, "METHOD") == 0) {
      if(foundMethod || strcmp(tempProp->propDescr, "\0") == 0) { // If it has already been found, or the descr doesn't exist...
        return INV_CAL;
      }
      foundMethod = true;
    } else {
      return INV_CAL; // Nothing, including version or prodid is valid here.
    }
    tempProp = NULL;
  }

  // Next we will want to look at and confirm each component is clean.
  // This will valid all aspects of an event before moving to the next one.
  // Only valid component is VEVENT. Events can have 0 or more VALARM components within. Plus properties.
  // Properties allowed in an event:
  /*
   * ATTACH. Unlimited use.
   * CATEGORIES. Unlimited use.
   * CLASS. May only occur once.
   * COMMENT. Unlimited use.
   * DESCRIPTION. May only occur once.
   * GEO. May only occur once.
   * LOCATION. May only occur once.
   * PRIORITY. May only occur once.
   * RESOURCES. Unlimited use.
   * STATUS. May only occur once.
   * SUMMARY. May only occur once.
   * DTEND. May only occur once, but may not occur with DURATION. Must be UTC. Must be later than DTSTART.
   * DTSTART. Required, may only occur once. Must be UTC. -- Just need to handle multiple occurences
   * Duration. May only occur once, but mat not occur with DTEND.
   * TRANSP. May only occur once.
   * ATTENDEE. Unlimited use.
   * CONTACT. Unlimited use.
   * ORGANIZER. May only occur once.
   * RECURRENCE-ID. Must occur in a recurring component. May only appear once.
   * RELATED-TO. Unlimited use.
   * URL. May only occur once.
   * UID. Required, may only occur once. -- Just need to handle multiple occurences
   * EXDATE. Can occur in recurring events. Unlimited use.
   * RDATE. Can occur in recurring events. Unlimited use.
   * RRULE. Can occur in recurring events. Should only occur once.
   * CREATED. May only occur once. Must be UTC.
   * DTSTAMP. Required, may only occur once. Must be UTC. -- Just need to handle multiple occurences
   * LAST-MODIFIED. May only occur once. Must be UTC.
   * SEQUENCE. May only occur once. Must be UTC.
   */
  ListIterator calEvents = createIterator(obj->events);
  Event *tempEvent = NULL;

  while((tempEvent = (Event *)nextElement(&calEvents)) != NULL) {
    // In each event we will look at the properties of the event and then the alarms (if any) in said event
    if(strlen(tempEvent->UID) == 0 || strlen(tempEvent->UID) > 999) {
      return INV_EVENT;
    } else if(strlen(tempEvent->creationDateTime.date) != 8) { // Leave room for the null!
      return INV_EVENT;
    } else if(strlen(tempEvent->creationDateTime.time) < 6) {
      return INV_EVENT;
    } else if(strlen(tempEvent->startDateTime.date) != 8) { // Leave room for the null!
      return INV_EVENT;
    } else if(strlen(tempEvent->startDateTime.time) < 6) {
      return INV_EVENT;
    } else if(tempEvent->properties == NULL) {
      return INV_EVENT;
    } else if(tempEvent->alarms == NULL) {
      return INV_EVENT;
    }

    // Now to loop through the properties
    bool hasClass = false, hasDescr = false, hasGeo = false;
    bool hasLocat = false, hasPrior = false, hasStatus = false;
    bool hasSum = false, hasTransp = false, hasOrgan = false;
    bool hasRecurid = false, hasUrl = false, hasRrule = false;
    bool hasCreate = false, hasLast = false, hasSeq = false;
    bool hasDtend = false, hasDur = false;

    ListIterator eventProps = createIterator(tempEvent->properties);
    while((tempProp = (Property *)nextElement(&eventProps)) != NULL) {
      if(strcmp(tempProp->propDescr, "\0") == 0) { // Regardless of the property, if it is malformed, it is bad
        return INV_EVENT;
      }

      if(strcmp(tempProp->propName, "CLASS") == 0) {
        if(hasClass) {
          return INV_EVENT;
        }
        hasClass = true;
      } else if(strcmp(tempProp->propName, "DESCRIPTION") == 0) {
        if(hasDescr) {
          return INV_EVENT;
        }
        hasDescr = true;
      } else if(strcmp(tempProp->propName, "GEO") == 0) {
        if(hasGeo) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasGeo = true;
      // Sanity Check: First set of bools done
      } else if(strcmp(tempProp->propName, "LOCATION") == 0) {
        if(hasLocat) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasLocat = true;
      } else if(strcmp(tempProp->propName, "PRIORITY") == 0) {
        if(hasPrior) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasPrior = true;
      } else if(strcmp(tempProp->propName, "STATUS") == 0) {
        if(hasStatus) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasStatus = true;
      // Sanity Check: Second set of bools done
      } else if(strcmp(tempProp->propName, "SUMMARY") == 0) {
        if(hasSum) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasSum = true;
      } else if(strcmp(tempProp->propName, "TRANSP") == 0) {
        if(hasTransp) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasTransp = true;
      } else if(strcmp(tempProp->propName, "ORGANIZER") == 0) {
        if(hasOrgan) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasOrgan = true;
      // Sanity Check: Third set of bools done
      } else if(strcmp(tempProp->propName, "RECURRENCE-ID") == 0) {
        if(hasRecurid) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasRecurid = true;
      } else if(strcmp(tempProp->propName, "URL") == 0) {
        if(hasUrl) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasUrl = true;
      } else if(strcmp(tempProp->propName, "RRULE") == 0) {
        if(hasRrule) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasRrule = true;
      // Sanity Check: Fourth set of bools done
      } else if(strcmp(tempProp->propName, "CREATED") == 0) {
        if(hasCreate) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasCreate = true;
      } else if(strcmp(tempProp->propName, "LAST-MODIFIED") == 0) {
        if(hasLast) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasLast = true;
      } else if(strcmp(tempProp->propName, "SEQUENCE") == 0) {
        if(hasSeq) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasSeq = true;
      // Sanity Check: fifth set of bools done
      } else if(strcmp(tempProp->propName, "DTEND") == 0) {
        if(hasDtend || hasDur) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasDtend = true;
      } else if(strcmp(tempProp->propName, "DURATION") == 0) {
        if(hasDur || hasDtend) { // If it has already been found, or the descr doesn't exist...
          return INV_EVENT;
        }
        hasDur = true;
      // Sanity Check: Last set of bools done
      } else { // And now for anything that is valid
        if(strcmp(tempProp->propName, "ATTACH") != 0 &&
          strcmp(tempProp->propName, "CATEGORIES") != 0 &&
          strcmp(tempProp->propName, "COMMENT") != 0 &&
          strcmp(tempProp->propName, "RESOURCES") != 0 &&
          strcmp(tempProp->propName, "ATTENDEE") != 0 &&
          strcmp(tempProp->propName, "CONTACT") != 0 &&
          strcmp(tempProp->propName, "RELATED-TO") != 0 &&
          strcmp(tempProp->propName, "EXDATE") != 0 &&
          strcmp(tempProp->propName, "RDATE") != 0) {
          return INV_EVENT; // Unless it is one of the above listed, it is bad.
        }
      }
      tempProp = NULL;
    }
    // After that disaster, it is time to check the alarms. FeelsGoodMan
    // Properties allowed in an alarm
    /*
     * ATTACH. May only occur once.
     * ACTION. Required, may only occur once.
     * REPEAT. May onlt occur once. Must be present if DURATION is present.
     * TRIGGER. Required, may only occur once. -- Just need to handle multiple occurences
     * DURATION. May only occur once. Must be present if REPEAT is present.
     */
    ListIterator eventAlarm = createIterator(tempEvent->alarms);
    Alarm *tempAlarm = NULL;
    while((tempAlarm = (Alarm *)nextElement(&eventAlarm)) != NULL) {
      if(strlen(tempAlarm->action) == 0 || strlen(tempAlarm->action) > 199) {
        return INV_ALARM;
      } else if(tempAlarm->trigger == NULL) {
        return INV_ALARM;
      } else if(tempAlarm->properties == NULL) {
        return INV_ALARM;
      } else if(getLength(tempAlarm->properties) > 3) {
        return INV_ALARM;
      }

      // And finally check the properties of the alarm
      ListIterator alarmProps = createIterator(tempAlarm->properties);
      bool hasRepeat = false, hasDur = false, hasAttach = false;
      while((tempProp = (Property *)nextElement(&alarmProps)) != NULL) {
        if(strcmp(tempProp->propDescr, "\0") == 0) {
          return INV_ALARM;
        }

        if(strcmp(tempProp->propName, "ATTACH") == 0) {
          if(hasAttach) {
            return INV_ALARM;
          }
          hasAttach = true;
        } else if(strcmp(tempProp->propName, "REPEAT") == 0) {
          if(hasRepeat) {
            return INV_ALARM;
          }
          hasRepeat = true;
        } else if(strcmp(tempProp->propName, "DURATION") == 0) {
          if(hasDur) {
            return INV_ALARM;
          }
          hasDur = true;
        } else { // Anything other than these properties is bad
          return INV_ALARM;
        }
        tempProp = NULL;
      }
      if((hasDur && !hasRepeat) || (hasRepeat && !hasDur)) { // Ensure that the alarm has both DURATION and REPEAT when exiting
        return INV_ALARM;
      }

      tempAlarm = NULL;
    }
    tempEvent = NULL;
  }

  return OK;
}

char *dtToJSON(DateTime prop) {
  char *temp = malloc(sizeof(char) * 60); // This should be enough for the required format
  if(prop.UTC == true) {
    sprintf(temp, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":true}", prop.date, prop.time);
  } else {
    sprintf(temp, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":false}", prop.date, prop.time);
  }
  return temp;
}

char *eventToJSON(const Event *event) {
  Property *findSum = NULL;
  bool found = false;
  char *temp = NULL;

  if(event == NULL) {
    temp = malloc(sizeof(char) * 10);
    sprintf(temp, "{}");
    return temp;
  }

  char *tempDT = dtToJSON(event->startDateTime);

  ListIterator iter = createIterator(event->properties);
  while((findSum = (Property *)nextElement(&iter)) != NULL) {
    if(strcmp(findSum->propName, "SUMMARY") == 0) {
      found = true;
      break;
    }
    findSum = NULL;
  }

  if(found) { // If there is no summary, just print nothing in its spot
    temp = malloc(sizeof(char) * (260 + strlen(findSum->propDescr)));
    sprintf(temp, "{\"startDT\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"%s\"}",
            tempDT, 3 + getLength(event->properties), getLength(event->alarms), findSum->propDescr);
  } else {
    temp = malloc(sizeof(char) * 260); // This can be much smaller since we know there is no summary to account for
    sprintf(temp, "{\"startDT\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"\"}",
            tempDT, 3 + getLength(event->properties), getLength(event->alarms));
  }

  free(tempDT);
  return temp;
}

char *eventListToJSON(const List *eventList) {
  char *temp = malloc(sizeof(char) * 30), *eventJSON = NULL; // Give temp a little bit of memory to start with

  if(eventList == NULL) {
    sprintf(temp, "[]");
    return temp;
  }

  List *list = (List *)eventList; // Because it is coming in as a const, it needs to be recast.
  int size = getLength(list), i = 0;

  if(size < 1) {
    sprintf(temp, "[]");
    return temp;
  }

  ListIterator iter = createIterator(list);
  Event *tempEvent = NULL;


  strcpy(temp, "[");

  while((tempEvent = (Event *)nextElement(&iter)) != NULL) {
    eventJSON = eventToJSON(tempEvent);
    temp = realloc(temp, strlen(temp) + strlen(eventJSON) + 5);
    i++;

    strcat(temp, eventJSON);

    if(i != size) {
      strcat(temp, ",");
    } else {
      strcat(temp, "]\0");
    }

    tempEvent = NULL;
    free(eventJSON);
    eventJSON = NULL;
  }

  return temp;
}

char *alarmToJSON(const Alarm *alarm) {
  char *temp = NULL;

  if(alarm == NULL) {
    temp = malloc(sizeof(char) * 10);
    sprintf(temp, "{}");
    return temp;
  }

  temp = malloc(sizeof(char) *  300);
  sprintf(temp, "{\"action\":\"%s\",\"trigger\":\"%s\",\"numProps\":%d}",
          alarm->action, alarm->trigger, getLength(alarm->properties) + 2);

  return temp;
}

char *alarmListToJSON(const List *alarmList) {
  char *temp = malloc(sizeof(char) * 30), *alarmJSON = NULL; // Give temp a little bit of memory to start with

  if(alarmList == NULL || getLength((List *)alarmList) < 1) {
    sprintf(temp, "[]");
    return temp;
  }

  ListIterator iter = createIterator((List *) alarmList);
  Alarm *tempAlarm = NULL;
  int i = 0;

  strcpy(temp, "[");

  while((tempAlarm = (Alarm *)nextElement(&iter)) != NULL) {
    alarmJSON = alarmToJSON(tempAlarm);
    temp = realloc(temp, strlen(temp) + strlen(alarmJSON) + 5);
    i ++;

    strcat(temp, alarmJSON);
    if(i != getLength((List *)alarmList)) {
      strcat(temp, ",");
    } else {
      strcat(temp, "]\0");
    }

    tempAlarm = NULL;
    free(alarmJSON);
    alarmJSON = NULL;
  }
  return temp;
}

char *propertyToJSON(const Property *prop) {
  char *temp = NULL;

  if(prop == NULL) {
    temp = malloc(sizeof(char) * 10);
    sprintf(temp, "{}");
    return temp;
  }

  temp = malloc(sizeof(char) *  4000);
//  temp = malloc(sizeof(char) * (strlen(prop->propName) + strlen(prop->propDescr) + 5));
  sprintf(temp, "{\"propName\":\"%s\",\"propDescr\":\"%s\"}",
          prop->propName, prop->propDescr);

  return temp;

}

char *propertyListToJSON(const List *propList) {
  char *temp = malloc(sizeof(char) * 30), *propJSON = NULL; // Give temp a little bit of memory to start with

  if(propList == NULL || getLength((List *)propList) < 1) {
    sprintf(temp, "[]");
    return temp;
  }

  ListIterator iter = createIterator((List *) propList);
  Property *tempProp = NULL;
  int i = 0;

  strcpy(temp, "[");

  while((tempProp = (Property *)nextElement(&iter)) != NULL) {
    propJSON = propertyToJSON(tempProp);
    temp = realloc(temp, strlen(temp) + strlen(propJSON) + 5);
    i ++;

    strcat(temp, propJSON);
    if(i != getLength((List *)propList)) {
      strcat(temp, ",");
    } else {
      strcat(temp, "]\0");
    }

    tempProp = NULL;
    free(propJSON);
    propJSON = NULL;
  }
  return temp;
}

char *calendarToJSON(const Calendar *cal) {
  char *temp = NULL;

  if(cal == NULL) {
    temp = malloc(sizeof(char) * 10); // Account for a large prodid
    sprintf(temp, "{}");
    return temp;
  }

  temp = malloc(sizeof(char) * 200 + strlen(cal->prodID)); // Account for a large prodid

  sprintf(temp, "{\"version\":%.0f,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d}",
              cal->version, cal->prodID, 2 + getLength(cal->properties), getLength(cal->events)); // The case of not having 1 event should never happen, as it wouldn't be a valid calendar otherwise

  return temp;
}

Calendar *JSONtoCalendar(const char *str) {
  if(str == NULL) {
    return NULL;
  }
  char *name, *descr = malloc(sizeof(char) * 1);
  float ver;
  strcpy(descr, "");

  /* TO DO: Convert this string searching into a function? */
  name = strstr(str, "\"VERSION\":\""); // Make sure the string is not malformed
  if(name == NULL) {
    free(descr);
    return NULL;
  }

  name = name + 11; // Move to end of the initial string
  int i = 0;

  while(name[0] != '"' && name[0] != '}') { // loop until we hit end of string or malformed
    descr = realloc(descr, sizeof(char) * (1 + (i + 1)));
    descr[i] = name[0]; // Assign characters
    i++; // progress pointers
    name = name + 1;
  }

  descr[i] = '\0';

  if(i == 0) { // if i is 0 then we found an end condition right away
    free(descr);
    return NULL;
  }

  ver = atof(descr);
  free(descr);
  descr = NULL;

  // Property 1 done. See a function would make this much cleaner
  descr = malloc(sizeof(char) * 1); // alloc a new string for the second property
  strcpy(descr, "");

  name = strstr(str, "\"PRODID\":\""); // Make sure the string is not malformed
  if(name == NULL) {
    free(descr);
    return NULL;
  }

  name = name + 10; // Move to end of the initial string

  i = 0;

  while(name[0] != '"' && name[0] != '}') { // loop until we hit end of string or malformed
    descr = realloc(descr, sizeof(char) * (1 + (i + 1)));
    descr[i] = name[0]; // Assign characters
    i++; // progress pointers
    name = name + 1;
  }
  descr[i] = '\0';

  if(i == 0) { // if i is 0 then we found an end condition right away
    free(descr);
    return NULL;
  }

  Calendar *temp = malloc(sizeof(Calendar));
  temp->version = -100;
  temp->version = ver;
  temp->prodID[0] = '\0';
  strcpy(temp->prodID, descr);
  temp->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
  temp->events = initializeList(&printEvent, &deleteEvent, &compareEvents);

  free(descr);
  return temp;
}

Event *JSONtoEvent(const char * str) {
  if(str == NULL) {
    return NULL;
  }
  char *name, *descr = malloc(sizeof(char) * 1);
  char *uid, *stamp, *start;
  strcpy(descr, "");

  name = strstr(str, "\"UID\":\""); // Make sure the string is not malformed
  if(name == NULL) {
    free(descr);
    return NULL;
  }

  name = name + 7; // Move to end of the initial string

  int i = 0;

  while(name[0] != '"' && name[0] != '}') { // loop until we hit end of string or malformed
    descr = realloc(descr, sizeof(char) * (1 + (i + 1)));
    descr[i] = name[0]; // Assign characters
    i++; // progress pointers
    name = name + 1;
  }
  if(i == 0) { // if i is 0 then we found an end condition right away
    free(descr);
    return NULL;
  }
  descr[i] = '\0';
//  strcat(descr, "\0");
  uid = malloc(strlen(descr) + 1);
  strcpy(uid, descr);

  free(descr);
  descr = NULL;

  // Search for dtstamp
  name = strstr(str, "\"DTSTAMP\":\""); // Make sure the string is not malformed
  if(name == NULL) {
    free(descr);
    return NULL;
  }

  name = name + 11; // Move to end of the initial string

  i = 0;

  while(name[0] != '"' && name[0] != '}') { // loop until we hit end of string or malformed
    descr = realloc(descr, sizeof(char) * (1 + (i + 1)));
    descr[i] = name[0]; // Assign characters
    i++; // progress pointers
    name = name + 1;
  }
  if(i == 0) { // if i is 0 then we found an end condition right away
    free(descr);
    return NULL;
  }
  descr[i] = '\0';
//  strcat(descr, "\0");
  stamp = malloc(strlen(descr) + 1);
  strcpy(stamp, descr);

  free(descr);
  descr = NULL;

  // Search for dtstamp
  name = strstr(str, "\"DTSTART\":\""); // Make sure the string is not malformed
  if(name == NULL) {
    free(descr);
    return NULL;
  }

  name = name + 11; // Move to end of the initial string

  i = 0;

  while(name[0] != '"' && name[0] != '}') { // loop until we hit end of string or malformed
    descr = realloc(descr, sizeof(char) * (1 + (i + 1)));
    descr[i] = name[0]; // Assign characters
    i++; // progress pointers
    name = name + 1;
  }
  if(i == 0) { // if i is 0 then we found an end condition right away
    free(descr);
    return NULL;
  }
  descr[i] = '\0';
//  strcat(descr, "\0");
  start = malloc(strlen(descr) + 1);
  strcpy(start, descr);

  free(descr);
  descr = NULL;

  // Search for summary
  name = strstr(str, "\"SUMMARY\":\""); // Make sure the string is not malformed
  if(name == NULL ) {
//    free(descr);
    return NULL;
  }

  name = name + 11; // Move to end of the initial string

  i = 0;

  while(name[0] != '"' && name[0] != '}') { // loop until we hit end of string or malformed
    descr = realloc(descr, sizeof(char) * (1 + (i + 1)));
    descr[i] = name[0]; // Assign characters
    i++; // progress pointers
    name = name + 1;
  }
//  if(i == 0) { // if i is 0 then we found an end condition right away
//    free(descr);
//    return NULL;
//  }
//  descr[i] = '\0';
//  strcat(descr, "\0");

  // Assign parsed info
  Event *temp = malloc(sizeof(Event)); // create event and initialize
  strcpy(temp->UID, "\0");
  strcpy(temp->UID, uid);

  char *token = strtok(stamp, "T");

  temp->creationDateTime.date[0] = '\0';
  strcpy(temp->creationDateTime.date, token);

  token = NULL;
  token = strtok(NULL, "");

  temp->creationDateTime.time[0] = '\0';
  strncpy(temp->creationDateTime.time, token, 6);

  if(strlen(token) == 6) {
    temp->creationDateTime.UTC = false;
  } else {
    temp->creationDateTime.UTC = false;
  }
  token = NULL;

  token = strtok(start, "T");

  temp->startDateTime.date[0] = '\0';
  strcpy(temp->startDateTime.date, token);

  token = NULL;
  token = strtok(NULL, "");

  temp->startDateTime.time[0] = '\0';
  strncpy(temp->startDateTime.time, token, 6);

  temp->startDateTime.UTC = temp->creationDateTime.UTC;

  temp->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
  temp->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);

  if(descr != NULL) {
    Property *tempProp = malloc(sizeof(Property) * (strlen(descr) + 1));
    strcpy(tempProp->propName, "SUMMARY\0");
    strcpy(tempProp->propDescr, "\0");
    strcpy(tempProp->propDescr, descr);
    insertBack(temp->properties, tempProp);
  }

  free(uid);
  free(stamp);
  free(start);
  free(descr);
  return temp;
}

void addEvent(Calendar *cal, Event *toBeAdded) {
  if(cal != NULL && toBeAdded != NULL) {
    insertBack(cal->events, toBeAdded);
  }

  return;
}

 /* A3 related functions */

char *createCalendarFromJSON(char *calStr, char *evtStr, char *file) {
    Event *tempEvt = JSONtoEvent(evtStr); //UID, DTSTAMP, DTSTART, SUMMARY
    Calendar *tempCal = JSONtoCalendar(calStr); //VERSION, PRODID
    ICalErrorCode tempCode = OK;

    addEvent(tempCal, tempEvt);

    if((tempCode = validateCalendar(tempCal)) == OK) {
      tempCode = writeCalendar(file, tempCal);
    }

    deleteCalendar(tempCal);

    if(tempCode == OK) {
      return "The new calendar was successfully created";
    } else {
      return printError(tempCode);
    }
}

char *addEventFromJSON(char *evtStr, char *file) {
    Event *tempEvt = JSONtoEvent(evtStr); //UID, DTSTAMP, DTSTART, SUMMARY
    Calendar *tempCal = NULL;
    ICalErrorCode tempCode = OK;

    tempCode = createCalendar(file, &tempCal);

    addEvent(tempCal, tempEvt);

    if((tempCode = validateCalendar(tempCal)) == OK) {
      tempCode = writeCalendar(file, tempCal);
    }

    deleteCalendar(tempCal);

    if(tempCode == OK) {
      return "The event was successfully added to the calendar";
    } else {
      return printError(tempCode);
    }
}

int callValidateCalendar(char *file) {
    Calendar *tempCal = NULL;
    ICalErrorCode err = OK;

    err = createCalendar(file, &tempCal);

    if(err != OK) {
      return 0;
    } else {
      return 1;
    }

    err = validateCalendar(tempCal);

    deleteCalendar(tempCal);
    if(err != OK) {
      return 0;
    } else {
      return 1;
    }
}

char *getCalJSON(char *file) {
  Calendar *tempCal = NULL;
  //ICalErrorCode temp;
  char *ret;

  // Won't validate since only valid files are displayed
  createCalendar(file, &tempCal);
  ret = calendarToJSON(tempCal);

  deleteCalendar(tempCal);
  return ret;
}

char *getEventsJSON(char *file) {
  Calendar *tempCal = NULL;
  char *ret;


  createCalendar(file, &tempCal);
  ret = eventListToJSON(tempCal->events);

  deleteCalendar(tempCal);
  return ret;
}

char *getEvtAlarms(char *evtNum, char *file) {
  int i = 1, num = atoi(evtNum);
  char *ret;
  Calendar *tempCal = NULL;
  Event *tempEvent;

  createCalendar(file, &tempCal);

  ListIterator iter = createIterator(tempCal->events);
  tempEvent = (Event *)nextElement(&iter);

  while(i != num) {
    tempEvent = (Event *)nextElement(&iter);
    i++;
  }

  ret = alarmListToJSON(tempEvent->alarms);

  deleteCalendar(tempCal);
  return ret;
}

char *getEvtProps(char *evtNum, char *file) {
  int i = 1, num = atoi(evtNum);
  char *ret;
  Calendar *tempCal = NULL;
  Event *tempEvent;

  createCalendar(file, &tempCal);

  ListIterator iter = createIterator(tempCal->events);
  tempEvent = (Event *)nextElement(&iter);

  while(i != num) {
    tempEvent = (Event *)nextElement(&iter);
    i++;
  }

  // Copy over the three built in properties such that the table is correct
  Property *tempProp;
  tempProp = malloc(sizeof(Property) + 1000);
  strcpy(tempProp->propName, "UID\0");
  strcpy(tempProp->propDescr, tempEvent->UID);
  insertBack(tempEvent->properties, tempProp);

  tempProp = NULL;

  tempProp = malloc(sizeof(Property) + 100);
  strcpy(tempProp->propName, "DTSTAMP\0");
  strcpy(tempProp->propDescr, tempEvent->creationDateTime.date);
  strcat(tempProp->propDescr, "T");
  strcat(tempProp->propDescr, tempEvent->creationDateTime.time);

  if(tempEvent->creationDateTime.UTC == true) {
      strcat(tempProp->propDescr, "Z");
  }
  insertBack(tempEvent->properties, tempProp);

  tempProp = malloc(sizeof(Property) + 100);
  strcpy(tempProp->propName, "DTSTART\0");
  strcpy(tempProp->propDescr, tempEvent->startDateTime.time);
  strcpy(tempProp->propDescr, tempEvent->startDateTime.date);
  strcat(tempProp->propDescr, "T");
  strcat(tempProp->propDescr, tempEvent->startDateTime.time);

  if(tempEvent->startDateTime.UTC == true) {
      strcat(tempProp->propDescr, "Z");
  }
  insertBack(tempEvent->properties, tempProp);

  ret = propertyListToJSON(tempEvent->properties);
  // Need to append UID, dtstamp and dtstart. Should be safe as we don't write this back
  deleteCalendar(tempCal);
  return ret;
}

// Additional functions

Property *parseProperty(char *startPtr, char* endPtr) {
  bool foundSeperator = false, foundText = false;
  char *tempPtr = startPtr;
  int i = 0, j = 0;

  Property *tempProperty;

  tempProperty = malloc(sizeof(Property) + 2000*sizeof(char));
  tempProperty->propName[0] = '\0';
  tempProperty->propDescr[0] = '\0';

  while(tempPtr < endPtr) {
    if (!foundText && *tempPtr == ';') {
     tempProperty->propName[0] = '\0';
     tempProperty->propDescr[0] = ';';
     tempProperty->propDescr[1] = '\0';
     return tempProperty; // This means we have found a comment
    }

    if((*tempPtr == ';' || *tempPtr == ':') && !foundSeperator && foundText) {
      foundSeperator = true;
    } else {
      if(!foundText || (foundText && !foundSeperator)) { // Load text into the name
        foundText = true;
        tempProperty->propName[i] = *tempPtr;
        i ++;
        tempProperty->propName[i] = '\0';
      } else if(foundText && foundSeperator) { // Load text into descr
        tempProperty->propDescr[j] = *tempPtr;
        j ++;
        tempProperty->propDescr[j] = '\0';
      }    }
    tempPtr += sizeof(char);
  }

  return tempProperty;
}

ICalErrorCode parseAlarm(Property *property, Alarm **alarm) {
  if(property->propName == '\0' && property->propDescr == '\0') {
    return INV_ALARM;
  }

  char *upperString = toUpper(property->propName);
  if(strcmp(upperString, "ACTION") == 0) {
    if(upperString != NULL) {
      free(upperString);
    }

    if(strlen((*alarm)->action) != 0) { // Duplicate actions
      free(property);
      return INV_ALARM;
    }

    strcpy((*alarm)->action, property->propDescr);
    free(property);
  } else if(strcmp(upperString, "TRIGGER") == 0) {
    if(upperString != NULL) {
      free(upperString);
    }

    if((*alarm)->trigger != NULL) { // Duplicate triggers
      free(property);
      return INV_ALARM;
    }

    (*alarm)->trigger = malloc(sizeof(char) * strlen(property->propDescr) + 1);
    (*alarm)->trigger[0] = '\0';

    strcpy((*alarm)->trigger, property->propDescr);
    (*alarm)->trigger[strlen(property->propDescr)] = '\0';
    free(property);
  } else {
    if(upperString) {
      free(upperString);
    }

    insertBack((*alarm)->properties, property);
  }
  return OK;
}

ICalErrorCode parseEvent(Property *property, Event **event) {
  char uidString[] = "UID", stampString[] = "DTSTAMP", startString[] = "DTSTART";
  char *token, *upperString = toUpper(property->propName);

  if(strcmp(upperString, uidString) == 0) { // Find the UID
    if(upperString != NULL) {
      free(upperString);
    }

    if(strcmp((*event)->UID, uidString) == 0) {
      free(property);
      return INV_EVENT;
    }

    strcpy((*event)->UID, property->propDescr);
    free(property);
  } else if(strcmp(upperString, stampString) == 0) { // Find the creation info
    if(upperString != NULL) {
      free(upperString);
    }

    if(strlen((*event)->creationDateTime.date) > 0) {
      free(property);
      return INV_EVENT;
    }

    token = strtok(property->propDescr, "T");

    if(token == NULL || strlen(token) != 8) {
      return INV_DT;
    }

    strcpy((*event)->creationDateTime.date, token); // Tokenize to extract date and time
    (*event)->creationDateTime.date[8] = '\0';

    token = strtok(NULL, "T");

    if(token == NULL || strlen(token) < 6) {
      return INV_DT;
    }

    strcpy((*event)->creationDateTime.time, token);

    if((*event)->creationDateTime.time[6] == 'Z') { // Check for UTC
      (*event)->creationDateTime.UTC = true;
    }

    (*event)->creationDateTime.time[6] = '\0';

    free(property);
  } else if(strcmp(upperString, startString) == 0) { // Find the start time info
    if(upperString != NULL) {
      free(upperString);
    }

    if(strlen((*event)->startDateTime.date) > 0) {
      free(property);
      return INV_EVENT;
    }

    token = strtok(property->propDescr, "T");

    if(token == NULL || strlen(token) != 8) {
      return INV_DT;
    }

    strcpy((*event)->startDateTime.date, token);
    (*event)->startDateTime.date[8] = '\0';

    token = strtok(NULL, "T");

    if(token == NULL || strlen(token) < 6) {
      return INV_DT;
    }

    strcpy((*event)->startDateTime.time, token);

    if((*event)->startDateTime.time[6] == 'Z') { // Check for UTC
      (*event)->startDateTime.UTC = true;
    }

    (*event)->startDateTime.time[6] = '\0';

    free(property);
  } else {
    if(upperString != NULL) {
      free(upperString);
    }
    insertBack((*event)->properties, property);
  }

  return OK;
}

char *toUpper(char *origText) {
  char *tempString = malloc(strlen(origText) + 1), *ptr = origText;
  int i = 0;
  while(i < strlen(origText)) {
    if(strcmp(ptr,(char *)"a") > 0 && strcmp(ptr, (char *)"z") < 0) {
      tempString[i] = *ptr - 32;
    } else {
      tempString[i] = *ptr;
    }
    ptr += sizeof(char);
    i ++;
  }
  tempString[i] = '\0';
  return tempString;
}

bool checkTags(char *openingTag, char *closingTag, char *text) {
  char *tempPtrOne, *tempPtrTwo;

  tempPtrOne = strstr(text, openingTag);
  if((void *) tempPtrOne != (void *)text) {
    return false;
  }
  tempPtrTwo = strstr(text, closingTag);

  if(tempPtrOne == NULL || tempPtrTwo == NULL) {
    return false;
  }

  return true;
}

void writeEventList(FILE *fPtr, List *list) {
  ListIterator iter = createIterator(list);
  Event *temp = NULL;

  while((temp = (Event *)nextElement(&iter)) != NULL) {
    fprintf(fPtr, "BEGIN:VEVENT\r\n");
    fprintf(fPtr, "UID:%s\r\n", temp->UID);
    if(temp->creationDateTime.UTC == true) {
      fprintf(fPtr, "DTSTAMP:%sT%sZ\r\n", temp->creationDateTime.date, temp->creationDateTime.time);
      fprintf(fPtr, "DTSTART:%sT%sZ\r\n", temp->startDateTime.date, temp->startDateTime.time);
    } else {
      fprintf(fPtr, "DTSTAMP:%sT%s\r\n", temp->creationDateTime.date, temp->creationDateTime.time);
      fprintf(fPtr, "DTSTART:%sT%s\r\n", temp->startDateTime.date, temp->startDateTime.time);
    }

    writePropertyList(fPtr, temp->properties);
    writeAlarmList(fPtr, temp->alarms);

    fprintf(fPtr, "END:VEVENT\r\n");
  }
}

void writeAlarmList(FILE *fPtr, List *list) {
  ListIterator iter = createIterator(list);
  Alarm *temp = NULL;

  while((temp = (Alarm *)nextElement(&iter)) != NULL) {
    fprintf(fPtr, "BEGIN:VALARM\r\n");
    fprintf(fPtr, "ACTION:%s\r\n", temp->action);
    fprintf(fPtr, "TRIGGER:%s\r\n", temp->trigger);

    writePropertyList(fPtr, temp->properties);

    fprintf(fPtr, "END:VALARM\r\n");
  }
}


void writePropertyList(FILE *fPtr, List *list) {
  ListIterator iter = createIterator(list);
  Property *temp = NULL;

  while((temp = (Property *)nextElement(&iter)) != NULL) {
    fprintf(fPtr, "%s:%s\r\n", temp->propName, temp->propDescr);
  }
}
