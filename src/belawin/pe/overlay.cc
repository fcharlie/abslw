// read pe overlay data
#include "internal.hpp"

namespace bela::pe {
bool File::LookupOverlay(std::vector<char> &overlayData, bela::error_code &ec, int64_t limitsize) const {
  int64_t overlayStart{0};
  for (const auto &sec : sections) {
    if (auto sectionEnd = static_cast<int64_t>(sec.Header.Offset + sec.Header.Size); sectionEnd > overlayStart) {
      overlayStart = sectionEnd;
    }
  }
  auto overlayLen = size - overlayStart;
  if (overlayLen <= 0) {
    ec = bela::make_error_code(ErrNoOverlay, L"no overlay data");
    return false;
  }
  if (overlayLen > limitsize) {
    ec = bela::make_error_code(ErrGeneral, L"overlay data size large over limit");
    return false;
  }
  if (!PositionAt(overlayStart, ec)) {
    return false;
  }
  overlayData.resize(static_cast<size_t>(overlayLen));
  return ReadFull(overlayData.data(), overlayData.size(), ec);
}
} // namespace bela::pe