# sock2wav
rtl_fmで受けたAM波（航空無線を想定)をsocketで受信し、1Mbye毎にwavファイルに変換するサンプル

SDRドングルを刺したラズパイで
rtl_fm -s 32000 -f 128.4M -M am |socat -u - TCP-LISTEN:8081
等で送信。
本プログラムでソケットを受け、.wavファイルに変換する。
1MByte受信する毎に、連番で新しいwavファイルが作成される。
ATC波の24時間記録システムの実験で作成した。

サーバー側(送信側)のIPアドレス、ポート等はハードコーディングしているので、適宜コードを編集するなり、
コマンドラインオプションで指定するように書き換えるなりするのが宜し。

## 起動方法
$ sock2wav [-i receiver IP address] [-P receiver port] [-p output_path] [-s sampling frequency(Hz)] [-S file split size(kByte)] output_fllename

$ sock2wav -i 192.168.0.10 -P 8080 -p /home/testuser/wavedat -s 32000 -S 1000 wavefile
上記の場合
レシーバーのIP 192.168.0.10
使用ポート 8080
出力ファイルパス /home/testuser/wavedat
サンプリング周波数 32k
ファイル分割 1MByte
出力ファイル名 wave(連番が付加される).wav




