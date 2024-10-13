# BonDriver_LinuxMirakc
Linux の EDCB で mirakc に接続するための BonDriver です。Mirakurunでは接続テストしていないので動くかわかりません。


以下をベースにLinuxで動くように移植しました。作者さまに感謝いたします。

・[BonDriver_mirakc](https://github.com/tkmsst/BonDriver_mirakc)

・[BonDriver_LinuxPTX](https://github.com/nns779/BonDriver_LinuxPTX)


## 設定ファイルについて

基本的には[BonDriver_mirakc](https://github.com/tkmsst/BonDriver_mirakc)のままですが、Unixドメインソケットに対応しています。
同サーバ内であれば利用できます。

以下のように設定ください。
```
SERVER_TYPE="unix"
SERVER_SOCKPATH="/var/run/mirakc.sock"
```

## ビルド方法

JSONの解析のために picojson が必要です。
include/picojson 配下に picojson のgitのツリーそのまま入れてください。

clone で行う方はする方は
> $ git clone "URL" ***--recurse-submodules***

とすることで picojson 含めてcloneされます。

ビルドはmakeコマンドにて実施してください。コンパイラは g++ です。
> $ make

## License
This software is released under the MIT License, see LICENSE.
