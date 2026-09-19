#include "CsvParser.hpp"

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

struct MappedFile {
    char* data = nullptr;
    size_t size = 0;
    explicit MappedFile(const std::string& path) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) return;
        struct stat st {};
        if (fstat(fd, &st) != 0) {
            close(fd);
            return;
        }
        size = static_cast<size_t>(st.st_size);
        void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (mapped == MAP_FAILED) {
            data = nullptr;
            size = 0;
            return;
        }
        data = static_cast<char*>(mapped);
    }
    ~MappedFile() {
        if (data) munmap(data, size);
    }
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;
};

inline const char* findChar(const char* p, const char* end, char c) {
    p = static_cast<const char*>(memchr(p, c, static_cast<size_t>(end - p)));
    return p ? p : end;
}

inline int64_t parseTimeMs(const char* s, const char* end) {
    // "DD.MM.YYYY HH:MM:SS.mmm" — skip the date and read the clock.
    const char* space = findChar(s, end, ' ');
    if (space == end || space + 12 > end) return -1;
    const char* t = space + 1;
    int h = (t[0] - '0') * 10 + (t[1] - '0');
    int m = (t[3] - '0') * 10 + (t[4] - '0');
    int sec = (t[6] - '0') * 10 + (t[7] - '0');
    int ms = (t[9] - '0') * 100 + (t[10] - '0') * 10 + (t[11] - '0');
    return int64_t(h) * 3600000 + int64_t(m) * 60000 + int64_t(sec) * 1000 + ms;
}

inline double parseDouble(const char* s, const char* end) {
    while (s < end && (*s == ' ' || *s == '\t')) ++s;
    double sign = 1.0;
    if (s < end && *s == '-') {
        sign = -1.0;
        ++s;
    }
    double v = 0.0;
    while (s < end && *s >= '0' && *s <= '9') {
        v = v * 10.0 + (*s - '0');
        ++s;
    }
    if (s < end && *s == '.') {
        ++s;
        double place = 0.1;
        while (s < end && *s >= '0' && *s <= '9') {
            v += (*s - '0') * place;
            place *= 0.1;
            ++s;
        }
    }
    return sign * v;
}

inline const char* nextLine(const char* p, const char* end) {
    p = findChar(p, end, '\n');
    if (p == end) return end;
    return p + 1;
}

}  // namespace

std::vector<CurrencyPairData> readCurrencyPairCsvs(
    const std::string& bidFilepath,
    const std::string& askFilepath) {
    MappedFile bidFile(bidFilepath);
    MappedFile askFile(askFilepath);
    if (!bidFile.data || !askFile.data) return {};

    const char* bp = nextLine(bidFile.data, bidFile.data + bidFile.size);
    const char* ap = nextLine(askFile.data, askFile.data + askFile.size);
    const char* be = bidFile.data + bidFile.size;
    const char* ae = askFile.data + askFile.size;

    std::vector<CurrencyPairData> out;
    out.reserve(std::min(bidFile.size, askFile.size) / 70);

    while (bp < be && ap < ae) {
        const char* bEnd = findChar(bp, be, '\n');
        const char* aEnd = findChar(ap, ae, '\n');
        const char* bLineEnd = (bEnd > bp && *(bEnd - 1) == '\r') ? bEnd - 1 : bEnd;
        const char* aLineEnd = (aEnd > ap && *(aEnd - 1) == '\r') ? aEnd - 1 : aEnd;
        if (bLineEnd <= bp || aLineEnd <= ap) break;

        const char* bC0 = findChar(bp, bLineEnd, ',');
        const char* aC0 = findChar(ap, aLineEnd, ',');
        if (bC0 == bLineEnd || aC0 == aLineEnd) {
            bp = (bEnd == be) ? be : bEnd + 1;
            ap = (aEnd == ae) ? ae : aEnd + 1;
            continue;
        }

        int64_t tsB = parseTimeMs(bp, bC0);
        int64_t tsA = parseTimeMs(ap, aC0);
        if (tsB >= 0 && tsB == tsA) {
            const char* col = bC0 + 1;
            for (int i = 0; i < 3 && col < bLineEnd; ++i) col = findChar(col, bLineEnd, ',') + 1;
            const char* colEnd = findChar(col, bLineEnd, ',');
            double bid = parseDouble(col, colEnd);

            col = aC0 + 1;
            for (int i = 0; i < 3 && col < aLineEnd; ++i) col = findChar(col, aLineEnd, ',') + 1;
            colEnd = findChar(col, aLineEnd, ',');
            double ask = parseDouble(col, colEnd);

            if (bid > 0.0 && ask > 0.0) out.push_back({tsB, bid, ask});
        }

        bp = (bEnd == be) ? be : bEnd + 1;
        ap = (aEnd == ae) ? ae : aEnd + 1;
    }
    return out;
}
