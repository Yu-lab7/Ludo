# 🎲 Ludo Game in C (ncurses)

![Language](https://img.shields.io/badge/Language-C-blue.svg)
![Library](https://img.shields.io/badge/Library-ncursesw-green.svg)
![Platform](https://img.shields.io/badge/Platform-Terminal-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

C言語とncursesライブラリで実装された、クラシックなルドーボードゲームです。ターミナル上で動作する、4人対戦のオフライン専用ゲームです。

## ✨ Gameplay

(ここにゲームプレイのスクリーンショットやGIFを挿入すると、リポジトリがより魅力的になります)

## 🚀 主な機能 (Features)

-   **4人対戦**: 4人のプレイヤーによるオフライン対戦。
-   **キャプチャー**: 他のプレイヤーの駒をスタートに戻す「追い出し」機能。
-   **ホームストレッチ**: 各プレイヤー専用の最終ストレート。
-   **勝利判定**: 3人のプレイヤーがゴールした時点で順位を決定し、ゲームを終了。
-   **マウス操作**: 全ての操作はマウスのクリックで行います。

## 🛠️ 必要なもの (Requirements)

-   `gcc` などのCコンパイラ
-   `ncursesw` ライブラリ (`w` はワイド文字対応版で、日本語などの表示に必要です)
-   マウス入力とUTF-8をサポートするターミナル環境

## 🎮 実行方法 (How to Run)

1.  **リポジトリをクローン**
    ```bash
    git clone [https://github.com/Yu-lab7/Ludo.git](https://github.com/Yu-lab7/Ludo.git)
    ```

2.  **ディレクトリに移動**
    ```bash
    cd あなたのリポジトリ名
    ```

3.  **コンパイル**
    以下のコマンドでコンパイルします。`-lncursesw` を忘れないでください。
    ```bash
    gcc Ludo.c -o Ludo -lncursesw
    ```

4.  **ゲームを実行！**
    ```bash
    ./Ludo
    ```

## 🐛 デバッグモード (Debug Mode)

このゲームには、結果表示画面などを素早く確認するためのデバッグモードが組み込まれています。

-   **有効にする**: `Ludo.c` の先頭にある `#define DEBUG_MODE 1` の行で有効になります。このモードでは、ゲーム開始時に3人のプレイヤーがすでにゴールした状態からスタートします。
-   **無効にする**: `#define DEBUG_MODE 0` に変更して再コンパイルすると、通常モードでプレイできます。

## 📄 ライセンス (License)

このプロジェクトはMITライセンスの下で公開されています。

Copyright (c) 2025 [Yu-lab7]
