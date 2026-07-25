Picking the right Data Structure (C++)
======================================

Start!
a = yes, b = no

0. Is the number of elements you're dealing with a fixed amount?
0a. If so, is all you're doing either pointing to and/or iterating over elements?
0aa. If so, use an array (either static or dynamically-allocated).
0ab. If not, can you change your data layout or processing strategy so that pointing to and/or iterating over elements would be all you're doing?
0aba. If so, do that and goto 0aa.
0abb. If not, goto 1.
0b. If not, is all you're doing inserting-to/erasing-from the back of the container and pointing to elements and/or iterating?
0ba. If so, do you know the largest possible maximum capacity you will ever have for this container, and is the lowest possible maximum capacity not too far away from that?
0baa. If so, use vector and reserve() the highest possible maximum capacity. Or use boost::static_vector for small amounts which can be initialized on the stack.
0bab. If not, use a vector and reserve() either the lowest possible, or most common, maximum capacity. Or boost::static_vector.
0bb. If not, can you change your data layout or processing strategy so that back insertion/erasure and pointing to elements and/or iterating would be all you're doing?
0bba. If so, do that and goto 0ba.
0bbb. If not, goto 1.


1. Is the use of the container stack-like, queue-like or ring-like?
1a. If stack-like, use plf::stack, if queue-like, use plf::queue (both are faster and configurable in terms of memory block sizes). If ring-like, use ring_span or ring_span lite.
1b. If not, goto 2.


2. Does each element need to be accessible via an identifier ie. key? ie. is the data associative.
2a. If so, is the number of elements small and the type sizeof not large?
2aa. If so, is the value of an element also the key?
2aaa. If so, just make an array or vector of elements, and sequentially-scan to lookup elements. Benchmark vs absl:: sets below.
2aab. If not, make a vector or array of key/element structs, and sequentially-scan to lookup elements based on the key. Benchmark vs absl:: maps below.
2ab. If not, do the elements need to have an order?
2aba. If so, is the value of the element also the key?
2abaa. If so, can multiple keys have the same value?
2abaaa. If so, use absl::btree_multiset.
2abaab. If not, use absl::btree_set.
2abab. If not, can multiple keys have the same value?
2ababa. If so, use absl::btree_multimap.
2ababb. If not, use absl::btree_map.
2abb. If no order needed, is the value of the element also the key?
2abba. If so, can multiple keys have the same value?
2abbaa. If so, use std::unordered_multiset or absl::btree_multiset.
2abbab. If not, is pointer stability to elements necessary?
2abbaba. If so, use absl::node_hash_set.
2abbabb. If not, use absl::flat_hash_set.
2abbb. If not, can multiple keys have the same value?
2abbba. If so, use std::unordered_multimap or absl::btree_multimap.
2abbbb. If not, is on-the-fly insertion and erasure common in your use case, as opposed to mostly lookups?
2abbbba. If so, use robin-map.
2abbbbb. If not, is pointer stability to elements necessary?
2abbbbba. If so, use absl::flat_hash_map<Key, std::unique_ptr<Value>>. Use absl::node_hash_map if pointer stability to keys is also necessary.
2abbbbbb. If not, use absl::flat_hash_map.
2b. If not, goto 3.

Note: if iteration over the associative container is frequent rather than rare, try the std:: equivalents to the absl:: containers or tsl::sparse_map. Also take a look at this page of benchmark conclusions for more definitive comparisons across more use-cases and hash map implementations.


3. Are stable pointers/iterators/references to elements which remain valid after non-back insertion/erasure required, and/or is there a need to sort non-movable/copyable elements?
3a. If so, is the order of elements important and/or is there a need to sort non-movable/copyable elements?
3aa. If so, will this container often be accessed and modified by multiple threads simultaneously?
3aaa. If so, use forward_list (for its lowered side-effects when erasing and inserting).
3aab. If not, do you require range-based splicing between two or more containers (as opposed to splicing of entire containers, or splicing elements to different locations within the same container)?
3aaba. If so, use std::list.
3aabb. If not, use plf::list.
3ab. If not, use hive.
3b. If not, goto 4.


4. Is the order of elements important?
4a. If so, are you almost entirely inserting/erasing to/from the back of the container?
4aa. If so, use vector, with reserve() if the maximum capacity is known in advance.
4ab. If not, are you mostly inserting/erasing to/from the front of the container?
4aba. If so, use deque.
4abb. If not, is insertion/erasure to/from the middle of the container frequent when compared to iteration or back erasure/insertion?
4abba. If so, is it mostly erasures rather than insertions, and can the processing of multiple erasures be delayed until a later point in processing, eg. the end of a frame in a video game?
4abbaa. If so, try the vector erase_if pairing approach listed at the bottom of this guide, and benchmark against plf::list to see which one performs best. Use deque with the erase_if pairing if the number of elements is very large.
4abbab. If not, goto 3aa.
4abbb. If not, are elements large or is there a very large number of elements?
4abbba. If so, benchmark vector against plf::list, or if there is a very large number of elements benchmark deque against plf::list.
4abbbb. If not, do you often need to insert/erase to/from the front of the container?
4abbbba. If so, use deque.
4abbbbb. If not, use vector.
4b. If not, goto 5.


5. Is non-back erasure frequent compared to iteration?
5a. If so, is the non-back erasure always at the front of the container?
5aa. If so, use deque.
5ab. If not, is the type large, non-trivially copyable/movable or non-copyable/movable?
5aba. If so, use hive.
5abb. If not, is the number of elements very large?
5abba. If so, use a deque with a swap-and-pop approach (to save memory vs vector - assumes standard deque implementation of fixed block sizes) ie. when erasing, swap the element you wish to erase with the back element, then pop_back(). Benchmark vs hive.
5abbb. If not, use a vector with a swap-and-pop approach and benchmark vs hive.
5b. If not, goto 6.


6. Can non-back erasures be delayed until a later point in processing eg. the end of a video game frame?
6a. If so, is the type large or is the number of elements large?
6aa. If so, use hive.
6ab. If not, is consistent latency more important than lower average latency?
6aba. If so, use hive.
6abb. If not, try the erase_if pairing approach listed below with vector, or with deque if the number of elements is large. Benchmark this approach against hive to see which performs best.
6b. If not, use hive.


Vector erase_if pairing approach:
Try pairing the type with a boolean, in a vector, then marking this boolean for erasure during processing, and then use erase_if with the boolean to remove multiple elements at once at the designated later point in processing. Alternatively if there is a condition in the element itself which identifies it as needing to be erased, try using this directly with erase_if and skip the boolean pairing. If the maximum number of elements is known in advance, use vector with reserve()
