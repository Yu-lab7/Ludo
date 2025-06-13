/*
*
* FILENAME: Ludo/common.h
* DESCRIPTION: サーバーとクライアントで共有されるデータ構造と定数を定義するヘッダーファイル
* Made by: Wilhelm Heinz 
* Date: 2025/06/13
* Version: 1.0
* LICENSE: This software is released under the MIT License.
* https://opensource.org/license/mit/
*
*/
#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <cstdint>

//定数定義
#define PORT 8080
#define MAX_PLAYERS 4
#define PIECES_PER_PLAYER 4
#define BOARD_TRACK_SIZE 52
#define BOARD_STRETCH_SIZE 6

//enum定義

//ゲームの状態
enum class GameState {
    WAITING,
    PLAYING,
    FINISHED
};

//駒の状態
enum class PieceState {
    NONE,
    TRACK,
    HOME_STRETCH,
    FINISHED
};

//プレイヤーの色
enum class PlayerColor {
    NONE,
    RED,
    GREEN,
    YELLOW,
    BLUE
};

// --- データ構造定義 ---

// 駒の構造体
struct Piece {
    int id;
    PieceState state;
    int position; // 経路上のインデックス
};

// プレイヤーの構造体
struct Player {
    int id; // ネットワーク上の識別子（ソケットディスクリプタ）
    char name[32];
    PlayerColor color;
    Piece pieces[PIECES_PER_PLAYER];
    bool connected;
};

// ゲーム全体の状況を保持する構造体
struct GameState {
    GameStatus status;
    char serverCode[10];
    int hostId;
    int playerCount;
    Player players[MAX_PLAYERS];
    int currentPlayerIndex;
    int diceValue;
    bool diceRolled;
    int winnerCount;
    int winnerRanks[MAX_PLAYERS]; // プレイヤーIDを順位順に格納
};

// --- ネットワーク通信プロトコル ---

// パケット種別
enum class PacketType : uint8_t {
    // クライアント -> サーバー
    C2S_JOIN_REQUEST,
    C2S_ROLL_DICE,
    C2S_MOVE_PIECE,
    C2S_FORCE_START,
    // サーバー -> クライアント
    S2C_GAME_STATE_UPDATE,
    S2C_JOIN_SUCCESS,
    S2C_ERROR_MSG
};

// クライアントからサーバーへのパケット
struct ClientPacket {
    PacketType type;
    int targetId; // MOVE_PIECEの対象となる駒IDなど
};

// サーバーからクライアントへのパケット
struct ServerPacket {
    PacketType type;
    GameState state; // 主要なデータとして常にGameState全体を同期する
    char message[256];
};

#endif // COMMON_H




