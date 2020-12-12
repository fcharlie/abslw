//
#ifndef HAZEL_MACHO_HPP
#define HAZEL_MACHO_HPP
#include <bela/endian.hpp>
#include "hazel.hpp"
#include "details/macho.h"

namespace hazel::macho {

#pragma pack(4)
struct FileHeader {
  uint32_t Magic;
  uint32_t Cpu;
  uint32_t SubCpu;
  uint32_t Type;
  uint32_t Ncmd;
  uint32_t Cmdsz;
  uint32_t Flags;
};
#pragma pack()

constexpr size_t fileHeaderSize32 = 7 * 4;
constexpr size_t fileHeaderSize64 = 8 * 4;

constexpr uint32_t Magic32 = 0xfeedface;
constexpr uint32_t Magic64 = 0xfeedfacf;
constexpr uint32_t MagicFat = 0xcafebabe;

constexpr uint32_t MachoObject = 1;
constexpr uint32_t MachoExec = 2;
constexpr uint32_t MachoDylib = 6;
constexpr uint32_t MachoBundle = 8;
constexpr uint32_t Machine64 = 0x01000000;

enum Machine : uint32_t {
  VAX = 1,
  MC680x0 = 0,
  I386 = 7,
  MC98000 = 9,
  HPPA = 11,
  MC88000 = 13,
  ARM = 12,
  SPARC = 14,
  I860 = 15,
  POWERPC = 18,
  AMD64 = I386 | Machine64,
  ARM64 = ARM | Machine64,
  POWERPC64 = POWERPC | Machine64,
};

struct Segment {
  bela::Buffer LoadBytes;
  uint32_t Cmd;
  uint32_t Len;
  std::string Name;
  uint64_t Addr;
  uint64_t Memsz;
  uint64_t Offset;
  uint64_t Filesz;
  uint32_t Maxprot;
  uint32_t Prot;
  uint32_t Nsect;
  uint32_t Flag;
};

struct Reloc {
  uint32_t Addr;
  uint32_t Value;
  uint8_t Type;
  uint8_t Len; // 0=byte, 1=word, 2=long, 3=quad
  bool Pcrel;
  bool Extern; // valid if Scattered == false
  bool Scattered;
};

struct Section {
  std::string Name;
  std::string Seg;
  uint64_t Addr;
  uint64_t Size;
  uint32_t Offset;
  uint32_t Align;
  uint32_t Reloff;
  uint32_t Nreloc;
  uint32_t Flags;
  std::vector<Reloc> Relocs;
};

struct Dylib {
  bela::Buffer LoadBytes;
  std::string Name;
  uint32_t NameIndex;
  uint32_t CurrentVersion;
  uint32_t CompatVersion;
};

struct Symbol {
  std::string Name;
  uint8_t Type;
  uint8_t Sect;
  uint16_t Desc;
  uint64_t Value;
};

struct Symtab {
  bela::Buffer LoadBytes;
  uint32_t Cmd;
  uint32_t Len;
  uint32_t Symoff;
  uint32_t Nsyms;
  uint32_t Stroff;
  uint32_t Strsize;
  std::vector<Symbol> Syms;
};

struct Rpath {
  bela::Buffer LoadBytes;
  std::string Path;
};

struct Dysymtab {
  bela::Buffer LoadBytes;
  uint32_t Cmd;
  uint32_t Len;
  uint32_t Ilocalsym;
  uint32_t Nlocalsym;
  uint32_t Iextdefsym;
  uint32_t Nextdefsym;
  uint32_t Iundefsym;
  uint32_t Nundefsym;
  uint32_t Tocoffset;
  uint32_t Ntoc;
  uint32_t Modtaboff;
  uint32_t Nmodtab;
  uint32_t Extrefsymoff;
  uint32_t Nextrefsyms;
  uint32_t Indirectsymoff;
  uint32_t Nindirectsyms;
  uint32_t Extreloff;
  uint32_t Nextrel;
  uint32_t Locreloff;
  uint32_t Nlocrel;
  std::vector<uint32_t> IndirectSyms;
};

class FatFile;

constexpr long ErrNotFat = static_cast<long>(MagicFat);

class File {
private:
  bool ParseFile(bela::error_code &ec);
  bool PositionAt(uint64_t pos, bela::error_code &ec) const {
    LARGE_INTEGER oli{0};
    if (SetFilePointerEx(fd, *reinterpret_cast<LARGE_INTEGER *>(&pos), &oli, SEEK_SET) != TRUE) {
      ec = bela::make_system_error_code(L"SetFilePointerEx: ");
      return false;
    }
    return true;
  }
  bool Read(void *buffer, size_t len, size_t &outlen, bela::error_code &ec) const {
    DWORD dwSize = {0};
    if (ReadFile(fd, buffer, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    outlen = static_cast<size_t>(len);
    return true;
  }
  bool ReadFull(void *buffer, size_t len, bela::error_code &ec) const {
    auto p = reinterpret_cast<uint8_t *>(buffer);
    size_t total = 0;
    while (total < len) {
      DWORD dwSize = 0;
      if (ReadFile(fd, p + total, static_cast<DWORD>(len - total), &dwSize, nullptr) != TRUE) {
        ec = bela::make_system_error_code(L"ReadFile: ");
        return false;
      }
      if (dwSize == 0) {
        ec = bela::make_error_code(ERROR_HANDLE_EOF, L"Reached the end of the file");
        return false;
      }
      total += dwSize;
    }
    return true;
  }
  // ReadAt ReadFull
  bool ReadAt(void *buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos + baseOffset, ec)) {
      return false;
    }
    return ReadFull(buffer, len, ec);
  }
  bool ReadAt(bela::Buffer &buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos + baseOffset, ec)) {
      return false;
    }
    if (!ReadFull(buffer.data(), len, ec)) {
      return false;
    }
    buffer.size() = len;
    return true;
  }

  void Free() {
    if (needClosed && fd != INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  void MoveFrom(File &&r) {
    Free();
    fd = r.fd;
    r.fd = INVALID_HANDLE_VALUE;
    needClosed = r.needClosed;
    r.needClosed = false;
    size = r.size;
    r.size = 0;
    baseOffset = r.baseOffset;
    r.baseOffset = 0;
    memcpy(&fh, &r.fh, sizeof(fh));
    memset(&r.fh, 0, sizeof(r.fh));
  }

  template <typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
  Integer EndianCast(Integer t) {
    if (en == bela::endian::Endian::native) {
      return t;
    }
    return bela::bswap(t);
  }
  template <typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
  Integer EndianCastPtr(const void *p) {
    if (en == bela::endian::Endian::native) {
      return *reinterpret_cast<const Integer *>(p);
    }
    return bela::bswap(*reinterpret_cast<const Integer *>(p));
  }

public:
  File() = default;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  File(File &&r) { MoveFrom(std::move(r)); }
  File &operator=(File &&r) {
    MoveFrom(std::move(r));
    return *this;
  }
  ~File() { Free(); }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  bool Is64Bit() const { return is64bit; }
  int64_t Size() const { return size; }

private:
  friend class FatFile;
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t baseOffset{0}; // when support fat
  int64_t size{bela::SizeUnInitialized};
  bela::endian::Endian en{bela::endian::Endian::native};
  FileHeader fh;
  bool is64bit{false};
  bool needClosed{false};
};

struct FatArchHeader {
  uint32_t Cpu;
  uint32_t SubCpu;
  uint32_t Offset;
  uint32_t Size;
  uint32_t Align;
};

struct FatArch {
  FatArchHeader fh;
  File file;
};

class FatFile {
private:
  bool ParseFile(bela::error_code &ec);
  bool PositionAt(uint64_t pos, bela::error_code &ec) const {
    LARGE_INTEGER oli{0};
    if (SetFilePointerEx(fd, *reinterpret_cast<LARGE_INTEGER *>(&pos), &oli, SEEK_SET) != TRUE) {
      ec = bela::make_system_error_code(L"SetFilePointerEx: ");
      return false;
    }
    return true;
  }
  bool Read(void *buffer, size_t len, size_t &outlen, bela::error_code &ec) const {
    DWORD dwSize = {0};
    if (ReadFile(fd, buffer, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    outlen = static_cast<size_t>(len);
    return true;
  }
  bool ReadFull(void *buffer, size_t len, bela::error_code &ec) const {
    auto p = reinterpret_cast<uint8_t *>(buffer);
    size_t total = 0;
    while (total < len) {
      DWORD dwSize = 0;
      if (ReadFile(fd, p + total, static_cast<DWORD>(len - total), &dwSize, nullptr) != TRUE) {
        ec = bela::make_system_error_code(L"ReadFile: ");
        return false;
      }
      if (dwSize == 0) {
        ec = bela::make_error_code(ERROR_HANDLE_EOF, L"Reached the end of the file");
        return false;
      }
      total += dwSize;
    }
    return true;
  }
  // ReadAt ReadFull
  bool ReadAt(void *buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos, ec)) {
      return false;
    }
    return ReadFull(buffer, len, ec);
  }
  bool ReadAt(bela::Buffer &buffer, size_t len, uint64_t pos, bela::error_code &ec) {
    if (!PositionAt(pos, ec)) {
      return false;
    }
    if (!ReadFull(buffer.data(), len, ec)) {
      return false;
    }
    buffer.size() = len;
    return true;
  }

  void Free() {
    if (needClosed && fd != INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  void MoveFrom(File &&r) {
    Free();
    fd = r.fd;
    r.fd = INVALID_HANDLE_VALUE;
    r.needClosed = false;
    size = r.size;
    arches = std::move(arches);
  }

public:
  FatFile() = default;
  FatFile(const FatFile &) = delete;
  FatFile &operator=(const FatFile &) = delete;
  ~FatFile() { Free(); }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec);
  const auto &Archs() const { return arches; }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t size{bela::SizeUnInitialized};
  std::vector<FatArch> arches;
  bool needClosed{false};
};

} // namespace hazel::macho

#endif