#include <ncurses.h>
#include "audio.h"
#include "ui.h"

float meters_decay = 100.0;

void display_meter(int y, int x, float value, float range, int width)
{
        int i;
        int strips = width*(value+range)/range;
        move(y, x);
        for (i = 0; i<width; ++i) addch(' ');
        move(y, x);
        for (i = 0; i<strips; ++i) addch('|');
        mvchgat(y, x, width-4, A_NORMAL, 1, NULL);
        mvchgat(y, x+width-4, 3, A_NORMAL, 2, NULL);
        mvchgat(y, x+width-1, 1, A_NORMAL, 3, NULL);
}

static int current_track = 0;

static int command(int key, struct track **tracks, int track_count)
{
        // Returns 0 on success, 1 when user wants to exit

        static enum {NORMAL, SET_MARK, GOTO_MARK} mode = NORMAL;
        static int mark[26];
        static int where;

        if (mode==NORMAL) {
                switch (key) {
                case KEY_DOWN:
                        if (current_track<track_count-1) ++current_track;
                        break;
                case KEY_UP:
                        if (current_track>0) --current_track;
                        break;
                case ' ':
                        if (transport==ROLLING) transport_stop();
                        else transport_start();
                        break;
                case 'z':
                        transport_locate(0);
                        break;
                case 'j':
                        transport_locate(frame-frame_rate*5);
                        break;
                case 'k':
                        transport_locate(frame+frame_rate*5);
                        break;
                case 'r':
                        tracks[current_track]->flags ^= TRACK_REC;
                        break;
                case 'm':
                        tracks[current_track]->flags ^= TRACK_MUTE;
                        break;
                case 's':
                        tracks[current_track]->flags ^= TRACK_SOLO;
                        break;
                case '-':
                        tracks[current_track]->vol -= 0.1;
                        if (tracks[current_track]->vol<=0.0) tracks[current_track]->vol = 0.0;
                        break;
                case '=':
                        tracks[current_track]->vol += 0.1;
                        if (tracks[current_track]->vol>=9.9) tracks[current_track]->vol = 9.9;
                        break;
                case ',':
                        tracks[current_track]->pan -= 0.1;
                        if (tracks[current_track]->pan<=0.0) tracks[current_track]->pan = 0.0;
                        break;
                case '.':
                        tracks[current_track]->pan += 0.1;
                        if (tracks[current_track]->pan>=1.0) tracks[current_track]->pan = 1.0;
                        break;
                case 'p':
                        mode = SET_MARK;
                        where = frame;
                        break;
                case 'P': /* set mark 5 secs before hitting the key */
                        mode = SET_MARK;
                        where = frame - frame_rate*5;
                        break;
                case '\'':
                        mode = GOTO_MARK;
                        break;
                case 'q':
                        return 1;
                }
        } else if (mode==SET_MARK) {
                if (key>='a' && key<='z') mark[key-'a'] = where;
                mode = NORMAL;
        } else if (mode==GOTO_MARK) {
                if (key>='a' && key<='z') transport_locate(mark[key-'a']);
                mode = NORMAL;
        }
        return 0;
}

static void display(struct track **tracks, int track_count)
{
        int t;
        for (t = 0; t<track_count; ++t) {
                //              -> R  M  S  vol   pan  name
                mvprintw(t+1, 0, "%s %s %s %s %4.2f %4.2f  %10s",
                         current_track==t?"->":"  ",
                         tracks[t]->flags&TRACK_REC ?"R":" ",
                         tracks[t]->flags&TRACK_MUTE?"M":" ",
                         tracks[t]->flags&TRACK_SOLO?"S":" ",
                         tracks[t]->vol,
                         tracks[t]->pan,
                         tracks[t]->name);

                display_meter(t+1, 34, tracks[t]->flags&TRACK_REC?tracks[t]->in_meter:tracks[t]->out_meter,
                              48.0f, 16);

                switch (transport) {
                case ROLLING: mvprintw(0, 0, "rolling"); break;
                case STOPPED: mvprintw(0, 0, "stopped"); break;
                }
                mvprintw(0, 10, "%5.1f s", (double)frame/frame_rate);
        }
        refresh();
}

int init_ui()
{
        initscr();
        cbreak();
        keypad(stdscr, TRUE);
        noecho();
        timeout(50);
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        return 0;
}

void main_loop(struct track **tracks, int track_count)
{
        int ch;
        do {
                display(tracks, track_count);
                ch = getch();
        } while (ch==ERR?1:command(ch, tracks, track_count)==0);
}

void shutdown_ui()
{
        endwin();
}
