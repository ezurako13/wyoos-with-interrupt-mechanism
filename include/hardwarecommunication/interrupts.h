
#ifndef __MYOS__HARDWARECOMMUNICATION__INTERRUPTMANAGER_H
#define __MYOS__HARDWARECOMMUNICATION__INTERRUPTMANAGER_H

#include <gdt.h>
#include <multitasking.h>
#include <common/types.h>
#include <hardwarecommunication/port.h>


namespace myos
{
	namespace hardwarecommunication
	{

		class InterruptManager;

		class InterruptHandler
		{
			protected:
				myos::common::uint8_t InterruptNumber;
				InterruptManager* interruptManager;
				InterruptHandler(InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber);
				~InterruptHandler();
			public:
				virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);
				TaskManager* getTaskManager();
		};


		class InterruptManager
		{
			friend class InterruptHandler;
			protected:

				static InterruptManager* ActiveInterruptManager;
				InterruptHandler* handlers[256];
				TaskManager *taskManager;

				struct GateDescriptor
				{
					myos::common::uint16_t handlerAddressLowBits;
					myos::common::uint16_t gdt_codeSegmentSelector;
					myos::common::uint8_t reserved;
					myos::common::uint8_t access;
					myos::common::uint16_t handlerAddressHighBits;
				} __attribute__((packed));

				static GateDescriptor interruptDescriptorTable[256];

				struct InterruptDescriptorTablePointer
				{
					myos::common::uint16_t size;
					myos::common::uint32_t base;
				} __attribute__((packed));

				myos::common::uint16_t hardwareInterruptOffset;
				static void SetInterruptDescriptorTableEntry(myos::common::uint8_t interrupt,
					myos::common::uint16_t codeSegmentSelectorOffset, void (*handler)(),
					myos::common::uint8_t DescriptorPrivilegeLevel, myos::common::uint8_t DescriptorType);


				static void InterruptIgnore();

				static void HandleInterruptRequest0x00();
				static void HandleInterruptRequest0x01();
				static void HandleInterruptRequest0x02();
				static void HandleInterruptRequest0x03();
				static void HandleInterruptRequest0x04();
				static void HandleInterruptRequest0x05();
				static void HandleInterruptRequest0x06();
				static void HandleInterruptRequest0x07();
				static void HandleInterruptRequest0x08();
				static void HandleInterruptRequest0x09();
				static void HandleInterruptRequest0x0A();
				static void HandleInterruptRequest0x0B();
				static void HandleInterruptRequest0x0C();
				static void HandleInterruptRequest0x0D();
				static void HandleInterruptRequest0x0E();
				static void HandleInterruptRequest0x0F();
				static void HandleInterruptRequest0x31();

				// !Important syscall icin 80e 20 eklemesini cozdum
				static void HandleSystemCall0x80();

				static void HandleException0x00(); // 0x00	Division by zero
				static void HandleException0x01(); // 0x01	Single-step interrupt (see trap flag)
				static void HandleException0x02(); // 0x02	NMI
				static void HandleException0x03(); // 0x03	Breakpoint (which benefits from the shorter 0xCC encoding of INT 3)
				static void HandleException0x04(); // 0x04	Overflow
				static void HandleException0x05(); // 0x05	Bound Range Exceeded
				static void HandleException0x06(); // 0x06	Invalid Opcode
				static void HandleException0x07(); // 0x07	Coprocessor not available
				static void HandleException0x08(); // 0x08	Double Fault
				static void HandleException0x09(); // 0x09	Coprocessor Segment Overrun (386 or earlier only)
				static void HandleException0x0A(); // 0x0A	Invalid Task State Segment
				static void HandleException0x0B(); // 0x0B	Segment not present
				static void HandleException0x0C(); // 0x0C	Stack Segment Fault
				static void HandleException0x0D(); // 0x0D	General Protection Fault
				static void HandleException0x0E(); // 0x0E	Page Fault
				static void HandleException0x0F(); // 0x0F	reserved
				static void HandleException0x10(); // 0x10	x87 Floating Point Exception
				static void HandleException0x11(); // 0x11	Alignment Check
				static void HandleException0x12(); // 0x12	Machine Check
				static void HandleException0x13(); // 0x13	SIMD Floating-Point Exception
				static void HandleException0x14(); // 0x14	Virtualization Exception
				static void HandleException0x15(); // 0x15	Control Protection Exception (only available with CET)

				static myos::common::uint32_t HandleInterrupt(myos::common::uint8_t interrupt, myos::common::uint32_t esp);
				myos::common::uint32_t DoHandleInterrupt(myos::common::uint8_t interrupt, myos::common::uint32_t esp);

				Port8BitSlow programmableInterruptControllerMasterCommandPort;
				Port8BitSlow programmableInterruptControllerMasterDataPort;
				Port8BitSlow programmableInterruptControllerSlaveCommandPort;
				Port8BitSlow programmableInterruptControllerSlaveDataPort;
				Port8BitSlow programmableIntervalTimerCommandPort;
				Port8BitSlow programmableIntervalTimerDataPort;

			public:
				InterruptManager(myos::common::uint16_t hardwareInterruptOffset, myos::GlobalDescriptorTable* globalDescriptorTable, myos::TaskManager* taskManager);
				~InterruptManager();
				myos::common::uint16_t HardwareInterruptOffset();
				void setPITFrequency(myos::common::uint32_t frequency);
				void Activate();
				void Deactivate();
		};
		
	}
}

#endif