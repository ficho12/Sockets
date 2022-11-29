#include <regex.h>
#include <stdio.h>

//Mirar si hay que poner los caracteres \r\n
//No creo porque esos caracteres son de finalizacion

#define regHELO "HELO [a-zA-Z]+\.[a-zA-Z]+"
#define regMAIL "MAIL FROM: <[a-zA-Z]+@[a-zA-Z]+\.[a-zA-Z]+>"
#define regRCPT "RCPT TO: <[a-zA-Z]+@[a-zA-Z]+\.[a-zA-Z]+>"
#define regDATA "DATA"
#define regPUNTO "\."
#define regQUIT "QUIT"

/*
int useRegex(char* textToCheck) {
    regex_t compiledRegex;
    int reti;
    int actualReturnValue = -1;
    char messageBuffer[100];

    //Compile regular expression
    reti = regcomp(&compiledRegex, "H[a-zA-Z]LO [a-zA-Z]+\\.[a-zA-Z]+", REG_EXTENDED | REG_ICASE);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        return -2;
    }

    // Execute compiled regular expression
    reti = regexec(&compiledRegex, textToCheck, 0, NULL, 0);
    if (!reti) {
        puts("Match");
        actualReturnValue = 0;
    } else if (reti == REG_NOMATCH) {
        puts("No match");
        actualReturnValue = 1;
    } else {
        regerror(reti, &compiledRegex, messageBuffer, sizeof(messageBuffer));
        fprintf(stderr, "Regex match failed: %s\n", messageBuffer);
        actualReturnValue = -3;
    }

    //Free memory allocated to the pattern buffer by regcomp()
    regfree(&compiledRegex);
    return actualReturnValue;
}
*/

//Devuelve 1 si el REGEX es correcto y 0 si es falso y -2 o -3 si da error de REGEX
int reg(char* textToCheck, char* regExp)
{
    regex_t compiledRegex;
    int reti;
    int actualReturnValue = -1;
    char messageBuffer[100];

    // Compile regular expression
    reti = regcomp(&compiledRegex, regExp, REG_EXTENDED | REG_ICASE);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        return -2;
    }

    // Execute compiled regular expression
    reti = regexec(&compiledRegex, textToCheck, 0, NULL, 0);
    if (!reti) {
        puts("Match");
        actualReturnValue = 1;
    } else if (reti == REG_NOMATCH) {
        puts("No match");
        actualReturnValue = 0;
    } else {
        regerror(reti, &compiledRegex, messageBuffer, sizeof(messageBuffer));
        fprintf(stderr, "Regex match failed: %s\n", messageBuffer);
        actualReturnValue = -3;
    }

    // Free memory allocated to the pattern buffer by regcomp()
    regfree(&compiledRegex);
    return actualReturnValue;
}