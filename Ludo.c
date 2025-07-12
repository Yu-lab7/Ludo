#define _XOPEN_SOURCE 700 // wcswidth関数のプロトタイプ宣言を有効にするため

/**
 * ルドーゲーム - 完成版 v1.0 (オフライン専用)
 *
 * このコードは、ルドーの主要なゲームルールを実装した安定版です。
 * オンライン機能を廃止し、4人でのオフライン対戦に特化しています。
 *
 * 主な機能:
 * - 4人でのオフライン対戦
 * - 追い出し(キャプチャー)処理
 * - ホームストレッチへの進入と移動
 * - ゴール、勝利判定、結果表示
 *
 * コンパイル方法 (重要):
 * gcc Ludo.c -o Ludo -lncursesw
 * 
 * clickedの座標がずれている
 * ゴール後も動かせないが判定されてしまうので修正
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

// --- 定数定義 ---
#define BOARD_H 31
#define BOARD_W 65 // 65
#define PATH_LENGTH 52
#define HOME_STRETCH_LENGTH 6
#define GOAL_POSITION 999
#define DEBUG_MODE 1

// --- 列挙型定義 ---
typedef enum {
    MENU_ITEM_START_GAME, MENU_ITEM_RULES, MENU_ITEM_EXIT, MENU_ITEM_BACK,
    MENU_ITEM_ROLL_DICE, BOARD_CLICK, MENU_ITEM_NONE
} MenuSelection;

typedef enum {
    C_NONE, C_RED, C_GREEN, C_YELLOW, C_BLUE,
    C_PATH, C_GOAL, C_GRID, C_PANEL_BG
} CellColor;

// --- 構造体定義 ---
typedef struct {
    const char *text;
    int y, x, height, width;
    MenuSelection action;
} MenuItem;

typedef struct { int y, x; } Point;

typedef struct {
    int id;
    int position;
    bool is_movable;
} Piece;

typedef struct {
    int id;
    CellColor color;
    Piece pieces[4];
    int pieces_at_goal;
    bool is_active;
    int rank;
} Player;

typedef enum {
    STATE_ROLLING, STATE_MOVING_PIECE, STATE_GAME_OVER
} TurnPhase;

typedef struct {
    Player players[4];
    int num_players;
    int current_turn_idx;
    int dice_value;
    char message_log[5][100];
    TurnPhase phase;
    int roll_count;
    int finished_players_count;
} GameState;

// --- グローバル盤面データ ---
const char* PIECE_SYMBOLS[] = {"1", "2", "3", "4"};

// --- グローバル変数 ---
MEVENT g_last_event;

// --- 関数プロトタイプ宣言 ---
void run();
void startGame();
void showMainMenu();
void showRulesScreen();
void showGameScreen(GameState *state);
void showResultScreen(GameState *state);
void drawBoard(GameState *state, int base_y, int base_x);
void drawMenu(const char* title, MenuItem items[], int num_items);
void drawInputField(int y, int x, int width, const char* label, const char* content);
void displayFileContent(const char *filepath);
void displayError(const char *message);
void initColors();
Point getGridCoords(const Piece* piece, Player* player);
MenuSelection handleInput(MenuItem items[], int num_items,
                        int panel_x, int panel_y, int panel_h, int panel_w,
                        int board_x, int board_y, int board_h, int board_w);
void handlePieceMove(GameState *state, Piece *clicked_piece);
void initializeNcurses();
void cleanupNcurses();
void addLog(GameState *state, const char* message);
void nextTurn(GameState *state);
int getHomeEntryPos(Player *p);
int getAbsolutePos(int relative_pos, Player *player);
void drawBox(int y, int x, int h, int w);
const char* colorToString(CellColor color);
int getDisplayWidth(const char* str);
void shutdown();

// --- メイン関数 ---
int main() {
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

void startGame() {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    state.num_players = 4;
    state.current_turn_idx = 0;
    state.phase = STATE_ROLLING;

    CellColor colors[] = {C_RED, C_GREEN, C_YELLOW, C_BLUE};
    for (int i = 0; i < state.num_players; i++) {
        Player *p = &state.players[i];
        p->id = i + 1;
        p->color = colors[i];
        p->is_active = true;
        for(int j = 0; j < 4; j++) {
            p->pieces[j].id = j;
            p->pieces[j].position = -1;
            p->pieces[j].is_movable = false;
        }
    }

     // ▼▼▼【このブロックを丸ごと置き換える】▼▼▼
    #if DEBUG_MODE
    // --- デバッグモード：3人がゴールした状態で開始 ---
    
    // Player 1, 2, 3 をゴール済みの状態に設定します。
    for (int i = 0; i < 3; i++) {
        Player *p = &state.players[i];
        p->pieces_at_goal = 4;
        p->rank = i + 1; // 順位を1位、2位、3位に設定
        for (int j = 0; j < 4; j++) {
            p->pieces[j].position = GOAL_POSITION;
        }
    }

    // ゴールしたプレイヤーの合計数を3に設定
    state.finished_players_count = 3;

    // ゲーム状態を「ゲームオーバー」に設定
    state.phase = STATE_GAME_OVER;

    // ログにデバッグモードであることを表示
    addLog(&state, "!!! DEBUG: 3 players finished. !!!");
    #endif
    // ▲▲▲【ここまで置き換え】▲▲▲

    addLog(&state, "ゲームへようこそ！");
    addLog(&state, "Player 1 (Red) のターンです。");
    showGameScreen(&state);
}

// --- 画面実装 ---
void showMainMenu() {
    timeout(-1);
    clear();
    const char *title = "Ludo Game";
    MenuItem items[] = {
        {"ゲームを開始", LINES / 2 - 2, 0, 1, 0, MENU_ITEM_START_GAME},
        {"ルール",     LINES / 2,     0, 1, 0, MENU_ITEM_RULES},
        {"終了",       LINES / 2 + 2, 0, 1, 0, MENU_ITEM_EXIT}
    };
    int num_items = sizeof(items) / sizeof(items[0]);
    for(int i = 0; i < num_items; i++) {
        items[i].width = getDisplayWidth(items[i].text) + 4;
        items[i].x = (COLS - items[i].width) / 2;
    }

    while(1) {
        drawMenu(title, items, num_items);
        MenuSelection choice = handleInput(items, num_items, 0,0,0,0, 0,0,0,0);
        if (choice == MENU_ITEM_START_GAME) { startGame(); return; }
        if (choice == MENU_ITEM_RULES) { showRulesScreen(); return; }
        if (choice == MENU_ITEM_EXIT) { shutdown(); }
    }
}

void showRulesScreen() {
    timeout(-1);
    clear();
    displayFileContent("./rule/rule.txt");
    MenuItem back_button[] = {{"戻る", LINES - 3, 0, 1, 0, MENU_ITEM_BACK}};
    back_button[0].width = getDisplayWidth(back_button[0].text) + 4;
    back_button[0].x = (COLS - back_button[0].width) / 2;
    drawMenu("", back_button, 1);
    while(handleInput(back_button, 1, 0,0,0,0, 0,0,0,0) != MENU_ITEM_BACK);
}

void showGameScreen(GameState *state) {
    timeout(100);
    int board_h = BOARD_H, board_w = BOARD_W, panel_w = 45;
    int total_w = board_w + panel_w;
    int start_y = (LINES - board_h) / 2;
    int start_x = (COLS - total_w) / 2;
    int panel_start_x = start_x + board_w;

    while(1) {
        if (state->phase == STATE_GAME_OVER) {
            showResultScreen(state);
            return;
        }
        Player* current_player = &state->players[state->current_turn_idx];
        erase();
        drawBoard(state, start_y, start_x);
        //drawBox(start_y, panel_start_x, board_h, panel_w);

        redrawwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();

        int current_y = 4;
        attron(A_BOLD);
        mvprintw(start_y+8, panel_start_x + 22, "現在のターン: Player %d (%s)", current_player->id, colorToString(current_player->color));
        attroff(A_BOLD);
        current_y += 2;

        /* char dice_text[20];
         if (state->dice_value == 0) { strcpy(dice_text, "..."); }
         else { sprintf(dice_text, "%d", state->dice_value); }
         mvprintw(start_y + current_y + 6, panel_start_x - 22, "サイコロの目: ");*/
        current_y += 3;

        mvprintw(start_y + current_y++ + 6, panel_start_x + 22, "--- Log ---");
        for(int i=0; i<5; i++) { mvprintw(start_y + current_y + i + 6, panel_start_x + 22, ">> %s", state->message_log[i]); }

        MenuItem buttons[1];
        int num_buttons = 0;
        if (state->phase == STATE_ROLLING) {
            buttons[num_buttons++] = (MenuItem){"サイコロを振る", 20, 5, 1, 22, MENU_ITEM_ROLL_DICE};
        }

        drawMenu("", buttons, num_buttons);

        refresh();

        MenuSelection choice = handleInput(buttons, num_buttons, panel_start_x, start_y, board_h, panel_w, start_x, start_y, board_h, board_w);

        if (choice == MENU_ITEM_ROLL_DICE) {
            if (state->phase == STATE_ROLLING) {
                state->dice_value = (rand() % 6) + 1;
                char log_msg[50];
                sprintf(log_msg, "サイコロを振り、%dが出ました。", state->dice_value);
                addLog(state, log_msg);
                bool can_move = false;
                for (int i = 0; i < 4; i++) {
                    Piece *p = &current_player->pieces[i];
                    p->is_movable = false;
                    if (p->position == GOAL_POSITION) continue;
                    if ((p->position == -1 && state->dice_value == 6) ||
                        (p->position >= 100 && (p->position % 100 + state->dice_value <= HOME_STRETCH_LENGTH)) ||
                        (p->position >= 0 && p->position < PATH_LENGTH)) {
                        p->is_movable = true;
                        can_move = true;
                    }
                }
                if (can_move) {
                    addLog(state, "動かす駒をクリックしてください。");
                    state->phase = STATE_MOVING_PIECE;
                } else {
                    addLog(state, "動かせる駒がありません。");
                    sleep(1);
                    nextTurn(state);
                }
            }
        } else if (choice == BOARD_CLICK) {
            if (state->phase == STATE_MOVING_PIECE) {
                MEVENT event = g_last_event;
                int clicked_grid_y = (event.y - start_y - 1 ) / 2;
                int clicked_grid_x = (event.x - start_x - 1) / 5;
                FILE *fp = fopen("debug3.log", "a");
                if(fp){
                    fprintf(fp,"event.y: %d, event.x: %d\n", event.y, event.x);
                    fprintf(fp,"start_y: %d, start_x: %d\n", start_y, start_x);
                    fprintf(fp, "Clicked grid: (%d, %d)\n", clicked_grid_y, clicked_grid_x);
                }
                for (int i = 0; i < 4; i++) {
                    Piece *p = &current_player->pieces[i];
                    if (!p->is_movable) continue;
                    Point grid_coords = getGridCoords(p, current_player);
                    int click_y = event.y;
                    int click_x = event.x;
                    if (fp) {
                        fprintf(fp, "Piece %d grid: (%d, %d)\n", i, grid_coords.y, grid_coords.x);
                    }
                    if ((clicked_grid_x <= grid_coords.x + 1.8 && clicked_grid_x >= grid_coords.x - 1.8) && clicked_grid_y == grid_coords.y) {
                        if (fp) {
                        // 駒クリック判定
                        handlePieceMove(state, p);
                        if (fp) fprintf(fp, "Piece %d clicked!\n", i);
                        fclose(fp);
                        break;
                        }
                    }
                }

                if (fp) fclose(fp);
            }
        }
    }
}

void showResultScreen(GameState *state) {
    clear();
    const char* title = "== ゲーム終了 ==";
    mvprintw(LINES / 4, (COLS - getDisplayWidth(title)) / 2, "%s", title);
    for (int i=0; i < state->num_players; i++) {
        for (int j=0; j < state->num_players; j++) {
            if (state->players[j].rank == i + 1) {
                mvprintw(LINES / 2 - 2 + i, (COLS - 30) / 2 + 4, "%d位: Player %d (%s)",
                        state->players[j].rank, state->players[j].id, colorToString(state->players[j].color));
                break;
            }
        }
    }
    mvprintw(LINES - 5, (COLS - 30) / 2, "10秒後にメインメニューに戻ります...");
    refresh();
    sleep(10);
}

void handlePieceMove(GameState *state, Piece *clicked_piece) {
    Player* current_player = &state->players[state->current_turn_idx];
    int dice = state->dice_value;

    if (clicked_piece->position == -1) { clicked_piece->position = 0; }
    else if (clicked_piece->position >= 100) {
        int home_pos = clicked_piece->position % 100;
        if (home_pos + dice == HOME_STRETCH_LENGTH) {
            clicked_piece->position = GOAL_POSITION;
            current_player->pieces_at_goal++;
            addLog(state, "駒がゴールしました！");
            if (current_player->pieces_at_goal == 4) {
                current_player->rank = ++state->finished_players_count;
                addLog(state, "全駒がゴール！");
                if (state->finished_players_count >= state->num_players - 1) {
                    state->phase = STATE_GAME_OVER;
                }
            }
        } else { clicked_piece->position += dice; }
    }
    else {
        int home_entry = getHomeEntryPos(current_player);
        if (clicked_piece->position <= home_entry && clicked_piece->position + dice > home_entry) {
            int to_home = home_entry - clicked_piece->position + 1;
            int remaining_move = dice - to_home;
            if (remaining_move < HOME_STRETCH_LENGTH) {
                clicked_piece->position = 100 + remaining_move;
            }
        } else {
            clicked_piece->position = (clicked_piece->position + dice) % PATH_LENGTH;
        }
    }

    if (clicked_piece->position < 100) {
        int target_abs_pos = getAbsolutePos(clicked_piece->position, current_player);
        for (int i = 0; i < state->num_players; i++) {
            if (i == state->current_turn_idx) continue;
            Player* other_player = &state->players[i];
            for (int j = 0; j < 4; j++) {
                Piece* other_piece = &other_player->pieces[j];
                if (other_piece->position < 0 || other_piece->position >= 100) continue;
                if (target_abs_pos == getAbsolutePos(other_piece->position, other_player)) {
                    other_piece->position = -1;
                    char log_msg[100];
                    sprintf(log_msg, "Player %d の駒をベースに戻した！", other_player->id);
                    addLog(state, log_msg);
                }
            }
        }
    }
    nextTurn(state);
}

void drawBoard(GameState *state, int base_y, int base_x) {
    static const int board_layout[15][15] = {
        {1,1,1,1,1,1, 5,5,5, 2,2,2,2,2,2},
        {1,1,0,0,1,1, 5,2,5, 2,2,0,0,2,2},
        {1,0,1,1,0,1, 5,2,5, 2,0,2,2,0,2},
        {1,0,1,1,0,1, 5,2,5, 2,0,2,2,0,2},
        {1,1,0,0,1,1, 5,2,5, 2,2,0,0,2,2},
        {1,1,1,1,1,1, 5,2,5, 2,2,2,2,2,2},
        {5,5,5,5,5,5, 5,6,5, 5,5,5,5,5,5},
        {5,1,1,1,1,1, 6,6,6, 3,3,3,3,3,5},
        {5,5,5,5,5,5, 5,6,5, 5,5,5,5,5,5},
        {4,4,4,4,4,4, 5,4,5, 3,3,3,3,3,3},
        {4,4,0,0,4,4, 5,4,5, 3,3,0,0,3,3},
        {4,0,4,4,0,4, 5,4,5, 3,0,3,3,0,3},
        {4,0,4,4,0,4, 5,4,5, 3,0,3,3,0,3},
        {4,4,0,0,4,4, 5,4,5, 3,3,0,0,3,3},
        {4,4,4,4,4,4, 5,5,5, 3,3,3,3,3,3},
    };
    for (int r=0; r<15; r++) for (int c=0; c<15; c++) {
        int color = board_layout[r][c];
        if (color == 0) {
            if (r<7 && c<7) color=1; else if (r<7 && c>7) color=2;
            else if (r>7 && c>7) color=3; else color=4;
        }
        attron(COLOR_PAIR(color));
        mvprintw(base_y+r*2+1, base_x+c*4+1, "    ");
        mvprintw(base_y+r*2+2, base_x+c*4+1, "    ");
        attroff(COLOR_PAIR(color));
    }
    attron(COLOR_PAIR(C_GRID));
    for (int r=0; r<=15; r++) { mvhline(base_y+r*2, base_x, 0, BOARD_W); }
    for (int c=0; c<=15; c++) { mvvline(base_y, base_x+c*4, 0, BOARD_H); }
    attroff(COLOR_PAIR(C_GRID));

    for(int i=0; i<state->num_players; i++) {
        Player* p = &state->players[i];
        for(int j=0; j<4; j++) {
            if (p->pieces[j].position != GOAL_POSITION) {
                Point grid_coords = getGridCoords(&p->pieces[j], p);
                int piece_y = base_y + grid_coords.y*2 + 1;
                int piece_x = base_x + grid_coords.x*4 + 1;
                // デバッグ用ログ
                //FILE *fp = fopen("debug.log", "a");
                //if (fp) {
                  //  fprintf(fp, "Player %d Piece %d at (%d, %d)\n", p->id, j, piece_y, piece_x);
                    //fclose(fp);
                //}
                //if(p->pieces[j].is_movable) { attron(A_BOLD); }
                attron(COLOR_PAIR(p->color));
                const char* symbol = (p->pieces[j].position == -1) ? PIECE_SYMBOLS[j] : "P";
                mvprintw(piece_y, piece_x+1, "%s", symbol);
                attroff(COLOR_PAIR(p->color));
                //if(p->pieces[j].is_movable) { attroff(A_BOLD); }
            }
        }
    }
}

// --- ヘルパー関数群 ---
void nextTurn(GameState *state) {
    Player *p = &state->players[state->current_turn_idx];
    for (int i=0; i<4; i++) { p->pieces[i].is_movable = false; }
    if (state->dice_value != 6 || state->roll_count >= 2) {
        do {
            state->current_turn_idx = (state->current_turn_idx + 1) % state->num_players;
        } while (state->players[state->current_turn_idx].pieces_at_goal == 4);
        state->roll_count = 0;
        char log_msg[50];
        sprintf(log_msg, "Player %d (%s) のターンです。", state->players[state->current_turn_idx].id, colorToString(state->players[state->current_turn_idx].color));
        addLog(state, log_msg);
    } else {
        state->roll_count++;
        addLog(state, "もう一度サイコロを振ってください。");
    }
    state->dice_value = 0;
    state->phase = STATE_ROLLING;
}

void addLog(GameState *state, const char* message) {
    for(int i=0; i<4; i++) { strcpy(state->message_log[i], state->message_log[i+1]); }
    snprintf(state->message_log[4], 100, "%s", message);
}

Point getGridCoords(const Piece* piece, Player* player) {
    static const Point base_map[4][4] = {
        {{1,1},{1,4},{4,1},{4,4}}, {{1,10},{1,13},{4,10},{4,13}},
        {{10,10},{10,13},{13,10},{13,13}}, {{10,1},{10,4},{13,1},{13,4}}
    };
    static const Point path_map[52] = {
        {6,1},{6,2},{6,3},{6,4},{6,5}, {5,6},{4,6},{3,6},{2,6},{1,6},{0,6},
        {0,7}, {0,8},{1,8},{2,8},{3,8},{4,8},{5,8}, {6,9},{6,10},{6,11},{6,12},{6,13},{6,14},
        {7,14}, {8,14},{8,13},{8,12},{8,11},{8,10},{8,9}, {9,8},{10,8},{11,8},{12,8},{13,8},{14,8},
        {14,7}, {14,6},{13,6},{12,6},{11,6},{10,6},{9,6}, {8,5},{8,4},{8,3},{8,2},{8,1},{8,0},
        {7,0}, {6,0}
    };
    static const Point home_map[4][6] = {
        {{7,1},{7,2},{7,3},{7,4},{7,5},{7,6}}, {{1,7},{2,7},{3,7},{4,7},{5,7},{6,7}},
        {{7,13},{7,12},{7,11},{7,10},{7,9},{7,8}}, {{13,7},{12,7},{11,7},{10,7},{9,7},{8,7}}
    };
    if (piece->position == -1) { return base_map[player->color-1][piece->id]; }
    if (piece->position >= 100) { return home_map[player->color-1][piece->position % 100]; }
    if (piece->position >= 0) { return path_map[getAbsolutePos(piece->position, player)]; }
    return (Point){-1,-1};
}

int getHomeEntryPos(Player *p) {
    switch (p->color) {
        case C_RED:    return 50; case C_GREEN:  return 11;
        case C_YELLOW: return 24; case C_BLUE:   return 37;
        default:       return -1;
    }
}

int getAbsolutePos(int relative_pos, Player *player) {
    int start_pos = 0;
    switch(player->color){
        case C_GREEN: start_pos = 13; break;
        case C_YELLOW: start_pos = 26; break;
        case C_BLUE: start_pos = 39; break;
        default: break;
    }
    return (relative_pos + start_pos) % PATH_LENGTH;
}

void initColors() {
    start_color(); use_default_colors();
    init_pair(C_RED, COLOR_WHITE, COLOR_RED); init_pair(C_GREEN, COLOR_WHITE, COLOR_GREEN);
    init_pair(C_YELLOW, COLOR_WHITE, COLOR_YELLOW); init_pair(C_BLUE, COLOR_WHITE, COLOR_BLUE);
    init_pair(C_PATH, COLOR_BLACK, COLOR_WHITE); init_pair(C_GOAL, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(C_GRID, COLOR_BLACK, -1); init_pair(C_PANEL_BG, COLOR_WHITE, COLOR_BLACK);
}

MenuSelection handleInput(MenuItem items[], int num_items,
                        int panel_x, int panel_y, int panel_h, int panel_w,
                        int board_x, int board_y, int board_h, int board_w) {
    int ch = getch();
    if (ch == ERR) { return MENU_ITEM_NONE; }
    if (ch == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK && (event.bstate & BUTTON1_CLICKED)) {
            bool in_panel = (event.y>=panel_y && event.y<panel_y+panel_h && event.x>=panel_x && event.x<panel_x+panel_w);
            bool in_board = (event.y>=board_y && event.y<board_y+board_h && event.x>=board_x && event.x<board_x+board_w);
            if (in_panel) {
                for (int i=0; i<num_items; i++) {
                    if (event.y == items[i].y+panel_y && event.x >= items[i].x+panel_x && event.x < items[i].x+panel_x+items[i].width) return items[i].action;
                }
            } else if (in_board) {
                g_last_event = event;
                return BOARD_CLICK;
            } else {
                for (int i = 0; i < num_items; i++) {
                    if (event.y == items[i].y && event.x >= items[i].x && event.x < items[i].x + items[i].width){
                        return items[i].action;
                    }
                }
            }
        }
    }
    return MENU_ITEM_NONE;
}

void drawMenu(const char* title, MenuItem items[], int num_items) {
    if(strlen(title) > 0) {
        attron(A_BOLD);
        mvprintw(LINES / 4, (COLS - getDisplayWidth(title)) / 2, "%s", title);
        attroff(A_BOLD);
    }
    for (int i=0; i<num_items; i++) {
        mvprintw(items[i].y, items[i].x, "[ %s ]", items[i].text);
    }
    refresh();
}

void displayFileContent(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) { displayError("エラー: rule.txtが見つかりませんでした。"); return; }
    char line[256]; int row = 18;
    const char *title = "== ルール ==";
    mvprintw(2, (COLS - getDisplayWidth(title)) / 2, "%s", title);

    int line_num = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if(line_num == 0){
            mvprintw(row, (COLS - getDisplayWidth(line)) / 2 + 15, "%s", line);
        } else if(line_num == 1){
            mvprintw(row, (COLS - getDisplayWidth(line)) / 2 + 10, "%s", line);
        } else if(line_num == 2){
            mvprintw(row, (COLS - getDisplayWidth(line)) / 2 + 15, "%s", line);
        } else if(line_num == 3){
            mvprintw(row, (COLS - getDisplayWidth(line)) / 2 + 15, "%s", line); 
        } else if(line_num == 4){
            mvprintw(row, (COLS - getDisplayWidth(line)) / 2 + 2, "%s", line);
        } 

        row++;
        line_num++;
    }
    fclose(file);
}

void displayError(const char *message) {
    int height = 5, width = getDisplayWidth(message) + 6;
    int start_y = (LINES - height) / 2, start_x = (COLS - width) / 2;
    WINDOW *win = newwin(height, width, start_y, start_x);
    box(win, 0, 0);
    wattron(win, A_BOLD);
    mvwprintw(win, 1, (width - strlen("エラー")) / 2, "エラー");
    wattroff(win, A_BOLD);
    mvwprintw(win, 2, 3, "%s", message);
    wrefresh(win); sleep(3); delwin(win);
}

const char* colorToString(CellColor color) {
    switch(color) { case C_RED:return"Red"; case C_GREEN:return"Green"; case C_YELLOW:return"Yellow"; case C_BLUE:return"Blue"; default:return"None"; }
}

int getDisplayWidth(const char* str) {
    wchar_t wcs[256]; mbstowcs(wcs, str, 256);
    int width = wcswidth(wcs, 256);
    return (width < 0) ? strlen(str) : width;
}

void initializeNcurses() {
    setlocale(LC_ALL, ""); initscr(); cbreak(); noecho(); curs_set(0);
    keypad(stdscr, TRUE);
    if (has_colors()) { start_color(); initColors(); }
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h\n");
}

void cleanupNcurses() {
    printf("\033[?1003l\n");
    endwin();
}

void drawBox(int y, int x, int h, int w) {
    mvhline(y, x + 1, ACS_HLINE, w - 2);
    mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
    mvvline(y + 1, x, ACS_VLINE, h - 2);
    mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
}

void shutdown() {
    endwin();
    exit(0);
}
