

.set IRQ_BASE, 0x20

.section .text

.extern _ZN4myos21hardwarecommunication16InterruptManager15HandleInterruptEhj


.macro HandleException num
.global _ZN4myos21hardwarecommunication16InterruptManager19HandleException\num\()Ev
_ZN4myos21hardwarecommunication16InterruptManager19HandleException\num\()Ev:
	movb $\num, (interruptnumber)
	# the error code is probably pushed on the stack by the processor
	jmp int_bottom
.endm


.macro HandleInterruptRequest num
.global _ZN4myos21hardwarecommunication16InterruptManager26HandleInterruptRequest\num\()Ev
_ZN4myos21hardwarecommunication16InterruptManager26HandleInterruptRequest\num\()Ev:
	movb $\num + IRQ_BASE, (interruptnumber)
	pushl $0	# push error code 0 because we don't have any error code
	jmp int_bottom
.endm

# !Important syscall icin 80e 20 eklemesini cozdum
.macro HandleSystemCall num
.global _ZN4myos21hardwarecommunication16InterruptManager20HandleSystemCall\num\()Ev
_ZN4myos21hardwarecommunication16InterruptManager20HandleSystemCall\num\()Ev:
	movb $\num, (interruptnumber)
	pushl $0	# push error code 0 because we don't have any error code
	jmp int_bottom
.endm


HandleException 0x00
HandleException 0x01
HandleException 0x02
HandleException 0x03
HandleException 0x04
HandleException 0x05
HandleException 0x06
HandleException 0x07
HandleException 0x08
HandleException 0x09
HandleException 0x0A
HandleException 0x0B
HandleException 0x0C
HandleException 0x0D
HandleException 0x0E
HandleException 0x0F
HandleException 0x10
HandleException 0x11
HandleException 0x12
HandleException 0x13
HandleException 0x14
HandleException 0x15

HandleInterruptRequest 0x00
HandleInterruptRequest 0x01
HandleInterruptRequest 0x02
HandleInterruptRequest 0x03
HandleInterruptRequest 0x04
HandleInterruptRequest 0x05
HandleInterruptRequest 0x06
HandleInterruptRequest 0x07
HandleInterruptRequest 0x08
HandleInterruptRequest 0x09
HandleInterruptRequest 0x0A
HandleInterruptRequest 0x0B
HandleInterruptRequest 0x0C
HandleInterruptRequest 0x0D
HandleInterruptRequest 0x0E
HandleInterruptRequest 0x0F
HandleInterruptRequest 0x31

# !Important syscall icin 80e 20 eklemesini cozdum
HandleSystemCall 0x80



int_bottom:

	# save registers
	#pusha
	#pushl %ds
	#pushl %es
	#pushl %fs
	#pushl %gs

	
	#cpu pushes rest of the registers
	pushl %ebp
	pushl %edi
	pushl %esi

	# assign 0x06 to edx
	# mov $0x06, %edx
	# mov $0x06, %ecx
	# mov $0x06, %ebx
	# mov $0x06, %eax

	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	# load ring 0 segment register
	#cld
	#mov $0x10, %eax
	#mov %eax, %eds
	#mov %eax, %ees

	# call C++ HandleInterrupt(interruptnumber, esp);
	pushl %esp  # this is second argument of HandleInterrupt
	push (interruptnumber) # this is first argument of HandleInterrupt
	call _ZN4myos21hardwarecommunication16InterruptManager15HandleInterruptEhj
	#add %esp, 6
	# now, eax has new stack pointer that we returned from HandleInterrupt
	mov %eax, %esp # switch the stack

	# restore registers
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx

	popl %esi
	popl %edi
	popl %ebp
	
	#pop %gs
	#pop %fs
	#pop %es
	#pop %ds
	#popa
	
	add $4, %esp # pops error code, I mean probably.
	# cpu pops rest of the registers

.global _ZN4myos21hardwarecommunication16InterruptManager15InterruptIgnoreEv
_ZN4myos21hardwarecommunication16InterruptManager15InterruptIgnoreEv:

	iret


.data
	interruptnumber: .byte 0
