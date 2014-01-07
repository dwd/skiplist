// -*- C++ -*-

// Copyright 2004-2006 Dave Cridland <dave@cridland.net>

#include <exception>
#include <stdexcept>
#include <memory>
#include <cstdlib>

// This spews. Don't define it unless everything breaks.
//#define SK_VERBOSE_DEBUG
// This checks skiplist integrity after every operation - VERY SLOW!
//#define SK_DEBUG_CHECK

namespace skip {
    /*
      Node of a skiplist.
      Conceptually, this contains a value_type,
      but this is managed by the skiplist itself.
      Also, the m_ptrs array is usually larger than 2, in reality.
    */
    template< typename V >
    class SkiplistNode {
    public:
	typedef V value_type;
	typedef SkiplistNode<V> my_type;
    private:
	// V m_value;
	unsigned int m_height;
	my_type * m_ptrs[2];

	// Unimplemented:
	SkiplistNode(SkiplistNode const &);
	SkiplistNode & operator = (SkiplistNode const &);
	
    public:
	SkiplistNode( unsigned int h ) : m_height( h ) {
	    for( unsigned int i(0); i<(m_height+1); ++i ) {
		m_ptrs[i]=0;
	    }
	}
	~SkiplistNode() {
	}
	my_type * getptr( int i ) {
	    return m_ptrs[i+1];
	}
	void setptr( int i, my_type * n ) {
	    m_ptrs[i+1] = n;
	}
	
	inline my_type *& operator[]( int i ) {
	    return m_ptrs[i+1];
	}
	
	inline value_type const & value() const {
	    return *value_ptr();
	}
	inline value_type & value() {
	    return *value_ptr();
	}
	inline const value_type * value_ptr() const {
	    return reinterpret_cast<const value_type*>( this ) - 1;
	}
	inline value_type * value_ptr() {
	    return reinterpret_cast< value_type*>( this ) - 1;
	}
	
	inline unsigned int height() const {
	    return m_height;
	}
	
	static inline unsigned int alloc_size( unsigned int h ) {
	    return sizeof(value_type)+sizeof(my_type)+(h*sizeof(my_type*));
	}
    };
    
    template<typename IP, typename IV> class sk_iterator {
    private:
	IP m_node;
    public:
	typedef IV value_type;
	typedef std::bidirectional_iterator_tag iterator_category;
	
	sk_iterator( sk_iterator<IP,IV> const & m ) : m_node( m.m_node ) {
	}
	template<typename IVX> sk_iterator( sk_iterator<IP,IVX> const & m ) : m_node( m.priv_node() ) {
	}
	sk_iterator( IP  n ) : m_node( n ) {
	}
	sk_iterator & operator++() {
	    m_node = (*m_node)[0];
	    return *this;
	}
	sk_iterator & operator--() {
	    m_node = (*m_node)[-1];
	    return *this;
	}
	IV & operator*() {
	    return m_node->value();
	}
	IV * operator->() {
	    return m_node->value_ptr();
	}
	bool operator==( sk_iterator const & i ) {
	    return m_node==i.m_node;
	}
	bool operator!=( sk_iterator const & i ) {
	    return m_node!=i.m_node;
	}
	
	IP const & priv_node() const {
	    return m_node;
	}
    };
    
    template< typename K, typename V, typename X, typename L, typename A > class Skiplist {
    public:
	// std::map typedefs
	typedef K key_type;
	typedef V value_type;
	typedef X extract_key;
	typedef L key_compare;
	typedef A allocator_type;
	// Local typedefs
	typedef Skiplist<K,V,X,L,A> my_type;
	typedef SkiplistNode<value_type> node_type;
	typedef typename allocator_type::template rebind< unsigned char >::other raw_allocator;
	typedef typename allocator_type::template rebind< node_type >::other node_allocator;
	typedef node_type * live_node_ptr;
	typedef typename node_allocator::pointer node_ptr;
	typedef typename node_allocator::const_pointer const_node_ptr;
	// std::map typedefs
	typedef typename allocator_type::reference reference;
	typedef typename allocator_type::const_reference const_reference;
	typedef typename allocator_type::size_type size_type;
	typedef typename allocator_type::difference_type difference_type;
	typedef typename allocator_type::pointer pointer;
	typedef typename allocator_type::const_pointer const_pointer;
	// And iterators:
	typedef sk_iterator<node_ptr,value_type> iterator;
	typedef sk_iterator<node_ptr,value_type const> const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	// Unimplemented:
	Skiplist(my_type const &);
	my_type & operator=(my_type const &);
    private:
	node_ptr m_head;
	key_compare m_comp;
	std::size_t m_size;
	unsigned int m_height;
	raw_allocator m_alloc;
	static const unsigned int maxheight = 64;
    public:
	Skiplist() : m_head( new_node(4) ), m_comp(), m_size( 0 ), m_height( m_head->height() ) {
	}
	Skiplist( L const & l, A const & a ) : m_head( new_node(4) ), m_comp(l), m_size(0), m_height( m_head->height() ), m_alloc( a ) {
	}
	virtual ~Skiplist() {
	    node_ptr current = m_head;
	    while( node_ptr next = (*current)[0] ) {
		if(current == m_head) {
		    destroy_node(current);
		} else {
		    delete_node(current);
		}
		current = next;
	    }
	    if(current == m_head) {
		destroy_node(current);
	    } else {
		delete_node(current);
	    }
	}
	
    private:
	unsigned int suitable_height() const {
	    long unsigned int s(m_size);
	    unsigned int h( 4 );
	    while( s>>=2 ) {
		h+=1;
		if( h == ( 8*sizeof(unsigned int) ) ) {
		    break;
		}
	    }
	    return h;
	}
	
	unsigned int pickheight() const {
	    unsigned int i(1);
	    unsigned int r = std::rand();
	    unsigned int mask = ( 0x01 << 31 ); // Top one bit set.
	    while( !(r & mask) ) {
		if( i == m_height ) {
#ifdef SK_VERBOSE_DEBUG
		    std::cout << "Picked max height!\n";
#endif
		    break;
		}
		++i;
		if( mask & 0x1 ) { // Must be all ones.
		    break;
		}
		mask >>= 2; // Shift right by two.
		mask |= ( 0x03 << 30 ); // Set two top bits.
	    }
	    return i;
	}
	
	void check_header() {
	    unsigned int h( suitable_height() );
	    if( h > maxheight ) {
		h = maxheight;
	    }
	    //std::cout << "Suitable height is " << h << " actual height is " << m_height.m_skiplist_height << std::endl;
	    if( h > m_height ) {
#ifdef SK_VERBOSE_DEBUG
		std::cout << "Using new header size of " << h << std::endl;
#endif
		node_ptr t( m_head );
		m_height = h;
		m_head = new_node( m_height );
		for( unsigned int i(0); i<h; ++i ) {
		    if( i < t->height() ) {
			(*m_head)[i] = (*t)[i];
		    } else {
			(*m_head)[i] = 0;
		    }
		}
		destroy_node( t );
	    }
	}
	
	node_ptr new_node(int height) {
#ifdef SK_VERBOSE_DEBUG
	    std::cout << "\nAllocating node for height " << height << std::endl;
#endif
	    std::size_t octets(node_type::alloc_size(height));
#ifdef SK_VERBOSE_DEBUG
	    std::cout << "==> " << octets << std::endl;
#endif
	    typename raw_allocator::pointer p(m_alloc.allocate(octets));
#ifdef SK_VERBOSE_DEBUG
	    std::cout << "==> Allocated at " << reinterpret_cast<void *>(p) << std::endl;
#endif
	    p += sizeof(value_type);
#ifdef SK_VERBOSE_DEBUG
	    std::cout << "==> Pointer moved to " << reinterpret_cast<void *>(p) << std::endl;
#endif
	    new( p ) node_type(height);
	    return (node_ptr)p;
	}
	node_ptr new_node(value_type const & v, int height) {
	    node_ptr p(new_node(height));
	    new (p->value_ptr()) value_type(v);
	    return p;
	}
	
	void delete_node( node_ptr p ) {
	    value_type * pv( p->value_ptr() );
	    pv->~value_type();
	    destroy_node(p);
	}

	void destroy_node(node_ptr p) {
	    typename raw_allocator::pointer pp( (typename raw_allocator::pointer)p->value_ptr() );
	    unsigned int h( p->height() );
	    p->~node_type();
	    m_alloc.deallocate( pp, node_type::alloc_size( h ) );
	}
	
	node_ptr find_next( K const & k, node_ptr update[] = 0, node_ptr thisone=node_ptr() ) const {
	    node_ptr current( m_head );
	    node_ptr next( 0 );
	    
#ifdef SK_VERBOSE_DEBUG    
	    std::cout << "\nfind_next is looking for " << k << std::endl;
#endif
	    
	    for( unsigned int i(m_height-1);; --i ) {
		if( i >= current->height() ) continue;
		next = (*current)[i];
#ifdef SK_VERBOSE_DEBUG_BROKEN
		std::cout << "Height is " << i << ", looking forward to ";
		if( next ) {
		    std::cout << next->value().first;
		    if( next->value().first > 100 ) {
			char **p(0);
			throw std::runtime_error( *p );
		    }
		} else {
		    std::cout << "NIL";
		}
		std::cout << std::endl;
#endif
		while( next && m_comp( extract_key()(next->value()), k )
		       && ( thisone ? next != thisone : true )
		       // If we have a start, continue if it does not match.
		       // If we do not, then continue always.
		) {
		    current = next;
		    next = (*current)[i];
#ifdef SK_VERBOSE_DEBUG
		    std::cout << "  - Moved forward to " << next << ": ";
		    if( next ) {
			std::cout << next->value().first;
		    } else {
			std::cout << "NIL";
		    }
		    std::cout << std::endl;
#endif
		}
		if( update ) {
		    update[i] = current;
		}
		if( 0==i ) break;
	    }
#ifdef SK_VERBOSE_DEBUG
	    std::cout << "Final, returning " << next << std::endl;
#endif
	    
	    return next;
	}
	
#ifdef SK_DEBUG_CHECK
	void check() {
	    for( unsigned int i(0); i<m_height; ++i ) {
		node_ptr current( m_head );
		bool found = false;
		// Look down until we find a NULL pointer. Should be none below here.
		for( unsigned int hh(current->height()); hh>0; --hh ) {
		    if( found ) {
			if( !(*current)[hh-1] ) {
			    char ** p(0);
			    throw std::runtime_error( *p );
			}
		    } else {
			if( (*current)[hh-1] ) {
			    found = true;
			}
		    }
		}
		node_ptr next( (*current)[i] );
		while( next ) {
		    if( current == next ) {
			char **p(0);
			throw std::runtime_error( *p );
		    }
		    current = next;
		    next = (*current)[i];
		}
	    }
	}
#endif
	
    public:
	std::pair<bool,node_ptr> insert_node(V const & v, bool allow_dups=true) {
	    check_header();
	    node_ptr update[m_height];
	    for( unsigned int i(0); i<m_height-1; ++i ) {
		update[i] = 0;
	    }

	    K const & k(extract_key()(v));
	    node_ptr next( find_next( k, update ) );
	    
	    if( allow_dups ) {
#ifdef SK_VERBOSE_DEBUG
		std::cout << "Allow dups is TRUE\n";
#endif
		if( next ) {
#ifdef SK_VERBOSE_DEBUG
		    std::cout << "Have next.\n";
#endif
		    if( !( m_comp( extract_key()(next->value()), k ) || m_comp( k, extract_key()(next->value()) ) ) ) {
			return std::make_pair(false,next);
		    }
		}
	    }
	    
	    
	    unsigned int height( pickheight() );
	    node_ptr node = new_node( v, height );
#ifdef SK_VERBOSE_DEBUG
	    std::cout << "Using height of " << height << std::endl;
	    std::cout << "Node extends from " << node->value_ptr() << " to " << (void*)(((char*)(node->value_ptr()))+node_type::alloc_size(height)) << std::endl;
#endif
	    ++m_size;
	    (*node)[-1] = update[0];
	    for( unsigned int i(0); i<height; ++i ) {
		(*node)[i] = (*(update[i]))[i];
#ifdef SK_VERBOSE_DEBUG
		std::cout << "node[" << i << "] = " << (*(update[i]))[i] << std::endl;
#endif
		(*(update[i]))[i] = node;
#ifdef SK_VERBOSE_DEBUG
		std::cout << "(*(update[" << i << "]))[" << i << "] = " << node << std::endl;
		std::cout << "At height " << i << ", adding before ";
		if( (*node)[i] ) {
		    std::cout << (*node)[i]->value().first;
		} else {
		    std::cout << "NIL";
		}
		std::cout << std::endl;
#endif
	    }
#ifdef SK_DEBUG_CHECK
	    check();
#endif
	    return std::make_pair(true,node);
	}
	
	node_ptr search_node( K const & k ) const {
	    node_ptr next( find_next( k ) );
	    
	    if( !next ) {
		return node_ptr();
	    }
	    if( m_comp( extract_key()(next->value()), k ) || m_comp( k, extract_key()(next->value()) ) ) {
		//std::cout << "k is " << k << ", next->key() is " << next->key() << std::endl;
		//std::cout << "next is " << next << ", head is " << m_head << std::endl;
		return node_ptr();
	    }
	    
	    return next;
	}
	
	size_t erase_node( K const & k, node_ptr start=node_ptr(), node_ptr end=node_ptr() ) {
	    node_ptr update[m_height];
	    for( unsigned int i(0); i<m_height-1; ++i ) {
		update[i] = 0;
	    }
	    node_ptr next( find_next( k, update, start ) );
	    
	    if( !next ) {
		return 0;
	    }
	    if( m_comp( extract_key()(next->value()), k ) || m_comp( k, extract_key()(next->value()) ) ) {
		return 0;
	    }
	    
	    if( !end ) {
		end = (*next)[0];
	    }
	    
	    size_t counter( 0 );
	    while( next!=end ) {
#ifdef SK_VERBOSE_DEBUG
		std::cout << "Next is " << next << ", end is " << end << std::endl;
#endif
		--m_size;
		(*next)[-1] = update[0];
		for( unsigned int i(0); i<m_height; ++i ) {
		    if( next->height() <= i ) break;
		    (*(update[i]))[i] = (*next)[i];
#ifdef SK_VERBOSE_DEBUG
		    std::cout << "Updated node " << update[i]->value().first << std::endl;
		    std::cout << "Now points to ";
		    if( (*next)[i] ) {
			std::cout << (*next)[i]->value().first << " [" << (*(update[i]))[i]->value().first << "]";
		    } else {
			std::cout << "NIL";
		    }
		    std::cout << " at height " << i;
		    std::cout << std::endl;	
#endif
		}
		node_ptr tmp( (*next)[0] );
		delete_node( next );
		next = tmp;
#ifdef SK_VERBOSE_DEBUG
		std::cout << "Now deleted " << counter << " nodes.\n";
#endif
		counter++;
#ifdef SK_DEBUG_CHECK
		check();
#endif
	    }
	    return counter;
	}
	
	size_t size() const {
	    return m_size;
	}
	
	node_ptr head() {
	    return m_head;
	}
	
	iterator begin() {
	    return iterator( (*m_head)[0] );
	}
	const_iterator begin() const {
	    return const_iterator( (*m_head)[0] );
	}
	iterator end() {
	    return iterator( node_ptr() );
	}
	const_iterator end() const {
	    return const_iterator( node_ptr() );
	}
	
	iterator lower_bound( key_type const & k ) {
	    return find_next( k );
	}
	iterator upper_bound( key_type const & k ) {
	    node_ptr p( find_next( k ) );
	    while( p && !m_comp( extract_key()(p->value()), k ) ) p=(*p)[0];
	    return p;
	}
	std::pair<iterator,iterator> equal_range( key_type const & k ) {
	    node_ptr f( find_next( k ) );
	    node_ptr e(f);
	    while( e && !m_comp( extract_key()(e->value()), k ) ) e=(*e)[0];
	    return std::make_pair( f, e );
	}
	
#ifdef SK_HEIGHT_DATA
	void height_data();
#endif
    };

    template< typename K, typename V > class ExtractFirst {
    public:
	K const & operator()(std::pair<const K, V> const & p) {
	    return p.first;
	}
    };
    
    template< typename K, typename V, typename L=std::less<K>, typename A=std::allocator< std::pair<K const,V> > > class map : private Skiplist<K,std::pair<const K,V>,ExtractFirst<K,V>,L,A> {
    public:
	typedef Skiplist<K,std::pair<const K,V>,ExtractFirst<K,V>,L,A> parent_type;
	typedef typename parent_type::iterator iterator;
	typedef typename parent_type::const_iterator const_iterator;
	typedef typename parent_type::value_type value_type;
	typedef typename parent_type::key_type key_type;
	typedef V mapped_type;
	explicit map( const L& comp=L(), const A& alloc=A() ) : parent_type( comp, alloc ) {}
	template< typename I > map( I first, I last, const L& comp=L(), const A& alloc=A() )
	    : parent_type( first, last, comp, alloc ) {
		for( I i( first ); i!=last; ++i ) {
		    insert_node( (*i).first, (*i).second, false );
		}
	    }

	// Both const and non-const forms:
	using parent_type::begin;
	using parent_type::end;
	// And these:
	using parent_type::lower_bound;
	using parent_type::upper_bound;
	using parent_type::size;
	
    protected:
	typedef typename parent_type::node_ptr node_ptr;
	
    public:
	
	V & operator[]( const K & k ) {
	    node_ptr n( this->search_node( k ) );
	    if( !n ) {
		n = this->insert_node(value_type(k, V()), false ).second;
	    }
	    return n->value().second;
	}
	
	iterator find( const K & k ) {
	    return iterator( this->search_node( k ) );
	}
	
	std::pair<bool,iterator> insert( value_type const & v ) {
	    return this->insert_node( v, false );
	}
	
	void erase( key_type const & k ) {
	    parent_type::erase_node( k );
	}
	void erase( iterator const & i ) {
	    parent_type::erase_node( (*i).first, i.node_ptr );
	}
    };
    
    template< typename K, typename V, typename L=std::less<K>, typename A=std::allocator< std::pair<K const,V> > > class multimap : private Skiplist<K,std::pair<const K,V>,ExtractFirst<K,V>,L,A> {
    public:
	typedef Skiplist<K,std::pair<const K,V>,ExtractFirst<K,V>,L,A> parent_type;
	typedef typename parent_type::iterator iterator;
	typedef typename parent_type::const_iterator const_iterator;
	typedef typename parent_type::value_type value_type;
	typedef typename parent_type::key_type key_type;
	typedef V mapped_type;
	explicit multimap( const L& comp=L(), const A& alloc=A() ) : parent_type( comp, alloc ) {}
	template< typename I > multimap( I first, I last, const L& comp=L(), const A& alloc=A() )
	    : parent_type( first, last, comp, alloc ) {
		for( I i( first ); i!=last; ++i ) {
		    insert_node( (*i).first, (*i).second, false );
		}
	    }

	// Both const and non-const forms:
	using parent_type::begin;
	using parent_type::end;
	// And these:
	using parent_type::lower_bound;
	using parent_type::upper_bound;
	using parent_type::size;
	
    protected:
	typedef typename parent_type::node_ptr node_ptr;
	
    public:
	
	iterator find( const K & k ) {
	    return iterator( search_node( k ) );
	}
	
	std::pair<bool,iterator> insert( value_type const & v ) {
	    return insert_node( v );
	}
	
	void erase( key_type const & k ) {
	    parent_type::erase_node( k );
	}
	void erase( iterator const & i ) {
	    parent_type::erase_node( (*i).first, i.node_ptr );
	}
    };
    
#ifdef SK_HEIGHT_DATA
    template< typename K, typename V, typename L, typename A>
    void Skiplist<K,V,L,A>::height_data() {
	try {
	    typedef map< unsigned int, std::pair<int,int> > t_heightmap;
	    t_heightmap heightmap;
	    for( node_ptr c( m_head ); c; c = (*c)[0] ) {
		unsigned int h = c->height();
#ifdef SK_VERBOSE_DEBUG
		std::cout << ">> Reported height: " << h << std::endl;
#endif
		while( h && (*c)[h]==0 ) --h;
#ifdef SK_VERBOSE_DEBUG
		std::cout << ">> Resolved height: " << h << std::endl;
#endif
		try {
		    ++heightmap[h].second;
		    ++heightmap[h].first;
		    for( ; h>0; --h ) {
#ifdef SK_VERBOSE_DEBUG
			std::cout << ">> Hitting height: " << h << std::endl;
#endif
			++heightmap[h-1].first;
		    }
		} catch( std::exception & e ) {
		    std::cout << "Caught " << e.what() << " while trying to do stuff with " << h << std::endl;
		    throw;
		}
	    }
	    std::cout << "Height map for " << m_size << " entries.\n";
	    for( t_heightmap::iterator i( heightmap.begin() ); i!=heightmap.end(); ++i ) {
		std::cout << (*i).first << " has " << (*i).second.first << "/" << (*i).second.second << std::endl;
	    }
	    std::cout << "Done.\n";
	} catch( std::exception & e ) {
	    std::cout << "Exception during heightmapping!" << std::endl;
	    throw;
	}
    }
#endif


}
