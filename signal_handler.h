#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

void do_clean_work();
void process_singal(int signo);
int set_signal_handler();

#endif // !SIGNAL_HANDLER_H
