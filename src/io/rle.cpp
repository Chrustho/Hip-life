#include "rle.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace hl {

namespace {

bool parse_header(const std::string& line, int& w, int& h, bool& has_rule, Rule& rule) {
    // "x = 3, y = 3, rule = B3/S23"
    w = h = 0;
    has_rule = false;
    size_t i = 0;
    auto skip_to_value = [&](char key) -> bool {
        while (i < line.size()) {
            if (std::tolower(line[i]) == key) {
                ++i;
                while (i < line.size() && (line[i] == ' ' || line[i] == '=')) ++i;
                return true;
            }
            ++i;
        }
        return false;
    };
    if (!skip_to_value('x')) return false;
    w = std::atoi(&line[i]);
    if (!skip_to_value('y')) return false;
    h = std::atoi(&line[i]);
    size_t r = line.find("rule");
    if (r != std::string::npos) {
        size_t eq = line.find('=', r);
        if (eq != std::string::npos) {
            std::string rest = line.substr(eq + 1);
            size_t start = rest.find_first_not_of(' ');
            if (start != std::string::npos) rest = rest.substr(start);
            Rule parsed{0, 0};
            if (rule_from_string(rest, parsed)) {
                rule = parsed;
                has_rule = true;
            }
        }
    }
    return w > 0 && h > 0;
}

}

std::optional<Pattern> parse_rle(const std::string& text, std::string& err) {
    std::istringstream in(text);
    std::string line;
    Pattern p;
    bool header_seen = false;
    std::string body;

    while (std::getline(in, line)) {
        if (!line.empty() && line[0] == '#') continue;
        if (!header_seen) {
            if (!parse_header(line, p.w, p.h, p.has_rule, p.rule)) {
                err = "invalid RLE header";
                return std::nullopt;
            }
            header_seen = true;
            p.cells.assign(size_t(p.w) * p.h, 0);
        } else {
            body += line;
        }
    }
    if (!header_seen) {
        err = "missing RLE header line";
        return std::nullopt;
    }

    int x = 0, y = 0, count = 0;
    for (char c : body) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            count = count * 10 + (c - '0');
        } else if (c == 'b' || c == 'o') {
            int n = count ? count : 1;
            uint8_t v = (c == 'o') ? 1 : 0;
            for (int k = 0; k < n; ++k) {
                if (x < p.w && y < p.h) p.cells[size_t(y) * p.w + x] = v;
                ++x;
            }
            count = 0;
        } else if (c == '$') {
            y += count ? count : 1;
            x = 0;
            count = 0;
        } else if (c == '!') {
            break;
        }
    }
    return p;
}

std::string write_rle(const Pattern& p, const std::string& name) {
    std::ostringstream out;
    if (!name.empty()) out << "#N " << name << "\n";
    out << "x = " << p.w << ", y = " << p.h
        << ", rule = " << rule_to_string(p.has_rule ? p.rule : rules::conway) << "\n";

    std::string line;
    auto flush_token = [&](int n, char tag) {
        std::string tok = (n > 1 ? std::to_string(n) : "") + tag;
        if (line.size() + tok.size() > 70) {
            out << line << "\n";
            line.clear();
        }
        line += tok;
    };

    for (int y = 0; y < p.h; ++y) {
        int last_alive = -1;
        for (int x = 0; x < p.w; ++x)
            if (p.at(x, y)) last_alive = x;
        int x = 0;
        while (x <= last_alive) {
            uint8_t v = p.at(x, y);
            int run = 1;
            while (x + run <= last_alive && p.at(x + run, y) == v) ++run;
            flush_token(run, v ? 'o' : 'b');
            x += run;
        }
        if (y + 1 < p.h) flush_token(1, '$');
    }
    line += "!";
    out << line << "\n";
    return out.str();
}

std::optional<Pattern> load_rle_file(const std::string& path, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot open " + path;
        return std::nullopt;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return parse_rle(ss.str(), err);
}

bool save_rle_file(const std::string& path, const Pattern& p, const std::string& name, std::string& err) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot write " + path;
        return false;
    }
    f << write_rle(p, name);
    return static_cast<bool>(f);
}

}
