#ifndef AXON_DMI_H_
#define AXON_DMI_H_

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#if defined(__BEOS__) || defined(__HAIKU__)
#define DMI_H_DEFAULT_MEM_DEV "/dev/misc/mem"
#else
#ifdef __sun
#define DMI_H_DEFAULT_MEM_DEV "/dev/xsvc"
#else
#define DMI_H_DEFAULT_MEM_DEV "/dev/mem"
#endif
#endif

/* Use mmap or not */
#ifndef __BEOS__
#define DMI_H_USE_MMAP
#endif

/* Use memory alignment workaround or not */
#ifdef __ia64__
#define DMI_H_ALIGNMENT_WORKAROUND
#endif

#ifdef DMI_H_ALIGNMENT_WORKAROUND
#ifdef BIGENDIAN
#define WORD(x) (u16)((x)[1] + ((x)[0] << 8))
#define DWORD(x) (u32)((x)[3] + ((x)[2] << 8) + ((x)[1] << 16) + ((x)[0] << 24))
#define QWORD(x) (U64(DWORD(x + 4), DWORD(x)))
#else /* BIGENDIAN */
#define WORD(x) (u16)((x)[0] + ((x)[1] << 8))
#define DWORD(x) (u32)((x)[0] + ((x)[1] << 8) + ((x)[2] << 16) + ((x)[3] << 24))
#define QWORD(x) (U64(DWORD(x), DWORD(x + 4)))
#endif /* BIGENDIAN */
#else /* ALIGNMENT_WORKAROUND */
#define WORD(x) (u16)(*(const u16 *)(x))
#define DWORD(x) (u32)(*(const u32 *)(x))
#define QWORD(x) (*(const u64 *)(x))
#endif /* ALIGNMENT_WORKAROUND */

#define DMI_H_SUPPORTED_SMBIOS_VER 0x0300

#define DMI_H_FLAG_NO_FILE_OFFSET (1 << 0)
#define DMI_H_FLAG_STOP_AT_EOT (1 << 1)
#define DMI_H_EFI_NOT_FOUND (-1)
#define DMI_H_EFI_NO_SMBIOS (-2)

#define DMI_H_SYS_ENTRY_FILE "/sys/firmware/dmi/tables/smbios_entry_point"
#define DMI_H_SYS_TABLE_FILE "/sys/firmware/dmi/tables/DMI"

namespace axon
{
	namespace identity
	{
		typedef unsigned char u8;
		typedef unsigned short u16;
		typedef signed short i16;
		typedef unsigned int u32;

		#ifdef BIGENDIAN
		typedef struct {
			u32 h;
			u32 l;
		} u64;
		#else
		typedef struct {
			u32 l;
			u32 h;
		} u64;
		#endif

		#ifdef ALIGNMENT_WORKAROUND
		static inline u64 U64(u32 low, u32 high)
		{
			u64 self;

			self.l = low;
			self.h = high;

			return self;
		}
		#endif

		struct dmi_header
		{
			u8 type;
			u8 length;
			u16 handle;
			u8 *data;
		};

		struct string_keyword
		{
			const char *keyword;
			u8 type;
			u8 offset;
		};

		struct opt
		{
			const char *devmem;
			unsigned int flags;
			u8 *type;
			const struct string_keyword *string;
			char *dumpfile;
		};

		class dmi
		{
			u8 *_buffer;
			char _uuid[128];

		public:
			dmi();
			~dmi();

			bool allocbuf(size_t length);
			void *read_file(size_t, const char *);
			int myread(int, u8 *, size_t, const char *);
			int checksum(const u8 *, size_t);
			void *mem_chunk(off_t, size_t, const char *);
			int is_printable(const u8 *, int);
			const char *dmi_string(const struct dmi_header *, u8);
			void dmi_system_uuid(const u8 *, u16);
			void dmi_fixup_type_34(struct dmi_header *);
			void dmi_decode(const struct dmi_header *, u16);
			void to_dmi_header(struct dmi_header *, u8 *);
			void dmi_table_string(const struct dmi_header *, const u8 *, u16);
			void dmi_table_decode(u8 *, u32, u16, u16, u32);
			void dmi_table(off_t, u32, u16, u16, const char *, u32);
			int smbios3_decode(u8 *, const char *, u32);
			int smbios_decode(u8 *, const char *, u32);
			int legacy_decode(u8 *, const char *, u32);
			int address_from_efi(off_t *);
			std::string getuuid();
		};
	}
}

#endif