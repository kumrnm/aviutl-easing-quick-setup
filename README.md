# イージング設定時短プラグイン

イージング設定時短プラグイン（easing_quick_setup.auf）は、AviUtl 拡張編集のイージング設定を補助する AviUtl プラグインです。

## 機能

トラックバー変化方法の選択時、以下の動作を行います。

- パラメーター付きのイージングが選択された場合、パラメーター設定ダイアログをその場で表示します。
- マウスカーソルをトラックバーのボタン上に移動します。（既定では無効）

## 対応バージョン

拡張編集 version 0.92 または 0.93rc1

## 導入方法

[Releaseページ](https://github.com/kumrnm/aviutl-easing-quick-setup/releases) の Assets 欄から auf ファイルをダウンロードし、aviutl.exe があるフォルダ（またはその直下の plugins フォルダ）に配置します。

## 設定

aviutl.ini から以下の項目を変更できます。
- `auto_popup` : 設定ダイアログ自動表示（有効: `1`&nbsp;&nbsp;無効: `0`）
- `move_cursor` : カーソル自動移動（有効: `1`&nbsp;&nbsp;無効: `0`）

## 注意

- 本プラグインを使用することで生じたいかなる損害についても、作者は一切の責任を負いかねます。

## バグ報告

[GitHub Issues](https://github.com/kumrnm/aviutl-easing-quick-setup/issues) または [作者のTwitter](https://twitter.com/kumrnm) にお寄せください。

## 開発者向け

- 本リポジトリは Visual Studio 2022 プロジェクトです。

- デバッグ環境の AviUtl ディレクトリがユーザーマクロ `AVIUTLDIR` に設定されています（既定：`C:\aviutl\`）。必要に応じて変更してください。
