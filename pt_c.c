#define _XOPEN_SOURCE 700 // wcswidth関数のプロトタイプ宣言を有効にするため
/**
 * ルドーゲーム - プロトタイプ (最終修正版)
 *
 * このコードは、提供された仕様書に基づいてUIと画面遷移を実装しています。
 * 今回の更新で、盤面の描画ロジックを全面的に刷新し、仕様書通りの
 * 正しいレイアウトを忠実に再現するように修正しました。
 *
 * コンパイル方法 (重要):
 * gcc this_file.c -o Ludo -lncursesw
 */

 /*盤面の機能が機能していない*/
 /*rule.txtの表示が壊滅*/
 /*参加コードの入力欄がおかしい*/
 /*drawBoxWithtitleの修正が必要*/

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // sleep()のため
#include <locale.h>     // 文字エンコーディング設定のために追加
#include <wchar.h>      // マルチバイト文字の幅計算のために追加
#include <ctype.h>      // isprint()のため
#include <time.h>       // 乱数生成のため

// --- 定数定義 ---
#define BOARD_H 31 // 15 * 2 + 1
#define BOARD_W 61
#define PATH_LENGTH 52
#define HOME_STRETCH_LENGTH 6

// --- 列挙型定義 ---
typedef enum {
    MENU_ITEM_START,
    MENU_ITEM_RULES,
    MENU_ITEM_EXIT,
    MENU_ITEM_CREATE_SERVER,
    MENU_ITEM_JOIN_SERVER,
    MENU_ITEM_SUBMIT_JOIN,
    MENU_ITEM_BACK,
    MENU_ITEM_ROLL_DICE,
    INPUT_AREA,
    MENU_ITEM_NONE
} MenuSelection;

typedef enum {
    C_NONE,
    C_RED,
    C_GREEN,
    C_YELLOW,
    C_BLUE,
    C_PATH,
    C_GOAL,
    C_GRID,
    C_PANEL_BG
} CellColor;

// --- 構造体定義 ---
typedef struct {
    const char *text;
    int y, x, height, width;
    MenuSelection action;
} MenuItem;

typedef struct {
    int y, x;
} Point;

typedef struct {
    int id;
    int position; // -1:ベース, 0-51:通常ルート, 101-106:赤ゴール, 201.. , 999:ゴール済
} Piece;

typedef struct {
    int id;
    CellColor color;
    Piece pieces[4];
    int pieces_at_goal;
    bool is_active;
} Player;

typedef struct {
    char server_code[7];
    Player players[4];
    int num_players;
    int current_turn_idx;
    int dice_value;
    char message_log[5][100];
    int your_player_id;
} GameState;

// --- グローバル盤面データ ---
const char* PIECE_SYMBOLS[] = {"1", "2", "3", "4"}; // [FIX] UI崩れを防ぐため単純な数字に変更

// --- 関数プロトタイプ宣言 ---
void run();
void shutdown_game();
void buildServer();
void connectToServer(const char* code);
void showMainMenu();
void showServerSelectMenu();
void showRulesScreen();
void showGameScreen(GameState *state);
void drawBoard(WINDOW* win, GameState *state);
void drawMenu(const char* title, MenuItem items[], int num_items, bool show_join_input, const char* code);
void drawInputField(int y, int x, int width, const char* label, const char* content);
void drawBoxWithTitle(WINDOW* win, int y, int x, int height, int width, const char* title, int color_pair_index);
void displayFileContent(const char *filepath);
void displayError(const char *message);
void initColors();
Point getPieceCoords(const Piece* piece, Player* player);
MenuSelection handleInput(MenuItem items[], int num_items, char* input_buffer, int buffer_size, WINDOW* clickable_win, int x_offset, int y_offset);
const char* colorToString(CellColor color);
void generateServerCode(char *code, size_t size);
int getDisplayWidth(const char* str);
void initializeNcurses();
void cleanupNcurses();

// --- メイン関数 ---
int main() {
    setlocale(LC_ALL, "");
    srand(time(NULL));
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
    // (未実装)
}

void buildServer() {
    GameState state;
    memset(&state, 0, sizeof(GameState));

    generateServerCode(state.server_code, sizeof(state.server_code));
    state.num_players = 1;
    state.your_player_id = 1;

    Player *p1 = &state.players[0];
    p1->id = 1;
    p1->color = C_RED;
    p1->is_active = true;
    for(int i=0; i<4; i++) {
        p1->pieces[i].id = i;
        p1->pieces[i].position = -1;
    }

    snprintf(state.message_log[0], sizeof(state.message_log[0]), "サーバーを作成しました。コード: %s", state.server_code);
    snprintf(state.message_log[1], sizeof(state.message_log[1]), "他のプレイヤーの参加を待っています...");

    showGameScreen(&state);
}

void connectToServer(const char* code) {
    clear();
    char message[100];
    snprintf(message, sizeof(message), "（未実装）コード「%s」でサーバーに接続...", code);
    mvprintw(LINES / 2, (COLS - getDisplayWidth(message)) / 2, "%s", message);
    refresh();
    sleep(2);
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

    while(1) {
        drawMenu(title, items, num_items, false, "");
        MenuSelection choice = handleInput(items, num_items, NULL, 0, stdscr, 0, 0);

        if (choice == MENU_ITEM_START) {
            showServerSelectMenu();
            return;
        } else if (choice == MENU_ITEM_RULES) {
            showRulesScreen();
            return;
        } else if (choice == MENU_ITEM_EXIT) {
            cleanupNcurses();
            exit(0);
        }
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
                items[i].x = (COLS - items[i].width) / 2;
            }
        }

        drawMenu(title, items, num_items, join_mode, server_code);
        MenuSelection choice = handleInput(items, num_items, server_code, sizeof(server_code), stdscr, 0, 0);

        if (choice == MENU_ITEM_CREATE_SERVER) {
            buildServer();
            timeout(-1);
            return;
        } else if (choice == MENU_ITEM_JOIN_SERVER) {
            if (!join_mode) {
                join_mode = true;
                curs_set(1);
            }
        } else if (choice == MENU_ITEM_SUBMIT_JOIN) {
            if (strlen(server_code) > 0) {
                connectToServer(server_code);
                timeout(-1);
                curs_set(0);
                return;
            }
        } else if (choice == MENU_ITEM_BACK) {
            timeout(-1);
            curs_set(0);
            return;
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

    while(handleInput(back_button, 1, NULL, 0, stdscr, 0, 0) != MENU_ITEM_BACK);
}

void showGameScreen(GameState *state) {
    timeout(100);
    int board_h = BOARD_H;
    int board_w = BOARD_W;
    int panel_w = 45;
    int total_w = board_w + panel_w;
    int start_x = 0; // ←左端に固定
    int start_y = (LINES - board_h) / 2;

    if (total_w > COLS) {
        // 画面幅が狭い場合はパネルを右端に配置
        start_x = COLS - total_w;
    }
    if (start_y < 0) start_y = 0;

    //WINDOW* board_win = newwin(board_h, board_w, start_y, start_x);
    WINDOW* panel_win = newwin(board_h, panel_w, start_y, start_x + board_w);
    

    while(1) {
        werase(board_win);
        werase(panel_win);

        drawBoard(board_win, state);

        // --- 右側パネルの描画 ---
        wattron(panel_win, COLOR_PAIR(C_PANEL_BG));
        box(panel_win, 0, 0);
        for(int r=1; r<board_h-1; r++){
            mvwaddch(panel_win, r, 0, ' '); // 背景色で塗りつぶし
        }
        wattroff(panel_win, COLOR_PAIR(C_PANEL_BG));

        int current_y = 1;
        mvwprintw(panel_win, current_y++, 10, "サーバーコード: %s", state->server_code);
        mvwprintw(panel_win, current_y++, 10, "あなたのユーザーID: %d", state->your_player_id);
        current_y++;

        // プレイヤー情報
        for(int i=0; i<state->num_players; i++) {
            Player *p = &state->players[i];
            char p_title[50];
            snprintf(p_title, sizeof(p_title), "Player %d (%s) %s", p->id, colorToString(p->color), (p->id == state->your_player_id) ? "(あなた)" : "");
            //drawBoxWithTitle(panel_win, current_y, 1, 4, panel_w - 2, p_title, p->id == state->your_player_id ? C_RED : C_NONE);
            mvwprintw(panel_win, current_y + 2, 10, "駒: %d/4 ゴール済み", p->pieces_at_goal);
            current_y += 4;
        }
        current_y+=2;

        // サイコロ
        char dice_text[20];
        if (state->dice_value == 0) {
            strcpy(dice_text, "-");
        } else {
            sprintf(dice_text, "%d", state->dice_value);
        }
        //drawBoxWithTitle(panel_win, current_y, 1, 5, panel_w - 2, "Dice", C_NONE);
        mvwprintw(panel_win, current_y + 2, (panel_w - getDisplayWidth(dice_text))/2, "%s", dice_text);

        // ログ
        //drawBoxWithTitle(panel_win, board_h - 15, 1, 6, panel_w - 2, "Log", C_NONE);
        for(int i=0; i<5; i++) {
            mvwprintw(panel_win, board_h - 14 + i, 2, "%s", state->message_log[i]);
        }

        // ボタン
        MenuItem buttons[] = {
            {"サイコロを振る", board_h - 8, (panel_w - 20)/2, 1, 20, MENU_ITEM_ROLL_DICE},
            {"メインメニューに戻る", board_h - 3, (panel_w-25)/2, 1, 22, MENU_ITEM_BACK}
        };
        mvwprintw(panel_win, buttons[0].y, buttons[0].x, "[ %s ]", buttons[0].text);
        mvwprintw(panel_win, buttons[1].y, buttons[1].x, "[ %s ]", buttons[1].text);

        wrefresh(board_win);
        wrefresh(panel_win);

        MenuSelection choice = handleInput(buttons, 2, NULL, 0, panel_win, start_x + board_w, start_y);
        if (choice == MENU_ITEM_ROLL_DICE) {
            state->dice_value = (rand() % 6) + 1;
        } else if (choice == MENU_ITEM_BACK) {
            break;
        }
    }
    delwin(board_win);
    delwin(panel_win);
    timeout(-1);
}

void drawBoard(WINDOW* win, GameState *state) {
    static const int board_layout[15][15] = {
        {1,1,1,1,1,1, 5,5,5, 2,2,2,2,2,2},
        {1,1,1,1,1,1, 5,2,5, 2,2,2,2,2,2},
        {1,1,1,1,1,1, 5,2,5, 2,2,2,2,2,2},
        {1,1,1,1,1,1, 5,2,5, 2,2,2,2,2,2},
        {1,1,1,1,1,1, 5,2,5, 2,2,2,2,2,2},
        {1,1,1,1,1,1, 5,2,5, 2,2,2,2,2,2},
        {5,5,5,5,5,5, 5,2,5, 5,5,5,5,5,5},
        {5,1,1,1,1,1, 6,6,6, 3,3,3,3,3,5},
        {5,5,5,5,5,5, 5,4,5, 5,5,5,5,5,5},
        {4,4,4,4,4,4, 5,4,5, 3,3,3,3,3,3},
        {4,4,4,4,4,4, 5,4,5, 3,3,3,3,3,3},
        {4,4,4,4,4,4, 5,4,5, 3,3,3,3,3,3},
        {4,4,4,4,4,4, 5,4,5, 3,3,3,3,3,3},
        {4,4,4,4,4,4, 5,4,5, 3,3,3,3,3,3},
        {4,4,4,4,4,4, 5,5,5, 3,3,3,3,3,3},
    };

    // 背景描画
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 15; c++) {
            int color = board_layout[r][c];
            wattron(win, COLOR_PAIR(color));
            mvwprintw(win, r * 2 + 1, c * 4 + 1, "    ");
            mvwprintw(win, r * 2 + 2, c * 4 + 1, "    ");
            wattroff(win, COLOR_PAIR(color));
        }
    }

    // グリッド描画
    wattron(win, COLOR_PAIR(C_GRID));
    for (int r = 0; r <= 15; r++) {
        mvwhline(win, r*2, 0, 0, BOARD_W);
    }
    for (int c = 0; c <= 15; c++) {
        mvwvline(win, 0, c*4, 0, BOARD_H);
    }
    wattroff(win, COLOR_PAIR(C_GRID));

    // 駒の描画
    for(int i=0; i < state->num_players; i++) {
        Player* p = &state->players[i];
        for(int j=0; j<4; j++) {
            if (p->pieces[j].position != 999) { // 999はゴール済み
                Point coords = getPieceCoords(&p->pieces[j], p);
                wattron(win, COLOR_PAIR(p->color));
                if (p->pieces[j].position == -1) { // ベース内
                    mvwprintw(win, coords.y + 1, coords.x + 1, PIECE_SYMBOLS[j]); // [FIX] 中央に表示
                } else { // 経路上
                    mvwprintw(win, coords.y + 1, coords.x + 1, " ● ");
                }
                wattroff(win, COLOR_PAIR(p->color));
            }
        }
    }
}


// --- ヘルパー関数群 ---
Point getPieceCoords(const Piece* piece, Player* player) {
    Point grid_p;
    static const Point base_map[4][4] = {
        {{1,1},{1,4},{4,1},{4,4}},       // Red
        {{1,10},{1,13},{4,10},{4,13}},    // Green
        {{10,10},{10,13},{13,10},{13,13}},// Yellow
        {{10,1},{10,4},{13,1},{13,4}}     // Blue
    };
    static const Point path_map[52] = {
        {6,1},{6,2},{6,3},{6,4},{6,5},{5,6},{4,6},{3,6},{2,6},{1,6},{0,6},{0,7},{0,8},
        {1,8},{2,8},{3,8},{4,8},{5,8},{6,9},{6,10},{6,11},{6,12},{6,13},{6,14},{7,14},{8,14},
        {8,13},{8,12},{8,11},{8,10},{8,9},{9,8},{10,8},{11,8},{12,8},{13,8},{14,8},{14,7},
        {14,6},{13,6},{12,6},{11,6},{10,6},{9,6},{8,5},{8,4},{8,3},{8,2},{8,1},{8,0},{7,0},{6,0}
    };
    static const Point home_map[4][6] = {
        {{7,1},{7,2},{7,3},{7,4},{7,5},{7,6}},       // Red
        {{1,7},{2,7},{3,7},{4,7},{5,7},{6,7}},       // Green
        {{7,13},{7,12},{7,11},{7,10},{7,9},{7,8}},    // Yellow
        {{13,7},{12,7},{11,7},{10,7},{9,7},{8,7}}     // Blue
    };

    if (piece->position == -1) { // ベース
        grid_p = base_map[player->color - 1][piece->id];
    } else if (piece->position >= 0 && piece->position < PATH_LENGTH) { // 通常パス
        grid_p = path_map[piece->position];
    } else if (piece->position >= 100) { // ホームストレッチ
        int home_idx = piece->position % 100 - 1;
        grid_p = home_map[player->color - 1][home_idx];
    } else {
        grid_p = (Point){-1,-1}; // エラー/ゴール済み
    }

    return (Point){ grid_p.y * 2, grid_p.x * 4 };
}

void initColors() {
    start_color();
    init_pair(C_RED,    COLOR_WHITE, COLOR_RED);
    init_pair(C_GREEN,  COLOR_WHITE, COLOR_GREEN);
    init_pair(C_YELLOW, COLOR_BLACK, COLOR_YELLOW);
    init_pair(C_BLUE,   COLOR_WHITE, COLOR_BLUE);
    init_pair(C_PATH,   COLOR_BLACK, COLOR_WHITE);
    init_pair(C_GOAL,   COLOR_BLACK, COLOR_CYAN);
    init_pair(C_GRID,   COLOR_BLACK, COLOR_BLACK);
}

MenuSelection handleInput(MenuItem items[], int num_items, char* input_buffer, int buffer_size, WINDOW* clickable_win, int x_offset, int y_offset) {
    int ch = getch();
    if (ch == ERR) {
        return MENU_ITEM_NONE;
    }

    if (ch == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK && (event.bstate & BUTTON1_CLICKED)) {
            int event_x = event.x - x_offset;
            int event_y = event.y - y_offset;
            WINDOW* target_win = (clickable_win == NULL) ? stdscr : clickable_win;

            if (wenclose(target_win, event_y, event_x)) {
                 if(clickable_win == NULL) {
                     event_x = event.x;
                     event_y = event.y;
                 }
                for (int i = 0; i < num_items; i++) {
                    if (event_y >= items[i].y && event_y < items[i].y + items[i].height &&
                        event_x >= items[i].x && event_x < items[i].x + items[i].width) {
                        return items[i].action;
                    }
                }
            }
        }
    } else if (input_buffer != NULL) {
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
            if (items[i].action == MENU_ITEM_START || items[i].action == MENU_ITEM_RULES || items[i].action == MENU_ITEM_EXIT) {
                 mvprintw(items[i].y, items[i].x, "%s", items[i].text);
            } else {
                mvprintw(items[i].y, items[i].x, "[ %s ]", items[i].text);
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

void displayFileContent(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        displayError("エラー: rule.txtが見つかりませんでした。");
        sleep(3);
        return;
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

void drawBoxWithTitle(WINDOW* win, int y, int x, int height, int width, const char* title, int color_pair_index) {
    WINDOW *sub_win = derwin(win, height, width, y, x);
    if (color_pair_index != C_NONE) {
        wbkgd(sub_win, COLOR_PAIR(color_pair_index));
    }
    box(sub_win, 0, 0);
    mvwprintw(sub_win, 0, (width - getDisplayWidth(title) - 2 ) / 2, " %s ", title);
    wrefresh(sub_win);
    delwin(sub_win);
}

void generateServerCode(char *code, size_t size) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < size - 1; i++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        code[i] = charset[key];
    }
    code[size - 1] = '\0';
}

const char* colorToString(CellColor color) {
    switch(color) {
        case C_RED:    return "red";
        case C_GREEN:  return "green";
        case C_YELLOW: return "yellow";
        case C_BLUE:   return "blue";
        default:       return "none";
    }
}

int getDisplayWidth(const char* str) {
    wchar_t wcs[256];
    int len = mbstowcs(wcs, str, 256);
    if (len < 0) {
        return strlen(str);
    }
    int width = wcswidth(wcs, len);
    return (width < 0) ? strlen(str) : width;
}

void initializeNcurses() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    initColors();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h\n"); // マウスイベントの詳細レポートを有効化
}

void cleanupNcurses() {
    printf("\033[?1003l\n"); // マウスイベントの詳細レポートを無効化
    endwin();
}