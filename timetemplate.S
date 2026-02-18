# timetemplate.S
# Written 2015 by F Lundevall
# Ported 2024/06 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

#############################################################
# Choose the macro syntax for RARS or DTEK-V board.         #
# The syntax for RARS is probably due to its MIPS heritage. #
#############################################################
###################
# Macros for RARS #
###################
#.macro PUSH (%reg)
# addi sp,sp,-4
# sw %reg,0(sp)
#.end_macro

#.macro POP (%reg)
# lw %reg,0(sp)
# addi sp,sp,4
#.end_macro
###################
# Macros for DTEK #
###################
.macro PUSH reg
 addi sp,sp,-4
 sw \reg,0(sp)
.endm

.macro POP reg
 lw \reg,0(sp)
 addi sp,sp,4
.endm
#############################################################

	.data
	.align 2
mytime:	.word 	0x5957
timstr:	.asciz 	"text more text lots of text\0"
	.text
	.globl timetemplate, tick, time2string, delay, display_string

# Function for displaying a string with a newline at the end	
jal timetemplate
display_string:	
	li	a7,	4
	ecall
	li	a0,	10
	li	a7,	11
	ecall
	jr 	ra
	
timetemplate:
	la	a0, timstr
	jal     display_string
	
	# wait a little
	li	a0, 2		# ms
	jal	delay
	
	# call tick
	la 	a0, mytime
	jal	tick
	
	# call your function time2string
	la	a0, timstr
	la	t0, mytime
	lw	a1, 0(t0)
	jal	time2string
	
	# go back and do it all again
	j	timetemplate

	
# tick: update time pointed to by $a0
tick:	lw	t0, 0(a0)	# get time
	addi	t0, t0, 1	# increase
	andi	t1, t0, 0xf	# check lowest digit
	sltiu	t2, t1, 0xa	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x6	# adjust lowest digit
	
	andi	t1, t0, 0xf0	# check next digit
	sltiu	t2, t1, 0x60	# if digit < 6, okay
	bnez	t2, tiend
	addi	t0, t0, 0xa0	# adjust digit
	
	li	t3, 0xF
	slli	t3, t3, 0x8
	and	t1, t0, t3	# check minute digit
	addi	t3, x0, 0xA
	slli	t3, t3, 0x8
	slt	t2, t1, t3	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x600	# adjust digit - this one's okay, it's lower than 0x7FF 
	
	li	t3, 0xF
	slli	t3, t3, 0xC
	and	t1, t0, t3	# check last digit
	addi	t3, x0, 0x6
	slli	t3, t3, 0xC
	slt	t2, t1, t3	# if digit < 6, okay
	bnez	t2, tiend
	
	li	t3, 0xA
	slli	t3, t3, 0xC
	add	t0, t0, t3	# adjust last digit
tiend:	sw	t0,0(a0)	# save updated result
	jr	ra		# return

#########################################################
# Place for your functions: time2string, hex2asc, delay.#
#########################################################

time2string:
	mv	t1, ra
	mv 	s0, a0   # in order to not overwrite a0 from hexacs
	
	# First digit (minute)
	srli   t0, a1, 12
	andi   t0, t0, 0xF
	mv     a0, t0
	jal    hexasc
	sb     a0, 0(s0)
	   
# Second digit (minute)
	srli   t0, a1, 8
	andi   t0, t0, 0xF
	mv     a0, t0
	jal    hexasc
	sb     a0, 1(s0)

# Colon
	li t0, 0x3A
	sb t0, 2(s0)

# Third digit (second)
	srli   t0, a1, 4
	andi   t0, t0, 0xF
	mv     a0, t0
	jal    hexasc
	sb     a0, 3(s0)

# Fourth digit (second)
    srli   t0, a1, 0
	andi   t0, t0, 0xF
	mv     a0, t0
	jal    hexasc
	sb     a0, 4(s0)
	
# Null
	sb 	zero, 5(s0)

	mv 	ra, t1
	jr	ra


hex2asc:
	li a0, 4 # test number (from 0 to 15)

	addi    sp,sp,-4
	sw      ra,0(sp)

	jal hexasc # call hexasc

	li a7, 11 # write a0 to stdout
	ecall

	lw      ra,0(sp)
	addi    sp,sp,4
	jr      ra

hexasc:
	andi a0, a0, 0xF # modulo 16, and operation with 1111, only focuses on the 4 lsb
	li t0, 10
	bge a0, t0, letter
	addi a0, a0, 0x30
	jr ra


letter:
	addi a0, a0, -10
	addi a0, a0, 0x41
	jr ra


delay:
	mv	t0, ra		# Save return address
	mv	t2, a0		# Load delay parameter from a0 register
	li	t1, 4711	# Inner loop counter (this controls delay granularity)
	li	t3, 0		# Initialize inner counter
	
outer:
	blez	t2, delay_done	# If delay parameter <= 0, we're done
	addi	t2, t2, -1	# Decrement outer counter (delay parameter)

inner:
	bge	t3, t1, outer	# If inner counter >= 4711, go back to outer
	addi	t3, t3, 1	# Increment inner counter
	j	inner		# Loop back to inner
	
delay_done:
	mv	ra, t0		# Restore return address
	jr	ra		# Return