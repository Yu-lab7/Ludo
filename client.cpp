/*
 * =====================================================================================
 * FILENAME: Ludo/client.cpp
 * DESCRIPTION: Ludoゲームクライアントのメインプログラム (ncurses使用)
 * =====================================================================================
 */
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <ncurses.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>

#include "common.h"

// --- グローバル変数 ---
int sock = 0;
int my_player_id = -1;
GameState local_game_state;
std::mutex state_mutex;
bool game_running = true;

// --- UI/描画関連 ---
void initialize_ncurses() {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    start_color(); mousemask(ALL_MOUSE_EVENTS, NULL); timeout(100);
    init_pair(1, COLOR_RED, COLOR_BLACK); init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);
}

void draw_text_centered(int y, const char* text) {
    mvprintw(y, (COLS - strlen(text)) / 2, "%s", text);
}

void draw_button(int y, int x, const char* text, bool selected) {
    if (selected) attron(A_REVERSE);
    mvprintw(y, x, "[ %s ]", text);
    if (selected) attroff(A_REVERSE);
}

void draw_game_screen(const GameState& state) {
    std::lock_guard<std::mutex> guard(state_mutex);
    clear();
    box(stdscr, 0, 0);

    // 盤面描画 (簡易版)
    int board_w = COLS / 2;
    WINDOW* board_win = newwin(LINES - 2, board_w, 1, 1);
    box(board_win, 0, 0);
    mvwprintw(board_win, 1, (board_w - 10) / 2, "Ludo Board");
    // ここに詳細な盤面描画ロジックを追加...
    wrefresh(board_win);
    delwin(board_win);

    // 情報パネル描画
    int info_x = board_w + 3;
    mvprintw(2, info_x, "サーバーコード: %s", state.serverCode);
    mvprintw(4, info_x, "プレイヤー情報:");
    for (int i = 0; i < state.playerCount; ++i) {
        const Player& p = state.players[i];
        if (!p.connected) continue;
        bool is_me = p.id == my_player_id;
        bool is_turn = i == state.currentPlayerIndex;
        if(is_turn) attron(A_BOLD);
        attron(COLOR_PAIR(static_cast<int>(p.color)));
        mvprintw(6 + i, info_x, "%s %s %s", (is_turn ? "->" : "  "), p.name, (is_me ? "(あなた)" : ""));
        attroff(COLOR_PAIR(static_cast<int>(p.color)));
        if(is_turn) attroff(A_BOLD);
    }
    
    // サイコロ
    mvprintw(15, info_x, "+-------+");
    mvprintw(16, info_x, "| Dice  |");
    mvprintw(17, info_x, "|   %d   |", state.diceValue);
    mvprintw(18, info_x, "+-------+");

    // 操作説明
    mvprintw(LINES - 3, 2, "[R]oll Dice, [0-3] Move Piece, [Q]uit");
    refresh();
}

void listen_to_server() {
    ServerPacket packet;
    while (game_running && recv(sock, &packet, sizeof(ServerPacket), 0) > 0) {
        std::lock_guard<std::mutex> guard(state_mutex);
        switch(packet.type){
            case PacketType::S2C_GAME_STATE_UPDATE:
                local_game_state = packet.state;
                break;
            case PacketType::S2C_JOIN_SUCCESS:
                my_player_id = packet.state.players[0].id;
                break;
            case PacketType::S2C_ERROR_MSG:
                // ... エラーメッセージ表示 ...
                break;
            default: break;
        }
    }
    game_running = false;
}

int main() {
    initialize_ncurses();
    
    draw_text_centered(LINES/2 - 2, "Ludo Game");
    draw_button(LINES/2, (COLS - 20) / 2, "サーバーに参加する", true);
    getch(); // 何かキーを押して開始

    // サーバー接続
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        endwin();
        std::cerr << "エラー: サーバーに接続できませんでした。" << std::endl;
        return -1;
    }

    ClientPacket join_packet;
    join_packet.type = PacketType::C2S_JOIN_REQUEST;
    send(sock, &join_packet, sizeof(ClientPacket), 0);

    std::thread server_listener(listen_to_server);

    while (game_running) {
        draw_game_screen(local_game_state);
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            game_running = false;
            break;
        }

        ClientPacket action_packet;
        bool should_send = false;
        if (ch == 'r' || ch == 'R') {
           action_packet.type = PacketType::C2S_ROLL_DICE;
           should_send = true;
        } else if (ch >= '0' && ch <= '3') {
           action_packet.type = PacketType::C2S_MOVE_PIECE;
           action_packet.targetId = ch - '0';
           should_send = true;
        }
        
        if (should_send) {
            send(sock, &action_packet, sizeof(ClientPacket), 0);
        }
    }

    server_listener.join();
    close(sock);
    endwin();
    return 0;
}