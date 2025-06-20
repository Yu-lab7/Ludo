/**
 * ルドーゲーム - UIプロトタイプ
 *
 * このコードは、提供された仕様書に基づいてUIと画面遷移を実装しています。
 * ターミナルグラフィックスとマウス制御にはncursesライブラリを使用しています。
 *
 * 注意: ゲームロジック、ネットワーク機能、実際のゲームボードは未実装であり、
 * スタブ関数で表されています。
 *
 * コンパイル方法:
 * gcc this_file.c -o Ludo -lncurses
 *
 * 必要なファイル構造:
 * ./Ludo (実行可能ファイル)
 * ./rule/rule.txt
 */
#define _XOPEN_SOURCE 700 

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // sleep()のため
#include <locale.h> // 文字エンコーディング設定のために追加
#include <wchar.h>  // マルチバイト文字の幅計算のために追加

// --- 列挙型定義 ---

// ユーザーのメニュー選択を表します。入力処理関数から返されます。
typedef enum {
    MENU_ITEM_START,
    MENU_ITEM_RULES,
    MENU_ITEM_EXIT,
    MENU_ITEM_CREATE_SERVER,
    MENU_ITEM_JOIN_SERVER,
    MENU_ITEM_BACK,
    MENU_ITEM_NONE // 未選択または範囲外のクリック
} MenuSelection;

// --- 構造体定義 ---

// クリック可能なメニュー項目をテキストと位置で定義します。
typedef struct {
    const char *text;
    int y, x;             // 左上隅の座標
    int height, width;    // クリック可能領域のサイズ
    MenuSelection action; // この項目に関連付けられたアクション
} MenuItem;

// --- 関数プロトタイプ宣言 ---

// メインアプリケーションフロー
void run();
void shutdown_game();

// 画面表示関数
void showMainMenu();
void showServerSelectMenu();
void showRulesScreen();
void displayFileContent(const char *filepath);
void displayError(const char *message);

// 入力処理
MenuSelection handleMouseClick(MenuItem items[], int num_items);

// ヘルパー関数
void initializeNcurses();
void cleanupNcurses();
void drawMenu(const char* title, MenuItem items[], int num_items, bool show_object_box);
void drawBoxWithTitle(int y, int x, int height, int width, const char* title);
int getDisplayWidth(const char* str);


// --- スタブ関数（今後の実装用） ---
void buildServer() {
    clear();
    mvprintw(LINES / 2, (COLS - getDisplayWidth("（未実装）サーバー作成処理...")) / 2, "（未実装）サーバー作成処理...");
    refresh();
    sleep(2);
}

void joinServer() {
    clear();
    mvprintw(LINES / 2, (COLS - getDisplayWidth("（未実装）サーバー参加処理...")) / 2, "（未実装）サーバー参加処理...");
    refresh();
    sleep(2);
}


// --- メイン関数 ---

/**
 * @brief プログラムのエントリポイント。ncursesを初期化し、メインアプリケーションを開始します。
 * @return 正常終了時は0、エラー時は1。
 */
int main() {
    setlocale(LC_ALL, ""); // UTF-8文字を正しく扱うためにロケールを設定

    initializeNcurses();
    run();
    cleanupNcurses();
    return 0;
}


// --- コアアプリケーションロジック ---

/**
 * @brief アプリケーション全体の流れを制御します。
 * メインメニューのループを処理し、他の画面へ遷移します。
 */
void run() {
    while (1) {
        showMainMenu();

        
    }
}

/**
 * @brief アプリケーションのシャットダウン処理をトリガーします。
 * 現在は、将来的なクリーンアップ処理（例：ソケットのクローズ）のためのプレースホルダーです。
 */
void shutdown_game() {
    // 将来的なクリーンアップ処理をここに追加できます。
}


// --- 画面実装 ---

/**
 * @brief 仕様書で定義されたメインメニューを表示し、ユーザーの選択を処理します。
 */
void showMainMenu() {
    const char *title = "Ludo";
    MenuItem items[] = {
        {"スタート", LINES / 2 - 1, 0, 1, 0, MENU_ITEM_START},
        {"ルール",   LINES / 2 + 1, 0, 1, 0, MENU_ITEM_RULES},
        {"終了",     LINES / 2 + 3, 0, 1, 0, MENU_ITEM_EXIT}
    };
    int num_items = sizeof(items) / sizeof(items[0]);

    // [FIX] メニュー項目の幅とX座標を動的に計算
    for(int i = 0; i < num_items; i++) {
        items[i].width = getDisplayWidth(items[i].text);
        items[i].x = (COLS - items[i].width) / 2;
    }

    drawMenu(title, items, num_items, false);
    MenuSelection choice = handleMouseClick(items, num_items);

    if (choice == MENU_ITEM_START) {
        showServerSelectMenu();
    } else if (choice == MENU_ITEM_RULES) {
        showRulesScreen();
    } else if (choice == MENU_ITEM_EXIT) {
        shutdown_game();
        cleanupNcurses();
        exit(0); // ループを抜けてプログラムを終了
    }
}

/**
 * @brief サーバー選択画面（「サーバーを作成」/「サーバーに参加」）を表示します。
 */
void showServerSelectMenu() {
    const char* title = "サーバー選択";
    MenuItem items[] = {
        {"サーバーを作成", LINES / 2 - 2, 0, 1, 0, MENU_ITEM_CREATE_SERVER},
        {"サーバーに参加", LINES / 2,     0, 1, 0, MENU_ITEM_JOIN_SERVER},
        {"メインメニューに戻る", LINES / 2 + 4, 0, 1, 0, MENU_ITEM_BACK}
    };
    int num_items = sizeof(items) / sizeof(items[0]);

    // [FIX] ボタンの幅とX座標を動的に計算
    for(int i = 0; i < num_items; i++) {
        items[i].width = getDisplayWidth(items[i].text) + 4; // "[ " と " ]" の分
        items[i].x = (COLS - items[i].width) / 2;
    }

    while (1) {
        drawMenu(title, items, num_items, true);
        MenuSelection choice = handleMouseClick(items, num_items);

        if (choice == MENU_ITEM_CREATE_SERVER) {
            buildServer();
            return;
        } else if (choice == MENU_ITEM_JOIN_SERVER) {
            joinServer();
            return;
        } else if (choice == MENU_ITEM_BACK) {
            return; // メインメニューに戻る
        }
    }
}

/**
 * @brief ファイルを読み込んでルール画面を表示します。
 *

/**
 * バグ:　文章が中央で表示しない、表示した後に画面を縮小するとUIが崩れてメインメニューに遷移できなくなる(戻るボタンが消滅する)、表示した後に画面を縮小するとルールの文章が崩れる
*/
void showRulesScreen() {
    clear();
    const char *filepath = "./rule/rule.txt";
    
    displayFileContent(filepath);

    MenuItem back_button[] = {
        {"戻る", LINES - 3, 0, 1, 0, MENU_ITEM_BACK}
    };
    // [FIX] ボタンの幅とX座標を動的に計算
    back_button[0].width = getDisplayWidth(back_button[0].text) + 4;
    back_button[0].x = (COLS - back_button[0].width) / 2;
    
    mvprintw(back_button[0].y, back_button[0].x, "[ %s ]", back_button[0].text);
    refresh();

    // ユーザーが「戻る」ボタンをクリックするのを待つ
    while(handleMouseClick(back_button, 1) != MENU_ITEM_BACK);
}


// --- 入力およびUIヘルパー ---

/**
 * @brief マウスクリックを待機して処理し、どのメニュー項目が選択されたかを判断します。
 * @param items クリック可能な領域を定義するMenuItem構造体の配列。
 * @param num_items 配列内の項目の数。
 * @return クリックされた項目に対応するMenuSelection、またはMENU_ITEM_NONEを返します。
 */
MenuSelection handleMouseClick(MenuItem items[], int num_items) {
    MEVENT event;
    int ch = getch();

    if (ch == KEY_MOUSE) {
        if (getmouse(&event) == OK) {
            if (event.bstate & BUTTON1_CLICKED) {
                // 定義されたメニュー項目に対するクリックを確認
                for (int i = 0; i < num_items; i++) {
                    if (event.y >= items[i].y && event.y < items[i].y + items[i].height &&
                        event.x >= items[i].x && event.x < items[i].x + items[i].width) {
                        return items[i].action;
                    }
                }
            }
        }
    }
    return MENU_ITEM_NONE; // 関連するクリックは発生しなかった
}

/**
 * @brief タイトル付きのメニューを描画します。
 * @param title 上部に表示するタイトル。
 * @param items 描画するメニュー項目の配列。
 * @param num_items 配列内の項目の数。
 * @param show_object_box 装飾オブジェクトのボックスを表示するかどうか。
 */
void drawMenu(const char* title, MenuItem items[], int num_items, bool show_object_box) {
    clear();
    mvprintw(2, (COLS - getDisplayWidth(title)) / 2, "%s", title);
    
    if(show_object_box) {
        drawBoxWithTitle(LINES / 2 - 3, COLS / 2 + 10, 7, 20, "オブジェクト");
    }

    // メニュー項目を描画
    for (int i = 0; i < num_items; i++) {
        // メインメニューは括弧なし、それ以外は括弧あり
        if (items[i].action == MENU_ITEM_START || items[i].action == MENU_ITEM_RULES || items[i].action == MENU_ITEM_EXIT) {
            mvprintw(items[i].y, items[i].x, "%s", items[i].text);
        } else {
            mvprintw(items[i].y, items[i].x, "[ %s ]", items[i].text);
        }
    }
    refresh();
}

/**
 * @brief テキストファイルを読み込み、その内容を画面に表示します。
 * @param filepath テキストファイルへのパス。
 */
void displayFileContent(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "エラー:ファイル「%s」を見つけることができませんでした。", filepath);
        displayError(error_msg);
        sleep(3);
        return;
    }

    char line[256];
    int row = 5;
    const char *title = "== ルール ==";
    mvprintw(2, (COLS - getDisplayWidth(title)) / 2, "%s", title);

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // 改行文字を削除
        mvprintw(row++, (COLS - getDisplayWidth(line)) / 2, "%s", line);
    }
    fclose(file);
}

/**
 * @brief 画面中央に枠線付きのエラーメッセージボックスを表示します。
 * @param message 表示するエラーメッセージ。
 */
void displayError(const char *message) {
    int height = 5;
    int width = getDisplayWidth(message) + 4; 
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    drawBoxWithTitle(start_y, start_x, height, width, "エラー");
    mvprintw(start_y + 2, start_x + 2, "%s", message);
    refresh();
}

/**
 * @brief タイトル付きのシンプルなボックスを描画します。
 */
void drawBoxWithTitle(int y, int x, int height, int width, const char* title) {
    WINDOW *win = newwin(height, width, y, x);
    box(win, 0, 0);
    mvwprintw(win, 0, (width - getDisplayWidth(title)) / 2 - 1, " %s ", title);
    wrefresh(win);
    delwin(win);
}

/**
 * @brief UTF-8文字列の表示幅を計算します。
 * @param str 幅を計算する文字列。
 * @return 計算された表示幅。エラー時はstrlenの結果を返す。
 */
int getDisplayWidth(const char* str) {
    wchar_t wcs[256];
    // マルチバイト文字列をワイド文字文字列に変換
    int len = mbstowcs(wcs, str, 256);
    if (len < 0) {
        return strlen(str); // 変換エラー時はstrlenにフォールバック
    }
    // ワイド文字文字列の幅を計算
    int width = wcswidth(wcs, len);
    return (width < 0) ? strlen(str) : width;
}


// --- Ncursesの初期化とクリーンアップ ---

/**
 * @brief ncursesライブラリを初期化し、ターミナルモードを設定します。
 */
void initializeNcurses() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); // すべてのマウスイベントを有効にする
    printf("\033[?1003h\n"); // マウス移動イベントを有効にする
}

/**
 * @brief ncursesをクリーンアップし、ターミナルを通常の状態に戻します。
 */
void cleanupNcurses() {
    printf("\033[?1003l\n"); // マウス移動イベントを無効にする
    endwin();
}