
#include <hardwarecommunication/interrupts.h>
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;


void printf(char* str);
void printfHex(uint8_t);
void printfHex32(uint32_t);





InterruptHandler::InterruptHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
{
	this->InterruptNumber = InterruptNumber;
	this->interruptManager = interruptManager;
	interruptManager->handlers[InterruptNumber] = this;
	// // todo sil bunlari
	// printf("InterruptHandler: ");
	// printfHex(InterruptNumber);
	// printf("\n");
}

InterruptHandler::~InterruptHandler()
{
	if(interruptManager->handlers[InterruptNumber] == this)
		interruptManager->handlers[InterruptNumber] = 0;
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp)
{
	return esp;
}

TaskManager* InterruptHandler::getTaskManager()
{
	return interruptManager->taskManager;
}









InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];
InterruptManager* InterruptManager::ActiveInterruptManager = 0;




void InterruptManager::SetInterruptDescriptorTableEntry(uint8_t interrupt,
	uint16_t CodeSegment, void (*handler)(), uint8_t DescriptorPrivilegeLevel, uint8_t DescriptorType)
{
	// address of pointer to code segment (relative to global descriptor table)
	// and address of the handler (relative to segment)
	interruptDescriptorTable[interrupt].handlerAddressLowBits = ((uint32_t) handler) & 0xFFFF;
	interruptDescriptorTable[interrupt].handlerAddressHighBits = (((uint32_t) handler) >> 16) & 0xFFFF;
	interruptDescriptorTable[interrupt].gdt_codeSegmentSelector = CodeSegment;

	const uint8_t IDT_DESC_PRESENT = 0x80;
	interruptDescriptorTable[interrupt].access = IDT_DESC_PRESENT | ((DescriptorPrivilegeLevel & 3) << 5) | DescriptorType;
	interruptDescriptorTable[interrupt].reserved = 0;
}


InterruptManager::InterruptManager(uint16_t hardwareInterruptOffset, GlobalDescriptorTable* globalDescriptorTable, TaskManager* taskManager)
	: programmableInterruptControllerMasterCommandPort(0x20),
	  programmableInterruptControllerMasterDataPort(0x21),
	  programmableInterruptControllerSlaveCommandPort(0xA0),
	  programmableInterruptControllerSlaveDataPort(0xA1),
	  programmableIntervalTimerCommandPort(0x43),
	  programmableIntervalTimerDataPort(0x40)
{
	this->taskManager = taskManager;
	this->hardwareInterruptOffset = hardwareInterruptOffset;
	uint32_t CodeSegment = globalDescriptorTable->CodeSegmentSelector();

	const uint8_t IDT_INTERRUPT_GATE = 0xE;
	for(uint8_t i = 255; i > 0; --i)
	{
		SetInterruptDescriptorTableEntry(i, CodeSegment, &InterruptIgnore, 0, IDT_INTERRUPT_GATE);
		handlers[i] = 0;
	}
	SetInterruptDescriptorTableEntry(0, CodeSegment, &InterruptIgnore, 0, IDT_INTERRUPT_GATE);
	handlers[0] = 0;

	SetInterruptDescriptorTableEntry(0x00, CodeSegment, &HandleException0x00, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x01, CodeSegment, &HandleException0x01, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x02, CodeSegment, &HandleException0x02, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x03, CodeSegment, &HandleException0x03, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x04, CodeSegment, &HandleException0x04, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x05, CodeSegment, &HandleException0x05, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x06, CodeSegment, &HandleException0x06, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x07, CodeSegment, &HandleException0x07, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x08, CodeSegment, &HandleException0x08, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x09, CodeSegment, &HandleException0x09, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x0A, CodeSegment, &HandleException0x0A, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x0B, CodeSegment, &HandleException0x0B, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x0C, CodeSegment, &HandleException0x0C, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x0D, CodeSegment, &HandleException0x0D, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x0E, CodeSegment, &HandleException0x0E, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x0F, CodeSegment, &HandleException0x0F, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x10, CodeSegment, &HandleException0x10, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x11, CodeSegment, &HandleException0x11, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x12, CodeSegment, &HandleException0x12, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x13, CodeSegment, &HandleException0x13, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x14, CodeSegment, &HandleException0x14, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(0x15, CodeSegment, &HandleException0x15, 0, IDT_INTERRUPT_GATE);

	/**
	 * What is the type of this interrupts? Can you list them?
	 * 0x20: Timer interrupt
	 * 0x21: Keyboard interrupt
	 * 0x2E: Ethernet interrupt
	 * 0x2F: Mouse interrupt
	 * 0x31: ATA interrupt
	 * 0x80: System call interrupt
	 * 
	 * 
	 * What about others included here below?
	 * Copilot: 
	 * 
	*/
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x00, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x01, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x02, CodeSegment, &HandleInterruptRequest0x02, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x03, CodeSegment, &HandleInterruptRequest0x03, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x04, CodeSegment, &HandleInterruptRequest0x04, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x05, CodeSegment, &HandleInterruptRequest0x05, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x06, CodeSegment, &HandleInterruptRequest0x06, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x07, CodeSegment, &HandleInterruptRequest0x07, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x08, CodeSegment, &HandleInterruptRequest0x08, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x09, CodeSegment, &HandleInterruptRequest0x09, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0A, CodeSegment, &HandleInterruptRequest0x0A, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0B, CodeSegment, &HandleInterruptRequest0x0B, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0D, CodeSegment, &HandleInterruptRequest0x0D, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0E, CodeSegment, &HandleInterruptRequest0x0E, 0, IDT_INTERRUPT_GATE);
	SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0F, CodeSegment, &HandleInterruptRequest0x0F, 0, IDT_INTERRUPT_GATE);

	// !Important syscall icin 80e 20 eklemesini cozdum
	SetInterruptDescriptorTableEntry(                          0x80, CodeSegment, &HandleSystemCall0x80, 0, IDT_INTERRUPT_GATE);

	programmableInterruptControllerMasterCommandPort.Write(0x11);
	programmableInterruptControllerSlaveCommandPort.Write(0x11);

	// remap
	programmableInterruptControllerMasterDataPort.Write(hardwareInterruptOffset);
	programmableInterruptControllerSlaveDataPort.Write(hardwareInterruptOffset+8);

	programmableInterruptControllerMasterDataPort.Write(0x04);
	programmableInterruptControllerSlaveDataPort.Write(0x02);

	programmableInterruptControllerMasterDataPort.Write(0x01);
	programmableInterruptControllerSlaveDataPort.Write(0x01);

	programmableInterruptControllerMasterDataPort.Write(0x00);
	programmableInterruptControllerSlaveDataPort.Write(0x00);

	InterruptDescriptorTablePointer idt_pointer;
	idt_pointer.size  = 256*sizeof(GateDescriptor) - 1;
	idt_pointer.base  = (uint32_t)interruptDescriptorTable;
	asm volatile("lidt %0" : : "m" (idt_pointer));
}

InterruptManager::~InterruptManager()
{
	Deactivate();
}

uint16_t InterruptManager::HardwareInterruptOffset()
{
	return hardwareInterruptOffset;
}


void InterruptManager::setPITFrequency(uint32_t frequency)
{
    uint32_t divisor = 1193180 / frequency;

	programmableIntervalTimerCommandPort.Write(0x36); // Command port, tell the PIT which channel we're setting
	programmableIntervalTimerDataPort.Write(divisor & 0xFF); // Data port, low byte
	programmableIntervalTimerDataPort.Write((divisor >> 8) & 0xFF); // Data port, high byte
}


void InterruptManager::Activate()
{
	if(ActiveInterruptManager != 0)
		ActiveInterruptManager->Deactivate();

	ActiveInterruptManager = this;
	asm("sti");
}

void InterruptManager::Deactivate()
{
	if(ActiveInterruptManager == this)
	{
		ActiveInterruptManager = 0;
		asm("cli");
	}
}

uint32_t InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp)
{
	if(ActiveInterruptManager != 0)
		return ActiveInterruptManager->DoHandleInterrupt(interrupt, esp);
	return esp;
}

// todo bunu duzelt
int girebilir = 3;
uint32_t InterruptManager::DoHandleInterrupt(uint8_t interrupt, uint32_t esp)
{
	// // print interrupt number
	// printf("INTERRUPT ");
	// printfHex(interrupt);
	// printf("\n");

	/**
	 * So, what is interrupt 20? Copilot: It is the timer interrupt.
	 * Did we define how it is triggered? Copilot: Yes, we did. It is triggered by the programmable interrupt controller.
	 * Where is it? Copilot: It is in the hardwarecommunication folder. 
	*/

	if(handlers[interrupt] != 0)
	{
		esp = handlers[interrupt]->HandleInterrupt(esp);
	}
	else if(interrupt != hardwareInterruptOffset && girebilir > 0)
	{
		girebilir--;

		// todo ayrica bunu da sil, yani buralari komple
		// printf("(unhdlInt 0x"); printfHex(interrupt); printf(", p: "); printfHex((uint8_t)taskManager->getCurrentTask()->getPid()); printf(") esp: "); printfHex32(esp); printf("\n");

		// printf("\r");
		//      if(interrupt == 0x00) printf("0x00: Division by zero\n");
		// else if(interrupt == 0x01) printf("0x01: Single-step interrupt (see trap flag)\n");
		// else if(interrupt == 0x02) printf("0x02: NMI\n");
		// else if(interrupt == 0x03) printf("0x03: Breakpoint (which benefits from the shorter 0xCC encoding of INT 3)\n");
		// else if(interrupt == 0x04) printf("0x04: Overflow\n");
		// else if(interrupt == 0x05) printf("0x05: Bound Range Exceeded\n");
		// else if(interrupt == 0x06) printf("0x06: Invalid Opcode\n");
		// else if(interrupt == 0x07) printf("0x07: Coprocessor not available\n");
		// else if(interrupt == 0x08) printf("0x08: Double Fault\n");
		// else if(interrupt == 0x09) printf("0x09: Coprocessor Segment Overrun (386 or earlier only)\n");
		// else if(interrupt == 0x0A) printf("0x0A: Invalid Task State Segment\n");
		// else if(interrupt == 0x0B) printf("0x0B: Segment not present\n");
		// else if(interrupt == 0x0C) printf("0x0C: Stack Segment Fault\n");
		// else if(interrupt == 0x0D) printf("0x0D: General Protection Fault\n");
		// else if(interrupt == 0x0E) printf("0x0E: Page Fault\n");
		// else if(interrupt == 0x0F) printf("0x0F: reserved\n");
		// else if(interrupt == 0x10) printf("0x10: x87 Floating Point Exception\n");
		// else if(interrupt == 0x11) printf("0x11: Alignment Check\n");
		// else if(interrupt == 0x12) printf("0x12: Machine Check\n");
		// else if(interrupt == 0x13) printf("0x13: SIMD Floating-Point Exception\n");
		// else if(interrupt == 0x14) printf("0x14: Virtualization Exception\n");
		// else if(interrupt == 0x15) printf("0x15: Control Protection Exception (only available with CET)\n");
		// else {
		// 	printf("UNHANDLED INTERRUPT 0x");
		// 	printfHex(interrupt);
		// 	printf("\n");
		// }
		
		// CPUState *cpu = (CPUState *)esp;
		// printf("eax:  "); printfHex32(cpu->eax); printf("\n");
		// printf("ebx:  "); printfHex32(cpu->ebx); printf("\n");
		// printf("ecx:  "); printfHex32(cpu->ecx); printf("\n");
		// printf("edx:  "); printfHex32(cpu->edx); printf("\n");
		// // printf("\n");
		// printf("esi:  "); printfHex32(cpu->esi); printf("\n");
		// printf("edi:  "); printfHex32(cpu->edi); printf("\n");
		// printf("ebp:  "); printfHex32(cpu->ebp); printf("\n");
		// // printf("\n");
		// printf("error "); printfHex32(cpu->error); printf("\n");
		// // printf("\n");
		// printf("eip:  "); printfHex32(cpu->eip); printf("\n");
		// printf("cs:   "); printfHex32(cpu->cs);  printf("\n");
		// printf("eflag "); printfHex32(cpu->eflags); printf("\n");
		// printf("esp:  "); printfHex32(cpu->esp); printf("\n");
		// printf("ss:   "); printfHex32(cpu->ss);  printf("\n");
	}


	/** 
	 * Sanirim buraya timer interrupt olunca giriyo. Dogru mu? 
	 * Copiliot: Evet, timer interrupt olunca giriyo.
	*/
	if(interrupt == hardwareInterruptOffset)
	{
		// printf("schedulle\n");
		// printf("process: "); printfHex32((uint32_t)taskManager->getCurrentTask()->getPid()); printf("\n");
		// printf("esp: "); printfHex32(esp); printf("\n");
		// CPUState *cpu = (CPUState *)esp;
		// printf("eip:  "); printfHex32(cpu->eip); printf("\n");
		// printf("cs:   "); printfHex32(cpu->cs);  printf("\n");
		// printf("eflag "); printfHex32(cpu->eflags); printf("\n");
		// printf("esp:  "); printfHex32(cpu->esp); printf("\n");
		// printf("ss:   "); printfHex32(cpu->ss);  printf("\n");
		// printf("\n");

		// todo bunu da silmeyi unutma
		// printf("(timerInt, p: "); printfHex((uint8_t)taskManager->getCurrentTask()->getPid()); printf(") esp: "); printfHex32(esp); printf("\n");
		
		esp = (uint32_t)taskManager->Schedule((CPUState *)esp);

		// printf("process: "); printfHex32((uint32_t)taskManager->getCurrentTask()->getPid()); printf("\n");
		// printf("esp: "); printfHex32(esp); printf("\n");
		// cpu = (CPUState *)esp;
		// printf("eip:  "); printfHex32(cpu->eip); printf("\n");
		// printf("cs:   "); printfHex32(cpu->cs);  printf("\n");
		// printf("eflag "); printfHex32(cpu->eflags); printf("\n");
		// printf("esp:  "); printfHex32(cpu->esp); printf("\n");
		// printf("ss:   "); printfHex32(cpu->ss);  printf("\n");
	}

	/**
	 * This part of the code is responsible for acknowledging 
	 * hardware interrupts to the Programmable Interrupt Controller (PIC).
	 * 
	 * When a hardware interrupt is triggered, the PIC sends 
	 * a signal to the CPU. After the CPU has handled the interrupt, 
	 * it needs to send an acknowledgement to the PIC to let it know that 
	 * it can now send more interrupts. This is what this code is doing.
	*/
	// hardware interrupts must be acknowledged
	if(hardwareInterruptOffset <= interrupt && interrupt < hardwareInterruptOffset+16)
	{
		programmableInterruptControllerMasterCommandPort.Write(0x20);
		if(hardwareInterruptOffset + 8 <= interrupt)
			programmableInterruptControllerSlaveCommandPort.Write(0x20);
	}

	// ((CPUState *)esp)->eax = esp;
	return esp;
}










