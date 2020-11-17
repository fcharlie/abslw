//
#include "internal.hpp"

namespace bela::pe {

void swaple(FileHeader &fh) {
  if constexpr (bela::IsBigEndian()) {
    fh.Characteristics = bela::swaple(fh.Characteristics);
    fh.Machine = static_cast<Machine>(bela::swaple(fh.Machine));
    fh.NumberOfSections = bela::swaple(fh.NumberOfSections);
    fh.NumberOfSymbols = bela::swaple(fh.NumberOfSymbols);
    fh.PointerToSymbolTable = bela::swaple(fh.PointerToSymbolTable);
    fh.TimeDateStamp = bela::swaple(fh.TimeDateStamp);
    fh.SizeOfOptionalHeader = bela::swaple(fh.SizeOfOptionalHeader);
  }
}

void File::Free() {
  if (fd != nullptr) {
    fclose(fd);
    fd = nullptr;
  }
}

void File::FileMove(File &&other) {
  Free();
  fd = other.fd;
  is64bit = other.is64bit;
  other.fd = nullptr;
  coffsymbol = std::move(other.coffsymbol);
  stringTable = std::move(other.stringTable);
  memcpy(&fh, &other.fh, sizeof(FileHeader));
}
// constexpr auto o64 = sizeof(OptionalHeader64);
// constexpr auto o32 = sizeof(OptionalHeader32);
std::optional<File> File::NewFile(std::wstring_view p, bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eno = _wfopen_s(&fd, p.data(), L"rb"); eno != 0) {
    ec = bela::make_stdc_error_code(eno, L"open file: ");
    return std::nullopt;
  }
  File file;
  file.fd = fd;
  DosHeader dh;
  if (fread(&dh, 1, sizeof(DosHeader), fd) != sizeof(DosHeader)) {
    ec = bela::make_stdc_error_code(ferror(fd), L"open file: ");
    return std::nullopt;
  }
  constexpr auto x = 0x3c;

  int64_t base = 0;
  if (bela::swaple(dh.e_magic) == IMAGE_DOS_SIGNATURE) {
    auto signoff = static_cast<int64_t>(bela::swaple(dh.e_lfanew));
    uint8_t sign[4];
    if (auto eno = _fseeki64(fd, signoff, SEEK_SET); eno != 0) {
      ec = bela::make_stdc_error_code(eno, L"Invalid PE COFF file signature of ");
      return std::nullopt;
    }
    if (fread(sign, 1, 4, fd) != 4) {
      ec = bela::make_stdc_error_code(ferror(fd), L"Invalid PE COFF file signature of ");
      return std::nullopt;
    }
    if (!(sign[0] == 'P' && sign[1] == 'E' && sign[2] == 0 && sign[3] == 0)) {
      ec = bela::make_error_code(1, L"Invalid PE COFF file signature of ['", int(sign[0]), L"','", int(sign[1]), L"','",
                                 int(sign[2]), L"','", int(sign[3]), L"']");
      return std::nullopt;
    }
    base = signoff + 4;
  }
  if (auto eno = _fseeki64(fd, base, SEEK_SET); eno != 0) {
    ec = bela::make_stdc_error_code(eno, L"unable seek to base");
    return std::nullopt;
  }
  if (fread(&file.fh, 1, sizeof(FileHeader), file.fd) != sizeof(FileHeader)) {
    ec = bela::make_stdc_error_code(ferror(file.fd), L"Invalid PE COFF file FileHeader ");
    return std::nullopt;
  }
  swaple(file.fh);
  file.is64bit = (file.fh.SizeOfOptionalHeader == sizeof(OptionalHeader64));
  if (!readStringTable(&file.fh, fd, file.stringTable, ec)) {
    return std::nullopt;
  }
  if (!readCOFFSymbols(&file.fh, file.fd, file.coffsymbol, ec)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(file));
}
} // namespace bela::pe