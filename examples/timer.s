.text.main
main:   add r5, 0 		
		ldc r0, 1073741824
        mov psw, r0     			; podesi psw za timer
		call r2, infl
.text.infloop
infl:   ldr psw, sp, -4 ; ne pop-ujemo samo front()
        cmp r1, 10	;while true
		callsne r5, infl
.end
