/*
The algorithm keeps track of the row/col position (and horz/vert direction) of
the letters on the grid. Knowing the horz/vert direction means we only need to
test the opposite direction when adding a word crossing that letter.

An initial word is placed at position 0,0 (randomly horz or vert).
Then random words are placed, connected to already-placed words, until MaxTries
words cannot be placed, at which point it gives up.

To place a word, it's letters are scanned and looked up in the letter table.
Then those positions are tested to see if the word can be placed there.
When a letter in the letter table is "double-crossed", it is removed from the
table since it can't be used again.

Problems:

This may leave some empty spots, although it seems to work pretty well.
To fill empty spots, the grid could be scanned for them, a new seed word
could be placed and the algorithm restarted.

It might also be good to prune the letter table a little more since even some
letters which obviously can't be crossed (due to close letters on either side)
are in the list.
*/

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <cctype>
using namespace std;

const int ShortestWord = 3, LongestWord = 8;
const int MaxTries = 10000;
const string WordFile  = "wordlist.txt";
const string WordFile2 = "wordlist2.txt"; // selected with -2 option

// Global Random Engine
default_random_engine RndEngine{ random_device{}() };

class WordList {
    using Dist = uniform_int_distribution<int>;
public:
    WordList(const string& filename) {
        ifstream in(filename);
        if (!in) throw runtime_error("Cannot open " + filename);
        for (string word; in >> word; )
            if (word.size() >= ShortestWord && word.size() <= LongestWord)
                m_words.push_back(word);
        m_dist.param(Dist::param_type(0, m_words.size() - 1));
    }
    string rnd_word() const {
        auto& d = m_dist;
        return m_words[const_cast<Dist&>(d)(RndEngine)];
    }
private:
    vector<string> m_words;
    Dist m_dist;
};

class Dir {
public:
    enum DirT { Horz, Vert };
    static Dir rnd_dir() {
        static uniform_int_distribution<> distDir(0, 1);
        return Dir(distDir(RndEngine) ? Horz : Vert);
    }
    Dir(DirT dir) : m_dir(dir) { }
    bool horz() const { return m_dir == Horz; }
    bool vert() const { return m_dir == Vert; }
    Dir operator!() const { return m_dir == Horz ? Vert : Horz; }
    friend ostream& operator<<(ostream& out, Dir dir) {
        return out << (dir.m_dir == Horz ? 'H' : 'V');
    }
private:
    DirT m_dir;
};

struct Point {
    int row, col;
    Dir dir;
    Point(int r, int c, Dir d) : row(r), col(c), dir(d) { }
    bool operator==(const Point& p) const
         { return row == p.row && col == p.col; }
    bool operator!=(const Point& p) const { return !(*this == p); }
};

class Letter {
public:
    using iterator = vector<Point>::iterator;

    void push_back(Point p) { pos.push_back(p); }
    auto begin() { return pos.begin(); }
    auto begin() const { return pos.begin(); }
    auto end() { return pos.end(); }
    auto end() const { return pos.end(); }
    void erase(iterator p) { pos.erase(p); }
private:
    vector<Point> pos;
};

class Letters {
public:
    Letter& operator[](char ch) {
        int i = toupper(ch) - 'A';
        if (i < 0 || i > 25) throw invalid_argument("Bad index");
        return m_letters[i];
    }
    const Letter& operator[](char ch) const {
        return (*const_cast<Letters*>(this))[ch];
    }
    void dump() const {
        for (char ch = 'A'; ch <= 'Z'; ++ch) {
            cout << ch << ": ";
            const auto& letter = (*this)[ch];
            for (const auto& p: letter)
                cout << p.row << ',' << p.col << ',' << p.dir << "  ";
            cout << '\n';
        }
    }
private:
    Letter m_letters[26];
};

class Grid {
    void add_word(const string& word, int w, Point cross_pnt);
    void place_initial_word(const string& word);
    bool can_place(const string& word, int w, const Letter::iterator p);
    bool place_crossing_word(const string& word);
public:
    static constexpr char Empty = '.';

    Grid(int size) : m_size(size), m_grid(new char[size * size])
        { fill((char*)m_grid, (char*)m_grid + size * size, Empty); }
    Grid(const Grid&) = delete;
    ~Grid() { delete[] m_grid; }

    char* operator[](size_t row) { return &m_grid[row * m_size]; }
    char& operator[](const Point& p) { return m_grid[p.row * m_size + p.col]; }
    int size() const { return m_size; }
    bool empty(int r, int c) const { return m_grid[r * m_size + c] == Empty; }

    void generate(const WordList& wordlist);
    void print() const;
private:
    int   m_size = 0;
    char *m_grid = nullptr;
    Letters m_letters;
};

void Grid::print() const {
    for (int r = 0; r < m_size; ++r) {
        for (int c = 0; c < m_size; ++c) {
            char ch = toupper(m_grid[r * m_size + c]);
            cout << (ch == '.' ? ' ' : ch) << ' ';
        }
        cout << '\n';
    }
}

void Grid::add_word(const string& word, int w, Point cross_pnt) {
    Point pnt(cross_pnt);
    pnt.dir = !pnt.dir;
    int& colrow = (pnt.dir.horz() ? pnt.col : pnt.row);
    colrow -= w;
    int start = colrow;
    for (unsigned pos = start, w = 0; pos < start + word.size(); ++pos) {
        char ch = word[w++];
        auto& letter = m_letters[ch];
        colrow = pos; // set pnt.col or pnt.row to pos
        if (pnt != cross_pnt) letter.push_back(pnt);
        (*this)[pnt] = ch;
    }
}

void Grid::place_initial_word(const string& word) {
    uniform_int_distribution<> distDir(0, 1);
    auto& letter = m_letters[word[0]];
    Point cross_pnt(0, 0, Dir::rnd_dir());
    letter.push_back(cross_pnt);
    add_word(word, 0, cross_pnt);
}

bool Grid::can_place(const string& word, int w, const Letter::iterator p) {
    int r = p->row, c = p->col;
    Dir dir = !p->dir;
    const int size = word.size();
    if (dir.horz()) {
        c -= w;
        if (c < 0 || c + size > m_size
         || (c > 0             && !empty(r, c - 1))
         || (c + size < m_size && !empty(r, c + size)))
            return false;
        for (int i = 0; i < size; ++i, ++c) {
            if (i == w) continue;
            if (                   !empty(r    , c)
             || (r > 0          && !empty(r - 1, c))
             || (r < m_size - 1 && !empty(r + 1, c)))
                return false;
        }
    }
    else {
        r -= w;
        if (r < 0 || r + size > m_size
         || (r > 0             && !empty(r - 1,    c))
         || (r + size < m_size && !empty(r + size, c)))
            return false;
        for (int i = 0; i < size; ++i, ++r) {
            if (i == w) continue;
            if (                   !empty(r, c    )
             || (c > 0          && !empty(r, c - 1))
             || (c < m_size - 1 && !empty(r, c + 1)))
                return false;
        }
    }
    return true;
}

bool Grid::place_crossing_word(const string& word) {
    for (unsigned w = 0; w < word.size(); ++w) {
        auto& letter = m_letters[word[w]];
        for (auto p = letter.begin(); p != letter.end(); ++p)
            if (can_place(word, w, p)) {
                Point cross_pnt(*p);
                letter.erase(p); // erase "double-crossed" letters from list
                add_word(word, w, cross_pnt);
                return true;
            }
    }
    return false;
}

void Grid::generate(const WordList& wordlist) {
    place_initial_word(wordlist.rnd_word());
    for (int i = 0; i < MaxTries; ++i) // give up after this many tries
        if (place_crossing_word(wordlist.rnd_word()))
            i = 0; // reset count whenever a word is placed
}

int main(int argc, char **argv) {
    const auto& wordFile =
        (argc == 2 && string(argv[1]) == "-2" ? WordFile2 : WordFile);
    Grid grid(30);
    grid.generate(WordList(wordFile));
    grid.print();
}