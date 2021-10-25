#include <axon.h>
#include <dmi.h>

namespace axon
{
	namespace identity
	{
		struct opt opt;

		dmi::dmi()
		{
			_buffer = nullptr;
		}

		dmi::~dmi()
		{
			if (_buffer != nullptr)
				delete[] _buffer;
		}

		bool dmi::allocbuf(size_t length)
		{
			if (_buffer != nullptr)
			{
				delete[] _buffer;
				_buffer = nullptr;
			}

			_buffer = new (std::nothrow) u8[length];

			if (_buffer == nullptr)
				return false;

			return true;
		}

		void *dmi::read_file(size_t length, const char *filename)
		{
			int fd;
			size_t r2 = 0;
			ssize_t r;
			u8 *p;

			if ((fd = open(filename, O_RDONLY)) == -1)
			{
				//if (errno != ENOENT)
				//	perror(filename);
				return(NULL);
			}

			if ((p = (u8 *) malloc(length)) == NULL)
			{
				return NULL;
			}

			do {
				r = read(fd, p + r2, length - r2);
				if (r == -1)
				{
					if (errno != EINTR)
					{
						close(fd);
						perror(filename);
						free(p);
						return NULL;
					}
				}
				else
					r2 += r;
			} while (r != 0);

			close(fd);
			return p;
		}

		int dmi::myread(int fd, u8 *buf, size_t count, const char *prefix)
		{
			ssize_t r = 1;
			size_t r2 = 0;

			while (r2 != count && r != 0)
			{
				r = read(fd, buf + r2, count - r2);
				if (r == -1)
				{
					if (errno != EINTR)
					{
						close(fd);
						perror(prefix);
						return -1;
					}
				}
				else
					r2 += r;
			}

			if (r2 != count)
			{
				close(fd);
				return -1;
			}

			return 0;
		}

		int dmi::checksum(const u8 *buf, size_t len)
		{
			u8 sum = 0;
			size_t a;

			for (a = 0; a < len; a++)
				sum += buf[a];
			return (sum == 0);
		}

		void *dmi::mem_chunk(off_t base, size_t len, const char *devmem)
		{
			void *p;
			int fd;
			off_t mmoffset;
			void *mmp;

			if ((fd = open(devmem, O_RDONLY)) == -1)
				return NULL;

			if ((p = malloc(len)) == NULL)
				return NULL;

		#ifdef _SC_PAGESIZE
			mmoffset = base % sysconf(_SC_PAGESIZE);
		#else
			mmoffset = base % getpagesize();
		#endif /* _SC_PAGESIZE */

			mmp = mmap(NULL, mmoffset + len, PROT_READ, MAP_SHARED, fd, base - mmoffset);

			if (mmp == MAP_FAILED)
				goto try_read;

			memcpy(p, (u8 *)mmp + mmoffset, len);

			if (munmap(mmp, mmoffset + len) == -1)
			{
				fprintf(stderr, "%s: ", devmem);
				perror("munmap");
			}

			goto out;

		try_read:

			if (lseek(fd, base, SEEK_SET) == -1)
			{
				fprintf(stderr, "%s: ", devmem);
				perror("lseek");
				free(p);
				return NULL;
			}

			if (myread(fd, (u8 *)p, len, devmem) == -1)
			{
				free(p);
				return NULL;
			}

		out:
			if (close(fd) == -1)
				perror(devmem);

			return p;
		}

		int dmi::is_printable(const u8 *data, int len)
		{
			int i;

			for (i = 0; i < len; i++)
				if (data[i] < 32 || data[i] >= 127)
					return 0;

			return 1;
		}

		const char *dmi::dmi_string(const struct dmi_header *dm, u8 s)
		{
			char *bp = (char *)dm->data;
			size_t i, len;

			if (s == 0)
				return "Not Specified";

			bp += dm->length;
			while (s > 1 && *bp)
			{
				bp += strlen(bp);
				bp++;
				s--;
			}

			if (!*bp)
				return "<BAD INDEX>";

			len = strlen(bp);
			for (i = 0; i < len; i++)
				if (bp[i] < 32 || bp[i] == 127)
					bp[i] = '.';

			return bp;
		}

		void dmi::dmi_system_uuid(const u8 *p, u16 ver)
		{
			int only0xFF = 1, only0x00 = 1;
			int i;

			for (i = 0; i < 16 && (only0x00 || only0xFF); i++)
			{
				if (p[i] != 0x00) only0x00 = 0;
				if (p[i] != 0xFF) only0xFF = 0;
			}

			if (only0xFF || only0x00)
			{
				sprintf(_uuid, "00000000000000000000000000000000");
				return;
			}

			if (ver >= 0x0206)
				sprintf(_uuid, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
					p[3], p[2], p[1], p[0], p[5], p[4], p[7], p[6],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
			else
				sprintf(_uuid, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
					p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}

		void dmi::dmi_fixup_type_34(struct dmi_header *h)
		{
			u8 *p = h->data;

			if (h->length == 0x10
			 && is_printable(p + 0x0B, 0x10 - 0x0B))
				h->length = 0x0B;
		}


		void dmi::dmi_decode(const struct dmi_header *h, u16 ver)
		{
			const u8 *data = h->data;

			switch (h->type)
			{

				case 1:
					dmi_system_uuid(data + 0x08, ver);
					break;

				default:
					break;
			}
		}

		void dmi::to_dmi_header(struct dmi_header *h, u8 *data)
		{
			h->type = data[0];
			h->length = data[1];
			h->handle = WORD(data + 2);
			h->data = data;
		}

		void dmi::dmi_table_string(const struct dmi_header *h, const u8 *data, u16 ver)
		{
			int key;
			u8 offset = opt.string->offset;

			if (offset >= h->length)
				return;

			key = (opt.string->type << 8) | offset;
			switch (key)
			{
				case 0x108:
					dmi_system_uuid(data + offset, ver);
					break;

				default:
					break;
			}
		}

		void dmi::dmi_table_decode(u8 *buf, u32 len, u16 num, u16 ver, u32 flags)
		{
			u8 *data;
			int i = 0;

			data = buf;

			while ((i < num || !num) && data + 4 <= buf + len)
			{
				u8 *next;
				struct dmi_header h;
				int display;

				to_dmi_header(&h, data);

				display = ((opt.type == NULL || opt.type[h.type])
					&& !((h.type == 126 || h.type == 127))
					&& !opt.string);

				if (h.length < 4)
					break;

				if (h.type == 34)
					dmi_fixup_type_34(&h);

				next = data + h.length;
				
				while ((unsigned long)(next - buf + 1) < len && (next[0] != 0 || next[1] != 0))
					next++;
				
				next += 2;
				
				if (display)
				{
					if ((unsigned long)(next - buf) <= len)
							dmi_decode(&h, ver);
				}
				else if (opt.string != NULL && opt.string->type == h.type)
					dmi_table_string(&h, data, ver);

				data = next;
				i++;

				if (h.type == 127 && (flags & DMI_H_FLAG_STOP_AT_EOT))
					break;
			}
		}

		void dmi::dmi_table(off_t base, u32 len, u16 num, u16 ver, const char *devmem, u32 flags)
		{
			u8 *buf;

			if (flags & DMI_H_FLAG_NO_FILE_OFFSET)
				base = 0;

			if ((buf = (u8 *) mem_chunk(base, len, devmem)) == NULL)
				return;

			dmi_table_decode(buf, len, num, ver, flags);

			free(buf);
		}

		int dmi::smbios3_decode(u8 *buf, const char *devmem, u32 flags)
		{
			u16 ver;
			u64 offset;

			if (!checksum(buf, buf[0x06]))
				return 0;

			ver = (buf[0x07] << 8) + buf[0x08];

			offset = QWORD(buf + 0x10);

			if (!(flags & DMI_H_FLAG_NO_FILE_OFFSET) && offset.h && sizeof(off_t) < 8)
				return 0;

			dmi_table(((off_t)offset.h << 32) | offset.l,
				  WORD(buf + 0x0C), 0, ver, devmem, flags | DMI_H_FLAG_STOP_AT_EOT);

			return 1;
		}

		int dmi::smbios_decode(u8 *buf, const char *devmem, u32 flags)
		{
			u16 ver;

			if (!checksum(buf, buf[0x05]) || memcmp(buf + 0x10, "_DMI_", 5) != 0 || !checksum(buf + 0x10, 0x0F))
				return 0;

			ver = (buf[0x06] << 8) + buf[0x07];

			switch (ver)
			{
				case 0x021F:
				case 0x0221:
					ver = 0x0203;
					break;
				case 0x0233:
					ver = 0x0206;
					break;
			}

			dmi_table(DWORD(buf + 0x18), WORD(buf + 0x16), WORD(buf + 0x1C), ver, devmem, flags);

			return 1;
		}

		int dmi::legacy_decode(u8 *buf, const char *devmem, u32 flags)
		{
			if (!checksum(buf, 0x0F))
				return 0;

			dmi_table(DWORD(buf + 0x08), WORD(buf + 0x06), WORD(buf + 0x0C),
				((buf[0x0E] & 0xF0) << 4) + (buf[0x0E] & 0x0F), devmem, flags);

			return 1;
		}

		int dmi::address_from_efi(off_t *address)
		{
			FILE *efi_systab;
			const char *filename;
			char linebuf[64];
			int ret;

			*address = 0;

			if ((efi_systab = fopen(filename = "/sys/firmware/efi/systab", "r")) == NULL && (efi_systab = fopen(filename = "/proc/efi/systab", "r")) == NULL)
				return DMI_H_EFI_NOT_FOUND;

			ret = DMI_H_EFI_NO_SMBIOS;

			while ((fgets(linebuf, sizeof(linebuf) - 1, efi_systab)) != NULL)
			{
				char *addrp = strchr(linebuf, '=');

				*(addrp++) = '\0';

				if (strcmp(linebuf, "SMBIOS3") == 0 || strcmp(linebuf, "SMBIOS") == 0)
				{
					ret = 0;
					break;
				}
			}

			if (fclose(efi_systab) != 0)
				perror(filename);

			return ret;
		}

		std::string dmi::getuuid()
		{
			int ret = 0;
			int found = 0;
			off_t fp;
			int efi;
			u8 *buf;

			if (getuid() != 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "You need to be a superuser to access this method");

			opt.devmem = DMI_H_DEFAULT_MEM_DEV;
			opt.flags = 0;

			if ((buf = (u8 *) read_file(0x20, DMI_H_SYS_ENTRY_FILE)) != NULL)
			{
				if (memcmp(buf, "_SM3_", 5) == 0)
				{
					if (smbios3_decode(buf, DMI_H_SYS_TABLE_FILE, DMI_H_FLAG_NO_FILE_OFFSET))
						found++;
				}
				else if (memcmp(buf, "_SM_", 4) == 0)
				{
					if (smbios_decode(buf, DMI_H_SYS_TABLE_FILE, DMI_H_FLAG_NO_FILE_OFFSET))
						found++;
				}
				else if (memcmp(buf, "_DMI_", 5) == 0)
				{
					if (legacy_decode(buf, DMI_H_SYS_TABLE_FILE, DMI_H_FLAG_NO_FILE_OFFSET))
						found++;
				}

				if (found)
					goto done;
			}

			efi = address_from_efi(&fp);

			switch (efi)
			{
				case DMI_H_EFI_NOT_FOUND:
					goto memory_scan;
				case DMI_H_EFI_NO_SMBIOS:
					ret = 1;
					goto exit_free;
			}

			if ((buf = (u8 *) mem_chunk(fp, 0x20, opt.devmem)) == NULL)
			{
				ret = 1;
				goto exit_free;
			}

			if (smbios_decode(buf, opt.devmem, 0))
				found++;
			goto done;

		memory_scan:
			if ((buf = (u8 *) mem_chunk(0xF0000, 0x10000, opt.devmem)) == NULL)
			{
				ret = 1;
				goto exit_free;
			}

			for (fp = 0; fp <= 0xFFF0; fp += 16)
			{
				if (memcmp(buf + fp, "_SM3_", 5) == 0 && fp <= 0xFFE0)
				{
					if (smbios3_decode(buf + fp, opt.devmem, 0))
					{
						found++;
						fp += 16;
					}
				}
				else if (memcmp(buf + fp, "_SM_", 4) == 0 && fp <= 0xFFE0)
				{
					if (smbios_decode(buf + fp, opt.devmem, 0))
					{
						found++;
						fp += 16;
					}
				}
				else if (memcmp(buf + fp, "_DMI_", 5) == 0)
				{
					if (legacy_decode(buf + fp, opt.devmem, 0))
						found++;
				}
			}

		done:
			free(buf);

		exit_free:
			free(opt.type);

			std::string retval;

			if (ret != 0)
				retval = "00000000000000000000000000000000";
			else
				retval = _uuid;

			return retval;
		}
	}
}