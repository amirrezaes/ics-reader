/** @file event_manager.c
 *  @brief A pipes & filters program that uses conditionals, loops, and string processing tools in C to process iCalendar
 *  events and printing them in a user-friendly format.
 *  @author Felipe R.
 *  @author Hausi M.
 *  @author Juan G.
 *  @author Amirreza Esmaeili
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief The maximum line length.
 *
 */
#define MAX_LINE_LEN 132
#define MAX_EVENTS 500


/*
 * Struct: Arg
 * ----------------------------
 *   Holds inputs from terminal
 */
struct Arg
{
    int start;
    int end;
    char file_name[MAX_LINE_LEN];
};


/*
 * Struct: Event
 * ----------------------------
 *   Holds all the properties of an event
 */
struct Event
{
    int valid_event; // if event exists in given time fram then it is valid
    int start;
    int end;
    char hstart[MAX_LINE_LEN]; // starting hour
    char hend[MAX_LINE_LEN];   // ending hour
    char location[MAX_LINE_LEN];
    char summary[MAX_LINE_LEN];
    int until;
    int repeat_date[5];
    
};

/*
 * Function: zerofill
 * ----------------------------
 *   fills the "valid_event" variable in the given struct with 0s.
 *
 *   events: pointer to the struct array
 *   returns: void
 */
void zerofill(struct Event *events);

/*
 * Function: arg_parser
 * ----------------------------
 *   parses and refines commandline args and inserts them into given array
 *
 *   argc: Number of arguments.
 *   argv: The list of arguments passed to the program.
 *   args: Array to be filled with refined arguments.
 *   returns: void
 */
void arg_parser(int argc, char *argv[], struct Arg* args);


/*
 * Function: format_time
 * ----------------------------
 *   converts the yyyy/m/d format to yyyymmdd
 *
 *   s: pointer to the string containing yyyy/m/d format
 *   returns: void
 */
void format_time(char *s);

/*
 * Function: file_parser
 * ----------------------------
 *   reads the file and calls event_extractor on proper places, responsible for opening and closing files
 *
 *   fime_name: to parse
 *   events: array of events to write in
 *   returns: void
 */
void file_parser(const char *file_name, struct Event *events);


/*
 * Function: event_extractor
 *
 *   extracts an event and and adds the proper values to event struct
 *
 *   stream: file stream that is pointing to the begining of an event
 *   event:  pointer to the event that will be modified.
 *   returns: void
 */
void event_extractor(FILE *stream, struct Event *event);


/*
 * Function: format_clock
 *
 *   convers clock from HHMMSS to HH:MM PM/AM
 *
 *   raw_date: unprocessed date read from file
 *   target:   final form will be wrriten in this string.
 *   returns: void
 */
void format_clock(char *raw_date, char *target);


/*
 * Function: filter
 *
 *   Eliminates events based on their date, and returs the number of valid events
 *
 *   events: pointer to event array
 *   start:  starting date of valid events
 *   end:    ending date of valid events
 *   returns: int - number of valid events
 */
int filter(struct Event * events, int start, int end);


/*
 * Function: pprint
 *
 *   Prints the events in the specific format, takes care of same-day events and weekly events
 *
 *   events: pointer to event array
 *   count: number of valid events in the array
 *   returns: void
 */
void pprint(struct Event *events, int count);


/*
 * Function: print_header
 *
 *   Takes a date with yyyymmdd format and prints it in Month_Name day, year format
 *   adds suitable amount of dashes under the date to make it a header
 *
 *   date: date in yyyymmdd format
 *   returns: void
 */
void print_header(int date);


/**
 * Function: main
 * --------------
 * @brief The main function and entry point of the program.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The list of arguments passed to the program.
 * @return int 0: No errors; 1: Errors produced.
 *
 */
int main(int argc, char *argv[])
{
    struct Arg args; // holds refined args: start, end, file_name
    struct Event events[MAX_EVENTS]; // all possible events in file
    int valid_event_count;
    zerofill(events);
    arg_parser(argc, argv, &args);
    file_parser(args.file_name, events);
    valid_event_count = filter(events, args.start, args.end);
    pprint(events, valid_event_count);
    
}


void zerofill(struct Event *events){
    for(int i=0; i<MAX_EVENTS; i++){
        events[i].valid_event = 0;
        events[i].until = 0;
    }
}


void arg_parser(int argc,char *argv[], struct Arg * args){
    char *arg; // temporary arg holder
    for(int i = 1; i < argc; i++){
        arg = strtok(argv[i], "=");

        if(strcmp(arg, "--start") == 0){
            arg += strlen("--start")+1;
            format_time(arg);
            args->start = atoi(arg);

        } else if(strcmp(arg, "--end") == 0){
            arg += strlen("--end")+1;
            format_time(arg);
            args->end = atoi(arg);

        } else if(strcmp(arg, "--file") == 0){
            arg += strlen("--file")+1;
            strcpy(args->file_name, arg);
        }
    }
}


void format_time(char *s){
    int writer = 0, reader = 0;
    while (s[reader]){

        if (s[reader]!='/'){
            s[writer++] = s[reader];

        } else if((s[reader] == '/' && s[reader+2] == '/') || s[reader+2] == '\0'){
            s[writer++] = '0';
        }
        reader++;
    }
    s[writer]=0;
}


void file_parser(const char *file_name, struct Event *events){
    FILE *stream = fopen(file_name, "r");
    char line[MAX_LINE_LEN];
    int event_index =0;

    while (fgets(line, MAX_LINE_LEN, stream) != NULL){

        if(strcmp(line, "BEGIN:VEVENT\n") == 0){
            event_extractor(stream, &events[event_index]);
            event_index+=1;
        }
   }

   fclose(stream);
}


void event_extractor(FILE * stream, struct Event *event){
    char line[MAX_LINE_LEN];
    char *token, *value;
    while (strcmp(fgets(line, MAX_LINE_LEN, stream), "END:VEVENT\n") != 0){
        token=strtok(line, ":"); // keyword (left hand side)
        value = token + (strlen(token)+1); // value of keyword (right hand side)
        value = strtok(value, "\n"); // geting rid of newline char;
        if(strcmp(token, "DTSTART") == 0){
            format_clock(value, event->hstart);
            event->start = atoi(strtok(value, "T"));
            event->valid_event = 1;

        } else if(strcmp(token, "DTEND") == 0){
            format_clock(value, event->hend);
            event->end = atoi(strtok(value, "T"));

        } else if (strcmp(token, "LOCATION") == 0){
            strcpy(event->location, value);

        } else if (strcmp(token, "SUMMARY") == 0){
            strcpy(event->summary, value);

        } else if (strcmp(token, "RRULE") == 0){
            if (strstr(value, "WEEKLY") != NULL){

                value += strlen("FREQ=WEEKLY;WKST=MO;UNTIL=");
                value = strtok(strtok(value, ";"), "T");
                
                event->until =  atoi(value);
            }
        }
    }
}


void format_clock(char *raw_date, char *target){
    char *date, clock[9] = "00:00 --";
    char pm_to_am[3];
    int clock_i = 0; 
    int date_i = 0;
    int n;
    date = strtok(raw_date, "T") + 9; // 9 is the lenght of raw_date
    while(clock[clock_i]!= ' '){ // when clock is full, we are done, ignore the seconds in date.
        clock[clock_i] = date[date_i];
        if(clock_i == 1) clock_i++; // skip the ":" index
        clock_i++;
        date_i++;
    }
    if(atoi(clock) >= 12){
        if(atoi(clock) > 12){
            n = atoi(clock);
            n -= 12;
            sprintf(pm_to_am, "%2d", n);
            clock[0] = pm_to_am[0];
            clock[1] = pm_to_am[1];
        }
        clock[6] = 'P';
        clock[7] = 'M';
    } else{
        if(clock[0] == '0') clock[0] = ' ';
        clock[6] = 'A';
        clock[7] = 'M';
    }
    strcpy(target, clock);
}


int filter(struct Event * events, int start, int end){
    int temp, index = 0; // this holds the possible dates for a repeating event
    int count = 0;
    for(int i = 0; i < MAX_EVENTS; i++){

        if(events[i].valid_event){

            if(events[i].until != 0){ // in case there is a frequency
                temp = events[i].start;
                while(temp <= events[i].until){
                    if(start <= temp && temp <= end){
                        count++;
                        events[i].repeat_date[index] = temp;
                        index++;
                    }
                    temp += 7; // incrementing by a week
                }
                events[i].valid_event = events[i].repeat_date[0] == 0 ? 0 : 1; // if any repeat, then valid event

            } else if(start <= events[i].start && events[i].start <= end){
                count++;
                continue;

            } else events[i].valid_event = 0;

        } else break;
    }
    return count;
}


void pprint(struct Event *events, int count){
    int index, last_date = 0;
    int first_event = 1;
    for(int i = 0; i < MAX_EVENTS; i++){
        if(events[i].valid_event == 1){
            index = 0;
            do{
                if(events[i].start != last_date){
                    if(first_event){
                        first_event = 0;
                    } else printf("\n");
                    print_header(events[i].start);
                }
                printf("%s to %s: %s {{%s}}", events[i].hstart, events[i].hend, events[i].summary, events[i].location);
                last_date = events[i].start;
                if(events[i].repeat_date[index] != 0){
                    events[i].start = events[i].repeat_date[index+1];
                }
                if(count > 0){
                    printf("\n");
                }
                index++;
                count--;
            }while(index < 5 && events[i].repeat_date[index] != 0);
        }
    }
}


void print_header(int date){
    char dashes[21]; // dash string buffer
    char months[13][10] = {"", "January", "February", "March", "April",
                         "May", "June", "July", "August",
                         "September", "October", "November", "December"};
    int day = date % 100; // first two digits represent day
    int month = ((date % 10000) - day); // second two digits represent month, we get rid of first two by dividing to 100
    int year = (date - (month + day)) / 10000; // last 4 digits are the year, we get rid of the rest by dividing to 10000
    month /= 100;
    //February 14, 2022
    printf("%s %02d, %d\n", months[month], day, year);
    snprintf(dashes, 21, "%s %02d, %d", months[month], day, year);
    for (int i = 0; i < ((int) strlen(dashes)); ++i){
        printf("-");
    }
    printf("\n");
}
