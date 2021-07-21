//
#ifndef BELA_INTERNAL_IMAGE_HPP
#define BELA_INTERNAL_IMAGE_HPP
#include <cstdint>
#include <string>
#include <bela/base.hpp>
#include <bela/phmap.hpp>
#include <bela/endian.hpp>

namespace bela::pe {
constexpr long ErrNoOverlay = 0xFF01;
constexpr int64_t LimitSectionSize = 256 * 1024 * 1024;

constexpr int DataDirExportTable = 0;            // Export table.
constexpr int DataDirImportTable = 1;            // Import table.
constexpr int DataDirResourceTable = 2;          // Resource table.
constexpr int DataDirExceptionTable = 3;         // Exception table.
constexpr int DataDirCertificateTable = 4;       // Certificate table.
constexpr int DataDirBaseRelocationTable = 5;    // Base relocation table.
constexpr int DataDirDebug = 6;                  // Debugging information.
constexpr int DataDirArchitecture = 7;           // Architecture-specific data.
constexpr int DataDirGlobalPtr = 8;              // Global pointer register.
constexpr int DataDirTLSTable = 9;               // Thread local storage (TLS) table.
constexpr int DataDirLoadConfigTable = 10;       // Load configuration table.
constexpr int DataDirBoundImport = 11;           // Bound import table.
constexpr int DataDirIAT = 12;                   // Import address table.
constexpr int DataDirDelayImportDescriptor = 13; // Delay import descriptor.
constexpr int DataDirCLRHeader = 14;             // CLR header.
constexpr int DataDirReserved = 15;              // Reserved.
constexpr int DataDirEntries = 16;               // Tables count.

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

// Mixed optionalHeader
struct OptionalHeader {
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
  DataDirectory DataDirectory[DataDirEntries];
  uint32_t BaseOfData32; // PE32 only, PE32+ zero
  bool Is64Bit;
  uint8_t Reserved[3];
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
    if (offset + 2 > rawdata.size()) {
      return 0;
    }
    return bela::cast_frombe<uint16_t>(rawdata.data() + offset);
  }

private:
  std::vector<char> rawdata;
};
} // namespace bela::pe

#endif