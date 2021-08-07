
#include "answer.h"
#include "state_machine.h"


void prints(const char* str);

bool answer_call_handler(const struct cmd_handler* handlers, char cmd, char arg)
{
    bool found = false;
    while(handlers->cmd)
    {
        if(handlers->cmd  == cmd) {
            (*handlers->handler)(cmd, arg);
            found = true;
        }
        handlers++;
    }
    
    return found;
}

