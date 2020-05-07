#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <stdexcept>

/* Holds its coordinates and a 'weight'.
 * If the word doesn't fit in the grid, weight is -1.
 * If the word fit, but there are no intersections with characters of other words, weight is 0.
 * If the word fit, and there are intersections with characters of other words, weight is the
 * sum of all intersected characters.
 */
struct Weight
{
    int x;   // column
    int y;   // row
    int wt;  // weight
    Weight(int xx=0, int yy=0, int ww=0)
    : x{xx}, y{yy}, wt{ww}
    {}
};

/* The crossword puzzle generator class :-)
 * Its grid at each border side is at each one field broader than its 'box' inside. That's for
 * easier testing the borders of the words.
 */
class Cwg
{
public:
    Cwg( int width, int height );
    Cwg();

    bool emplaceWord( const std::string & word, bool horizontally );

private:
    Weight highestWeight( const std::string & word, int dx, int dy ) const;
    Weight doWeight( const std::string & word, int width, int height,
		     int dx, int dy ) const;

    // short-hand for the width and height
    unsigned width() const {return m_grid[0].size(); }
    unsigned height() const {return m_grid.size(); }

    std::vector<std::vector<char>> m_grid;
    friend std::ostream & operator<<( std::ostream &, const Cwg & );
};

Cwg::Cwg( int width, int height )
{
    if( width < 3 ) width = 3;
    if( height < 3) height = 3;

    // Consider that the 'effective' room is width x height.
    std::vector<char> row(width, '.');
    for( int h = 0; h < height; ++h )
    {
        m_grid.push_back(row);
    }
}
Cwg::Cwg() : Cwg{16,16} {}

// Set dx and dy to increment one position horizontally or
// vertically, depending on horizontal
void setDeltas(bool horizontal, int &dx, int &dy)
{
    if (horizontal) {
	dx = 1;
	dy = 0;
    } else {
	dx = 0;
	dy = 1;
    }
}

/* This method emplaces a word on the grid depending on the return value of Cwg::highestWeight().
 * The word will positioned at the coordinates of that Weight object.
 * If no position could found, the method returns false, otherwise true.
 *
 * See also Cwg::doWeight()
 */
bool
Cwg::emplaceWord( const std::string & word, bool horizontally = true )
{
    int dx, dy;
    setDeltas(horizontally, dx, dy);
    // check that the word is not too long for the grid
    if (word.length()*dx > width()-2 || word.length()*dy > height()-2) {
	return false;
    }

    Weight weight = highestWeight( word, dx, dy);
    //std::cerr << word << ':' << '(' << weight.x << ',' << weight.y << ','
    //          << weight.wt << ')'<< '\n';
    if( weight.wt == -1 )
        return false;   // word doesn't match within the grid

    // Place it
    // std::cout << "Emplace " << word << ' '
    // << (horizontally ? "horizontal" : "vertical")
    // << " at " << weight.x+1 << ',' << weight.y+1 << '\n';
    for( unsigned p = 0; p < word.length(); ++p) {
	m_grid[weight.y][weight.x] = word[p];
	weight.x += dx;
	weight.y += dy;
    }

    //std::cerr << *this << '\n';
    return true;
}

/* This method returns the position for a word at the grid with the highest weight.
 * See Cwg::doWeight()
 *
 * Consider that the the there is a border of 1 field at each side of the grid.
 */
Weight
Cwg::highestWeight( const std::string & word, int dx, int dy) const
{
    Weight weight(1,1,-1);

    for( unsigned h = 1; h < height() - dy*word.size() - 1; ++h ) {
	for( unsigned w = 1; w < width() - dx*word.size() - 1; ++w ) {
	    Weight tmpWeight = doWeight( word, w, h, dx, dy);
	    if( tmpWeight.wt > weight.wt ) {
		weight = tmpWeight;
	    }
	}
    }

    //std::cerr << "x("<< weight.x <<','<< weight.y <<','<< weight.wt <<')';
    return weight;
}


/* This method tests if a word matches within a distinct grid position.
 * If the word fits into the position, it returns by default a weight of 0.
 * For each matching intersection with another word the weight increases by 1.
 * If the word doesn't match at the position, the weight gets -1.
 */
Weight
Cwg::doWeight( const std::string & word, int width, int height,
	       int dx, int dy) const
{
    // Each word needs at minimum a distance by one field in each direction to its neighbours.

    Weight weight(width,height,0);  // Needs to be 0 weighted!

    // Are there blank spaces before and after the word?
    if (m_grid[height-dy][width-dx] != '.' ||
	m_grid[height+dy*word.size()][width+dx*word.size()] != '.') {
	weight.wt = -1;
	//	std::cout << "Can't place at " << width+1 << "," << height+1
	// << ": No blank before or after\n";
	return weight;
    }

    int oh=height, ow = width;
    for( unsigned p = 0; p < word.length(); ++p, height += dy, width += dx ) {
            // test the place as such
	char g = m_grid[height][width]; // shorthand
	if( word[p] == g) {
	    // It matches
	    ++weight.wt;
	    continue;
	} else if( g != '.') {
	    // That space is occupied. Technically the
	    // comparison below will catch this case
	    // also, but what the heck.
	    weight.wt = -1;
	    // std::cout << "Can't place " << word << ' ' << p
	    // << " at " << ow+1 << "," << oh+1
	    // << ": " << width+1 << "," << height+1
	    // << " is " << g << " instead of " << word[p] << '\n';
	    break;
	} else if (m_grid[height+dx][width] != '.' ||
		   m_grid[height][width+dy] != '.' ||
		   m_grid[height-dx][width] != '.' ||
		   m_grid[height][width-dy] != '.') {
	    // neighboring cell is occupied
	    // std::cout << "Can't place at " << ow+1 << "," << oh+1
	    // << ": adjacent space is occupied\n";
	    weight.wt = -1;
	    break;
	}
    }
    //std::cerr << '('<<weight.x<<','<<weight.y<<','<<weight.wt<<')';  // correct
    return weight;
}

/* Overloads operator<< at std::ostream for << the grid.
 */
std::ostream & operator<<( std::ostream & os, const Cwg & cwg )
{
    for( const auto & row : cwg.m_grid ){
        for( const char c : row )
            os <<' ' << (c=='.' ? ' ' : c);
        os <<  '\n';
    }
    return os;
}

// for debugging proposals
std::vector<std::string> dictionary = { "apache", "anchor", "banana", "beaver", "bear", "bussard",
"chocolate", "driver", "elephant", "eagle", "fog", "gear", "agony", "host", "harrassment",
"ice", "icebear", "bicycle", "rotten", "dread", "loo", "christmas", "handle", "theatre", "solvent",
"mouse", "rabbit", "dere", "sailor", "craftsman", "hooligan", "ananas", "cherry", "cranberry" };

int main( int argc, char * argv[] )
{
    // The file handling:

    std::string dictName = "dictionary.txt";
    if( argc > 1 ) dictName = argv[1];
    std::ifstream ifile( dictName );
    if( !ifile ) {
        std::cerr << "File '" << dictName << "'could't opened!\n";
        return 1;
    }
    std::vector<std::string> dictionary;
    std::string tmp;
    while ( ifile >> tmp ) {
	for (auto &c : tmp) {
	    c = std::toupper(c);
	}
	dictionary.push_back( tmp );
    }

    std::default_random_engine eng( std::random_device{}() );
    std::uniform_int_distribution<> dist( 0, dictionary.size()-1 );


    // The crossword puzzle generator in action:

    Cwg cwg{30,30};

    for( int i = 0; i < 100; ++i )
    {
        std::string hor = dictionary[ dist(eng) ];
        std::string vert = dictionary[ dist(eng) ];
        cwg.emplaceWord(hor,true);
        cwg.emplaceWord(vert,false);
    }

    std::cout << cwg << '\n';
}