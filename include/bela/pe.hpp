///
#ifndef BELA_PE_HPP
#define BELA_PE_HPP
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include "base.hpp"
#include "endian.hpp"
#include "phmap.hpp"
#include "types.hpp"
#include "ascii.hpp"
#include "match.hpp"
#include "os.hpp"

namespace bela::pe {
constexpr long ErrNoOverlay = 0xFF01;
constexpr long LimitOverlaySize = 64 * 1024 * 1024;
// Machine Types
// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#machine-types
enum class Machine : uint16_t {
  UNKNOWN = 0,
  TARGET_HOST = 0x0001, // Useful for indicating we want to interact with the
                        // host and not a WoW guest.
  I386 = 0x014c,        // Intel 386.
  R3000 = 0x0162,       // MIPS little-endian, 0x160 big-endian
  R4000 = 0x0166,       // MIPS little-endian
  R10000 = 0x0168,      // MIPS little-endian
  WCEMIPSV2 = 0x0169,   // MIPS little-endian WCE v2
  ALPHA = 0x0184,       // Alpha_AXP
  SH3 = 0x01a2,         // SH3 little-endian
  SH3DSP = 0x01a3,
  SH3E = 0x01a4,  // SH3E little-endian
  SH4 = 0x01a6,   // SH4 little-endian
  SH5 = 0x01a8,   // SH5
  ARM = 0x01c0,   // ARM Little-Endian
  THUMB = 0x01c2, // ARM Thumb/Thumb-2 Little-Endian
  ARMNT = 0x01c4, // ARM Thumb-2 Little-Endian
  AM33 = 0x01d3,
  POWERPC = 0x01F0, // IBM PowerPC Little-Endian
  POWERPCFP = 0x01f1,
  IA64 = 0x0200,      // Intel 64
  MIPS16 = 0x0266,    // MIPS
  ALPHA64 = 0x0284,   // ALPHA64
  MIPSFPU = 0x0366,   // MIPS
  MIPSFPU16 = 0x0466, // MIPS
  TRICORE = 0x0520,   // Infineon
  CEF = 0x0CEF,
  EBC = 0x0EBC,   // EFI Byte Code
  AMD64 = 0x8664, // AMD64 (K8)
  M32R = 0x9041,  // M32R little-endian
  ARM64 = 0xAA64, // ARM64 Little-Endian
  RISCV32 = 0x5032,
  RISCV64 = 0x5064,
  RISCV128 = 0x5128,
  CHPEX86 = 0x3A64,
  // 10.0.22000.0/km/ntimage.h LINE-245
  // #define IMAGE_FILE_MACHINE_CHPE_X86          0x3A64
  // #define IMAGE_FILE_MACHINE_ARM64EC           0xA641
  // #define IMAGE_FILE_MACHINE_ARM64X            0xA64E
  ARM64EC = 0xA641,
  ARM64X = 0xA64E,
  CEE = 0xC0EE
};
enum class Subsystem : uint16_t {
  UNKNOWN = 0,
  NATIVE = 1,
  GUI = 2,
  CUI = 3, //
  OS2_CUI = 5,
  POSIX_CUI = 7,
  NATIVE_WINDOWS = 8,
  WINDOWS_CE_GUI = 9,
  EFI_APPLICATION = 10,
  EFI_BOOT_SERVICE_DRIVER = 11,
  EFI_RUNTIME_DRIVER = 12,
  EFI_ROM = 13,
  XBOX = 14,
  WINDOWS_BOOT_APPLICATION = 16,
  XBOX_CODE_CATALOG = 17
};

// https://docs.microsoft.com/en-us/windows/win32/menurc/resource-types
enum ResourceTypes : uint32_t {
  CURSOR = 1,        // Hardware-dependent cursor resource.
  BITMAP = 2,        // Bitmap resource.
  ICON = 3,          // Hardware-dependent icon resource.
  MENU = 4,          // Menu resource.
  DIALOG = 5,        // Dialog box.
  STRING = 6,        // String-table entry.
  FONTDIR = 7,       // Font directory resource.
  FONT = 8,          // Font resource.
  ACCELERATOR = 9,   // Accelerator table.
  RCDATA = 10,       // Application-defined resource (raw data).
  MESSAGETABLE = 11, // Message-table entry.
  GROUP_CURSOR = 12, // Hardware-independent cursor resource.
  GROUP_ICON = 13,   // Hardware-independent icon resource.
  VERSION = 16,      // Version resource.
  DLGINCLUDE = 17,   // Allows a resource editing tool to associate a string with an .rc file.
  PLUGPLAY = 19,     // Plug and Play resource.
  VXD = 20,          // VXD
  ANICURSOR = 21,    // Animated cursor.
  ANIICON = 22,      // Animated icon.
  HTML = 23,         // HTML resource.
  MANIFEST = 24,     // Side-by-Side Assembly Manifest.
};

#pragma pack(push, 1)
struct DosHeader {     // DOS .EXE header
  uint16_t e_magic;    // Magic number
  uint16_t e_cblp;     // Bytes on last page of file
  uint16_t e_cp;       // Pages in file
  uint16_t e_crlc;     // Relocations
  uint16_t e_cparhdr;  // Size of header in paragraphs
  uint16_t e_minalloc; // Minimum extra paragraphs needed
  uint16_t e_maxalloc; // Maximum extra paragraphs needed
  uint16_t e_ss;       // Initial (relative) SS value
  uint16_t e_sp;       // Initial SP value
  uint16_t e_csum;     // Checksum
  uint16_t e_ip;       // Initial IP value
  uint16_t e_cs;       // Initial (relative) CS value
  uint16_t e_lfarlc;   // File address of relocation table
  uint16_t e_ovno;     // Overlay number
  uint16_t e_res[4];   // Reserved words
  uint16_t e_oemid;    // OEM identifier (for e_oeminfo)
  uint16_t e_oeminfo;  // OEM information; e_oemid specific
  uint16_t e_res2[10]; // Reserved words
  uint32_t e_lfanew;   // File address of new exe header
};

struct FileHeader {
  uint16_t Machine;
  uint16_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
};
struct DataDirectory {
  uint32_t VirtualAddress;
  uint32_t Size;
};

constexpr uint32_t NumberOfDirectoryEntries = 16;
struct OptionalHeader32 {
  uint16_t Magic;
  uint8_t MajorLinkerVersion;
  uint8_t MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint32_t BaseOfData;
  uint32_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t DllCharacteristics;
  uint32_t SizeOfStackReserve;
  uint32_t SizeOfStackCommit;
  uint32_t SizeOfHeapReserve;
  uint32_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  DataDirectory DataDirectory[NumberOfDirectoryEntries];
};

struct OptionalHeader64 {
  uint16_t Magic;
  uint8_t MajorLinkerVersion;
  uint8_t MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint64_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t DllCharacteristics;
  uint64_t SizeOfStackReserve;
  uint64_t SizeOfStackCommit;
  uint64_t SizeOfHeapReserve;
  uint64_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  DataDirectory DataDirectory[NumberOfDirectoryEntries];
};

struct SectionHeader32 {
  uint8_t Name[8]; // UTF-8
  uint32_t VirtualSize;
  uint32_t VirtualAddress;
  uint32_t SizeOfRawData;
  uint32_t PointerToRawData;
  uint32_t PointerToRelocations;
  uint32_t PointerToLineNumbers;
  uint16_t NumberOfRelocations;
  uint16_t NumberOfLineNumbers;
  uint32_t Characteristics;
};

struct Reloc {
  uint32_t VirtualAddress;
  uint32_t SymbolTableIndex;
  uint16_t Type;
};

// COFFSymbol represents single COFF symbol table record.
struct COFFSymbol {
  uint8_t Name[8]; // UTF-8
  uint32_t Value;
  int16_t SectionNumber;
  uint16_t Type;
  uint8_t StorageClass;
  uint8_t NumberOfAuxSymbols;
};
#pragma pack(pop)

struct Section {
  std::string Name; // UTF-8
  uint32_t VirtualSize;
  uint32_t VirtualAddress;
  uint32_t Size;
  uint32_t Offset;
  uint32_t PointerToRelocations;
  uint32_t PointerToLineNumbers;
  uint16_t NumberOfRelocations;
  uint16_t NumberOfLineNumbers;
  uint32_t Characteristics;
  std::vector<Reloc> Relocs;
};

constexpr uint32_t COFFSymbolSize = sizeof(COFFSymbol);
constexpr int64_t SectionSizeLimit = 1024 * 1024 * 1024;

// StringTable: Programs written in golang will customize stringtable
struct StringTable {
  uint8_t *data{nullptr};
  size_t length{0};
  StringTable() = default;
  StringTable(const StringTable &) = delete;
  StringTable &operator=(const StringTable &) = delete;
  StringTable(StringTable &&other) { MoveFrom(std::move(other)); }
  StringTable &operator=(StringTable &&other) {
    MoveFrom(std::move(other));
    return *this;
  }
  ~StringTable();
  void MoveFrom(StringTable &&other);
  std::string String(uint32_t start, bela::error_code &ec) const;
};

// Symbol is similar to COFFSymbol with Name field replaced
// by Go string. Symbol also does not have NumberOfAuxSymbols.
struct Symbol {
  std::string Name; // UTF-8
  uint32_t Value;
  int16_t SectionNumber;
  uint16_t Type;
  uint8_t StorageClass;
};

struct ExportedSymbol {
  std::string Name; // UTF-8
  std::string UndecoratedName;
  std::string ForwardName;
  DWORD Address;
  unsigned short Ordinal{0xFFFF};
  int Hint{0};
};

struct Function {
  Function(std::string &&name, int index = 0, int ordinal = 0)
      : Name(std::move(name)), Index(index), Ordinal(ordinal) {}
  Function(const std::string &name, int index = 0, int ordinal = 0) : Name(name), Index(index), Ordinal(ordinal) {}
  std::string Name;
  int Index{0};
  int Ordinal{0};
  int GetIndex() const {
    if (Ordinal != 0) {
      return Ordinal;
    }
    return Index;
  }
};

struct FunctionTable {
  using symbols_map_t = bela::flat_hash_map<std::string, std::vector<Function>>;
  symbols_map_t imports;
  symbols_map_t delayimprots;
  std::vector<ExportedSymbol> exports;
};

using symbols_map_t = bela::flat_hash_map<std::string, std::vector<Function>>;

struct Version {
  std::wstring CompanyName;
  std::wstring FileDescription;
  std::wstring FileVersion;
  std::wstring InternalName;
  std::wstring LegalCopyright;
  std::wstring OriginalFileName;
  std::wstring ProductName;
  std::wstring ProductVersion;
  std::wstring Comments;
  std::wstring LegalTrademarks;
  std::wstring PrivateBuild;
  std::wstring SpecialBuild;
};

struct FileInfo {
  DWORD dwSignature;        /* e.g. 0xfeef04bd */
  DWORD dwStrucVersion;     /* e.g. 0x00000042 = "0.42" */
  DWORD dwFileVersionMS;    /* e.g. 0x00030075 = "3.75" */
  DWORD dwFileVersionLS;    /* e.g. 0x00000031 = "0.31" */
  DWORD dwProductVersionMS; /* e.g. 0x00030010 = "3.10" */
  DWORD dwProductVersionLS; /* e.g. 0x00000031 = "0.31" */
  DWORD dwFileFlagsMask;    /* = 0x3F for version "0.42" */
  DWORD dwFileFlags;        /* e.g. VFF_DEBUG | VFF_PRERELEASE */
  DWORD dwFileOS;           /* e.g. VOS_DOS_WINDOWS16 */
  DWORD dwFileType;         /* e.g. VFT_DRIVER */
  DWORD dwFileSubtype;      /* e.g. VFT2_DRV_KEYBOARD */
  DWORD dwFileDateMS;       /* e.g. 0 */
  DWORD dwFileDateLS;       /* e.g. 0 */
};

struct DotNetMetadata {
  std::string version;
  std::string flags;
  std::vector<std::string> imports;
};

class SectionBuffer {
public:
  SectionBuffer() = default;
  void resize(size_t size) { rawdata.resize(size); }
  char *data() { return rawdata.data(); }
  const char *data() const { return rawdata.data(); } // read section
  size_t size() const { return rawdata.size(); }
  std::string_view substr(size_t pos = 0) const {
    if (pos > rawdata.size()) {
      return std::string_view();
    }
    return std::string_view{rawdata.data() + pos, rawdata.size() - pos};
  }
  std::string_view cstring_view(size_t offset, size_t cslength = std::string_view::npos) const {
    if (offset > rawdata.size()) {
      return std::string_view();
    }
    cslength = (std::min)(cslength, rawdata.size());
    for (auto end = offset; end < cslength; end++) {
      if (rawdata[end] == 0) {
        return std::string_view(rawdata.data() + offset, end - offset);
      }
    }
    return std::string_view();
  }
  template <typename T> const T *direct_cast(size_t offset) const {
    if (offset + sizeof(T) > rawdata.size()) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(rawdata.data() + offset);
  }
  template <typename T> const T *bit_cast(T *t, size_t offset) const {
    if (offset + sizeof(T) > rawdata.size()) {
      return nullptr;
    }
    return reinterpret_cast<T *>(memcpy(t, rawdata.size() + offset, sizeof(T)));
  }
  uint16_t function_hit(size_t offset) const {
    if (offset + 2 < rawdata.size()) {
      return 0;
    }
    return bela::cast_frombe<uint16_t>(rawdata.data() + offset);
  }

private:
  std::vector<char> rawdata;
};

// PE File resolve
// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format
class File {
private:
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
  bool ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec) const {
    if (!bela::os::file::Seek(fd, pos, ec)) {
      return false;
    }
    return ReadFull(buffer, len, ec);
  }
  bool parseFile(bela::error_code &ec);
  std::string sectionFullName(SectionHeader32 &sh) const;
  const DataDirectory *getDataDirectory(uint32_t dirIndex) const {
    if (dirIndex >= NumberOfDirectoryEntries) {
      return nullptr;
    }
    uint32_t ddlen = 0;
    const DataDirectory *dd = nullptr;
    // grab the number of data directory entries
    // grab the $dirIndex data directory entry
    if (is64bit) {
      ddlen = oh.NumberOfRvaAndSizes;
      dd = &(oh.DataDirectory[dirIndex]);
    } else {
      auto oh3 = reinterpret_cast<const OptionalHeader32 *>(&oh);
      ddlen = oh3->NumberOfRvaAndSizes;
      dd = &(oh3->DataDirectory[dirIndex]);
    }
    // check that the length of data directory entries is large
    // enough to include the imports directory.
    if (ddlen < IMAGE_DIRECTORY_ENTRY_IMPORT + 1) {
      return nullptr;
    }
    return dd;
  }
  // getSection figure out which section contains the 'dd' directory table
  const Section *getSection(const DataDirectory *dd) const {
    // Also, do not assume that the RVAs in this table point to the beginning of a section or that the sections that
    // contain specific tables have specific names.
    for (const auto &s : sections) {
      if (s.VirtualAddress <= dd->VirtualAddress && dd->VirtualAddress < s.VirtualAddress + s.VirtualSize) {
        return &s;
      }
    }
    return nullptr;
  }
  // getSection by name
  const Section *getSection(std::string_view name) const {
    for (const auto &s : sections) {
      if (s.Name == name) {
        return &s;
      }
    }
    return nullptr;
  }
  bool readCOFFSymbols(std::vector<COFFSymbol> &symbols, bela::error_code &ec) const;
  bool readRelocs(Section &sec) const;
  bool readSectionData(const Section &sec, std::vector<char> &data) const;
  bool readSectionData(const Section &sec, SectionBuffer &sb) const;
  bool readStringTable(bela::error_code &ec);
  bool LookupDelayImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;
  bool LookupImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;
  bool lookupImports32(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;
  bool lookupImports64(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const;

public:
  File() = default;
  ~File() {
    if (fd != INVALID_HANDLE_VALUE && needClosed) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  File(const File &) = delete;
  File &operator=(const File &&) = delete;
  // export FD() to support
  HANDLE FD() const { return fd; }
  template <typename AStringT> void SplitStringTable(std::vector<AStringT> &sa) const {
    auto sv = std::string_view{reinterpret_cast<const char *>(stringTable.data), stringTable.length};
    for (;;) {
      auto p = sv.find('\0');
      if (p == std::string_view::npos) {
        if (sv.size() != 0) {
          sa.emplace_back(sv);
        }
        break;
      }
      sa.emplace_back(sv.substr(0, p));
      sv.remove_prefix(p + 1);
    }
  }
  bool LookupExports(std::vector<ExportedSymbol> &exports, bela::error_code &ec) const;
  bool LookupFunctionTable(FunctionTable &ft, bela::error_code &ec) const;
  bool LookupSymbols(std::vector<Symbol> &syms, bela::error_code &ec) const;
  bool LookupOverlay(std::vector<char> &overlayData, bela::error_code &ec, int64_t limitsize = LimitOverlaySize) const;
  std::optional<DotNetMetadata> LookupDotNetMetadata(bela::error_code &ec) const;
  std::optional<Version> LookupVersion(bela::error_code &ec) const; // WIP
  const FileHeader &Fh() const { return fh; }
  const OptionalHeader64 *Oh64() const { return &oh; }
  const OptionalHeader32 *Oh32() const { return reinterpret_cast<const OptionalHeader32 *>(&oh); }
  const auto &Sections() const { return sections; }
  bool Is64Bit() const { return is64bit; }
  bela::pe::Machine Machine() const { return static_cast<bela::pe::Machine>(fh.Machine); }
  bela::pe::Subsystem Subsystem() const {
    return static_cast<bela::pe::Subsystem>(is64bit ? oh.Subsystem : Oh32()->Subsystem);
  }
  // NewFile resolve pe file
  bool NewFile(std::wstring_view p, bela::error_code &ec);
  bool NewFile(HANDLE fd_, int64_t sz, bela::error_code &ec) {
    if (fd != INVALID_HANDLE_VALUE) {
      ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
      return false;
    }
    fd = fd_;
    size = sz;
    return parseFile(ec);
  }
  int64_t Size() const { return size; }
  int64_t OverlayOffset() const { return overlayOffset; }
  int64_t OverlayLength() const { return size - overlayOffset; }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  FileHeader fh;
  int64_t size{SizeUnInitialized};
  // The OptionalHeader64 structure is larger than OptionalHeader32. Therefore, we can store OptionalHeader32 in oh64.
  // Conversion by pointer.
  OptionalHeader64 oh;
  std::vector<Section> sections;
  StringTable stringTable;
  int64_t overlayOffset{-1};
  bool is64bit{false};
  bool needClosed{false};
};

class SymbolSearcher {
private:
  using SymbolTable = bela::flat_hash_map<std::string, std::vector<bela::pe::ExportedSymbol>>;
  SymbolTable table;
  std::vector<std::wstring> Paths;
  std::optional<std::string> LoadOrdinalFunctionName(std::string_view dllname, int ordinal, bela::error_code &ec);

public:
  SymbolSearcher(std::wstring_view exe, Machine machine);
  SymbolSearcher(std::vector<std::wstring> &&paths) : Paths(std::move(paths)) {}
  SymbolSearcher(const SymbolSearcher &) = delete;
  SymbolSearcher &operator=(const SymbolSearcher &) = delete;
  std::optional<std::string> LookupOrdinalFunctionName(std::string_view dllname, int ordinal, bela::error_code &ec);
};

// https://docs.microsoft.com/en-us/windows/win32/api/winver/nf-winver-getfileversioninfoexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-getfileversioninfosizeexw
// https://docs.microsoft.com/zh-cn/windows/win32/api/winver/nf-winver-verqueryvaluew
// version.lib
std::optional<Version> Lookup(std::wstring_view file, bela::error_code &ec);

inline bool IsSubsystemConsole(std::wstring_view p) {
  constexpr const wchar_t *suffix[] = {
      // console suffix
      L".bat", // batch
      L".cmd", // batch
      L".vbs", // Visual Basic script files
      L".vbe", // Visual Basic script files (encrypted)
      L".js",  // JavaScript
      L".jse", // JavaScript (encrypted)
      L".wsf", // WScript
      L".wsh", // Windows Script Host Settings File
  };
  File file;
  bela::error_code ec;
  if (!file.NewFile(p, ec)) {
    auto lp = bela::AsciiStrToLower(p);
    for (const auto s : suffix) {
      if (bela::EndsWith(lp, s)) {
        return true;
      }
    }
    return false;
  }
  return file.Subsystem() == Subsystem::CUI;
}

} // namespace bela::pe

#endif