///
#include <hazel/zip.hpp>
#include <hazel/hazel.hpp>
#include <bela/path.hpp>
#include <bela/endian.hpp>
#include <bela/algorithm.hpp>
#include <bela/bufio.hpp>
#include <numeric>

namespace hazel::zip {
// https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
// https://en.wikipedia.org/wiki/ZIP_(file_format)
// https://en.wikipedia.org/wiki/Comparison_of_file_archivers
// https://en.wikipedia.org/wiki/List_of_archive_formats
constexpr int fileHeaderSignature = 0x04034b50;
constexpr int directoryHeaderSignature = 0x02014b50;
constexpr int directoryEndSignature = 0x06054b50;
constexpr int directory64LocSignature = 0x07064b50;
constexpr int directory64EndSignature = 0x06064b50;
constexpr uint32_t dataDescriptorSignature = 0x08074b50; // de-facto standard; required by OS X Finder
constexpr uint32_t fileHeaderLen = 30;                   // + filename + extra
constexpr int directoryHeaderLen = 46;                   // + filename + extra + comment
constexpr int directoryEndLen = 22;                      // + comment
constexpr int dataDescriptorLen = 16;   // four uint32: descriptor signature, crc32, compressed size, size
constexpr int dataDescriptor64Len = 24; // descriptor with 8 byte sizes
constexpr int directory64LocLen = 20;   //
constexpr int directory64EndLen = 56;   // + extra

// Constants for the first byte in CreatorVersion.
constexpr int creatorFAT = 0;
constexpr int creatorUnix = 3;
constexpr int creatorNTFS = 11;
constexpr int creatorVFAT = 14;
constexpr int creatorMacOSX = 19;

// Version numbers.
constexpr int zipVersion20 = 20; // 2.0
constexpr int zipVersion45 = 45; // 4.5 (reads and writes zip64 archives)

// Limits for non zip64 files.
constexpr auto uint16max = (std::numeric_limits<uint16_t>::max)();
constexpr auto uint32max = (std::numeric_limits<uint32_t>::max)();

// Extra header IDs.
//
// IDs 0..31 are reserved for official use by PKWARE.
// IDs above that range are defined by third-party vendors.
// Since ZIP lacked high precision timestamps (nor a official specification
// of the timezone used for the date fields), many competing extra fields
// have been invented. Pervasive use effectively makes them "official".
//
// See http://mdfs.net/Docs/Comp/Archiving/Zip/ExtraField
constexpr int zip64ExtraID = 0x0001;       // Zip64 extended information
constexpr int ntfsExtraID = 0x000a;        // NTFS
constexpr int unixExtraID = 0x000d;        // UNIX
constexpr int extTimeExtraID = 0x5455;     // Extended timestamp
constexpr int infoZipUnixExtraID = 0x5855; // Info-ZIP Unix extension
constexpr int winzipAesExtraID = 0x9901;   // winzip AES Extra Field

inline bool IsSuperficialPath(std::string_view sv) {
  auto pv = bela::SplitPath(sv);
  return pv.size() <= 3;
}

int findSignatureInBlock(const bela::Buffer &b) {
  for (auto i = static_cast<int>(b.size()) - directoryEndLen; i >= 0; i--) {
    if (b[i] == 'P' && b[i + 1] == 'K' && b[i + 2] == 0x05 && b[i + 3] == 0x06) {
      auto n = static_cast<int>(b[i + directoryEndLen - 2]) | (static_cast<int>(b[i + directoryEndLen - 1]) << 8);
      if (n + directoryEndLen + i <= static_cast<int>(b.size())) {
        return i;
      }
    }
  }
  return -1;
}
bool Reader::readDirectory64End(int64_t offset, directoryEnd &d, bela::error_code &ec) {
  bela::Buffer buf(directory64EndLen);
  if (!ReadAt(buf, directory64EndLen, offset, ec)) {
    return false;
  }
  bela::endian::LittenEndian b(buf.data(), buf.size());

  if (auto sig = b.Read<uint32_t>(); sig != directory64EndSignature) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  b.Discard(16);
  d.diskNbr = b.Read<uint32_t>();    // number of this disk
  d.dirDiskNbr = b.Read<uint32_t>(); // number of the disk with the start of the central directory
                                     // total number of entries in the central directory on this disk
  d.dirRecordsThisDisk = b.Read<uint64_t>();
  d.directoryRecords = b.Read<uint64_t>(); // total number of entries in the central directory
  d.directorySize = b.Read<uint64_t>();    // size of the central directory
  // offset of start of central directory with respect to the starting disk number
  d.directoryOffset = b.Read<uint64_t>();
  return true;
}

int64_t Reader::findDirectory64End(int64_t directoryEndOffset, bela::error_code &ec) {
  auto locOffset = directoryEndOffset - directory64LocLen;
  if (locOffset < 0) {
    return -1;
  }
  bela::Buffer buf(directory64LocLen);
  if (!ReadAt(buf, directory64LocLen, locOffset, ec)) {
    return -1;
  }
  bela::endian::LittenEndian b(buf.data(), buf.size());
  if (auto sig = b.Read<uint32_t>(); sig != directory64LocSignature) {
    return -1;
  }
  if (b.Read<uint32_t>() != 0) {
    return -1;
  }
  auto p = b.Read<uint64_t>();
  if (b.Read<uint32_t>() != 1) {
    return -1;
  }
  return static_cast<int64_t>(p);
}

// github.com\klauspost\compress@v1.11.3\zip\reader.go
bool Reader::readDirectoryEnd(directoryEnd &d, bela::error_code &ec) {
  bela::Buffer buf(16 * 1024);
  int64_t directoryEndOffset = 0;
  constexpr int64_t offrange[] = {1024, 65 * 1024};
  bela::endian::LittenEndian b;
  for (size_t i = 0; i < bela::ArrayLength(offrange); i++) {
    auto blen = offrange[i];
    if (blen > size) {
      blen = size;
    }
    buf.grow(blen);
    if (!ReadAt(buf, blen, size - blen, ec)) {
      return false;
    }
    if (auto p = findSignatureInBlock(buf); p >= 0) {
      b.Reset(reinterpret_cast<const char *>(buf.data()) + p, buf.size() - p);
      directoryEndOffset = size - blen + p;
      break;
    }
    if (i == 1 || blen == size) {
      ec = bela::make_error_code(L"zip: not a valid zip file");
      return false;
    }
  }
  b.Discard(4);
  d.diskNbr = b.Read<uint16_t>();
  d.dirDiskNbr = b.Read<uint16_t>();
  d.dirRecordsThisDisk = b.Read<uint16_t>();
  d.directoryRecords = b.Read<uint16_t>();
  d.directorySize = b.Read<uint32_t>();
  d.directoryOffset = b.Read<uint32_t>();
  d.commentLen = b.Read<uint16_t>();
  if (static_cast<size_t>(d.commentLen) > b.Size()) {
    ec = bela::make_error_code(L"zip: invalid comment length");
    return false;
  }
  d.comment.assign(b.Data(), d.commentLen);
  if (d.directoryRecords == 0xFFFF || d.directorySize == 0xFFFF || d.directoryOffset == 0xFFFFFFFF) {
    ec.clear();
    auto p = findDirectory64End(directoryEndOffset, ec);
    if (!ec && p > 0) {
      readDirectory64End(p, d, ec);
    }
    if (ec) {
      return false;
    }
  }
  if (auto o = static_cast<int64_t>(d.directoryOffset); o < 0 || 0 >= size) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  return true;
}
using bufioReader = bela::bufio::Reader<4096>;

constexpr uint32_t SizeMin = 0xFFFFFFFFu;
constexpr uint64_t OffsetMin = 0xFFFFFFFFull;

// Thanks github.com\klauspost\compress@v1.11.3\zip\reader.go

bool readDirectoryHeader(bufioReader &br, bela::Buffer &buffer, File &file, bela::error_code &ec) {
  uint8_t buf[directoryHeaderLen];
  if (br.ReadFull(buf, sizeof(buf), ec) != sizeof(buf)) {
    return false;
  }
  bela::endian::LittenEndian b(buf, sizeof(buf));
  if (auto n = static_cast<int>(b.Read<uint32_t>()); n != directoryHeaderSignature) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  file.cversion = b.Read<uint16_t>();
  file.rversion = b.Read<uint16_t>();
  file.flags = b.Read<uint16_t>();
  file.method = b.Read<uint16_t>();
  auto dosTime = b.Read<uint16_t>();
  auto dosDate = b.Read<uint16_t>();
  file.crc32 = b.Read<uint32_t>();
  file.compressedSize = b.Read<uint32_t>();
  file.uncompressedSize = b.Read<uint32_t>();
  auto filenameLen = b.Read<uint16_t>();
  auto extraLen = b.Read<uint16_t>();
  auto commentLen = b.Read<uint16_t>();
  b.Discard(4);
  file.externalAttrs = b.Read<uint32_t>();
  file.position = b.Read<uint32_t>();
  auto totallen = filenameLen + extraLen + commentLen;
  buffer.grow(totallen);
  if (br.ReadFull(buffer.data(), totallen, ec) != totallen) {
    return false;
  }
  file.name.assign(reinterpret_cast<const char *>(buffer.data()), filenameLen);
  file.extra.assign(reinterpret_cast<const char *>(buffer.data() + filenameLen), extraLen);
  file.comment.assign(reinterpret_cast<const char *>(buffer.data() + filenameLen + extraLen), commentLen);
  auto needUSize = file.uncompressedSize == SizeMin;
  auto needSize = file.compressedSize == SizeMin;
  auto needOffset = file.position == OffsetMin;
  file.utf8 = (file.flags & 0x800) != 0;

  bela::Time modified;

  bela::endian::LittenEndian extra(file.extra.data(), file.extra.size());
  for (; extra.Size() >= 4;) {
    auto fieldTag = extra.Read<uint16_t>();
    auto fieldSize = static_cast<int>(extra.Read<uint16_t>());
    if (extra.Size() < fieldSize) {
      break;
    }
    auto fb = extra.Sub(fieldSize);
    if (fieldTag == zip64ExtraID) {
      if (needUSize) {
        needUSize = false;
        if (fb.Size() < 8) {
          ec = bela::make_error_code(L"zip: not a valid zip file");
          return false;
        }
        file.uncompressedSize = fb.Read<uint64_t>();
      }
      if (needSize) {
        needSize = false;
        if (fb.Size() < 8) {
          ec = bela::make_error_code(L"zip: not a valid zip file");
          return false;
        }
        file.compressedSize = fb.Read<uint64_t>();
      }
      if (needOffset) {
        needOffset = false;
        if (fb.Size() < 8) {
          ec = bela::make_error_code(L"zip: not a valid zip file");
          return false;
        }
        file.position = fb.Read<uint64_t>();
      }
      continue;
    }
    if (fieldTag == ntfsExtraID) {
      if (fb.Size() < 4) {
        continue;
      }
      fb.Discard(4);
      for (; fb.Size() >= 4;) {
        auto attrTag = fb.Read<uint16_t>();
        auto attrSize = fb.Read<uint16_t>();
        if (fb.Size() < attrSize) {
          break;
        }
        auto ab = fb.Sub(attrSize);
        if (attrTag != 1 || attrSize != 24) {
          break;
        }
        modified = bela::FromWindowsPreciseTime(ab.Read<uint64_t>());
      }
      continue;
    }
    if (fieldTag == unixExtraID || fieldTag == infoZipUnixExtraID) {
      if (fb.Size() < 8) {
        continue;
      }
      fb.Discard(4);
      file.time = bela::FromUnixSeconds(static_cast<int64_t>(fb.Read<uint32_t>()));
      continue;
    }
    if (fieldTag == extTimeExtraID) {
      if (fb.Size() < 5 || (fb.Pick() & 1) == 0) {
        continue;
      }
      modified = bela::FromUnixSeconds(static_cast<int64_t>(fb.Read<uint32_t>()));
      continue;
    }
    // https://www.winzip.com/win/en/aes_info.html
    if (fieldTag == winzipAesExtraID) {
      if (fb.Size() < 7) {
        continue;
      }
      file.aesVersion = fb.Read<uint16_t>();
      fb.Discard(2); // VendorID 'AE'
      file.aesStrength = fb.Pick();
      file.method = fb.Read<uint16_t>();
      continue;
    }
    ///
  }
  file.time = bela::FromDosDateTime(dosDate, dosTime);
  if (bela::ToUnixSeconds(modified) != 0) {
    file.time = modified;
  }
  if (needSize || needOffset) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  return true;
}

bool Reader::Initialize(bela::error_code &ec) {
  directoryEnd d;
  if (!readDirectoryEnd(d, ec)) {
    return false;
  }
  if (d.directoryRecords > static_cast<uint64_t>(size) / fileHeaderLen) {
    ec = bela::make_error_code(1, L"zip: TOC declares impossible ", d.directoryRecords, L" files in ", size,
                               L" byte zip");
    return false;
  }
  comment.assign(std::move(d.comment));
  files.reserve(d.directoryRecords);
  if (!PositionAt(d.directoryOffset, ec)) {
    return false;
  }
  bela::Buffer buffer(16 * 1024);
  bufioReader br(fd);
  for (uint64_t i = 0; i < d.directoryRecords; i++) {
    File file;
    if (!readDirectoryHeader(br, buffer, file, ec)) {
      return false;
    }
    uncompressedSize += file.uncompressedSize;
    compressedSize += file.compressedSize;
    files.emplace_back(std::move(file));
  }
  return true;
}

bool Reader::OpenReader(std::wstring_view file, bela::error_code &ec) {
  if (fd != INVALID_HANDLE_VALUE) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd = CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  needClosed = true;
  LARGE_INTEGER li;
  if (!GetFileSizeEx(fd, &li)) {
    ec = bela::make_system_error_code(L"GetFileSizeEx: ");
    return false;
  }
  size = li.QuadPart;
  return Initialize(ec);
}

bool Reader::OpenReader(HANDLE nfd, int64_t sz, bela::error_code &ec) {
  if (fd != INVALID_HANDLE_VALUE) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd = nfd;
  size = sz;
  if (size == bela::SizeUnInitialized) {
    LARGE_INTEGER li;
    if (!GetFileSizeEx(fd, &li)) {
      ec = bela::make_system_error_code(L"GetFileSizeEx: ");
      return false;
    }
    size = li.QuadPart;
  }
  return Initialize(ec);
}

const wchar_t *Method(uint16_t m) {
  struct method_kv_t {
    hazel::zip::zip_method_t m;
    const wchar_t *name;
  };
  constexpr const method_kv_t methods[] = {
      {zip_method_t::ZIP_STORE, L"store"},
      {zip_method_t::ZIP_SHRINK, L"shrunk"},
      {zip_method_t::ZIP_REDUCE_1, L"ZIP_REDUCE_1"},
      {zip_method_t::ZIP_REDUCE_2, L"ZIP_REDUCE_2"},
      {zip_method_t::ZIP_REDUCE_3, L"ZIP_REDUCE_3"},
      {zip_method_t::ZIP_REDUCE_4, L"ZIP_REDUCE_4"},
      {zip_method_t::ZIP_IMPLODE, L"IMPLODE"},
      {zip_method_t::ZIP_DEFLATE, L"deflate"},
      {zip_method_t::ZIP_DEFLATE64, L"deflate64"},
      {zip_method_t::ZIP_PKWARE_IMPLODE, L"ZIP_PKWARE_IMPLODE"},
      {zip_method_t::ZIP_BZIP2, L"bzip2"},
      {zip_method_t::ZIP_LZMA, L"lzma"},
      {zip_method_t::ZIP_TERSE, L"IBM TERSE"},
      {zip_method_t::ZIP_LZ77, L"LZ77"},
      {zip_method_t::ZIP_LZMA2, L"lzma2"},
      {zip_method_t::ZIP_ZSTD, L"zstd"},
      {zip_method_t::ZIP_XZ, L"xz"},
      {zip_method_t::ZIP_JPEG, L"Jpeg"},
      {zip_method_t::ZIP_WAVPACK, L"WavPack"},
      {zip_method_t::ZIP_PPMD, L"PPMd"},
      {zip_method_t::ZIP_AES, L"AES"},
  };
  for (const auto &i : methods) {
    if (static_cast<uint16_t>(i.m) == m) {
      return i.name;
    }
  }
  return L"NONE";
}

} // namespace hazel::zip