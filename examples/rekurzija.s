.data
	output:     .long 8192      ; memorija za output
	input: 		.long 4096      ; memorija za input
.text.prvi
faktorijel: 
        str lr, sp, 0, postinc ; stavi na stek lr
        muls r5, r0				; rezultat je u r5
        sub  r0, 1 				; dekrementiraj
        cmps r0, 1
		callne r2, faktorijel			; pozovi opet ovu funkciju
        ldr lr, sp, 0, predec ; skini lr sa steka
		mov pc, lr				; vrati se van rekurzije
main: 	ldc r5, 0
		add r5, input
		ldc r6, 0
		add r6, output
		in  r1, r5
		out r1, r6
        ldc r0, 5      ; izracunaj faktorijel ovog broja!
		ldc r5, 1 			   ; ocisti r5
		ldc r2, 0				; zbog call
		call r2, facktorijel
        out r5, r6		; ispisi rezultat
.end
	
