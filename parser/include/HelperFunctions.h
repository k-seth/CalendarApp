/**
 *
 * Author: Seth Kuipers
 * Class: CIS*2750
 * Due Date: February 4, 2018
 * Project: Assignment One
 *
 */

#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include "CalendarParser.h"
// Parser Functions

/**
 * This function parses through all text constrained within the passed pointers to extract the properties
 * In: char * - represents the pointer denoting the beginning of the text
       char * - represents the pointer denoting the end of the text
 * Out: Property * - represents a malloc'd (or null in the event of comments) property containing data
 */
Property *parseProperty(char *, char *);

/**
 * This function parses through all text constrained within the passed pointers to extract the alarms
 * In: Property * - represents a pointer to an existing property struct
       Alarm ** - represents a double pointer a malloced alarm
 * Out: ICalErrorCode * - represents an error code depending on parse state
 */
ICalErrorCode parseAlarm(Property *, Alarm **);

/**
 * This function looks at a property that has been parsed and assigns it to an event
 * In: Property * - represents a pointer to the property that was parsed. Line info may contain alarm information
       Event ** - represents the event to be malloc'd and created
 * Out: ICalErrorCode - represents the state of the event creation. OK is reported by default
 */
ICalErrorCode parseEvent(Property *, Event **);

// Other helper functions

/**
 * This function is converts all given text to upper case
 * In: char * - represents the original text
 * Out: char * - represents the newly converted text
 */
char *toUpper(char *);

/**
 * This function checks to ensure that the given opening and closing tags exist within a block of text
 * In: char * - represents the text for the opening tag to find
       char * - represents the text for the closing tag to find
       char * - represents the original text
 * Out: bool - returns true if both string are found, otherwise returns false
 */
bool checkTags(char *, char *, char *);

/**
 * This function writes iterates through each alarm and writes all related information
 * In: FILE * - represents an open file
       List * - represents a list of data to be iterated
 * Out: None
 */
void writeEventList(FILE *, List *);

/**
 * This function writes iterates through each alarm and writes all related information
 * In: FILE * - represents an open file
       List * - represents a list of data to be iterated
 * Out: None
 */
void writeAlarmList(FILE *, List *);

/**
 * This function writes iterates through each property and writes all related information
 * In: FILE * - represents an open file
       List * - represents a list of data to be iterated
 * Out: None
 */
void writePropertyList(FILE *, List *);

/**
 * This function coverts an alarm in to a JSON string
 * In: const Alarm * - represents the alarm to be converted
 * Out: char * - Represents the returned JSON string
 */
char *alarmToJSON(const Alarm *);

/**
 * This function coverts a list of alarms into a JSON string
 * In: const List * - represents the list to be converted
 * Out: char * - Represents the returned JSON string
 */
char *alarmListToJSON(const List *);

/**
 * This converts a property into a JSON string
 * In: const Property * - represents the property to be converted
 * Out: char * - Represents the returned JSON string
 */
char *propertyToJSON(const Property *);

/**
 * This function coverts a list of properties in to a JSON string
 * In: const List * - represents the alarm to be converted
 * Out: char * - Represents the returned JSON string
 */
char *propertyListToJSON(const List *);
#endif
