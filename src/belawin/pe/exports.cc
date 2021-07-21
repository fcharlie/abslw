// exports
#include "internal.hpp"
#include <algorithm>

namespace bela::pe {
bool File::LookupExports(std::vector<ExportedSymbol> &exports, bela::error_code &ec) const {
  auto exd = getDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
  if (exd == nullptr) {
    return true;
  }
  auto ds = getSection(exd);
  if (ds == nullptr) {
    return true;
  }
  auto sdata = readSectionData(*ds, ec);
  if (!sdata) {
    return false;
  }
  // seek to the virtual address specified in the export data directory
  auto N = exd->VirtualAddress - ds->VirtualAddress;
  auto cied = sdata->direct_cast<IMAGE_EXPORT_DIRECTORY>(N);
  if (cied == nullptr) {
    return true;
  }
  IMAGE_EXPORT_DIRECTORY ied;
  if constexpr (bela::IsLittleEndian()) {
    memcpy(&ied, cied, sizeof(IMAGE_EXPORT_DIRECTORY));
  } else {
    ied.Characteristics = bela::fromle(cied->Characteristics);
    ied.TimeDateStamp = bela::fromle(cied->TimeDateStamp);
    ied.MajorVersion = bela::fromle(cied->MajorVersion);
    ied.MinorVersion = bela::fromle(cied->MinorVersion);
    ied.Name = bela::fromle(cied->Name);
    ied.Base = bela::fromle(cied->Base);
    ied.NumberOfFunctions = bela::fromle(cied->NumberOfFunctions);
    ied.NumberOfNames = bela::fromle(cied->NumberOfNames);
    ied.AddressOfFunctions = bela::fromle(cied->AddressOfFunctions);       // RVA from base of image
    ied.AddressOfNames = bela::fromle(cied->AddressOfNames);               // RVA from base of image
    ied.AddressOfNameOrdinals = bela::fromle(cied->AddressOfNameOrdinals); // RVA from base of image
  }
  if (ied.NumberOfNames == 0) {
    return true;
  }
  auto ordinalBase = static_cast<uint16_t>(ied.Base);
  exports.resize(ied.NumberOfNames);
  if (ied.AddressOfNameOrdinals > ds->VirtualAddress &&
      ied.AddressOfNameOrdinals < ds->VirtualAddress + ds->VirtualSize) {
    auto L = ied.AddressOfNameOrdinals - ds->VirtualAddress;
    if (sdata->size() - L > exports.size() * 2) {
      for (size_t i = 0; i < exports.size(); i++) {
        exports[i].Ordinal = sdata->cast_fromle<uint16_t>(L + i * 2) + ordinalBase;
        exports[i].Hint = static_cast<int>(i);
      }
    }
  }
  if (ied.AddressOfNames > ds->VirtualAddress && ied.AddressOfNames < ds->VirtualAddress + ds->VirtualSize) {
    auto N = ied.AddressOfNames - ds->VirtualAddress;
    if (sdata->size() - N >= exports.size() * 4) {
      for (size_t i = 0; i < exports.size(); i++) {
        exports[i].Name = sdata->cstring_view(sdata->cast_fromle<uint32_t>(N + i * 4) - ds->VirtualAddress);
      }
    }
  }
  if (ied.AddressOfFunctions > ds->VirtualAddress && ied.AddressOfFunctions < ds->VirtualAddress + ds->VirtualSize) {
    auto L = ied.AddressOfFunctions - ds->VirtualAddress;
    for (size_t i = 0; i < exports.size(); i++) {
      if (sdata->size() - L > static_cast<size_t>(exports[i].Ordinal * 4 + 4)) {
        exports[i].Address = sdata->cast_fromle<uint32_t>(L + static_cast<int>(exports[i].Ordinal - ordinalBase) * 4);
      }
    }
  }
  std::sort(exports.begin(), exports.end(), [](const ExportedSymbol &a, const ExportedSymbol &b) -> bool {
    //
    return a.Ordinal < b.Ordinal;
  });
  return true;
}

} // namespace bela::pe