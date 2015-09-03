	.file	"sendToLED.c"
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__SREG__ = 0x3f
__tmp_reg__ = 0
__zero_reg__ = 1
	.text
	.type	sendByte.isra.0, @function
sendByte.isra.0:
/* prologue: function */
/* frame size = 0 */
/* stack size = 0 */
.L__stack_usage = 0
/* #APP */
 ;  12 "sendToLED.c" 1
	bitLoop:                   
  ori r24, 4;      
  out 24, r24;         
  ldi r25, 8;               
  sbrc r18, 0;              
  rjmp highBit;            
lowBit:                    
  nop;                     
  andi r24, -5;      
  out 24, r24;         
  nop; nop; nop; nop; nop; 
  nop; nop;                
  lsr r18;                  
  subi r25, 1;              
  rjmp loopTest;           
highBit:                   
  nop; nop; nop; nop; nop; 
  nop; nop; nop; nop;      
  lsr r18;                  
  subi r25, 1;              
  andi r24, -5;      
  out 24, r24;         
  nop;                     
loopTest:                  
  cpi r25, 0;               
  brne bitLoop;            

 ;  0 "" 2
/* #NOAPP */
	sts i,r25
	sts p,r24
	ret
	.size	sendByte.isra.0, .-sendByte.isra.0
.global	sendToLED
	.type	sendToLED, @function
sendToLED:
	push r28
/* prologue: function */
/* frame size = 0 */
/* stack size = 1 */
.L__stack_usage = 1
	in r24,0x18
	sts p,r24
	cbi 0x18,2
	ldi r24,lo8(83)
	1: dec r24
	brne 1b
	nop
	ldi r28,lo8(30)
.L4:
	rcall sendByte.isra.0
	rcall sendByte.isra.0
	rcall sendByte.isra.0
	subi r28,lo8(-(-1))
	brne .L4
/* epilogue start */
	pop r28
	ret
	.size	sendToLED, .-sendToLED
	.local	p
	.comm	p,1,1
	.local	i
	.comm	i,1,1
	.ident	"GCC: (GNU) 4.8.2"
.global __do_clear_bss
