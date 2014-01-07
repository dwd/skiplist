
// Copyright 2004-2006 Dave Cridland <dave@cridland.net>

#include "skiplist.h"
#include <map>
#include <iostream>

class attr_name {
public:
    attr_name(std::string const &);
    ~attr_name();
};

class attribute {
public:
    attribute(unsigned long);
    ~attribute();
};

class entry {
public:
    entry(unsigned long long);
    ~entry();
private:
    skip::map<unsigned long, attribute> m_attributes;
};

class dset {
public:
    dset();
    ~dset();
private:
    skip::map<unsigned long long,entry> m_entries;
};

class dstore {
public:
    dstore();
    ~dstore();
private:
    skip::map<unsigned long long,dset> m_datasets;
};

int main( int argc, char ** argv ) {
    try {
	MAP_TYPE< int, int > sl;
	std::cout << "SL init." << std::endl;
	sl[ 1 ] = 1;
	sl[ 3 ] = 9;
	sl[ 2 ] = 4;
	std::cout << "SL Search." << std::endl;
	std::cout << "SL has " << sl[ 2 ] << std::endl;
	sl.erase( 2 );
	std::cout << "Erased okay." << std::endl;
	std::cout << "SL has " << sl[ 1 ] << std::endl;
	std::cout << "SL has " << sl[ 3 ] << std::endl;
	sl[ 2 ] = 4;
	std::cout << "SL has " << sl[ 2 ] << std::endl;
	for( int i( 4 ); i < 20; ++i ) {
	    sl.insert( std::make_pair( i, i*i ) );
	}
	std::cout << "New inserts..." << std::endl;
	std::cout << "SL has " << sl[ 2 ] << std::endl;
	sl.erase( 2 );
	std::cout << "Still okay." << std::endl;
	for(MAP_TYPE<int,int>::const_iterator i(sl.begin()); i!=sl.end(); ++i) {
	    std::cout << "SL[" << (*i).first << "] = " << (*i).second << ";\n";
	}
	sl.insert( std::make_pair( 2, 4 ) );
	std::cout << "SL has " << sl[ 2 ] << std::endl;
	for( int i( 20 ); i<NUM_ENTRIES; ++i ) {
	    sl.insert( std::make_pair( i, i*i ) );
	    if( i%REPORT_EVERY == 0 ) {
		std::cout << "\r" << i << " inserts              " << std::flush;
	    }
	}
	std::cout << "\nSearching through " << sl.size() << " entries." << std::endl;
	for( int i( 1 ); i<NUM_ENTRIES; ++i ) {
	    try {
		if( (i*i) != sl[ i ] ) {
		    throw std::runtime_error( "Looks like data corruption." );
		}
	    } catch( std::exception & e ) {
		std::cout << "Exception while searching for " << i << ": " << e.what() << std::endl;
		throw;
	    }
	    if( i%REPORT_EVERY == 0 ) {
		std::cout << "\r" << i << " searches              " << std::flush;
	    }
	}
#ifdef SK_HEIGHT_DATA
	sl.height_data();
#endif
	std::cout << "SL[500] is " << sl[500] << std::endl;
	sl.erase( 500 );
	std::cout << "SL has " << sl[500] << " for 500.\n";
    } catch( std::exception & e ) {
	std::cout << "Exception: " << e.what() <<std::endl;
    }
    try {
	MAP_TYPE<std::string,std::string> sl;
	sl["a"] = "alpha";
	sl["d"] = "delta";
	sl["b"] = "bravo";
	sl["e"] = "echo";
	sl["c"] = "charlie";
	sl["v"] = "victor";
	std::string dave( "dave" );
	for( std::string::size_type i(0); i<dave.length(); ++i ) {
	    std::cout << sl[ dave.substr( i, 1 ) ] << " ";
	}
	std::cout << std::endl;
    } catch( std::exception & e ) {
	std::cout << "Exception (stage 2): " << e.what() << std::endl;
    }
    try {
	MAP_TYPE<char,std::string> m;
	m['a'] = "Alpha";
	m['b'] = "Beta";
	m['c'] = "Charlie";
	m['d'] = "Delta";
	m['e'] = "Echo";
	m['f'] = "Foxtrot";
	m['g'] = "Golf";
	m['h'] = "Hotel";
	m['i'] = "Indigo";
	m['j'] = "Juliet";
	m['k'] = "Kilo";
	m['l'] = "Lima";
	
	std::string kellie("kellie");
	
	for( std::string::size_type i(0); i<kellie.length(); ++i ) {
	    std::cout << m[kellie[i]] << " ";
	}
	std::cout << std::endl;
    } catch( std::exception & e ) {
	std::cout << "Whoops, this shouldn't throw exceptions!" << std::endl;
    }
}
