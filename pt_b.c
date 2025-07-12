#define _XOPEN_SOURCE 700 // wcswidth関数のプロトタイプ宣言を有効にするため
/**
 * ルドーゲーム - UIプロトタイプ
 *
 * このコードは、提供された仕様書に基づいてUIと画面遷移を実装しています。
 * ターミナルグラフィックスとマウス制御にはncursesライブラリを使用しています。
 *
 * 注意: ゲームロジック、ネットワーク機能、実際のゲームボードは未実装であり、
 * スタブ関数で表されています。
 *
 * コンパイル方法 (重要):
 * gcc this_file.c -o Ludo -lncursesw
 *
 * 必要なファイル構造:
 * ./Ludo (実行可能ファイル)
 * ./rule/rule.txt
 */
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // sleep()のため
#include <locale.h> // 文字エンコーディング設定のために追加
#include <wchar.h>  // マルチバイト文字の幅計算のために追加
#include <ctype.h>  // isprint()のため

// --- 列挙型定義 ---
typedef enum {
    MENU_ITEM_START, MENU_ITEM_RULES, MENU_ITEM_EXIT,
    MENU_ITEM_CREATE_SERVER, MENU_ITEM_JOIN_SERVER,
    MENU_ITEM_SUBMIT_JOIN, MENU_ITEM_BACK,
    INPUT_AREA, MENU_ITEM_NONE
} MenuSelection;

// --- 構造体定義 ---
typedef struct {
    const char *text;
    int y, x, height, width;
    MenuSelection action;
} MenuItem;

// --- 関数プロトタイプ宣言 ---
void run();
void shutdown_game();
void showMainMenu();
void showServerSelectMenu();
void showRulesScreen();
void displayFileContent(const char *filepath);
void displayError(const char *message);
MenuSelection handleInput(MenuItem items[], int num_items, char* input_buffer, int buffer_size);
void initializeNcurses();
void cleanupNcurses();
void drawMenu(const char* title, MenuItem items[], int num_items, bool show_join_input, const char* code);
void drawInputField(int y, int x, int width, const char* label, const char* content);
void drawBoxWithTitle(int y, int x, int height, int width, const char* title); // [FIX] 抜け落ちていたプロトタイプ宣言を追加
int getDisplayWidth(const char* str);
void buildServer();
void connectToServer(const char* code);

// --- メイン関数 ---
int main() {
    setlocale(LC_ALL, "");
    initializeNcurses();
    run();
    cleanupNcurses();
    return 0;
}

// --- コアアプリケーションロジック ---
void run() {
    while (1) {
        showMainMenu();
    }
}

void shutdown_game() {
    // 将来的なクリーンアップ処理
}

// --- 画面実装 ---
void showMainMenu() {
    timeout(-1);
    const char *title = "Ludo";
    MenuItem items[] = {
        {"スタート", LINES / 2 - 1, 0, 1, 0, MENU_ITEM_START},
        {"ルール",   LINES / 2 + 1, 0, 1, 0, MENU_ITEM_RULES},
        {"終了",     LINES / 2 + 3, 0, 1, 0, MENU_ITEM_EXIT}
    };
    int num_items = sizeof(items) / sizeof(items[0]);

    for(int i = 0; i < num_items; i++) {
        items[i].width = getDisplayWidth(items[i].text);
        items[i].x = (COLS - items[i].width) / 2;
    }

    drawMenu(title, items, num_items,false, "");
    MenuSelection choice = handleInput(items, num_items, NULL, 0);

    if (choice == MENU_ITEM_START) showServerSelectMenu();
    else if (choice == MENU_ITEM_RULES) showRulesScreen();
    else if (choice == MENU_ITEM_EXIT) {
        shutdown_game();
        cleanupNcurses();
        exit(0);
    }
}

void showServerSelectMenu() {
    timeout(100);
    const char* title = "サーバー選択";
    char server_code[7] = "";
    bool join_mode = false;

    while (1) {
        MenuItem items[5];
        int num_items = 0;

        int y_pos = LINES / 2 - 3;
        items[num_items++] = (MenuItem){"サーバーを作成", y_pos, 0, 1, 0, MENU_ITEM_CREATE_SERVER};
        y_pos += 2;
        items[num_items++] = (MenuItem){"サーバーに参加", y_pos, 0, 1, 0, MENU_ITEM_JOIN_SERVER};
        y_pos += 2;
        
        if (join_mode) {
            items[num_items++] = (MenuItem){"参加用コードを入力", y_pos, 0, 3, 24, INPUT_AREA};
            y_pos += 4;
            items[num_items++] = (MenuItem){"参加", y_pos, 0, 1, 0, MENU_ITEM_SUBMIT_JOIN};
            y_pos += 2;
        }
        items[num_items++] = (MenuItem){"メインメニューに戻る", y_pos, 0, 1, 0, MENU_ITEM_BACK};
        
        for(int i = 0; i < num_items; i++) {
            if(items[i].action != INPUT_AREA) {
                 items[i].width = getDisplayWidth(items[i].text) + 4;
            }
            items[i].x = (COLS - items[i].width) / 2;
        }
        
        drawMenu(title, items, num_items, true, join_mode, server_code);
        MenuSelection choice = handleInput(items, num_items, server_code, sizeof(server_code));

        if (choice == MENU_ITEM_CREATE_SERVER) {
            buildServer(); timeout(-1); return;
        } else if (choice == MENU_ITEM_JOIN_SERVER) {
            if (!join_mode) { join_mode = true; curs_set(1); }
        } else if (choice == MENU_ITEM_SUBMIT_JOIN) {
            if (strlen(server_code) > 0) { connectToServer(server_code); timeout(-1); curs_set(0); return; }
        } else if (choice == MENU_ITEM_BACK) {
            timeout(-1); curs_set(0); return;
        }
    }
}

void showRulesScreen() {
    timeout(-1);
    clear();
    const char *filepath = "./rule/rule.txt";
    displayFileContent(filepath);

    MenuItem back_button[] = {{"戻る", LINES - 3, 0, 1, 0, MENU_ITEM_BACK}};
    back_button[0].width = getDisplayWidth(back_button[0].text) + 4;
    back_button[0].x = (COLS - back_button[0].width) / 2;
    
    mvprintw(back_button[0].y, back_button[0].x, "[ %s ]", back_button[0].text);
    refresh();

    while(handleInput(back_button, 1, NULL, 0) != MENU_ITEM_BACK);
}

// --- 入力およびUIヘルパー ---
MenuSelection handleInput(MenuItem items[], int num_items, char* input_buffer, int buffer_size) {
    int ch = getch();
    
    if (ch == ERR) {
        return MENU_ITEM_NONE;
    }
    
    if (ch == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK && (event.bstate & BUTTON1_CLICKED)) {
            for (int i = 0; i < num_items; i++) {
                if (event.y >= items[i].y && event.y < items[i].y + items[i].height &&
                    event.x >= items[i].x && event.x < items[i].x + items[i].width) {
                    return items[i].action;
                }
            }
        }
    } 
    else if (input_buffer != NULL) {
        int len = strlen(input_buffer);
        if ((isprint(ch)) && (len < buffer_size - 1)) {
            input_buffer[len] = ch;
            input_buffer[len + 1] = '\0';
        } else if ((ch == KEY_BACKSPACE || ch == 127) && len > 0) {
            input_buffer[len - 1] = '\0';
        } else if (ch == '\n' || ch == KEY_ENTER) {
             return MENU_ITEM_SUBMIT_JOIN;
        }
    }
    return MENU_ITEM_NONE;
}

void drawMenu(const char* title, MenuItem items[], int num_items, bool show_join_input, const char* code) {
    clear();
    mvprintw(2, (COLS - getDisplayWidth(title)) / 2, "%s", title);

    for (int i = 0; i < num_items; i++) {
        if (items[i].action == INPUT_AREA) {
            if (show_join_input) {
                drawInputField(items[i].y, items[i].x, items[i].width, items[i].text, code);
            }
        } else {
            const char* text = items[i].text;
            if (items[i].action == MENU_ITEM_START || items[i].action == MENU_ITEM_RULES || items[i].action == MENU_ITEM_EXIT) {
                 mvprintw(items[i].y, items[i].x, "%s", text);
            } else {
                 mvprintw(items[i].y, items[i].x, "[ %s ]", text);
            }
        }
    }

    if(show_join_input) {
        for(int i=0; i<num_items; ++i) {
            if(items[i].action == INPUT_AREA) {
                move(items[i].y + 1, items[i].x + 2 + strlen(code));
                break;
            }
        }
    }
    refresh();
}

void drawInputField(int y, int x, int width, const char* label, const char* content) {
    WINDOW* win = newwin(3, width, y, x);
    wborder(win, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
    mvwprintw(win, 0, (width - getDisplayWidth(label) - 2) / 2, " %s ", label);
    mvwprintw(win, 1, 2, "%s", content);
    wrefresh(win);
    delwin(win);
}

// --- スタブ関数とその他ヘルパー関数 ---
void buildServer() {
    clear();
    mvprintw(LINES / 2, (COLS - getDisplayWidth("（未実装）サーバー作成処理...")) / 2, "（未実装）サーバー作成処理...");
    refresh(); sleep(2);
}

void connectToServer(const char* code) {
    clear();
    char message[100];
    snprintf(message, sizeof(message), "（未実装）コード「%s」でサーバーに接続...", code);
    mvprintw(LINES / 2, (COLS - getDisplayWidth(message)) / 2, "%s", message);
    refresh(); sleep(2);
}

void displayFileContent(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        displayError("エラー: rule.txtが見つかりませんでした。");
        sleep(3); return;
    }
    char line[256];
    int row = 5;
    const char *title = "== ルール ==";
    mvprintw(2, (COLS - getDisplayWidth(title)) / 2, "%s", title);
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        mvprintw(row++, (COLS - getDisplayWidth(line)) / 2, "%s", line);
    }
    fclose(file);
}

void displayError(const char *message) {
    int height = 5;
    int width = getDisplayWidth(message) + 4; 
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW *win = newwin(height, width, start_y, start_x);
    box(win, 0, 0);
    mvwprintw(win, 0, (width - getDisplayWidth("エラー")) / 2 - 1, " %s ", "エラー");
    mvwprintw(win, 2, 2, "%s", message);
    wrefresh(win);
    delwin(win);
}

void drawBoxWithTitle(int y, int x, int height, int width, const char* title) {
    WINDOW *win = newwin(height, width, y, x);
    box(win, 0, 0);
    mvwprintw(win, 0, (width - getDisplayWidth(title) -2 ) / 2, " %s ", title);
    wrefresh(win);
    delwin(win);
}

int getDisplayWidth(const char* str) {
    wchar_t wcs[256];
    int len = mbstowcs(wcs, str, 256);
    if (len < 0) return strlen(str);
    int width = wcswidth(wcs, len);
    return (width < 0) ? strlen(str) : width;
}

void initializeNcurses() {
    initscr(); cbreak(); noecho();
    curs_set(0); keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h\n");
}

void cleanupNcurses() {
    printf("\033[?1003l\n");
    endwin();
}
