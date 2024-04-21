## 目標
* 給定 transactions 和 min support (頻率)，實作演算法找出 frequent patterns
* 可使用 python3 或是 C++
* 演算法不限，Apriori、FP Growth 等皆可
* 不得使用 frequent patterns 相關的 library

## 輸入
* 輸入存有 transactions 的 txt 檔
* Item 以數字表示，範圍為 0~999
* Transactions 最多 100,000 筆
* 每筆 transaction 最多 200 個 item
* 每一行代表一筆 transaction，每筆 transaction 的 Item 之間用 “,” 區隔無空格
* 換行採用 \n (LF)，而不是 \r\n (CRLF)
* input範例: sample.txt

## 輸出
* 輸出一個 txt 檔
* 一行為一組 frequent pattern，frequent pattern後接上 ”:”，再接上support (出現頻率)
* Example: 1,2,3:0.2500
* Support 四捨五入到小數點後第4位
* 輸出的部分不需要特別排序，助教評分時會自行處理
* 範例: sample.txt (min support = 0.2) 計算出的 output

## 要求
* C++ 或 python3 擇一，程式檔名為你的學號_hw1.cpp or 你的學號_hw1.py
* 不得使用 frequent patterns 相關的 library；Python 禁止使用 apyori、pyfpgrowth 等相關的套件，若是不確定某個library/package能否使用，請在eeclass提問跟助教確認
* 在執行程式時需在後面依序輸入3個參數: min support、輸入檔名、輸出檔名
* 輸入輸出檔名請不要寫死！(無法順利執行以零分計)
* C++ 執行方式
	* Compile: g++ -std=c++2a -pthread -fopenmp -O2 -o 你的學號_hw1 你的學號_hw1.cpp
	* Run: ./你的學號_hw1 [min support] [輸入檔名] [輸出檔名]
		(windows環境下為你的學號_hw1.exe [min support] [輸入檔名] [輸出檔名])
	* Ex: ./12345_hw1 0.2 input1.txt ouput1.txt
* Python 執行方式
	* Run: python3 你的學號_hw1.py [min support] [輸入檔名] [輸出檔名] 

## 評分標準
* 共 5 筆測資，分數根據過的筆數 (一筆全對才有分) 0~5 依序為 0、60、70、80、85、100(or 90 or 95)。
* 最後一筆測資如果輸出正確的話，會根據速度給分，前 33% 快的得15分，中 33% 的得 10 分，後 33% 的得 5 分。
* https://prnt.sc/wK8BZuVfk0Oj

## 執行環境
* CPU: i7-8700k
* RAM: 32G
* OS: Ubuntu 20.04.3 LTS
* GPU: RTX 2080
* gcc vesion: 11.3.0 
* Python version: python 3.9.13 

## For Your Reference
* 設定 VSCode
	* https://code.visualstudio.com/docs/cpp/config-mingw
	* https://code.visualstudio.com/docs/cpp/config-wsl
* https://gcc.gnu.org/projects/cxx-status.html
* https://sourceforge.net/projects/mingw-w64/
* Add #include <climits> if you use INT_MAX, INT_MIN, etc.
* Add #include <cstring> if you use strcpy, strtok, etc.c

## YF's word:
這是照上課投影片刻出來的 Apriori 演算法，因為比較直觀，而且 FP Tree 看起來好難，就先求能跑再說
期中複習投影片才發現這是 ECLAT

_hw.cpp 是初版，測資 1~5 時間如下
0	0.46	2.57	7.03	40.64
pp_hw.cpp 是平行化版本，1~5:
0	0.16	0.51	1.22	6.92

平行化的概念是分派 itemset pair 給各 thread