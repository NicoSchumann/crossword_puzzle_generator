#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

void print_help()
{
    std::cout
    << "The program needs as argument a dictionary with alphabetic words.\n"
    << "The program sorts all words by size and converts all characters to "
    << "uppercase and writes an index header to the begin of the output file:\n"
    << " xx        : header size\n"
    << " xx xx ... : word length and index to the first word of such size.\n"
    << "!The indices don't include the header's size, so it must handled as offset.\n"
    ;
}

int main( int argc, char *argv[] )
{
    std::ifstream ifs;
    std::ofstream ofs;
    std::string filename;

    if( argc <= 1 ) {
        print_help();
        return 1;
    }
    else {
        filename = argv[1];
    }
    ifs.open( filename );
    if( ! ifs ) {
        std::cout << "Dictionary file couldn't opened!\n";
        return 2;
    }
    ofs.open( std::string("idx_") + filename );
    if( ! ofs ) {
        std::cout << "Output file couldn't opened!\n";
        return 3;
    }

    // sort the words by length
    std::vector<std::vector<std::string>> words_table(128);
    for( std::string word; ifs >> word; )
    {
        if( word.size() > 32) continue;
        for( const auto chr : word ) {
            if( ! std::isalpha(chr) ) {
                std::cerr << "The dictionary contains non-alphabetic stuff!\n";
                return 4;
            }
        }
        words_table[word.length() - 1].push_back( word );
    }

    // generate indices for words lengt and start position in output file
    std::vector<std::pair<int,int>> header;
    for( int i = 0, count = 0; i < words_table.size(); ++i )
    {
        if( words_table[i].size() == 0 ) continue;
        header.push_back( std::make_pair( i+1, count ) );
        count += words_table[i].size();
    }
       // write the header
    ofs << header.size()+1 << '\n';
    for( const auto & pair : header ) {
        ofs << pair.first <<' '<< pair.second << '\n';
    }

    // write the words
    for( auto & word_list : words_table) {
        if( word_list.size() == 0 ) continue;
        for( auto & word : word_list) {
            for( auto & chr : word ) {
                chr = std::toupper( chr );
            }
            ofs << word << '\n';
        }
    }
}