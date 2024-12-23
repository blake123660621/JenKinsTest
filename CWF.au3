#include <MsgBoxConstants.au3>


$TimeInput = "3" ; value 1
$delays = $TimeInput * 10 ; 10 seconds delay
$delays = $delays * 200 ; miliseconds count/second



Example()
Func Example()
		Local $hWnd = WinGetHandle("SPECTRA-LIGHT QC  Remote Control   V 4.2                                                                                                                                                                        X-RITE")
		
		
        If @error Then
          sleep($delays) ; delaying second ControlClick()	
          MsgBox($MB_SYSTEMMODAL, "", "delay 3 second.")
                MsgBox($MB_SYSTEMMODAL, "", "An error occurred when trying to retrieve the window handle of Notepad.")
                Exit
        EndIf
	WinActivate($hWnd)
	ControlClick($hWnd, "", "[CLASS:Button; INSTANCE:6]")	  
                       
		  sleep($delays) ; delaying second ControlClick()	
		  
		   MsgBox($MB_SYSTEMMODAL, "", "delay 3 second.")
	
	ControlClick($hWnd, "", "[CLASS:Button; INSTANCE:2]")	
	
	WinSetState($hWnd, "", @SW_MINIMIZE )
EndFunc   ;==>Example
