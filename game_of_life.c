#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define X_FIELD_SIZE 80
#define Y_FIELD_SIZE 25
#define SLEEP_TIME 2          // in ms
#define DEFAULT_TURN_TIME 90  // in ms

int **create_matrix(int h, int w);
void clear_matrix(int **matrix, int h);
void render_fieldw(int **matrix, int h, int w);
void reset_matrix(int **matrix, int h, int w);
int change_field(int **matrix, int h, int w);
void copy_matrix(int **start_matrix, int **finish_matrix, int h, int w);
int live_cells_around(int **matrix, int h, int w, int y_pos, int x_pos);

void menu();
int upload(int **matrix, int h, int w);
void edit(int **matrix, int h, int w);
void game(int **matrix, int h, int w);

int main() {
    menu();
    return 0;
}

int **create_matrix(int h, int w) {
    int **matrix = malloc(h * sizeof(int *));
    if (matrix) {
        for (int i = 0; i < h; i++) {
            matrix[i] = malloc(w * sizeof(int));
            if (matrix[i]) {
                for (int j = 0; j < w; j++) {
                    matrix[i][j] = 0;
                }
            }
        }
    }

    return matrix;
}

void clear_matrix(int **matrix, int h) {
    if (matrix) {
        for (int i = 0; i < h; i++) {
            if (matrix[i]) {
                free(matrix[i]);
            }
        }
    }
    free(matrix);
}

void render_fieldw(int **matrix, int h, int w) {
    move(0, 0);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);  // inactive
    attron(COLOR_PAIR(3));

    for (int x = 0; x < w + 2; x++) {
        printw("-");
    }
    printw("\n");

    for (int y = 0; y < h; y++) {
        printw("|");
        for (int x = 0; x < w; x++) {
            if (matrix[y][x]) {
                printw("O");
            } else {
                printw(".");
            }
        }
        printw("|");
        printw("\n");
    }

    for (int x = 0; x < w + 2; x++) {
        printw("-");
    }
    printw("\n");
}

void copy_matrix(int **start_matrix, int **finish_matrix, int h, int w) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            finish_matrix[i][j] = start_matrix[i][j];
        }
    }
}

// <2 живых клеток: живая -> мертвая
// 2-3 живых клеток: живая -> живая
// >3 живых клеток: живая -> мертвая
// ==3 живых клеток: мертвая -> живая

int change_field(int **matrix, int h, int w) {
    int **buffer_matrix = create_matrix(h, w);
    copy_matrix(matrix, buffer_matrix, h, w);
    int flag = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int cells_around = live_cells_around(buffer_matrix, h, w, i, j);
            if (buffer_matrix[i][j]) {
                if (cells_around < 2 || cells_around > 3) {
                    matrix[i][j] = 0;
                }
                flag = 1;
            } else {
                if (cells_around == 3) {
                    matrix[i][j] = 1;
                }
            }
        }
    }

    clear_matrix(buffer_matrix, h);
    return flag;
}

int live_cells_around(int **matrix, int h, int w, int y_pos, int x_pos) {
    int res = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if ((i || j) && matrix[(h + y_pos + i) % h][(w + x_pos + j) % w]) {
                res++;
            }
        }
    }
    return res;
}

void menu() {
    int **field = create_matrix(Y_FIELD_SIZE, X_FIELD_SIZE);
    initscr();
    noecho();
    start_color();
    curs_set(0);
    keypad(stdscr, 1);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);  // active
    init_pair(2, COLOR_WHITE, COLOR_BLACK);  // inactive

    int c = 0;
    int line = 0;
    int flag = 1;
    while ((c != '\n' || line) && flag) {
        c = 0;
        render_fieldw(field, Y_FIELD_SIZE, X_FIELD_SIZE);
        attron(COLOR_PAIR(line == 0 ? 1 : 2));
        mvprintw(30, 20, "START");
        attron(COLOR_PAIR(line == 1 ? 1 : 2));
        mvprintw(31, 20, "UPLOAD");
        attron(COLOR_PAIR(line == 2 ? 1 : 2));
        mvprintw(32, 20, "EDIT");
        attron(COLOR_PAIR(line == 3 ? 1 : 2));
        mvprintw(33, 20, "EXIT");
        c = getch();
        if (c == KEY_UP) {
            line = (4 + line - 1) % 4;
        }
        if (c == KEY_DOWN) {
            line = (4 + line + 1) % 4;
        }

        if (c == '\n') {
            switch (line) {
                case 0:
                    game(field, Y_FIELD_SIZE, X_FIELD_SIZE);
                    break;
                case 1:

                    upload(field, Y_FIELD_SIZE, X_FIELD_SIZE);
                    break;

                case 2:
                    edit(field, Y_FIELD_SIZE, X_FIELD_SIZE);
                    break;

                case 3:
                    flag = 0;
                    break;
            }
        }
        napms(SLEEP_TIME);
    }

    endwin();
    clear_matrix(field, Y_FIELD_SIZE);
}

int upload(int **matrix, int h, int w) {
    int flag = 1;
    clear();
    refresh();

    curs_set(1);
    move(10, 0);
    echo();
    printw("Enter file name: ");
    char *string = malloc(101 * sizeof(char));
    char c = 0;
    FILE *file;
    if (scanw("%s", string) != 1) {
        flag = 0;
    }
    if (flag) {
        file = fopen(string, "r");
        if (!file) {
            flag = 0;
            move(10, 0);
            printw("file cannot be opened\npress any key to continue");
            getch();
        }
    }

    if (flag) {
        reset_matrix(matrix, h, w);
        for (int i = 0; i < h && flag && c != EOF; i++) {
            for (int j = 0; j < w && flag && c != EOF; j++) {
                if (fscanf(file, "%c", &c) != 1) {
                    flag = 0;
                    break;
                }
                if (c == '\n' || c == EOF) {
                    break;
                }
                switch (c) {
                    case '1':
                        matrix[i][j] = 1;
                        break;
                    default:
                        matrix[i][j] = 0;
                        break;
                }
            }
        }
    }
    noecho();
    start_color();
    keypad(stdscr, 1);
    curs_set(0);
    refresh();

    return flag;
}

void reset_matrix(int **matrix, int h, int w) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            matrix[i][j] = 0;
        }
    }
}

void edit(int **matrix, int h, int w) {
    int cursor_x = 4;
    int cursor_y = 4;
    clear();
    refresh();
    mvprintw(0, w + 5, "move cursor: arrow keys");
    mvprintw(1, w + 5, "change cell: spacebar");
    mvprintw(2, w + 5, "exit: q");
    while (1) {
        mvprintw(0, w + 5, "move cursor: arrow keys");
        mvprintw(1, w + 5, "change cell: spacebar");
        mvprintw(2, w + 5, "exit: q");
        render_fieldw(matrix, h, w);
        mvaddch(cursor_y + 1, cursor_x + 1, 'X');

        int key = getch();
        switch (key) {
            case 'q':
                clear();
                refresh();
                return;
            case KEY_DOWN:
                cursor_y = (cursor_y + 1 + h) % h;
                break;
            case KEY_UP:
                cursor_y = (cursor_y - 1 + h) % h;
                break;
            case KEY_LEFT:
                cursor_x = (cursor_x - 1 + w) % w;
                break;
            case KEY_RIGHT:
                cursor_x = (cursor_x + w + 1) % w;
                break;
            case ' ':
                matrix[cursor_y][cursor_x] = !matrix[cursor_y][cursor_x];
                break;
        }
        napms(SLEEP_TIME);
    }
}

void game(int **matrix, int h, int w) {
    int change_time = DEFAULT_TURN_TIME;
    unsigned int time_key = 1;
    int flag = 1;
    clear();
    timeout(1);
    refresh();
    render_fieldw(matrix, h, w);
    while (flag) {
        refresh();
        if (time_key) {
            if (!change_field(matrix, h, w)) {
                flag = 0;
            }
            render_fieldw(matrix, h, w);
        }
        napms(change_time);
        int key = getch();
        switch (key) {
            case KEY_LEFT:
                if (time_key > 0) {
                    time_key--;
                }
                break;
            case KEY_RIGHT:
                if (time_key < 3) {
                    time_key++;
                }
                break;
            case 'q':
                flag = 0;
                break;
            default:
                break;
        }
        if (time_key) {
            change_time = DEFAULT_TURN_TIME / time_key / time_key / time_key;
        }
    }
    render_fieldw(matrix, h, w);
    timeout(-1);
    getch();
}