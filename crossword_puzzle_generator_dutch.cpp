#include <iostream>
#include <fstream>
#include <vector>
#include <random>
using namespace std;

const int GridSize = 30;
const int ShortestWord = 4, LongestWord = 7;
const int MaxTries = 10000;
const string WordFile = "words_alpha.txt";

// Global Random Engine
default_random_engine RndEngine{ random_device{}() };

enum Dir { DirHorz, DirVert };
Dir operator!(Dir dir) { return dir == DirHorz ? DirVert : DirHorz; }

struct Point {
    int row, col;
    Dir dir;
    Point(int r, int c, Dir d) : row(r), col(c), dir(d) { }
    bool operator==(const Point& p) const {return row == p.row && col == p.col;}
};

using Grid      = char[GridSize][GridSize];
using PointList = vector<Point>;
using Letters   = PointList[26];  // one for each letter (0 is 'A', etc.)
using WordList  = vector<string>;

string pick_rnd_word(const WordList& wordList) {
    uniform_int_distribution<> dist(0, wordList.size() - 1);
    return wordList[dist(RndEngine)];
}

/* Fills the Grid with the word
 * Saves all letter-positions within the 'letters' array.
 */
void add_word(Grid grid, Letters& letters, const string& word, Point pnt) {
    auto& line = (pnt.dir == DirHorz ? pnt.col : pnt.row);
    int start = line;
    for (unsigned pos = start, w = 0; pos < start + word.size(); ++pos) {
        char ch = toupper(word[w++]);
        auto& letter = letters[ch - 'A'];
        line = pos;    // pnt will be updated by 'line'
        letter.push_back(pnt);
        //std::cerr<<"row:"<<pnt.row<<"col"<<pnt.col;
        grid[pnt.row][pnt.col] = ch;
    }
    //std::cerr<< '\n';
}

void place_initial_word(Grid grid, const string& word, Letters& letters) {
    uniform_int_distribution<> distDir(0, 1);
    add_word(grid, letters, word,
             Point(0, 0, distDir(RndEngine) ? DirHorz : DirVert));
}

bool can_place(Grid grid, const string& word, int ch_idx, const PointList::iterator p) {
    int r = p->row, c = p->col, dir = !p->dir;
    if (dir == DirHorz) {
        c -= ch_idx;
        if (c < 0 || c + word.size() > GridSize  // check for valid grid index
         || (c > 0 && grid[r][c - 1] != '.')     // check for left neighbored word
         || (c + word.size() < GridSize && grid[r][c + word.size()] != '.'))
            return false; // above: check for right neighbored word

        // check the word place as such and upper & lower border
        for (int i = 0; i < int(word.size()); ++i, ++c) {
            if (i == ch_idx) continue;
            if (grid[r][c] ==
            if (                     grid[r    ][c] != '.'
             || (r > 0            && grid[r - 1][c] != '.')
             || (r < GridSize - 1 && grid[r + 1][c] != '.'))
                return false;
        }
    }
    else {
        r -= ch_idx;
        if (r < 0 || r + word.size() > GridSize
         || (r > 0 && grid[r - 1][c] != '.')
         || (r + word.size() < GridSize && grid[r + word.size()][c] != '.'))
            return false;
        for (int i = 0; i < int(word.size()); ++i, ++r) {
            if (i == ch_idx) continue;
            if (                     grid[r][c    ] != '.'
             || (c > 0            && grid[r][c - 1] != '.')
             || (c < GridSize - 1 && grid[r][c + 1] != '.'))
                return false;
        }
    }
    return true;
}

bool place_crossing_word(Grid grid, const WordList& wordList, Letters& letters) {
    for (int i = 0; i < MaxTries; ++i) { // give up after this many tries
        string word = pick_rnd_word(wordList);
        for (unsigned ch_idx = 0; ch_idx < word.size(); ++ch_idx) {
            auto& points = letters[toupper(word[ch_idx]) - 'A'];
            for (auto p = points.begin(); p != points.end(); ++p)
                if (can_place(grid, word, ch_idx, p)) {
                    Point pnt(p->row, p->col, !p->dir);
                    if (pnt.dir == DirHorz) pnt.col -= ch_idx; else pnt.row -= ch_idx;
                    add_word(grid, letters, word, pnt);
                    return true;
                }
        }
    }
    return false;
}

void print_grid(const Grid grid) {
    for (int r = 0; r < GridSize; ++r) {
        for (int c = 0; c < GridSize; ++c)
            cout << (grid[r][c] == '.' ? ' ' : grid[r][c]) << ' ';
        cout << '\n';
    }
}

WordList read_word_list(const string& filename) {
    WordList wordList;
    ifstream in(filename);
    for (string word; in >> word; )
        if (word.size() >= ShortestWord && word.size() <= LongestWord)
            wordList.push_back(word);
    return wordList;
}

int main() {
    Letters letters;
    WordList wordList = read_word_list(WordFile);
    Grid grid;
    std::fill((char*)grid, (char*)grid + GridSize*GridSize, '.');
    place_initial_word(grid, pick_rnd_word(wordList), letters);
    while (place_crossing_word(grid, wordList, letters)) ;
    print_grid(grid);
}
