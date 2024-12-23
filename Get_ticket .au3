#include <GUIConstantsEx.au3>
#include <MsgBoxConstants.au3>

; 創建GUI窗口
GUICreate("Time Display", 300, 100)
$label = GUICtrlCreateLabel("時間: ", 10, 10, 240, 30)
GUICtrlSetFont($label, 20)

; 顯示窗口
GUISetState(@SW_SHOW)


;Sleep(5000)  ; 延遲1秒後重試
;PressButton()   ; 執行按鈕點擊函數

;Exit

While 1
	$msg = GUIGetMsg()
	If $msg = $GUI_EVENT_CLOSE Then ExitLoop

	; 取得當前時間
	Local $hour = @HOUR, $min = @MIN, $sec = @SEC
	GUICtrlSetData($label, "目前時間: " & $hour & ":" & $min & ":" & $sec)
	;  ConsoleWrite("Current_Time: " & $hour & ":" & $min & ":" & $sec & @CRLF)

	; 檢查是否時間為12:00:00
	If $hour = 12 And $min = 00 And $sec = 0 Then

		PressButton()  ; 執行按鈕點擊函數

	;	MsgBox(0, "提醒", "時間到！已執行按鈕點擊")
		Break
	EndIf

	; 每毫秒更新一次
	;  Sleep(0)
WEnd

GUIDelete()

; 自動點擊按鈕的函數
Func PressButton()
	Local $TimeInput = 1 ; 用戶定義的延遲秒數
	Local $delays = $TimeInput * 900 ; 1秒延遲（以毫秒為單位）

	; 取得應用程式視窗句柄
	Local $hWnd = WinGetHandle("周杰倫 嘉年華 世界巡迴演唱會 @ 臺北大巨蛋 | 2024/12/05 (四) ~ 2024/12/08 (日) | tixcraft拓元售票 - Google Chrome")
;	Local $hWnd = WinGetHandle("motorola呈獻 周湯豪REALIVE (R2) 特仕版 @ 台北小巨蛋 | 2024/11/23 (六) ~ 2024/11/24 (日) | tixcraft拓元售票 - Google Chrome")

	; 檢查是否成功取得句柄
	If @error Then
		MsgBox($MB_SYSTEMMODAL, "錯誤", "找不到視窗，3秒後重試")
		Sleep($delays) ; 延遲1秒後重試
		Return
	EndIf

	; 激活應用程式窗口
	WinActivate($hWnd)

	; 點擊指定的按鈕（模擬按鈕按下）
	; ControlClick($hWnd, "", "[CLASS:Chrome_WidgetWin_1; INSTANCE:1]")
	;  Sleep($delays) ; 延遲再點擊下一個按鈕


; 模擬按下方向鍵 "Down" 300 次

;For $i = 1 To 18
   ; Send("{DOWN}")
;Next

 ;  MouseWheel("down", 3) ; 向下滾動 600 行
	;Sleep($delays) ; 延遲後可以進行下一步點擊

	; 使用 MouseClick 進行點擊
	; 第一個參數是點擊類型（"left"），第二個和第三個參數是 X 和 Y 座標
	MouseClick("left", 430, 240) ; 這裡的座標需要你根據實際按鈕位置調整

	Sleep($delays) ; 延遲後可以進行下一步點擊



	MouseClick("left", 1375, 525) ; 這裡的座標需要你根據實際按鈕位置調整

;   MouseClick("left", 1475, 525) ; 這裡的座標需要你根據實際按鈕位置調整
	Sleep($delays) ; 延遲後可以進行下一步點擊


;	MouseClick("left", 1375, 720) ; 這裡的座標需要你根據實際按鈕位置調整

;	Sleep($delays) ; 延遲後可以進行下一步點擊


For $i = 1 To 3
    Send("{PgDn}")
Next





	; 點擊第二個按鈕
	;  ControlClick($hWnd, "", "[CLASS:Intermediate D3D Window; INSTANCE:2]")

	; 最小化窗口
	;    WinSetState($hWnd, "", @SW_MINIMIZE)

	; 顯示完成訊息
	;MsgBox($MB_SYSTEMMODAL, "完成", "按鈕點擊完成並最小化窗口")
EndFunc   ;==>Example
