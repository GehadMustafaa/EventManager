#include "Storage.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

constexpr int kFormatVersion = 2;

std::vector<std::string> split(const std::string &line) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  for (;;) {
    std::size_t pos = line.find('|', start);
    if (pos == std::string::npos) {
      parts.push_back(line.substr(start));
      break;
    }
    parts.push_back(line.substr(start, pos - start));
    start = pos + 1;
  }
  return parts;
}

void requireFields(const std::vector<std::string> &parts, std::size_t count) {
  if (parts.size() != count)
    throw std::runtime_error("malformed record");
}

std::string csvEscape(const std::string &value) {
  if (value.find_first_of(",\"\r\n") == std::string::npos)
    return value;
  std::string out = "\"";
  for (char ch : value) {
    if (ch == '"')
      out += "\"\"";
    else
      out += ch;
  }
  out += '"';
  return out;
}

std::string base64Encode(const std::string &input) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  int val = 0;
  int valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(kAlphabet[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(kAlphabet[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

std::string base64Decode(const std::string &input) {
  static constexpr int kInvalid = -1;
  int table[256];
  std::fill(std::begin(table), std::end(table), kInvalid);
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (int i = 0; i < 64; ++i)
    table[static_cast<unsigned char>(kAlphabet[i])] = i;

  std::string out;
  int val = 0;
  int valb = -8;
  for (unsigned char c : input) {
    if (c == '=')
      break;
    int d = table[c];
    if (d == kInvalid)
      throw std::runtime_error("invalid base64 character");
    val = (val << 6) + d;
    valb += 6;
    if (valb >= 0) {
      out.push_back(static_cast<char>((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}

} // namespace

void Storage::save(const EventManager &manager, std::ostream &out) {
  out << "V|" << kFormatVersion << '\n';

  for (const Attendee &a : manager.attendees())
    out << "A|" << a.id << '|' << a.name << '|' << a.email << '\n';

  for (const Event *e : manager.events()) {
    out << "E|" << e->id() << '|' << e->name() << '|' << e->date() << '|'
        << e->venue() << '|' << e->capacity() << '|'
        << base64Encode(e->description()) << '|' << e->time() << '\n';
    for (int id : e->confirmedIds())
      out << "C|" << e->id() << '|' << id << '\n';
    for (int id : e->waitlistIds())
      out << "W|" << e->id() << '|' << id << '\n';
  }
}

void Storage::save(const EventManager &manager, const std::string &path) {
  std::error_code ec;
  const auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty())
    std::filesystem::create_directories(parent, ec);

  const std::string tmpPath = path + ".tmp";
  {
    std::ofstream file(tmpPath);
    if (!file)
      throw std::runtime_error("cannot open file for writing: " + tmpPath);

    save(manager, file);

    if (!file) {
      std::filesystem::remove(tmpPath, ec);
      throw std::runtime_error("write failed: " + tmpPath);
    }
  }

  ec.clear();
  if (std::filesystem::exists(path, ec)) {
    std::filesystem::copy_file(
        path, backupPath(path),
        std::filesystem::copy_options::overwrite_existing, ec);
  }

  ec.clear();
  std::filesystem::rename(tmpPath, path, ec);
  if (ec) {
    std::filesystem::remove(tmpPath, ec);
    throw std::runtime_error("cannot replace file: " + path);
  }
}

std::string Storage::backupPath(const std::string &path) {
  return path + ".bak";
}

bool Storage::hasBackup(const std::string &path) {
  std::error_code ec;
  return std::filesystem::exists(backupPath(path), ec);
}

void Storage::restoreBackup(EventManager &manager, const std::string &path) {
  const std::string bak = backupPath(path);
  if (!hasBackup(path))
    throw std::runtime_error("no backup file found: " + bak);
  load(manager, bak);
  save(manager, path);
}

void Storage::load(EventManager &manager, std::istream &in) {
  manager.clear();
  std::string line;
  bool firstRecord = true;
  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    std::vector<std::string> p = split(line);
    const std::string &tag = p[0];
    try {
      if (tag == "V") {
        if (!firstRecord)
          throw std::runtime_error("unexpected version header");
        requireFields(p, 2);
        int version = std::stoi(p[1]);
        if (version < 1 || version > kFormatVersion)
          throw std::runtime_error("unsupported format version");
        firstRecord = false;
        continue;
      }
      firstRecord = false;
      if (tag == "A") {
        requireFields(p, 4);
        manager.restoreAttendee(Attendee{std::stoi(p[1]), p[2], p[3]});
      } else if (tag == "E") {
        if (p.size() < 6 || p.size() > 8)
          throw std::runtime_error("malformed record");
        std::string desc =
            p.size() >= 7 && !p[6].empty() ? base64Decode(p[6]) : std::string{};
        std::string time =
            p.size() == 8 && !p[7].empty() ? p[7] : std::string("09:00 AM");
        manager.restoreEvent(std::stoi(p[1]), p[2], p[3], p[4], std::stoi(p[5]),
                             desc, time);
      } else if (tag == "C") {
        requireFields(p, 3);
        manager.restoreConfirmed(std::stoi(p[1]), std::stoi(p[2]));
      } else if (tag == "W") {
        requireFields(p, 3);
        manager.restoreWaitlisted(std::stoi(p[1]), std::stoi(p[2]));
      } else {
        throw std::runtime_error("unknown record type");
      }
    } catch (const std::exception &ex) {
      manager.clear();
      throw std::runtime_error(std::string("load failed at '") + line +
                               "': " + ex.what());
    }
  }
}

void Storage::load(EventManager &manager, const std::string &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("cannot open file for reading: " + path);
  load(manager, file);
}

void Storage::exportCsv(const EventManager &manager, const std::string &path) {
  std::error_code ec;
  const auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty())
    std::filesystem::create_directories(parent, ec);

  std::ofstream file(path);
  if (!file)
    throw std::runtime_error("cannot open CSV file for writing: " + path);

  file << "Section,ID,Field1,Field2,Field3,Field4,Field5,Field6\n";
  for (const Event *e : manager.events()) {
    file << "Event," << e->id() << ',' << csvEscape(e->name()) << ','
         << csvEscape(e->date()) << ',' << csvEscape(e->time()) << ','
         << csvEscape(e->venue()) << ',' << e->capacity() << ','
         << csvEscape(e->description()) << '\n';
  }
  for (const Attendee &a : manager.attendees()) {
    file << "Attendee," << a.id << ',' << csvEscape(a.name) << ','
         << csvEscape(a.email) << ",,,,\n";
  }
  for (const Event *e : manager.events()) {
    for (int id : e->confirmedIds()) {
      file << "Booking," << e->id() << ',' << id << ",Confirmed,,,,\n";
    }
    int pos = 1;
    for (int id : e->waitlistIds()) {
      file << "Booking," << e->id() << ',' << id << ",Waitlisted," << pos++
           << ",,,\n";
    }
  }
  if (!file)
    throw std::runtime_error("CSV write failed: " + path);
}
