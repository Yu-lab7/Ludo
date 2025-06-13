/*
 * =====================================================================================
 * FILENAME: Ludo/server.cpp
 * DESCRIPTION: Ludoゲームサーバーのメインプログラム
 * =====================================================================================
 */
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <algorithm>
#include <map>

#include "common.h"

// --- グローバル変数 ---
GameState gameState;
std::vector<int> client_sockets;
std::mutex state_mutex;
std::map<int, int> socket_to_player_index;

// --- プロトタイプ宣言 ---
void broadcast_gamestate();
void handle_client(int client_socket);
void initialize_game();
void add_player(int client_socket);
void remove_player(int client_socket);
void process_packet(int client_socket, const ClientPacket& packet);
void handle_roll_dice(int player_index);
void handle_move_piece(int player_index, int piece_id);
int get_next_player_index(int current_index);

// --- ゲームロジック実装 ---
void initialize_game() {
    srand(time(0));
    strcpy(gameState.serverCode, "LUDO1"); // 簡単なサーバーコード
    gameState.status = GameStatus::WAITING;
    gameState.hostId = 0;
    gameState.playerCount = 0;
    gameState.currentPlayerIndex = -1;
    gameState.diceValue = 0;
    gameState.diceRolled = false;
    gameState.winnerCount = 0;

    for (int i = 0; i < MAX_PLAYERS; ++i) {
        gameState.players[i].id = -1;
        gameState.players[i].color = PlayerColor::NONE;
        gameState.players[i].connected = false;
    }
}

void add_player(int client_socket) {
    std::lock_guard<std::mutex> guard(state_mutex);
    if (gameState.playerCount >= MAX_PLAYERS) {
        // ... エラー処理: 満員 ...
        close(client_socket);
        return;
    }

    int player_index = gameState.playerCount;
    socket_to_player_index[client_socket] = player_index;
    
    Player& new_player = gameState.players[player_index];
    new_player.id = client_socket;
    if (gameState.playerCount == 0) gameState.hostId = client_socket;
    sprintf(new_player.name, "Player %d", player_index + 1);
    new_player.color = static_cast<PlayerColor>(player_index + 1);
    new_player.connected = true;

    for (int i = 0; i < PIECES_PER_PLAYER; ++i) {
        new_player.pieces[i] = {i, PieceState::BASE, -1};
    }

    gameState.playerCount++;
    std::cout << "プレイヤー " << new_player.name << " が参加しました (Socket: " << client_socket << ")" << std::endl;
    
    // 参加成功を通知
    ServerPacket success_packet;
    success_packet.type = PacketType::S2C_JOIN_SUCCESS;
    success_packet.state.players[0].id = client_socket; // IDを通知
    send(client_socket, &success_packet, sizeof(ServerPacket), 0);

    // 4人揃ったら自動でゲーム開始
    if (gameState.playerCount == MAX_PLAYERS && gameState.status == GameStatus::WAITING) {
        std::cout << "4人揃ったため、ゲームを開始します。" << std::endl;
        gameState.status = GameStatus::PLAYING;
        gameState.currentPlayerIndex = 0;
    }
    broadcast_gamestate();
}

void remove_player(int client_socket) {
    std::lock_guard<std::mutex> guard(state_mutex);
    if (socket_to_player_index.find(client_socket) == socket_to_player_index.end()) return;

    int player_index = socket_to_player_index[client_socket];
    std::cout << "プレイヤー " << gameState.players[player_index].name << " が離脱しました。" << std::endl;
    gameState.players[player_index].connected = false;
    
    // もしターンプレイヤーが抜けたら、次の人へ
    if (gameState.currentPlayerIndex == player_index) {
        gameState.currentPlayerIndex = get_next_player_index(player_index);
        gameState.diceRolled = false;
        gameState.diceValue = 0;
    }
    
    // (任意)全員が抜けたらゲームをリセット
    // ...

    client_sockets.erase(std::remove(client_sockets.begin(), client_sockets.end(), client_socket), client_sockets.end());
    socket_to_player_index.erase(client_socket);
    close(client_socket);
    broadcast_gamestate();
}


void broadcast_gamestate() {
    ServerPacket packet;
    packet.type = PacketType::S2C_GAME_STATE_UPDATE;
    packet.state = gameState;
    for (int sock : client_sockets) {
        if (send(sock, &packet, sizeof(ServerPacket), 0) < 0) {
            perror("broadcast send failed");
        }
    }
}

int get_next_player_index(int current_index) {
    int attempts = 0;
    do {
        current_index = (current_index + 1) % gameState.playerCount;
        attempts++;
    } while (!gameState.players[current_index].connected && attempts < MAX_PLAYERS);
    return current_index;
}

void handle_roll_dice(int player_index) {
    if (gameState.status != GameStatus::PLAYING || gameState.currentPlayerIndex != player_index || gameState.diceRolled) {
        return;
    }
    gameState.diceValue = (rand() % 6) + 1;
    gameState.diceRolled = true;
    std::cout << gameState.players[player_index].name << " がサイコロを振り、" << gameState.diceValue << " が出ました。" << std::endl;

    // 簡易ロジック: 動かせる駒がなければターンをパス（6以外の場合）
    // 本来は駒選択を待つ
    bool can_move = false;
    for (int i = 0; i < PIECES_PER_PLAYER; ++i) {
        Piece& p = gameState.players[player_index].pieces[i];
        if ((p.state == PieceState::BASE && gameState.diceValue == 6) || p.state == PieceState::TRACK) {
            can_move = true;
            break;
        }
    }
    if (!can_move && gameState.diceValue != 6) {
        gameState.currentPlayerIndex = get_next_player_index(player_index);
        gameState.diceRolled = false;
    }
}

void handle_move_piece(int player_index, int piece_id) {
    if (gameState.status != GameStatus::PLAYING || gameState.currentPlayerIndex != player_index || !gameState.diceRolled) {
        return;
    }

    Player& player = gameState.players[player_index];
    Piece& piece = player.pieces[piece_id];

    // 簡易移動ロジック
    bool moved = false;
    if (piece.state == PieceState::BASE && gameState.diceValue == 6) {
        piece.state = PieceState::TRACK;
        piece.position = 0;
        moved = true;
    } else if (piece.state == PieceState::TRACK) {
        piece.position = (piece.position + gameState.diceValue) % BOARD_TRACK_SIZE;
        moved = true;
    }
    
    if (moved) {
        if (gameState.diceValue != 6) {
            gameState.currentPlayerIndex = get_next_player_index(player_index);
        }
        gameState.diceRolled = false;
        gameState.diceValue = 0;
    }
}

void process_packet(int client_socket, const ClientPacket& packet) {
    std::lock_guard<std::mutex> guard(state_mutex);
    if (socket_to_player_index.find(client_socket) == socket_to_player_index.end()) return;
    int player_index = socket_to_player_index[client_socket];

    switch (packet.type) {
        case PacketType::C2S_ROLL_DICE:
            handle_roll_dice(player_index);
            break;
        case PacketType::C2S_MOVE_PIECE:
            handle_move_piece(player_index, packet.targetId);
            break;
        case PacketType::C2S_FORCE_START:
            if(gameState.hostId == client_socket && gameState.status == GameStatus::WAITING && gameState.playerCount >= 2){
                gameState.status = GameStatus::PLAYING;
                gameState.currentPlayerIndex = 0;
            }
            break;
        default:
            break;
    }
    broadcast_gamestate();
}

void handle_client(int client_socket) {
    add_player(client_socket);
    ClientPacket packet;
    while (recv(client_socket, &packet, sizeof(ClientPacket), 0) > 0) {
        process_packet(client_socket, packet);
    }
    remove_player(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    initialize_game();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_PLAYERS);

    std::cout << "Ludoサーバーがポート " << PORT << " で起動しました。" << std::endl;

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) { continue; }
        
        client_sockets.push_back(new_socket);
        std::thread client_thread(handle_client, new_socket);
        client_thread.detach();
    }
    return 0;
}