# Just builds tests

skiplist-map: skiplist.cc skiplist.h
	g++ -DMAP_TYPE=std::map -DNUM_ENTRIES=10000000 -DREPORT_EVERY=1000000 skiplist.cc -o skiplist-map

skiplist-sk: skiplist.cc skiplist.h
	g++ -DMAP_TYPE=skip::map -DNUM_ENTRIES=10000000 -DREPORT_EVERY=1000000 skiplist.cc -o skiplist-map
