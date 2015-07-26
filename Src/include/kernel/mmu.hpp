//
// Tomato OS
// [Internal] MMU 支持
// (c) 2015 SunnyCase
// 创建日期：2015-4-19
#pragma once
#include <stdint.h>
#include <intrin.h>

#if (_MSC_VER < 1900) && !defined(noexcept)
#define noexcept throw()
#endif

namespace Tomato {
	namespace OS
	{

#pragma pack(push, 1)

#ifdef _M_X64
#define MSR_IA32_EFER 0xC0000080

		union tagMSR_IA32_EFER
		{
			struct
			{
				uint64_t SCE : 1;			// SYSCALL Enable
				uint64_t Reserved_0 : 7;
				uint64_t LME : 1;			// IA-32e Mode Enable
				uint64_t Reserved_1 : 1;
				uint64_t LMA : 1;			// IA-32e Mode Active
				uint64_t NXE : 1;			// Execute Disable Bit Enable
				uint64_t Reserved_2 : 52;
			};
			uint64_t value;

			tagMSR_IA32_EFER(uint64_t value)
				:value(value)
			{}
		};

		union tagCR0
		{
			struct
			{
				uint32_t PE : 1;
				uint32_t MP : 1;
				uint32_t EM : 1;
				uint32_t TS : 1;
				uint32_t ET : 1;
				uint32_t NE : 1;
				uint32_t Reserved_0 : 10;
				uint32_t WP : 1;
				uint32_t Reserved_1 : 1;
				uint32_t AM : 1;
				uint32_t Reserved_2 : 10;
				uint32_t NW : 1;
				uint32_t CD : 1;		// 禁用 Cache
				uint32_t PG : 1;		// 启用分页
			};
			uint32_t value;

			tagCR0(uint32_t value)
				:value(value)
			{}
		};

		union tagCR4
		{
			struct
			{
				uint64_t VME : 1;
				uint64_t PVI : 1;
				uint64_t TSD : 1;
				uint64_t DE : 1;
				uint64_t PSE : 1;
				uint64_t PAE : 1;
				uint64_t MCE : 1;
				uint64_t PGE : 1;
				uint64_t PCE : 1;
				uint64_t OSFXSR : 1;
				uint64_t OSXMMEXCPT : 1;
				uint64_t Reserved_3 : 2;
				uint64_t VMXE : 1;
				uint64_t SMXE : 1;
				uint64_t Reserved_2 : 1;
				uint64_t FSGSBASE : 1;
				uint64_t PCIDE : 1;			// Enables process-context identifiers 
				uint64_t OSXSAVE : 1;
				uint64_t Reserved_1 : 1;
				uint64_t SMEP : 1;
				uint64_t SMAP : 1;
				uint64_t Reserved_0 : 42;
			};
			uint64_t value;

			tagCR4(uint64_t value)
				:value(value)
			{}
		};

#define PHYSIC_ADDR_MASK 0x7FFFFFFFFF000ui64
#define PHYSIC_ADDR_SHIFT 12

		// Page Table Entry
		union PTEntry
		{
			struct
			{
				uint64_t Present : 1;			// 可用
				uint64_t ReadWrite : 1;			// 可读写，否则只读
				uint64_t User : 1;				// 用户模式可访问，否则需要特权
				uint64_t PWT : 1;
				uint64_t PCD : 1;				// 页级禁用 Cache
				uint64_t Accessed : 1;			// Accessed
				uint64_t Dirty : 1;
				uint64_t PAT : 1;
				uint64_t Global : 1;
				uint64_t Ignored_1 : 3;
				uint64_t PhysicAlignAddr : 39;
				uint64_t Reserved_0 : 2;
				uint64_t Ignored_0 : 10;
				uint64_t XD : 1;				// 禁用执行
			};
			uint64_t value;

			void* GetPhysicalAddress() const noexcept
			{
				return (void*)((PhysicAlignAddr << PHYSIC_ADDR_SHIFT) & PHYSIC_ADDR_MASK);
			}

			void SetPhysicalAddress(void* addr) noexcept
			{
				PhysicAlignAddr = (uint64_t(addr) & PHYSIC_ADDR_MASK) >> PHYSIC_ADDR_SHIFT;
			}
		};

		static_assert(sizeof(PTEntry) == 8, "size of PTEntry must be 8.");

		typedef PTEntry PageTable[512];

#define PT_ADDR_MASK 0x7FFFFFFFFF000ui64
#define PT_ADDR_SHIFT 12

		// Page Directory Entry
		union PDEntry
		{
			struct
			{
				uint64_t Present : 1;			// 可用
				uint64_t ReadWrite : 1;			// 可读写，否则只读
				uint64_t User : 1;				// 用户模式可访问，否则需要特权
				uint64_t PWT : 1;
				uint64_t PCD : 1;				// 页级禁用 Cache
				uint64_t Accessed : 1;			// Accessed
				uint64_t Ignored_2 : 1;
				uint64_t PS : 1;
				uint64_t Ignored_1 : 4;
				uint64_t PTAlignAddr : 39;
				uint64_t Reserved_0 : 2;
				uint64_t Ignored_0 : 10;
				uint64_t XD : 1;				// 禁用执行
			};
			uint64_t value;

			PageTable& GetPageTableAddress() const noexcept
			{
				PTEntry* addr = (PTEntry*)((PTAlignAddr << PT_ADDR_SHIFT) & PT_ADDR_MASK);
				return reinterpret_cast<PageTable&>(*addr);
			}

			void SetPageTableAddress(const PageTable& addr) noexcept
			{
				const PTEntry* value = addr;
				PTAlignAddr = (uint64_t(value) & PT_ADDR_MASK) >> PT_ADDR_SHIFT;
			}
		};

		static_assert(sizeof(PDEntry) == 8, "size of PDEntry must be 8.");

		typedef PDEntry PageDirectory[512];

#define PD_ADDR_MASK 0x7FFFFFFFFF000ui64
#define PD_ADDR_SHIFT 12

		// Page Directory Pointer Table Entry
		union PDPTEntry
		{
			struct
			{
				uint64_t Present : 1;			// 可用
				uint64_t ReadWrite : 1;			// 可读写，否则只读
				uint64_t User : 1;				// 用户模式可访问，否则需要特权
				uint64_t PWT : 1;
				uint64_t PCD : 1;				// 页级禁用 Cache
				uint64_t A : 1;					// Accessed
				uint64_t Ignored_2 : 1;
				uint64_t PS : 1;
				uint64_t Ignored_1 : 4;
				uint64_t PDAlignAddr : 19;
				uint64_t Reserved_0 : 2;
				uint64_t Ignored_0 : 10;
				uint64_t XD : 1;				// 禁用执行
			};
			uint64_t value;

			PageDirectory& GetPageDirectoryAddress() const noexcept
			{
				PDEntry* addr = (PDEntry*)((PDAlignAddr << PD_ADDR_SHIFT) & PD_ADDR_MASK);
				return reinterpret_cast<PageDirectory&>(*addr);
			}

			void SetPageDirectoryAddress(const PageDirectory& addr) noexcept
			{
				const PDEntry* value = addr;
				PDAlignAddr = (uint64_t(value) & PD_ADDR_MASK) >> PD_ADDR_SHIFT;
			}
		};

		static_assert(sizeof(PDPTEntry) == 8, "size of PDPTEntry must be 8.");

		typedef PDPTEntry PDPTable[512];

#define PDPT_ADDR_MASK 0x7FFFFFFFFF000ui64
#define PDPT_ADDR_SHIFT 12

		union PML4Entry
		{
			struct
			{
				uint64_t Present : 1;			// 可用
				uint64_t ReadWrite : 1;			// 可读写，否则只读
				uint64_t User : 1;				// 用户模式可访问，否则需要特权
				uint64_t PWT : 1;
				uint64_t PCD : 1;				// 页级禁用 Cache
				uint64_t A : 1;					// Accessed
				uint64_t Ignored_2 : 1;
				uint64_t PS_Reserved : 1;
				uint64_t Ignored_1 : 4;
				uint64_t PDPTAlignAddr : 39;
				uint64_t Reserved_0 : 2;
				uint64_t Ignored_0 : 10;
				uint64_t XD : 1;				// 禁用执行
			};
			uint64_t value;

			PDPTable& GetPDPTableAddress() const noexcept
			{
				auto addr = (PDPTEntry*)((PDPTAlignAddr << PDPT_ADDR_SHIFT) & PDPT_ADDR_MASK);
				return reinterpret_cast<PDPTable&>(*addr);
			}

			void SetPDPTableAddress(const PDPTable& addr) noexcept
			{
				const PDPTEntry* value = addr;
				PDPTAlignAddr = (uint64_t(value) & PDPT_ADDR_MASK) >> PDPT_ADDR_SHIFT;
			}
		};

		typedef PML4Entry PML4Table[512];

		static_assert(sizeof(tagMSR_IA32_EFER) == 8, "size of tagMSR_IA32_EFER must be 8.");
		static_assert(sizeof(tagCR0) == 4, "size of tagCR0 must be 4.");
		static_assert(sizeof(tagCR4) == 8, "size of tagCR4 must be 8.");
		static_assert(sizeof(PML4Entry) == 8, "size of PML4Entry must be 8.");

		enum : uint64_t
		{
			PTEntryManageSize = 4 * 1024,		// 4 KB
			PDEntryManageSize = 512 * PTEntryManageSize, // 2 MB
			PDPEntryManageSize = 512 * PDEntryManageSize, // 1 GB
			PML4EntryManageSize = 512 * PDPEntryManageSize, // 512 GB,
			PML4TableManageSize = 512 * PML4EntryManageSize // 256 TB
		};

#define CR3_PML4_MASK 0xFFFFFFFFFF000ui64

		inline void EnableIA32ePaging(const PML4Table& pml4Table)
		{
			const PML4Entry* addr = pml4Table;
			uint64_t cr3 = __readcr3();
			cr3 &= ~CR3_PML4_MASK;
			cr3 |= ((uint64_t)addr) & CR3_PML4_MASK;
			// 将页表存入 cr3
			__writecr3(cr3);

			// 启用分页
			tagCR0 cr0 = __readcr0();
			cr0.PG = 1;
			__writecr0(cr0.value);

			// 启用 PAE
			tagCR4 cr4 = __readcr4();
			cr4.PAE = 1;
			__writecr4(cr4.value);

			// 启用 IA32e 分页
			tagMSR_IA32_EFER ia32Efer = __readmsr(MSR_IA32_EFER);
			ia32Efer.LME = 1;
			__writemsr(MSR_IA32_EFER, ia32Efer.value);
		}

#endif
#pragma pack(pop)
	}
}